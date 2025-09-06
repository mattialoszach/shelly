#pragma once
#include <string>
#include <functional>

class OllamaClient {
public:
    bool chatStream(const std::string& model, const std::string& prompt,
                    const std::function<void(const std::string&)>& streamCallback,
                    const std::string& url = "http://127.0.0.1:11434/api/chat");
};