
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <system_error>
#include <vector>

using namespace geode::prelude;

namespace nh::lua {
namespace {

constexpr int         kMaxSounds    = 32;
constexpr int         kMaxImages    = 32;
constexpr int         kMaxDownloads = 8;
constexpr std::size_t kMaxImageSize = 8u * 1024u * 1024u;
constexpr std::size_t kMaxUrlLength = 2048;

struct SoundSlot {
    int           handle = 0;
    std::string   owner;
    FMOD::Sound*  sound   = nullptr;
    FMOD::Channel* channel = nullptr;
    bool          pending = false;
    float         volume  = 1.f;
    bool          dead    = false;
};

struct ImageSlot {
    int             handle = 0;
    std::string     owner;
    CCTexture2D*    texture = nullptr;
    bool            ready   = false;
    bool            failed  = false;
    std::string     file;
};

struct DownloadResult {
    std::mutex  mutex;
    bool        finished = false;
    bool        ok       = false;
    std::string body;
};

struct PendingDownload {
    std::shared_ptr<DownloadResult> result = std::make_shared<DownloadResult>();
    int         imageHandle = -1;
    std::string owner;
    std::string url;
    bool        done = false;
};

std::vector<SoundSlot> g_sounds;
std::vector<ImageSlot> g_images;
std::vector<std::unique_ptr<PendingDownload>> g_downloads;

int g_nextSound = 1;
int g_nextImage = 1;

std::string ownerId(lua_State* L) {
    Script* s = ownerOf(L);
    return s ? s->id() : std::string();
}

void logFor(lua_State* L, const std::string& text) {
    Script* s = ownerOf(L);
    Manager::get().log((s ? s->fileName() + ": " : std::string()) + text);
}

SoundSlot* soundByHandle(int handle) {
    for (auto& s : g_sounds)
        if (s.handle == handle && !s.dead) return &s;
    return nullptr;
}

ImageSlot* imageByHandle(int handle) {
    for (auto& i : g_images)
        if (i.handle == handle) return &i;
    return nullptr;
}

bool hasExtension(const std::string& name, std::initializer_list<const char*> list) {
    const auto dot = name.find_last_of('.');
    if (dot == std::string::npos) return false;

    std::string ext = name.substr(dot);
    for (char& c : ext) c = (char)std::tolower((unsigned char)c);

    for (const char* candidate : list)
        if (ext == candidate) return true;

    return false;
}

std::string resolveLocal(lua_State* L, const char* name, bool sound) {
    const std::string file = name ? name : "";

    if (file.empty() || file.size() > 128)
        luaL_error(L, "invalid file name");

    if (file.find('/')  != std::string::npos ||
        file.find('\\') != std::string::npos ||
        file.find("..") != std::string::npos ||
        file.find(':')  != std::string::npos)
        luaL_error(L, "only plain file names are allowed, no paths");

    const bool ok = sound
        ? hasExtension(file, { ".mp3", ".ogg", ".wav", ".flac" })
        : hasExtension(file, { ".png", ".jpg", ".jpeg" });

    if (!ok)
        luaL_error(L, sound ? "sounds must be .mp3, .ogg, .wav or .flac"
                            : "images must be .png, .jpg or .jpeg");

    const auto folder = sound ? Manager::get().soundsDir() : Manager::get().imagesDir();
    return geode::utils::string::pathToString(folder / file);
}

std::string checkUrl(lua_State* L, const char* raw) {
    const std::string url = raw ? raw : "";

    if (url.size() > kMaxUrlLength)
        luaL_error(L, "that URL is too long");

    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0)
        luaL_error(L, "only http:// and https:// URLs are allowed");

