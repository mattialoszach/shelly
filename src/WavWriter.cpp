#include "WavWriter.hpp"
#include <sndfile.h>

namespace wav {
    bool writeWav16(const std::string& path, const std::vector<float>& mono, int sampleRate) {
        SF_INFO info{};
        info.samplerate = sampleRate;
        info.channels = 1;
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

        SNDFILE* sf = sf_open(path.c_str(), SFM_WRITE, &info);
        if (!sf) return false;

        // float -1..1 -> int16
        std::vector<short> pcm(mono.size());
        for (size_t i = 0; i < mono.size(); ++i) {
            float x = std::max(-1.0f, std::min(1.0f, mono[i]));
            pcm[i] = static_cast<short>(x * 32767.0f);
        }
        sf_write_short(sf, pcm.data(), static_cast<sf_count_t>(pcm.size()));
        sf_close(sf);
        return true;
    }
}