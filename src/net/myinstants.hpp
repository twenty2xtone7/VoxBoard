#pragma once

#include <string>
#include <vector>

namespace Net {

struct InstantSound {
    std::string name;
    std::string slug;
    std::string sound_url;
    std::string color;
};

struct SearchResult {
    int count = 0;
    std::string next_url;
    std::vector<InstantSound> items;
};

class MyInstantsClient {
public:
    static MyInstantsClient& instance();

    MyInstantsClient();
    ~MyInstantsClient();

    SearchResult search(const std::string &query, int page = 1);
    bool download_sound(const std::string &sound_url, const std::string &dest_path);

private:
    static size_t write_string_cb(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t write_file_cb(void *contents, size_t size, size_t nmemb, void *userp);
};

} // namespace Net
