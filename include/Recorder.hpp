#pragma once
#include <vector>
#include <portaudio.h>

class Recorder {
public:
    Recorder(int sampleRate = 16000, int channels = 1);
    ~Recorder();

    void start();
    void stop();
    const std::vector<float>& data() const { return buffer_; }
    void clear() { buffer_.clear(); }
private:
    static int paCallback(const void* input, void*, unsigned long frameCount, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData);
    PaStream* stream_{nullptr};
    int sampleRate_;
    int channels_;
    std::vector<float> buffer_;
};