    return url;
}

FMODAudioEngine* audioEngine() { return FMODAudioEngine::sharedEngine(); }

int createSoundSlot(lua_State* L, const std::string& target, bool stream) {
    if ((int)g_sounds.size() >= kMaxSounds)
        return luaL_error(L, "too many sounds are open (%d max)", kMaxSounds);

    FMODAudioEngine* engine = audioEngine();
    if (!engine || !engine->m_system) {
        lua_pushnil(L);
        return 1;
    }

    FMOD_MODE mode = FMOD_2D | FMOD_LOOP_OFF;
    if (stream) mode |= FMOD_CREATESTREAM | FMOD_NONBLOCKING;

    FMOD::Sound* sound = nullptr;
    const auto result = engine->m_system->createSound(target.c_str(), mode, nullptr, &sound);

    if (result != FMOD_OK || !sound) {
        lua_pushnil(L);
        return 1;
    }

    SoundSlot slot;
    slot.handle  = g_nextSound++;
    slot.owner   = ownerId(L);
    slot.sound   = sound;
    slot.pending = stream;

    if (!stream)
        engine->m_system->playSound(sound, nullptr, false, &slot.channel);

    g_sounds.push_back(slot);

    lua_pushinteger(L, slot.handle);
    return 1;
}

int l_audio_play_local(lua_State* L) {
    const std::string path = resolveLocal(L, luaL_checkstring(L, 1), true);
    return createSoundSlot(L, path, false);
}

int l_audio_play_url(lua_State* L) {
    const std::string url = checkUrl(L, luaL_checkstring(L, 1));
    logFor(L, "streaming audio from " + url);
    return createSoundSlot(L, url, true);
}

int l_audio_play_sfx(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    if (std::strchr(name, '/') || std::strchr(name, '\\') || std::strstr(name, ".."))
        return luaL_error(L, "only plain file names are allowed, no paths");

    if (auto* engine = audioEngine())
        engine->playEffect(name);

    return 0;
}

void killSound(SoundSlot& slot) {
    if (slot.channel) slot.channel->stop();
    if (slot.sound)   slot.sound->release();

    slot.channel = nullptr;
    slot.sound   = nullptr;
    slot.dead    = true;
}

int l_audio_stop(lua_State* L) {
    SoundSlot* slot = soundByHandle((int)luaL_checkinteger(L, 1));
    if (slot) killSound(*slot);
    return 0;
}

int l_audio_stop_all(lua_State* L) {
    const std::string owner = ownerId(L);
    for (auto& slot : g_sounds)
        if (!slot.dead && slot.owner == owner) killSound(slot);
    return 0;
}

int l_audio_set_volume(lua_State* L) {
    SoundSlot* slot = soundByHandle((int)luaL_checkinteger(L, 1));
    if (!slot) return 0;

    double v = luaL_checknumber(L, 2);
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;

    slot->volume = (float)v;
    if (slot->channel) slot->channel->setVolume(slot->volume);
    return 0;
}

int l_audio_is_playing(lua_State* L) {
    SoundSlot* slot = soundByHandle((int)luaL_checkinteger(L, 1));

    bool playing = false;
    if (slot) {
        if (slot->pending) {
            playing = true;
        }
        else if (slot->channel) {
            bool value = false;
            if (slot->channel->isPlaying(&value) == FMOD_OK) playing = value;
        }
    }

    lua_pushboolean(L, playing);
    return 1;
}

std::string hashOf(const std::string& text) {
    std::uint64_t hash = 1469598103934665603ull;
    for (unsigned char c : text) {
        hash ^= c;
        hash *= 1099511628211ull;
    }

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%016llx", (unsigned long long)hash);
    return buffer;
}

int newImageSlot(lua_State* L) {
    ImageSlot slot;
    slot.handle = g_nextImage++;
    slot.owner  = ownerId(L);
    g_images.push_back(slot);
    return slot.handle;
}

int l_draw_image_local(lua_State* L) {
    if ((int)g_images.size() >= kMaxImages)
        return luaL_error(L, "too many images are loaded (%d max)", kMaxImages);

    const std::string path = resolveLocal(L, luaL_checkstring(L, 1), false);

    const int  handle = newImageSlot(L);
    ImageSlot* slot   = imageByHandle(handle);

    CCTexture2D* tex = CCTextureCache::sharedTextureCache()->addImage(path.c_str(), false);
    if (tex) {
        tex->retain();
        slot->texture = tex;
        slot->ready   = true;
        slot->file    = path;
    }
    else {
        slot->failed = true;
    }

    lua_pushinteger(L, handle);
    return 1;
}

void finishDownload(PendingDownload* pending, const std::string& body) {
    ImageSlot* slot = imageByHandle(pending->imageHandle);
    if (!slot) return;

    if (body.empty() || body.size() > kMaxImageSize) {
        slot->failed = true;
        return;
    }

    const bool jpeg = pending->url.find(".jpg")  != std::string::npos ||
                      pending->url.find(".jpeg") != std::string::npos;

    const auto file = Manager::get().imagesDir() / "cache" /
                      (hashOf(pending->url) + (jpeg ? ".jpg" : ".png"));

    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);

