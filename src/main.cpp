#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "Recorder.hpp"
#include "WavWriter.hpp"
#include "OllamaClient.hpp"
#include "WhisperOffline.hpp"

int main(int argc, char** argv) {
    std::string modelPath_Whisper = "models/ggml-base.en.bin";
    // if (argc > 1) modelPath_Whisper = argv[1];

    std::cout << "🗣️ Hold ENTER → Speak → release ENTER. Type 'q' + ENTER to quit.\n";

    const int sr = 16000; // Sampling rate
    Recorder rec(sr, 1);
    WhisperOffline whisper(modelPath_Whisper);
    OllamaClient ollama;

    while (true) {
        std::cout << "\nReady. Press ENTER to start recording...";
        std::string line;
        if (!std::getline(std::cin, line)) break; // Waits for ENTER
        if (line == "q") break;

        std::cout << "🎙️  Recording... (press ENTER to stop)";
        rec.clear();
        rec.start();
        std::getline(std::cin, line);
        rec.stop();
        std::cout << "\n⏹️  Stopped. Frames: " << rec.data().size() << "\n";
        if (rec.data().empty()) continue;

        std::cout << "🧠 Transcribing (in-process)...\n";
        std::string userText = whisper.transcribe(rec.data(), "auto");
        std::cout << "👤 You: " << userText << "\n";
        if (userText.empty()) continue;

        std::cout << "🐚 Assistant: ";
        std::string collected; // For Mac System-TTS
        bool ok = ollama.chatStream("llama3", userText, [&](const std::string& token){
            std::cout << token << std::flush;
            collected += token;
        });
        std::cout << "\n";
    }

    std::cout << "Bye!\n";
    return 0;
}