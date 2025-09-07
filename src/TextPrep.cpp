#include "TextPrep.hpp"
#include <algorithm>
#include <cctype>

namespace {
    static inline void trim_inplace(std::string& s) {
        auto not_space = [](unsigned char c){ return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
        s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    }

    static inline void collapse_spaces(std::string& s) {
        std::string out;
        out.reserve(s.size());
        bool prev_space = false;
        for (unsigned char c: s) {
            if (std::isspace(c)) {
                if (!prev_space) out.push_back(' ');
                prev_space = true;
            } else {
                out.push_back(char(c));
                prev_space = false;
            }
        }
        s.swap(out);
    }

    static inline void replace_all(std::string& s, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    static bool looks_like_url_char(char c) {
        return std::isalnum((unsigned char)c) || c == '/' || c == ':' || c == '.' || c == '-' || c == '_' || c == '?' || c == '&' || c == '=' || c == '%';
    }

    static void strip_markdown(std::string& s) {
        // Remove fenced code blocks "```...```"
        for (;;) {
            auto a = s.find("```\n");
            if (a == std::string::npos) break;
            auto b = s.find("```", a + 4);
            if (b == std::string::npos) { s.erase(a, s.size() - a); break; }
            s.erase(a, (b + 3) - a);
        }
        // Inline code/backticks
        s.erase(std::remove(s.begin(), s.end(), '`'), s.end());

        // Bold/italic markers
        s.erase(std::remove(s.begin(), s.end(), '*'), s.end());
        s.erase(std::remove(s.begin(), s.end(), '_'), s.end());

        // Strip markdown links [text](url) -> text
        for (;;) {
            auto lb = s.find('[');
            if (lb == std::string::npos) break;
            auto rb = s.find(']', lb + 1);
            if (rb == std::string::npos) break;
            if (rb + 1 < s.size() && s[rb + 1] == '(') {
                auto rp = s.find(')', rb + 2);
                if (rp != std::string::npos) {
                    // keep text inside []
                    auto keep = s.substr(lb + 1, rb - lb - 1);
                    s.replace(lb, (rp + 1) - lb, keep);
                    continue;
                }
            }
            break;
        }

        // Remove list markers at start of lines
        std::string out; out.reserve(s.size());
        bool bol = true;
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (bol) {
                if ((c == '-' || c == '*') && (i + 1 < s.size()) && std::isspace((unsigned char)s[i+1])) {
                    // skip marker and following single space
                    ++i; // skip marker
                    if (i + 1 < s.size() && s[i+1] == ' ') ++i; // best-effort
                    out.push_back(' ');
                    bol = false;
                    continue;
                }
            }
            out.push_back(c);
            bol = (c == '\n');
        }
        s.swap(out);

        // Replace raw URLs with word "Link"
        out.clear(); out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ) {
            if ((s.compare(i, 7, "http://") == 0) || (s.compare(i, 8, "https://") == 0)) {
                // consume URL chars
                size_t j = i; while (j < s.size() && looks_like_url_char(s[j])) ++j;
                out += "Link";
                i = j;
            } else {
                out.push_back(s[i++]);
            }
        }
        s.swap(out);
    }

    static bool is_abbrev_end(const std::string& s, size_t dotPos) {
        // Guard against splitting on common abbreviations and initials
        // Check a small window before the dot
        const char* abbrev[] = {
            "z.B", "bzw", "d.h", "u.a", "u.U", "z.T", "usw", "ca", "Nr", "Dr", "Prof", "Dipl", "etc", "vs", "vgl",
            "e.g", "i.e", "Mr", "Mrs", "Ms", "Dr", "Jr", "Sr"
        };
        size_t start = (dotPos > 6) ? dotPos - 6 : 0;
        std::string w = s.substr(start, dotPos - start);
        // Remove spaces in the window for patterns like "z. B"
        w.erase(std::remove_if(w.begin(), w.end(), [](unsigned char c){ return std::isspace(c); }), w.end());
        for (auto a: abbrev) {
            size_t alen = std::char_traits<char>::length(a);
            if (w.size() >= alen && w.compare(w.size() - alen, alen, a) == 0) return true;
        }
        // digits around dot (e.g., 3.14)
        if (dotPos > 0 && dotPos + 1 < s.size() && std::isdigit((unsigned char)s[dotPos-1]) && std::isdigit((unsigned char)s[dotPos+1]))
            return true;
        return false;
    }
}

std::string clean_for_tts(const std::string& in) {
    std::string s = in;
    strip_markdown(s);
    collapse_spaces(s);
    trim_inplace(s);
    return s;
}

std::vector<std::string> SentenceBuffer::append(const std::string& chunk) {
    std::vector<std::string> out;
    if (chunk.empty()) return out;
    buf_ += chunk;

    size_t start = 0;
    for (size_t i = 0; i < buf_.size(); ++i) {
        char c = buf_[i];
        bool eos = false;
        if (c == '.' || c == '!' || c == '?') {
            if (!is_abbrev_end(buf_, i)) {
                // require a following space/newline or end-of-buffer
                if (i + 1 >= buf_.size() || std::isspace((unsigned char)buf_[i+1])) eos = true;
            }
        } else if (c == '\n') {
            eos = true;
        } else if (c == ':') {
            // treat colon as a soft boundary
            eos = (i + 1 >= buf_.size() || std::isspace((unsigned char)buf_[i+1]));
        }

        if (eos) {
            std::string s = buf_.substr(start, i - start + 1);
            // trim
            {
                auto t = s; // local copy for trimming
                // quick inline trim
                auto not_space = [](unsigned char x){ return !std::isspace(x); };
                t.erase(t.begin(), std::find_if(t.begin(), t.end(), not_space));
                t.erase(std::find_if(t.rbegin(), t.rend(), not_space).base(), t.end());
                s.swap(t);
            }
            if (!s.empty()) out.push_back(std::move(s));
            start = i + 1;
        }
    }

    if (start > 0) {
        buf_.erase(0, start);
    }
    return out;
}

std::string SentenceBuffer::flush() {
    std::string rest;
    std::swap(rest, buf_);
    // trim
    {
        auto t = rest; // local copy for trimming
        auto not_space = [](unsigned char x){ return !std::isspace(x); };
        t.erase(t.begin(), std::find_if(t.begin(), t.end(), not_space));
        t.erase(std::find_if(t.rbegin(), t.rend(), not_space).base(), t.end());
        rest.swap(t);
    }
    return rest;
}
