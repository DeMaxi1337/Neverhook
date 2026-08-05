#include "EndscreenPhrases.hpp"

#include <algorithm>
#include <cctype>
#include <random>

namespace nh::phrases {

const std::vector<std::string>& pool() {
    static const std::vector<std::string> kPool = {
        "No Aimware users here. Get Neverhook.",
        "Neverhook - best features without compromising security.",
        "Neverhook > all. Refund your copy-pasted trash.",
        "server cvar 'sv_rekt' changed to 1.",
        "This server is VAC secured. Get Neverhook.",
        "STAY BLUHGANG $",
        "Imagine paying for a paste mod menu. Neverhook on top.",
        "by SANCHEZj hvh boss.",
        "Get good. Get Neverhook.",
    };
    return kPool;
}

std::string random() {
    const auto& p = pool();

    if (p.empty())
        return "";
    if (p.size() == 1)
        return p[0];

    static std::mt19937 rng(std::random_device{}());
    static std::size_t lastIndex = static_cast<std::size_t>(-1);

    std::uniform_int_distribution<std::size_t> dist(0, p.size() - 1);

    std::size_t index = dist(rng);
    while (index == lastIndex)
        index = dist(rng);

    lastIndex = index;
    return p[index];
}

bool isReservedMessage(const std::string& msg) {
    if (msg.empty())
        return false;

    std::string lower = msg;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    static const char* kKeys[] = {
        "verified",
        "collect all coins",
        "start pos",
        "startpos",
    };

    for (const char* key : kKeys) {
        if (lower.find(key) != std::string::npos)
            return true;
    }

    return false;
}

}
