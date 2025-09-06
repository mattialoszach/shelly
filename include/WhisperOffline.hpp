#pragma once
#include <string>
#include <vector>
#include <memory>

struct whisper_context;

class WhisperOffline {
public:
    explicit WhisperOffline(const std::string& modelPath);
    ~WhisperOffline();

    std::string transcribe(const std::vector<float>& pcm, const std::string& language = "auto");
private:
    whisper_context* ctx_{nullptr};
};