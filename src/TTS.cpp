#include "TTS.hpp"
#include <vector>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

TTS::TTS(std::string voice, int rateWPM, bool enabled)
    : voice_(std::move(voice)), rate_(rateWPM), enabled_(enabled) {}

TTS::~TTS() { stop(); }

void TTS::start() {
    std::lock_guard<std::mutex> lk(m_);
    if (th_.joinable()) return;
    stopping_ = false;
    th_ = std::thread([this]{ workerLoop(); });
}

void TTS::stop() {
    {
        std::lock_guard<std::mutex> lk(m_);
        if (!th_.joinable()) return;
        stopping_ = true;
    }
    cv_.notify_all();
    th_.join();
}

void TTS::setEnabled(bool on) {
    std::lock_guard<std::mutex> lk(m_);
    enabled_ = on;
}

void TTS::enqueue(const std::string& text) {
    {
        std::lock_guard<std::mutex> lk(m_);
        if (text.empty()) return;
        q_.push(text);
    }
    cv_.notify_one();
}

void TTS::workerLoop() {
    for(;;) {
        std::string item;
        {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this]{ return stopping_ || !q_.empty(); });
            if (stopping_ && q_.empty()) break;
            item = std::move(q_.front());
            q_.pop();
        }

        if (enabled_) speak(item);
    }
}

void TTS::speak(const std::string& text) {
    // Prefer macOS `say`. If unavailable, try `espeak`.
    // We use posix_spawnp to avoid shell quoting issues.
    // macOS `say` usage: say -v <voice> -r <wpm> <text>

    auto spawn_and_wait = [&](const std::vector<std::string>& args) {
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& s: args) argv.push_back(const_cast<char*>(s.c_str()));
        argv.push_back(nullptr);
        pid_t pid = 0;
        int rc = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
        if (rc != 0) return rc;
        int status = 0;
        (void)waitpid(pid, &status, 0);
        return 0;
    };

    // Try say
    {
        std::vector<std::string> args = {
            "say",
            "-v", voice_.empty() ? std::string("Anna") : voice_,
            "-r", std::to_string(rate_ > 0 ? rate_ : 190),
            text
        };
        int rc = spawn_and_wait(args);
        if (rc == 0) return;
    }

    // Fallback to espeak if present
    {
        std::vector<std::string> args = {
            "espeak",
            text
        };
        (void)spawn_and_wait(args);
    }
}

