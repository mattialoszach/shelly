#include "Recorder.hpp"
#include <stdexcept>

Recorder::Recorder(int sampleRate, int channels)
: sampleRate_(sampleRate), channels_(channels) {
    PaError err = Pa_Initialize();
    if (err != paNoError) throw std::runtime_error("PortAudio init failed");
}

Recorder::~Recorder() {
    if (stream_) Pa_CloseStream(stream_);
    Pa_Terminate();
}

int Recorder::paCallback(const void* input, void*, unsigned long frameCount,
                         const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {
    auto* self = static_cast<Recorder*>(userData);
    const float* in = static_cast<const float*>(input);
    if (in) {
        self->buffer_.insert(self->buffer_.end(), in, in + frameCount * self->channels_);
    }
    return paContinue;
}

void Recorder::start() {
    PaStreamParameters inParams{};
    inParams.device = Pa_GetDefaultInputDevice();
    if (inParams.device == paNoDevice) throw std::runtime_error("No input device");
    inParams.channelCount = channels_;
    inParams.sampleFormat = paFloat32;
    inParams.suggestedLatency = Pa_GetDeviceInfo(inParams.device)->defaultLowInputLatency;
    inParams.hostApiSpecificStreamInfo = nullptr;

    if (Pa_OpenStream(&stream_, &inParams, nullptr, sampleRate_, 512, paNoFlag, &Recorder::paCallback, this) != paNoError)
        throw std::runtime_error("Pa_OpenStream failed (try 48000 Hz if this persists)");
    if (Pa_StartStream(stream_) != paNoError)
        throw std::runtime_error("Pa_StartStream failed");
}

void Recorder::stop() {
    if (stream_) {
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
}