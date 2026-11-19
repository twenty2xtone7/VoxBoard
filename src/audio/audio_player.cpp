#include "audio_player.hpp"
#include <sndfile.h>
#include <samplerate.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Audio {

constexpr int TARGET_RATE = 48000;
constexpr int TARGET_CHANNELS = 2;
constexpr size_t BUFFER_FRAMES = 512;

AudioPlayer& AudioPlayer::instance() {
    static AudioPlayer inst;
    return inst;
}

AudioPlayer::AudioPlayer() = default;

AudioPlayer::~AudioPlayer() {
    shutdown();
}

bool AudioPlayer::init(const std::string &output_sink, const std::string &input_source) {
    if (running_) return true;

    current_output_sink_ = output_sink;
    meter_device_ = input_source;

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = TARGET_RATE;
    ss.channels = TARGET_CHANNELS;

    pa_buffer_attr attr;
    attr.maxlength = (uint32_t)-1;
    attr.tlength = BUFFER_FRAMES * sizeof(float) * TARGET_CHANNELS * 2;
    attr.prebuf = (uint32_t)-1;
    attr.minreq = (uint32_t)-1;
    attr.fragsize = (uint32_t)-1;

    int error = 0;
    // Monitor stream (user-selected sink or default headphones)
    const char *out_dev = output_sink.empty() ? NULL : output_sink.c_str();
    s_monitor_ = pa_simple_new(NULL, "VoxBoard", PA_STREAM_PLAYBACK, out_dev, "VoxBoard Monitor", &ss, NULL, &attr, &error);
    if (!s_monitor_) {
        std::cerr << "[AudioPlayer] Failed to open monitor playback stream: " << pa_strerror(error) << "\n";
    }

    // Mic stream (VoxBoard_Sink null sink)
    s_mic_ = pa_simple_new(NULL, "VoxBoard", PA_STREAM_PLAYBACK, "VoxBoard_Sink", "VoxBoard Soundboard", &ss, NULL, &attr, &error);
    if (!s_mic_) {
        std::cerr << "[AudioPlayer] Note: VoxBoard_Sink not ready or null-sink missing: " << pa_strerror(error) << "\n";
    }

    running_ = true;
    audio_thread_ = std::thread(&AudioPlayer::audio_loop, this);

    meter_running_ = true;
    meter_thread_ = std::thread(&AudioPlayer::mic_meter_loop, this);

    return true;
}

void AudioPlayer::shutdown() {
    running_ = false;
    meter_running_ = false;

    if (audio_thread_.joinable()) {
        audio_thread_.join();
    }
    if (meter_thread_.joinable()) {
        meter_thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(stream_mutex_);
        if (s_monitor_) {
            pa_simple_free(s_monitor_);
            s_monitor_ = nullptr;
        }
        if (s_mic_) {
            pa_simple_free(s_mic_);
            s_mic_ = nullptr;
        }
    }
    stop_all();
}

bool AudioPlayer::set_output_device(const std::string &sink_name) {
    std::lock_guard<std::mutex> lock(stream_mutex_);
    current_output_sink_ = sink_name;

    if (s_monitor_) {
        pa_simple_free(s_monitor_);
        s_monitor_ = nullptr;
    }

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = TARGET_RATE;
    ss.channels = TARGET_CHANNELS;

    pa_buffer_attr attr;
    attr.maxlength = (uint32_t)-1;
    attr.tlength = BUFFER_FRAMES * sizeof(float) * TARGET_CHANNELS * 2;
    attr.prebuf = (uint32_t)-1;
    attr.minreq = (uint32_t)-1;
    attr.fragsize = (uint32_t)-1;

    int error = 0;
    const char *out_dev = sink_name.empty() ? NULL : sink_name.c_str();
    s_monitor_ = pa_simple_new(NULL, "VoxBoard", PA_STREAM_PLAYBACK, out_dev, "VoxBoard Monitor", &ss, NULL, &attr, &error);
    return s_monitor_ != nullptr;
}

void AudioPlayer::set_meter_input_device(const std::string &source_name) {
    std::lock_guard<std::mutex> lock(meter_mutex_);
    meter_device_ = source_name;
}

