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

static void ggml_quiet_logger(enum ggml_log_level level, const char* msg, void*) {
    if (level >= GGML_LOG_LEVEL_ERROR) {
        fputs(msg, stderr);
    }
}

int main(int argc, char** argv) {
    ggml_log_set(ggml_quiet_logger, nullptr);
    whisper_log_set(ggml_quiet_logger, nullptr);
    std::string modelPath_Whisper = "models/ggml-base.en.bin";

    std::cout << "🗣️ Hold ENTER → Speak → release ENTER. Type 'q' + ENTER to quit.\n";

    const int sr = 16000; // Sampling rate
    Recorder rec(sr, 1);
    WhisperOffline whisper(modelPath_Whisper);
    OllamaClient ollama;

    // TTS configuration
    TTS tts;
    tts.start();

    while (true) {
        std::cout << "\nReady. Press ENTER to start recording...\n";
        std::string line;
        if (!std::getline(std::cin, line)) break; // Waits for ENTER
        if (line == "q") {
            break;
        }

        std::cout << "🎙️  Recording... (press ENTER to stop)";
        rec.clear();
        rec.start();
        std::getline(std::cin, line);
        rec.stop();
        std::cout << "\n⏹️  Stopped. Frames: " << rec.data().size() << "\n";
        if (rec.data().empty()) continue;

        std::cout << "🧠 Transcribing (in-process)...\n";
        std::string userText = whisper.transcribe(rec.data(), "auto"); // Actual User Input
        sysprompt::SystemPrompt sysprompt("Shelly");
        std::string prompt = sysprompt.build(userText); // Transformed Prompt with System Prompt

        std::cout << "\n👤 You: " << userText << "\n";
        if (userText.empty()) continue;

        std::cout << "\n🐚 Assistant: ";
        std::string collected;
        SentenceBuffer sbuf;
        bool ok = ollama.chatStream("llama3", prompt, [&](const std::string& token){
            std::cout << token << std::flush;
            collected += token;

            // As tokens arrive, emit complete sentences for TTS
            for (auto& s : sbuf.append(token)) {
                auto clean = clean_for_tts(s);
                tts.enqueue(clean);
            }
        });
        // Flush any remainder at end of stream
        {
            auto rem = sbuf.flush();
            if (!rem.empty()) tts.enqueue(clean_for_tts(rem));
        }
        std::cout << "\n";
    }

    std::cout << "Bye!\n";
    tts.stop();
    return 0;
}