    const ByteVector bytes(body.begin(), body.end());
    const auto written = geode::utils::file::writeBinary(file, bytes);
    if (!written) {
        slot->failed = true;
        return;
    }

    const auto path = geode::utils::string::pathToString(file);

    CCTexture2D* tex = CCTextureCache::sharedTextureCache()->addImage(path.c_str(), false);
    if (!tex) {
        slot->failed = true;
        return;
    }

    tex->retain();
    slot->texture = tex;
    slot->ready   = true;
    slot->file    = path;
}

int l_draw_image_url(lua_State* L) {
    if ((int)g_images.size() >= kMaxImages)
        return luaL_error(L, "too many images are loaded (%d max)", kMaxImages);

    if ((int)g_downloads.size() >= kMaxDownloads)
        return luaL_error(L, "too many downloads are running at once");

    const std::string url = checkUrl(L, luaL_checkstring(L, 1));
    logFor(L, "downloading image from " + url);

    const int handle = newImageSlot(L);

    auto pending = std::make_unique<PendingDownload>();
    pending->imageHandle = handle;
    pending->owner       = ownerId(L);
    pending->url         = url;

    std::shared_ptr<DownloadResult> shared = pending->result;

    std::thread([shared, url]() {
        web::WebRequest request;
        request.timeout(std::chrono::seconds(20));

        web::WebResponse response = request.getSync(url);

        const bool  ok   = response.ok();
        std::string body = ok ? response.string().unwrapOr(std::string()) : std::string();

        std::lock_guard<std::mutex> lock(shared->mutex);
        shared->ok       = ok;
        shared->body     = std::move(body);
        shared->finished = true;
    }).detach();

    g_downloads.push_back(std::move(pending));

    lua_pushinteger(L, handle);
    return 1;
}

int l_draw_image_path(lua_State* L) {
    ImageSlot* slot = imageByHandle((int)luaL_checkinteger(L, 1));
    if (!slot || !slot->ready || slot->file.empty()) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, slot->file.c_str());
    return 1;
}

int l_draw_image_ready(lua_State* L) {
    ImageSlot* slot = imageByHandle((int)luaL_checkinteger(L, 1));

    lua_pushboolean(L, slot && slot->ready);
    lua_pushboolean(L, slot && slot->failed);
    return 2;
}

int l_draw_image_size(lua_State* L) {
    ImageSlot* slot = imageByHandle((int)luaL_checkinteger(L, 1));
    if (!slot || !slot->ready || !slot->texture) {
        lua_pushnil(L);
        return 1;
    }

    const CCSize size = slot->texture->getContentSize();
    lua_pushnumber(L, size.width);
    lua_pushnumber(L, size.height);
    return 2;
}

int l_draw_image(lua_State* L) {
    ImageSlot* slot = imageByHandle((int)luaL_checkinteger(L, 1));
    if (!slot || !slot->ready || !slot->texture) return 0;

    const float x = (float)luaL_checknumber(L, 2);
    const float y = (float)luaL_checknumber(L, 3);

    const CCSize native = slot->texture->getContentSize();
    const float  w = (float)luaL_optnumber(L, 4, native.width);
    const float  h = (float)luaL_optnumber(L, 5, native.height);

    double alpha = luaL_optnumber(L, 6, 255.0);
    if (alpha < 0.0)   alpha = 0.0;
    if (alpha > 255.0) alpha = 255.0;

    const ImU32 tint = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, (float)(alpha / 255.0)));

    ImGui::GetForegroundDrawList()->AddImage(
        (ImTextureID)(std::intptr_t)slot->texture->getName(),
        ImVec2(x, y), ImVec2(x + w, y + h),
        ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), tint);
    return 0;
}

void freeImageSlot(ImageSlot& slot) {
    if (slot.texture) {
        slot.texture->release();
        slot.texture = nullptr;
    }
    slot.ready  = false;
    slot.failed = true;
}

