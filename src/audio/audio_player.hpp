#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>

struct pa_simple;

namespace Audio {

struct SoundBuffer {
    std::vector<float> samples; // 48000Hz interleaved stereo (L, R)
    size_t total_frames = 0;
};

struct ActiveVoice {
    std::shared_ptr<SoundBuffer> buffer;
    size_t frame_cursor = 0;
    float volume = 1.0f;
    bool monitor_only = false;
};

class AudioPlayer {
public:
    static AudioPlayer& instance();

    AudioPlayer();
    ~AudioPlayer();

    bool init(const std::string &output_sink = "", const std::string &input_source = "");
    void shutdown();

    void play(const std::string &path, float volume = 1.0f);
    void play_preview(const std::string &path, float volume = 1.0f);
    void stop_all();

    void set_monitor_volume(float v) { monitor_volume_ = v; }
    void set_mic_volume(float v) { mic_volume_ = v; }
    void set_media_to_mic_active(bool active) { media_to_mic_active_ = active; }

    void set_monitor_muted(bool muted) { monitor_muted_ = muted; }
    void set_mic_muted(bool muted) { mic_muted_ = muted; }
    bool is_monitor_muted() const { return monitor_muted_; }
    bool is_mic_muted() const { return mic_muted_; }

    bool set_output_device(const std::string &sink_name);
    void set_meter_input_device(const std::string &source_name);

    // Live level meters (0.0f - 1.0f)
    float get_monitor_level() const { return peak_monitor_.load(); }
    float get_mic_level() const { return peak_mic_.load(); }
    float get_input_mic_level() const { return peak_input_mic_.load(); }

    bool is_playing();

private:
    std::shared_ptr<SoundBuffer> get_or_load(const std::string &path);
    void audio_loop();
    void mic_meter_loop();

    std::unordered_map<std::string, std::shared_ptr<SoundBuffer>> cache_;
    std::mutex cache_mutex_;

    std::vector<ActiveVoice> active_voices_;
    std::mutex voice_mutex_;

    std::atomic<bool> running_{false};
    std::thread audio_thread_;

    std::atomic<bool> meter_running_{false};
    std::thread meter_thread_;
    std::string meter_device_;
    std::mutex meter_mutex_;

    std::atomic<float> monitor_volume_{1.0f};
    std::atomic<float> mic_volume_{1.0f};
    std::atomic<bool> media_to_mic_active_{false};
    std::atomic<bool> monitor_muted_{false};
    std::atomic<bool> mic_muted_{false};

    std::atomic<float> peak_monitor_{0.0f};
    std::atomic<float> peak_mic_{0.0f};
    std::atomic<float> peak_input_mic_{0.0f};

    std::mutex stream_mutex_;
    pa_simple *s_monitor_ = nullptr;
    pa_simple *s_mic_ = nullptr;
    std::string current_output_sink_;
};

} // namespace Audio
