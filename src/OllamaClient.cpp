#include "OllamaClient.hpp"
#include "json.hpp"
#include <curl/curl.h>

using json = nlohmann::json;

struct StreamCtx {
    std::function<void(const std::string&)> cb;
    std::string carry;
};

static size_t writeOllama(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ctx = static_cast<StreamCtx*>(userdata);
    ctx->carry.append(static_cast<char*>(ptr), size * nmemb);

    size_t pos;
    while ((pos = ctx->carry.find('\n')) != std::string::npos) {
        std::string line = ctx->carry.substr(0, pos);
        ctx->carry.erase(0, pos + 1);
        if (line.empty()) continue;

        auto j = json::parse(line, nullptr, false);
        if (!j.is_discarded() && j.contains("message") && j["message"].contains("content")) {
            ctx->cb(j["message"]["content"].get<std::string>());
        }
    }
    return size * nmemb;
}

bool OllamaClient::chatStream(const std::string& model, const std::string& prompt,
                              const std::function<void(const std::string&)>& cb,
                              const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    json body{
        {"model", model},
        {"stream", true},
        {"messages", json::array({ json{{"role","user"},{"content",prompt}} })}
    };
    std::string bodyStr = body.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    StreamCtx ctx{cb, {}};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeOllama);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    auto ok = (curl_easy_perform(curl) == CURLE_OK);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ok;
}