std::shared_ptr<SoundBuffer> AudioPlayer::get_or_load(const std::string &path) {
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(path);
        if (it != cache_.end()) return it->second;
    }

    SF_INFO sfinfo;
    sfinfo.format = 0;
    SNDFILE *sf = sf_open(path.c_str(), SFM_READ, &sfinfo);
    if (!sf) {
        std::cerr << "[AudioPlayer] Failed to open sound file: " << path << "\n";
        return nullptr;
    }

    size_t in_frames = (size_t)sfinfo.frames;
    int in_channels = sfinfo.channels;
    int in_rate = sfinfo.samplerate;

    std::vector<float> raw_pcm(in_frames * in_channels);
    sf_count_t read_count = sf_readf_float(sf, raw_pcm.data(), in_frames);
    sf_close(sf);

    if (read_count <= 0) return nullptr;

    // Resample if needed
    std::vector<float> resampled_pcm;
    size_t resampled_frames = read_count;
    if (in_rate != TARGET_RATE) {
        double ratio = (double)TARGET_RATE / in_rate;
        resampled_frames = (size_t)(read_count * ratio + 16);
        resampled_pcm.resize(resampled_frames * in_channels);

        SRC_DATA src;
        src.data_in = raw_pcm.data();
        src.input_frames = read_count;
        src.data_out = resampled_pcm.data();
        src.output_frames = resampled_frames;
        src.src_ratio = ratio;

        int err = src_simple(&src, SRC_SINC_FASTEST, in_channels);
        if (err != 0) {
            std::cerr << "[AudioPlayer] Resampling error: " << src_strerror(err) << "\n";
            return nullptr;
        }
        resampled_frames = src.output_frames_gen;
    } else {
        resampled_pcm = std::move(raw_pcm);
    }

    // Convert to stereo
    auto buffer = std::make_shared<SoundBuffer>();
    buffer->total_frames = resampled_frames;
    buffer->samples.resize(resampled_frames * TARGET_CHANNELS);

    if (in_channels == 1) {
        for (size_t f = 0; f < resampled_frames; ++f) {
            float val = resampled_pcm[f];
            buffer->samples[f * 2] = val;
            buffer->samples[f * 2 + 1] = val;
        }
    } else if (in_channels == 2) {
        std::copy(resampled_pcm.begin(), resampled_pcm.begin() + (resampled_frames * 2), buffer->samples.begin());
    } else {
        for (size_t f = 0; f < resampled_frames; ++f) {
            buffer->samples[f * 2] = resampled_pcm[f * in_channels];
            buffer->samples[f * 2 + 1] = resampled_pcm[f * in_channels + 1];
        }
    }

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (path.find("preview") == std::string::npos) {
            cache_[path] = buffer;
        }
    }
    return buffer;
}

void AudioPlayer::play(const std::string &path, float volume) {
    auto buf = get_or_load(path);
    if (!buf) return;

    ActiveVoice voice;
    voice.buffer = buf;
    voice.frame_cursor = 0;
    voice.volume = volume;
    voice.monitor_only = false;
    voice.path = path;

    std::lock_guard<std::mutex> lock(voice_mutex_);
    active_voices_.push_back(voice);
}

void AudioPlayer::play_preview(const std::string &path, float volume) {
    auto buf = get_or_load(path);
    if (!buf) return;

    ActiveVoice voice;
    voice.buffer = buf;
    voice.frame_cursor = 0;
    voice.volume = volume;
    voice.monitor_only = true;
    voice.path = path;

    std::lock_guard<std::mutex> lock(voice_mutex_);
    active_voices_.push_back(voice);
}

float AudioPlayer::get_sound_level(const std::string &path) const {
    std::lock_guard<std::mutex> lock(sound_level_mutex_);
    auto it = sound_levels_.find(path);
    return it != sound_levels_.end() ? it->second : 0.0f;
}

void AudioPlayer::stop_all() {
    {
        std::lock_guard<std::mutex> lock(voice_mutex_);
        active_voices_.clear();
    }
    std::lock_guard<std::mutex> lock(stream_mutex_);
    int err = 0;
    if (s_monitor_) pa_simple_flush(s_monitor_, &err);
    if (s_mic_) pa_simple_flush(s_mic_, &err);
}

bool AudioPlayer::is_playing() {
    std::lock_guard<std::mutex> lock(voice_mutex_);
    return !active_voices_.empty();
}

