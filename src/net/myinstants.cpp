#include "myinstants.hpp"
#include <curl/curl.h>
#include <yyjson.h>
#include <fstream>
#include <iostream>

namespace Net {

MyInstantsClient& MyInstantsClient::instance() {
    static MyInstantsClient inst;
    return inst;
}

MyInstantsClient::MyInstantsClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

MyInstantsClient::~MyInstantsClient() {
    curl_global_cleanup();
}

size_t MyInstantsClient::write_string_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    std::string *s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), total);
    return total;
}

size_t MyInstantsClient::write_file_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    std::ofstream *out = static_cast<std::ofstream*>(userp);
    size_t total = size * nmemb;
    out->write(static_cast<char*>(contents), total);
    return total;
}

SearchResult MyInstantsClient::search(const std::string &query, int page) {
    SearchResult res;
    CURL *curl = curl_easy_init();
    if (!curl) return res;

    std::string url = "https://www.myinstants.com/api/v1/instants/?format=json&page=" + std::to_string(page);
    if (!query.empty()) {
        char *esc = curl_easy_escape(curl, query.c_str(), query.length());
        if (esc) {
            url += "&name=" + std::string(esc);
            curl_free(esc);
        }
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "VoxBoard/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        std::cerr << "[MyInstants] HTTP error: " << curl_easy_strerror(code) << "\n";
        return res;
    }

    yyjson_doc *doc = yyjson_read(response.c_str(), response.size(), 0);
    if (!doc) return res;

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return res;
    }

    yyjson_val *count_val = yyjson_obj_get(root, "count");
    if (count_val && yyjson_is_num(count_val)) {
        res.count = (int)yyjson_get_int(count_val);
    }

    yyjson_val *next_val = yyjson_obj_get(root, "next");
    if (next_val && yyjson_is_str(next_val)) {
        res.next_url = yyjson_get_str(next_val);
    }

    yyjson_val *results_arr = yyjson_obj_get(root, "results");
    if (results_arr && yyjson_is_arr(results_arr)) {
        size_t idx, max;
        yyjson_val *item_val;
        yyjson_arr_foreach(results_arr, idx, max, item_val) {
            if (!yyjson_is_obj(item_val)) continue;

            InstantSound sound;
            yyjson_val *v;

            v = yyjson_obj_get(item_val, "name");
            if (v && yyjson_is_str(v)) sound.name = yyjson_get_str(v);

            v = yyjson_obj_get(item_val, "slug");
            if (v && yyjson_is_str(v)) sound.slug = yyjson_get_str(v);

            v = yyjson_obj_get(item_val, "sound");
            if (v && yyjson_is_str(v)) sound.sound_url = yyjson_get_str(v);

            v = yyjson_obj_get(item_val, "color");
            if (v && yyjson_is_str(v)) sound.color = yyjson_get_str(v);

            if (!sound.sound_url.empty()) {
                res.items.push_back(sound);
            }
        }
    }

    yyjson_doc_free(doc);
    return res;
}

bool MyInstantsClient::download_sound(const std::string &sound_url, const std::string &dest_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;

    std::ofstream out(dest_path, std::ios::binary);
    if (!out.is_open()) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, sound_url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "VoxBoard/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    out.close();

    return code == CURLE_OK;
}

} // namespace Net
