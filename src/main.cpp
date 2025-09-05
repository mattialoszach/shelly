#include <iostream>
#include <thread>
#include <chrono>
#include "Recorder.hpp"

int main() {
    try {
        Recorder rec(16000, 1);
        std::cout << "Recording 3 seconds...\n";
        rec.start();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        rec.stop();
        std::cout << "Frames captured: " << rec.data().size() << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}