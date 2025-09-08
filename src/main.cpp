#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
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

int main(int argc, char** argv) {
    bool mute = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-mute" || arg == "-m") {
            mute = true;
        } else if (arg == "-help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [-mute] [-help]\n";
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

    tui::header();
    tui::button_idle();

    while (true) {
        // Input line
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line == "q") {
            tui::header();
            tui::bye();
            break;
        }

        // Recording UI
        tui::header();
        tui::button_recording();
        std::cout << tui::ansi::fg_gray << " ↪ capturing mic…" << tui::ansi::reset << "\n";

        rec.clear();
        rec.start();
        // wait for second ENTER
        std::getline(std::cin, line);
        rec.stop();

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