int l_draw_image_free(lua_State* L) {
    const int handle = (int)luaL_checkinteger(L, 1);

    for (std::size_t i = 0; i < g_images.size(); ++i) {
        if (g_images[i].handle != handle) continue;

        freeImageSlot(g_images[i]);
        g_images.erase(g_images.begin() + (long)i);
        break;
    }
    return 0;
}

void addFunctions(lua_State* L, const char* global, const luaL_Reg* fns) {
    lua_getglobal(L, global);
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }

    for (const luaL_Reg* f = fns; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_pop(L, 1);
}

}

void updateMedia() {

    FMODAudioEngine* engine = audioEngine();

    for (auto& slot : g_sounds) {
        if (slot.dead || !slot.pending || !slot.sound) continue;

        FMOD_OPENSTATE state = FMOD_OPENSTATE_READY;
        unsigned int   percent = 0;
        bool           starving = false;
        bool           busy = false;

        if (slot.sound->getOpenState(&state, &percent, &starving, &busy) != FMOD_OK)
            continue;

        if (state == FMOD_OPENSTATE_READY) {
            slot.pending = false;
            if (engine && engine->m_system) {
                engine->m_system->playSound(slot.sound, nullptr, false, &slot.channel);
                if (slot.channel) slot.channel->setVolume(slot.volume);
            }
        }
        else if (state == FMOD_OPENSTATE_ERROR) {
            slot.pending = false;
            killSound(slot);
        }
    }

    for (auto& slot : g_sounds) {
        if (slot.dead || slot.pending || !slot.channel) continue;

        bool playing = false;
        if (slot.channel->isPlaying(&playing) != FMOD_OK || !playing)
            killSound(slot);
    }

    g_sounds.erase(
        std::remove_if(g_sounds.begin(), g_sounds.end(),
            [](const SoundSlot& s) { return s.dead; }),
        g_sounds.end());

    for (auto& download : g_downloads) {
        if (!download || download->done) continue;

        bool        ok = false;
        std::string body;
        {
            std::lock_guard<std::mutex> lock(download->result->mutex);
            if (!download->result->finished) continue;

            ok   = download->result->ok;
            body = std::move(download->result->body);
        }

        if (ok) finishDownload(download.get(), body);
        else if (ImageSlot* slot = imageByHandle(download->imageHandle)) slot->failed = true;

        download->done = true;
    }

    g_downloads.erase(
        std::remove_if(g_downloads.begin(), g_downloads.end(),
            [](const std::unique_ptr<PendingDownload>& d) { return !d || d->done; }),
        g_downloads.end());
}

void releaseScriptMedia(const std::string& owner) {
    for (auto& slot : g_sounds)
        if (slot.owner == owner) killSound(slot);

    g_sounds.erase(
        std::remove_if(g_sounds.begin(), g_sounds.end(),
            [](const SoundSlot& s) { return s.dead; }),
        g_sounds.end());

    for (auto& slot : g_images)
        if (slot.owner == owner) freeImageSlot(slot);

    g_images.erase(
        std::remove_if(g_images.begin(), g_images.end(),
            [&](const ImageSlot& i) { return i.owner == owner; }),
        g_images.end());

    for (auto& download : g_downloads) {
        if (download && download->owner == owner) {
            download->imageHandle = -1;
            download->done        = true;
        }
    }
}

void registerMediaApi(lua_State* L) {
    static const luaL_Reg kAudio[] = {
        { "play_local", l_audio_play_local },
        { "play_url",   l_audio_play_url   },
        { "play_sfx",   l_audio_play_sfx   },
        { "stop",       l_audio_stop       },
        { "stop_all",   l_audio_stop_all   },
        { "set_volume", l_audio_set_volume },
        { "is_playing", l_audio_is_playing },
        { nullptr,      nullptr            },
    };

    lua_newtable(L);
    for (const luaL_Reg* f = kAudio; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, "audio");

    static const luaL_Reg kImages[] = {
        { "image_local", l_draw_image_local },
        { "image_url",   l_draw_image_url   },
        { "image_ready", l_draw_image_ready },
        { "image_size",  l_draw_image_size  },
        { "image_path",  l_draw_image_path  },
        { "image",       l_draw_image       },
        { "image_free",  l_draw_image_free  },
        { nullptr,       nullptr            },
    };
    addFunctions(L, "draw", kImages);
}

}
