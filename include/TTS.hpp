#pragma once
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// Simple TTS queue using macOS `say` (default) or `espeak` if available.
class TTS {
public:
    // rateWPM: words per minute (say's -r)
    explicit TTS(std::string voice = "Samantha", int rateWPM = 190, bool enabled = true);
    ~TTS();

    void start();
    void stop();
    void enqueue(const std::string& text);
    void setEnabled(bool on);

private:
    void workerLoop();
    void speak(const std::string& text);

    std::string voice_;
    int rate_;
    bool enabled_;
    std::queue<std::string> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool stopping_ = false;
    std::thread th_;
};

