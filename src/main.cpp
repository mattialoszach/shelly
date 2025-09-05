#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "Recorder.hpp"
#include "WavWriter.hpp"

int main() {
    try {
        const int sr = 16000; // Sampling rate
        Recorder rec(sr, 1);
        std::cout << "Recording 3 seconds...\n";
        rec.start();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        rec.stop();
        std::cout << "Frames captured: " << rec.data().size() << "\n";
        
        // Saving Audio in WAV Format
        const std::string wavPath = "test.wav";
        if (!wav::writeWav16(wavPath, rec.data(), sr)) {
            std::cerr << "WAV write failed.\n";
            return 1;
        }
        std::cout << "Saved to " << wavPath << "\n";

        // Apple OS only: Play back WAV File (just for testing)
#ifdef __APPLE__
        std::cout << "Playing back...\n";
        std::string cmd = "afplay " + wavPath;
        std::system(cmd.c_str());
#endif

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}