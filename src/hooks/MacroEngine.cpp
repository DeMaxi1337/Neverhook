#include "MacroEngine.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <fstream>

using namespace geode::prelude;

namespace {
    // File magic so we don't try to parse junk files as macros.
    constexpr char kMagic[4] = { 'N', 'H', 'M', '1' };
}

namespace nh {

MacroEngine& MacroEngine::get() {
    static MacroEngine inst;
    return inst;
}

void MacroEngine::startRecording() {
    inputs.clear();
    inputs.reserve(8192);  // pre-grow once so mid-run push_back never reallocs
    playbackIndex = 0;
    started = false;
    pendingRewind = false;
    state = MacroState::Recording;
}

void MacroEngine::startPlayback() {
    playbackIndex = 0;
    started = false;  // decided by the attempt gate in the hook
    state = MacroState::Playing;
}

void MacroEngine::stop() {
    state = MacroState::Idle;
    started = false;
}

void MacroEngine::onLevelReset() {
    if (state == MacroState::Recording) {
        // Do NOT wipe the whole recording here. On a full restart the next tick
        // is at frame 0, so recordTick drops everything anyway; but on a
        // practice checkpoint respawn we must keep every input BEFORE the
        // checkpoint and only re-record the part after it. recordTick handles
        // both once it sees the frame we rewound to.
        pendingRewind = true;
    } else if (state == MacroState::Playing) {
        // Rewind to the first input; the attempt gate re-decides `started`.
        playbackIndex = 0;
        started = false;
    }
}

void MacroEngine::recordInput(uint32_t frame, int button, bool down, bool player1) {
    if (state != MacroState::Recording) return;
    if (button < 1 || button > 3) return;
    inputs.push_back(MacroInput{ frame, static_cast<uint8_t>(button), down, player1 });
}

void MacroEngine::recordTick(uint32_t frame) {
    if (state != MacroState::Recording) return;
    if (!pendingRewind) return;
    pendingRewind = false;

    // We just reset / respawned to `frame`. Drop everything recorded at or
    // after it so this pass re-records cleanly from here. Inputs are stored in
    // ascending frame order and MacroInput is trivially destructible, so we do
    // one binary search + a single truncation instead of popping element by
    // element. That keeps a full restart (frame 0 -> whole buffer) from
    // stalling a frame, which is what caused the post-death hitch.
    if (frame == 0) {
        inputs.clear();  // keeps capacity; no realloc next attempt
        return;
    }
    auto cut = std::lower_bound(
        inputs.begin(), inputs.end(), frame,
        [](const MacroInput& in, uint32_t f) { return in.frame < f; });
    inputs.erase(cut, inputs.end());
}

// -----------------------------------------------------------------------------
// Persistence
// -----------------------------------------------------------------------------

std::filesystem::path MacroEngine::macrosDir() {
    // Pure path, no I/O -- this is called from the render loop, so it must be
    // cheap. Directory creation happens lazily in save().
    return Mod::get()->getConfigDir() / "macros";
}

std::filesystem::path MacroEngine::macroPath(const std::string& name) {
    return macrosDir() / (name + ".nhm");
}

bool MacroEngine::save(const std::string& name) {
    if (name.empty()) return false;

    std::error_code ec;
    std::filesystem::create_directories(macrosDir(), ec);

    std::ofstream fd(macroPath(name), std::ios::binary);
    if (!fd) {
        log::error("macro: failed to open {} for writing", name);
        return false;
    }

    uint32_t count = static_cast<uint32_t>(inputs.size());
    fd.write(kMagic, sizeof(kMagic));
    fd.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& in : inputs) {
        fd.write(reinterpret_cast<const char*>(&in.frame), sizeof(in.frame));
        fd.write(reinterpret_cast<const char*>(&in.button), sizeof(in.button));
        uint8_t d = in.down ? 1 : 0;
        uint8_t p = in.player1 ? 1 : 0;
        fd.write(reinterpret_cast<const char*>(&d), sizeof(d));
        fd.write(reinterpret_cast<const char*>(&p), sizeof(p));
    }

    currentName = name;
    log::info("macro: saved {} ({} actions)", name, count);
    return true;
}

bool MacroEngine::load(const std::string& name) {
    if (name.empty()) return false;

    std::ifstream fd(macroPath(name), std::ios::binary);
    if (!fd) {
        log::error("macro: failed to open {} for reading", name);
        return false;
    }

    char magic[4] = {};
    fd.read(magic, sizeof(magic));
    if (std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) {
        log::error("macro: {} has a bad header", name);
        return false;
    }

    uint32_t count = 0;
    fd.read(reinterpret_cast<char*>(&count), sizeof(count));

    std::vector<MacroInput> loaded;
    loaded.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        MacroInput in{};
        uint8_t d = 0, p = 0;
        fd.read(reinterpret_cast<char*>(&in.frame), sizeof(in.frame));
        fd.read(reinterpret_cast<char*>(&in.button), sizeof(in.button));
        fd.read(reinterpret_cast<char*>(&d), sizeof(d));
        fd.read(reinterpret_cast<char*>(&p), sizeof(p));
        if (!fd) break;
        in.down = d != 0;
        in.player1 = p != 0;
        loaded.push_back(in);
    }

    inputs = std::move(loaded);
    playbackIndex = 0;
    started = false;
    currentName = name;
    log::info("macro: loaded {} ({} actions)", name, inputs.size());
    return true;
}

std::vector<std::string> MacroEngine::listMacros() {
    std::vector<std::string> names;
    std::error_code ec;
    auto dir = macrosDir();
    for (auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".nhm") continue;
        names.push_back(entry.path().stem().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

void MacroEngine::openFolder() {
    (void)geode::utils::file::openFolder(macrosDir());
}

} // namespace nh
