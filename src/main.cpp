#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <atomic>
#include "Recorder.hpp"
#include "WavWriter.hpp"
#include "OllamaClient.hpp"
#include "WhisperOffline.hpp"
#include "whisper.h"
#include "ggml.h"
#include "TTS.hpp"
#include "TextPrep.hpp"
#include "SystemPrompt.hpp"
#include "TerminalUI.hpp"

static void ggml_quiet_logger(enum ggml_log_level level, const char* msg, void*) {
    if (level >= GGML_LOG_LEVEL_ERROR) {
        fputs(msg, stderr);
    }
}

// Raw terminal guard (RAII)
struct RawGuard {
    termios old{};
    bool ok{false};
    RawGuard() {
        if (tcgetattr(STDIN_FILENO, &old) == 0) {
            termios raw = old;
            raw.c_lflag &= ~(ICANON | ECHO);   // single-char, no echo
            raw.c_cc[VMIN]  = 1;               // read returns after 1 byte
            raw.c_cc[VTIME] = 0;               // no timeout
            ok = (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0);
        }
    }
    ~RawGuard() { if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &old); }
};

int readKey() {
    unsigned char c;
    ssize_t n = ::read(STDIN_FILENO, &c, 1);
    if (n == 1) return static_cast<int>(c);
    return -1;
}

bool isEnter(int k) {
    return (k == '\n') || (k == '\r');
}

int main(int argc, char** argv) {
    bool mute = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-mute" || arg == "-m") {
            mute = true;
        } else if (arg == "-help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [-mute|-m] [-help|-h]\n";
            return 0;
        }
    }

    ggml_log_set(ggml_quiet_logger, nullptr);
    whisper_log_set(ggml_quiet_logger, nullptr);
    std::string modelPath_Whisper = "models/ggml-base.en.bin";
    
    const int sr = 16000; // Sampling rate
    Recorder rec(sr, 1);
    WhisperOffline whisper(modelPath_Whisper);
    OllamaClient ollama;

    // TTS configuration
    TTS tts;
    if (!mute) tts.start();

    tui::CursorGuard cg;
    RawGuard rg;

    tui::header();
    tui::button_idle();

    while (true) {
        // Input line
        int key = readKey();
        if (key == 'q' || key == 27) {
            tui::header();
            tui::bye();
            if (!mute) tts.stop();
            return 0;
        }
        if (!isEnter(key)) {
            continue;
        }

        // Recording UI
        tui::header();
        tui::button_recording();
        std::cout << tui::ansi::fg_gray << " ↪ capturing mic…" << tui::ansi::reset << "\n";

        rec.clear();
        rec.start();

        // wait for second ENTER
        while (true) {
            int key2 = readKey();
            if (key2 == 'q' || key2 == 27) {
                rec.stop();
                tui::header();
                tui::bye();
                if (!mute) tts.stop();
                return 0;
            }
            if (isEnter(key2)) {
                rec.stop();
                break;
            }
        }

        // Show frames count and transition to ASR
        tui::header();
        tui::button_recording(" ⏹ STOPPED ");
        std::cout << tui::ansi::fg_gray << "(frames: " << rec.data().size() << tui::ansi::reset << ")\n";
        if (rec.data().empty()) {
            continue;
        }

        // Transcribe with a small spinner
        tui::Spinner sp("transcribing");
        sp.start();
        std::string userText = whisper.transcribe(rec.data(), "auto");
        sp.stop();

        tui::header();
        tui::button_idle(" READY "); // back to idle look while we think
        tui::block_user(userText.empty() ? "(no speech detected)" : userText);
        if (userText.empty()) {
            continue;
        }

        // Build prompt with system context
        sysprompt::SystemPrompt sysp("Shelly");
        std::string prompt = sysp.build(userText);

        // Stream assistant
        tui::block_assistant_prefix();
        std::string collected;
        SentenceBuffer sbuf;
        bool ok = ollama.chatStream("llama3", prompt, [&](const std::string& token){
            std::cout << token << std::flush;
            collected += token;
            for (auto& s : sbuf.append(token)) {
                auto clean = clean_for_tts(s);
                if (!mute) tts.enqueue(clean);
            }
        });

        // Flush remainder to TTS
        {
            auto rem = sbuf.flush();
            if (!rem.empty()) tts.enqueue(clean_for_tts(rem));
        }
        std::cout << "\n";

        // Ready again at the top
        tui::button_idle();
    }

    if (!mute) tts.stop();
    return 0;
}
