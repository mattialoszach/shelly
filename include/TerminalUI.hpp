#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

// Simple ANSI helpers (POSIX terminals)
namespace tui {

// Styles
namespace ansi {
    inline constexpr const char* reset   = "\x1b[0m";
    inline constexpr const char* bold    = "\x1b[1m";
    inline constexpr const char* dim     = "\x1b[2m";
    inline constexpr const char* italic  = "\x1b[3m";
    inline constexpr const char* invert  = "\x1b[7m";

    // Foreground
    inline constexpr const char* fg_gray  = "\x1b[38;5;245m";
    inline constexpr const char* fg_green = "\x1b[32m";
    inline constexpr const char* fg_cyan  = "\x1b[36m";
    inline constexpr const char* fg_yellow= "\x1b[33m";
    inline constexpr const char* fg_red   = "\x1b[31m";
    inline constexpr const char* fg_blue  = "\x1b[34m";

    // Background (used for “buttons”)
    inline constexpr const char* bg_gray  = "\x1b[48;5;236m";
    inline constexpr const char* bg_blue  = "\x1b[44m";
    inline constexpr const char* bg_green = "\x1b[42m";
    inline constexpr const char* bg_red   = "\x1b[41m";
}

// Clear current screen and move cursor to top-left, but
// DO NOT wipe scrollback (so user can still scroll up).
inline void clearToTop() {
    std::cout << "\x1b[H\x1b[2J"; // cursor home + clear screen
}

// Thin horizontal rule
inline void rule() {
    std::cout << ansi::fg_gray << "────────────────────────────────────────────────────────────" << ansi::reset << "\n";
}

// Cursor guard (hide while in scope, restore on destruction)
class CursorGuard {
public:
    CursorGuard()  { std::cout << "\x1b[?25l" << std::flush; } // hide
    ~CursorGuard() { std::cout << "\x1b[?25h" << std::flush; } // show
};

// Tiny spinner for blocking steps (use sparingly)
class Spinner {
public:
    explicit Spinner(std::string prefix = "") : prefix_(std::move(prefix)) {}
    void start() { running_ = true; th_ = std::thread([this]{ loop(); }); }
    void stop()  { running_ = false; if (th_.joinable()) th_.join(); std::cout << "\r" << std::string(80,' ') << "\r"; }
private:
    void loop() {
        static const char* frames = "|/-\\";
        size_t i = 0;
        while (running_) {
            std::cout << "\r" << ansi::fg_cyan << prefix_ << " " << frames[i%4] << ansi::reset << std::flush;
            i++; std::this_thread::sleep_for(std::chrono::milliseconds(90));
        }
    }
    std::string prefix_;
    bool running_ = false;
    std::thread th_;
};

// “Button-like” labels
inline void button_idle(const std::string& text = " PRESS ENTER TO START ") {
    std::cout << "\n" << ansi::bg_blue << ansi::bold << text << ansi::reset
              << "  " << ansi::fg_gray << "(type 'q' or 'ESC' to quit)" << ansi::reset << "\n";
}
inline void button_recording(const std::string& text = " ● RECORDING — PRESS ENTER TO STOP ") {
    std::cout << "\n" << ansi::bg_red << ansi::bold << text << ansi::reset << "\n";
}
inline void button_quit(const std::string& text = " EXITING ") {
    std::cout << "\n" << ansi::bg_gray << ansi::bold << text << ansi::reset << "\n";
}

// Header & footer
inline void header() {
    clearToTop();
    std::cout << ansi::bold << "\n🐚 Shelly" << ansi::reset
              << ansi::fg_gray << " — Terminal Voice Assistant\n" << ansi::reset;
    rule();
}

// Blocks
inline void block_user(const std::string& s) {
    std::cout << "\n" << ansi::fg_blue << ansi::bold << "You" << ansi::reset << ": " << s << "\n";
}
inline void block_assistant_prefix() {
    std::cout << "\n" << ansi::fg_cyan << ansi::bold << "Assistant" << ansi::reset << ": ";
}

inline void bye() {
    button_quit();
    std::cout << ansi::fg_gray << "\nBye!\n" << ansi::reset;
}

}