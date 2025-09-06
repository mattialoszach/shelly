#include "WhisperOffline.hpp"
#include "whisper.h"
#include <stdexcept>
#include <sstream>

WhisperOffline::WhisperOffline(const std::string& modelPath) {
    ctx_ = whisper_init_from_file(modelPath.c_str());
    if (!ctx_) throw std::runtime_error("Failed to init whisper from model: " + modelPath);
}

WhisperOffline::~WhisperOffline() {
    if (ctx_) whisper_free(ctx_);
}

std::string WhisperOffline::transcribe(const std::vector<float>& pcm,
                                       const std::string& language) {
    if (!ctx_) throw std::runtime_error("whisper context null");

    // Default-Parameter
    auto params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress   = false;
    params.print_timestamps = false;
    params.print_special    = false;
    params.translate        = false;
    params.no_context       = true;
    params.single_segment   = false;
    params.max_len          = 0;
    params.temperature      = 0.0f;   // deterministic
    params.language         = language == "auto" ? "auto" : language.c_str();

    // Inferenz
    if (whisper_full(ctx_, params, pcm.data(), (int)pcm.size()) != 0) {
        throw std::runtime_error("whisper_full failed");
    }

    // Collect Segments
    const int n = whisper_full_n_segments(ctx_);
    std::ostringstream oss;
    for (int i = 0; i < n; ++i) {
        const char* txt = whisper_full_get_segment_text(ctx_, i);
        if (txt) {
            if (i && oss.tellp() > 0) oss << ' ';
            oss << txt;
        }
    }
    return oss.str();
}