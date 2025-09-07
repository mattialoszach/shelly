#pragma once
#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <array>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <ctime>
#include <cstdlib>

namespace sysprompt {

// Helper Functions
std::string trim(std::string s) {
    auto ns = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), ns));
    s.erase(std::find_if(s.rbegin(), s.rend(), ns).base(), s.end());
    return s;
}

std::string run(const std::string& cmd) {
    std::array<char, 1024> buf{};
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return out;
    while (fgets(buf.data(), (int)buf.size(), p)) out += buf.data();
    pclose(p);
    return out;
}

void add(std::unordered_map<std::string,std::string>& m, const std::string& k, const std::string& v) {
    if (!v.empty()) m[k] = trim(v);
}

void oneLine(std::string& s) {
    std::replace(s.begin(), s.end(), '\n', ' ');
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c){ return !std::isprint(c); }), s.end());
    s = trim(s);
}

std::string escape_json(std::string s) {
    std::string out; out.reserve(s.size()+8);
    for (unsigned char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) { // control chars
                    char buf[7]; std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else out += c;
        }
    }
    return out;
}

// Collectors
std::unordered_map<std::string,std::string> collect_time() {
    std::unordered_map<std::string,std::string> kv;
    std::time_t t = std::time(nullptr);
    char buf[64]{};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", std::localtime(&t));
    add(kv, "time", buf);
    return kv;
}

std::unordered_map<std::string,std::string> collect_user_host_cwd() {
    std::unordered_map<std::string,std::string> kv;
    char host[256]{}; gethostname(host, sizeof(host));
    add(kv, "host", host);

    if (auto* pw = getpwuid(getuid())) add(kv, "user", pw->pw_name);

    char cwd[1024]{}; if (getcwd(cwd, sizeof(cwd))) add(kv, "cwd", cwd);
    return kv;
}

std::unordered_map<std::string,std::string> collect_os_basic() {
    std::unordered_map<std::string,std::string> kv;
    struct utsname u{};
    if (uname(&u) == 0) {
        add(kv, "os", u.sysname);
        add(kv, "os_ver", u.release);
        add(kv, "arch", u.machine);
    }
    return kv;
}

std::unordered_map<std::string,std::string> collect_git_simple() {
    std::unordered_map<std::string,std::string> kv;
    if (trim(run("git rev-parse --is-inside-work-tree 2>/dev/null")) != "true") return kv;
    add(kv, "git_branch", run("git rev-parse --abbrev-ref HEAD 2>/dev/null"));
    add(kv, "git_dirty",  run("git status --porcelain 2>/dev/null").empty() ? "clean" : "dirty");
    return kv;
}

std::unordered_map<std::string,std::string> collect_network_simple() {
    std::unordered_map<std::string,std::string> kv;
#if defined(__APPLE__)
    add(kv, "ip", run("ipconfig getifaddr en0 2>/dev/null"));
    if (kv.find("ip")==kv.end()) add(kv, "ip", run("ipconfig getifaddr en1 2>/dev/null"));
#endif
    return kv;
}

std::unordered_map<std::string,std::string> collect_battery_simple() {
    std::unordered_map<std::string,std::string> kv;
#if defined(__APPLE__)
    auto out = run("pmset -g batt 2>/dev/null | grep -Eo '\\d+%|discharging|charging|charged' | tr '\\n' ' '");
    add(kv, "battery", out);
#endif
    return kv;
}

std::unordered_map<std::string,std::string> collect_weather_simple() {
    std::unordered_map<std::string,std::string> kv;

    const char* envCity = std::getenv("CITY");
    std::string city = envCity ? envCity : "";

    std::string url = city.empty()
        ? "https://wttr.in/?format=4"
        : ("https://wttr.in/" + city + "?format=4");

    std::string out = run("curl -m 1 -s -L \"" + url + "\" 2>/dev/null");
    if (out.empty()) return kv;

    oneLine(out);
    // Split "City: weather"
    std::string parsedCity, weather;
    auto pos = out.find(':');
    if (pos != std::string::npos) {
        parsedCity = trim(out.substr(0, pos));
        weather    = trim(out.substr(pos + 1));
    } else {
        weather = out;
    }

    if (!weather.empty()) add(kv, "weather", weather);

    if (!parsedCity.empty()) add(kv, "city", parsedCity);
    else if (!city.empty())  add(kv, "city", city);

    return kv;
}

// Prompt Builder
class SystemPrompt {
public:
    explicit SystemPrompt(std::string assistantName = "Shelly")
        : name_(std::move(assistantName)) {}

    std::string build(const std::string& userInput) {
        auto ctx = gather();
        std::ostringstream out;
        out << "You are " << name_ << ", a fast terminal assistant. "
            << "Be concise. Use the context if relevant. Otherwise answer the question AI-assistant-like.\n";
        out << "Context: {";
        bool first = true;
        for (auto& [k,v] : ctx) {
            if (!first) out << ", ";
            first = false;
            auto vv = escape_json(v);
            out << "\"" << k << "\": \"" << vv << "\"";
        }
        out << "}\n\nUser: " << userInput;
        return out.str();
    }

    std::unordered_map<std::string,std::string> gather() {
        std::unordered_map<std::string,std::string> kv;
        auto add_all = [&](const auto& m){ kv.insert(m.begin(), m.end()); };

        add_all(collect_time());
        add_all(collect_user_host_cwd());
        add_all(collect_os_basic());
        add_all(collect_git_simple());
        add_all(collect_network_simple());
        add_all(collect_battery_simple());
        add_all(collect_weather_simple());
        return kv;
    }

private:
    std::string name_;
};

}