void AudioPlayer::audio_loop() {
    std::vector<float> mix_buf_monitor(BUFFER_FRAMES * TARGET_CHANNELS);
    std::vector<float> mix_buf_mic(BUFFER_FRAMES * TARGET_CHANNELS);

    while (running_) {
        // Decay every per-sound level so bars glide back down after a sound finishes.
        {
            std::lock_guard<std::mutex> lvl_lock(sound_level_mutex_);
            for (auto &kv : sound_levels_) {
                // Slow envelope decay (~200ms half-life) so the UI receives a
                // smooth target instead of a spiky per-block peak.
                kv.second *= 0.965f;
                if (kv.second < 0.0005f) kv.second = 0.0f;
            }
        }

        bool has_voices = false;
        {
            std::lock_guard<std::mutex> lock(voice_mutex_);
            has_voices = !active_voices_.empty();
        }

        if (!has_voices) {
            float old_mon = peak_monitor_.load();
            peak_monitor_.store(old_mon * 0.8f);
            float old_mic = peak_mic_.load();
            peak_mic_.store(old_mic * 0.8f);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        std::fill(mix_buf_monitor.begin(), mix_buf_monitor.end(), 0.0f);
        std::fill(mix_buf_mic.begin(), mix_buf_mic.end(), 0.0f);

        {
            std::lock_guard<std::mutex> lock(voice_mutex_);
            for (auto it = active_voices_.begin(); it != active_voices_.end();) {
                auto &voice = *it;
                size_t remaining_frames = voice.buffer->total_frames - voice.frame_cursor;
                size_t frames_to_mix = std::min(BUFFER_FRAMES, remaining_frames);

                const float *src = voice.buffer->samples.data() + (voice.frame_cursor * TARGET_CHANNELS);
                float vol = voice.volume;

                float voice_peak = 0.0f;
                for (size_t i = 0; i < frames_to_mix * TARGET_CHANNELS; ++i) {
                    float s = src[i] * vol;
                    float a = std::fabs(s);
                    if (a > voice_peak) voice_peak = a;
                    mix_buf_monitor[i] += s;
                    if (!voice.monitor_only) {
                        mix_buf_mic[i] += s;
                    }
                }

                // Keep this sound's visualizer level topped up while it plays.
                if (!voice.path.empty()) {
                    std::lock_guard<std::mutex> lvl_lock(sound_level_mutex_);
                    float &sl = sound_levels_[voice.path];
                    if (voice_peak > sl) sl = voice_peak;
                }

                voice.frame_cursor += frames_to_mix;
                if (voice.frame_cursor >= voice.buffer->total_frames) {
                    it = active_voices_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        float m_vol = monitor_volume_.load();
        float mic_vol = mic_volume_.load();
        bool media_active = media_to_mic_active_.load();
        bool mon_muted = monitor_muted_.load();
        bool mic_muted = mic_muted_.load();

        float mon_peak = 0.0f;
        for (float &s : mix_buf_monitor) {
            if (mon_muted) {
                s = 0.0f;
            } else {
                s = std::clamp(s * m_vol, -1.0f, 1.0f);
                float a = std::fabs(s);
                if (a > mon_peak) mon_peak = a;
            }
        }

        float mic_peak = 0.0f;
        for (float &s : mix_buf_mic) {
            if (mic_muted) {
                s = 0.0f;
            } else {
                s = std::clamp(s * mic_vol, -1.0f, 1.0f);
                float a = std::fabs(s);
                if (a > mic_peak) mic_peak = a;
            }
        }

        float old_mon = peak_monitor_.load();
        peak_monitor_.store(std::max(mon_peak, old_mon * 0.85f));
        float old_mic = peak_mic_.load();
        peak_mic_.store(std::max(mic_peak, old_mic * 0.85f));

        int err = 0;
        std::lock_guard<std::mutex> lock(stream_mutex_);
        if (s_monitor_) {
            pa_simple_write(s_monitor_, mix_buf_monitor.data(), mix_buf_monitor.size() * sizeof(float), &err);
        }

        if (!s_mic_) {
            pa_sample_spec ss;
            ss.format = PA_SAMPLE_FLOAT32LE;
            ss.rate = TARGET_RATE;
            ss.channels = TARGET_CHANNELS;
            int error = 0;
            s_mic_ = pa_simple_new(NULL, "VoxBoard", PA_STREAM_PLAYBACK, "VoxBoard_Sink", "VoxBoard Soundboard", &ss, NULL, NULL, &error);
        }
        if (s_mic_) {
            pa_simple_write(s_mic_, mix_buf_mic.data(), mix_buf_mic.size() * sizeof(float), &err);
        }
    }
}

void AudioPlayer::mic_meter_loop() {
    pa_simple *s_record = nullptr;
    std::string active_dev;

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = 16000;
    ss.channels = 1;

    pa_buffer_attr attr;
    attr.maxlength = (uint32_t)-1;
    attr.tlength = (uint32_t)-1;
    attr.prebuf = (uint32_t)-1;
    attr.minreq = (uint32_t)-1;
    attr.fragsize = 256 * sizeof(float);

    float buf[256];

    while (meter_running_) {
        std::string target_dev;
        {
            std::lock_guard<std::mutex> lock(meter_mutex_);
            target_dev = meter_device_;
        }

        if (!s_record || target_dev != active_dev) {
            if (s_record) {
                pa_simple_free(s_record);
                s_record = nullptr;
            }
            int error = 0;
            const char *dev = target_dev.empty() ? NULL : target_dev.c_str();
            s_record = pa_simple_new(NULL, "VoxBoardMeter", PA_STREAM_RECORD, dev, "VoxBoard Mic Level Meter", &ss, NULL, &attr, &error);
            if (!s_record) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            active_dev = target_dev;
        }

        int error = 0;
        if (pa_simple_read(s_record, buf, sizeof(buf), &error) < 0) {
            pa_simple_free(s_record);
            s_record = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        float max_val = 0.0f;
        for (int i = 0; i < 256; ++i) {
            float a = std::fabs(buf[i]);
            if (a > max_val) max_val = a;
        }

        float old_val = peak_input_mic_.load();
        peak_input_mic_.store(std::max(max_val, old_val * 0.85f));
    }

    if (s_record) {
        pa_simple_free(s_record);
        s_record = nullptr;
    }
}

} // namespace Audio
