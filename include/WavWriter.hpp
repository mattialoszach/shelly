#pragma once
#include <string>
#include <vector>

namespace wav {
    bool writeWav16(const std::string& path, const std::vector<float>& mono, int sampleRate);
}