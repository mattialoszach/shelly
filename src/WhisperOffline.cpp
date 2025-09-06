#include "WhisperOffline.hpp"
#include "whisper.h"
#include <stdexcept>
#include <sstream>
#include <thread>

WhisperOffline::WhisperOffline(const std::string& modelPath) {
    struct whisper_context_params wparams = whisper_context_default_params();

    ctx_ = whisper_init_from_file_with_params(modelPath.c_str(), wparams);
    if (!ctx_) throw std::runtime_error("Failed to init whisper from model: " + modelPath);
}

WhisperOffline::~WhisperOffline() {
    if (ctx_) whisper_free(ctx_);
}

std::string WhisperOffline::transcribe(const std::vector<float>& pcm,
                                       const std::string& language) {
    if (!ctx_) throw std::runtime_error("whisper context null");

    auto params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress   = false;
    params.print_timestamps = false;
    params.print_special    = false;
    params.translate        = false;
    params.no_context       = true;
    params.single_segment   = false;
    params.max_len          = 0;
    params.temperature      = 0.0f;
    unsigned int hc = std::thread::hardware_concurrency();
    params.n_threads = (hc > 0 ? hc : 1);

    params.language         = (language == "auto") ? "auto" : language.c_str();

    if (whisper_full(ctx_, params, pcm.data(), (int)pcm.size()) != 0) {
        throw std::runtime_error("whisper_full failed");
    }

    const int n = whisper_full_n_segments(ctx_);
    std::ostringstream oss;
    for (int i = 0; i < n; ++i) {
        if (const char* txt = whisper_full_get_segment_text(ctx_, i)) {
            if (i) oss << ' ';
            oss << txt;
        }
    }
    return oss.str();
}