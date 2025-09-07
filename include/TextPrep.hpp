#pragma once
#include <string>
#include <vector>


// Remove markdown, code fences, links, and normalize whitespace for TTS.
std::string clean_for_tts(const std::string& in);

// Incrementally collect text and emit complete sentences/clauses for TTS.
class SentenceBuffer {
public:
    // Append new text chunk; returns any complete sentences ready to speak.
    std::vector<std::string> append(const std::string& chunk);

    // Flush remaining partial text at the end.
    std::string flush();

private:
    std::string buf_;
};

