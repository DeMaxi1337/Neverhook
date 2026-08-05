
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using namespace geode::prelude;

namespace nh::lua {
namespace {

constexpr std::size_t kMaxFileSize    = 4u * 1024u * 1024u;
constexpr std::size_t kMaxPathLength  = 160;
constexpr int         kMaxListEntries = 512;

constexpr int         kMaxRequests    = 8;
constexpr std::size_t kMaxUrlLength   = 2048;
constexpr std::size_t kMaxBodySize    = 4u * 1024u * 1024u;
constexpr int         kMaxHeaders     = 16;

char kImportCacheKey = 0;

std::string ownerId(lua_State* L) {
    Script* script = ownerOf(L);
    return script ? script->id() : std::string();
}

Script* scriptById(const std::string& id) {
    for (auto& script : Manager::get().scripts())
        if (script && script->id() == id) return script.get();
    return nullptr;
}

void logFor(lua_State* L, const std::string& text) {
    Script* script = ownerOf(L);
    Manager::get().log((script ? script->fileName() : std::string("script")) + ": " + text);
}

std::filesystem::path dataRoot() {
    return Manager::get().dir() / "data";
}

std::filesystem::path libRoot() {
    return Manager::get().dir() / "lib";
}

void ensureDir(const std::filesystem::path& path) {
    std::error_code code;
    if (!std::filesystem::exists(path, code))
        std::filesystem::create_directories(path, code);
}

bool safeRelative(const char* raw) {
    if (!raw || !*raw) return false;

    const std::size_t length = std::strlen(raw);
    if (length > kMaxPathLength) return false;

    if (raw[0] == '/' || raw[0] == '\\') return false;
    if (std::strchr(raw, '\\')) return false;
    if (std::strchr(raw, ':'))  return false;
    if (std::strstr(raw, "..")) return false;

    bool lastWasSlash = true;
    for (std::size_t i = 0; i < length; ++i) {
        const char c = raw[i];

        if (c == '/') {
            if (lastWasSlash) return false;
            lastWasSlash = true;
            continue;
        }

        if (c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            return false;

        lastWasSlash = false;
    }

    return !lastWasSlash;
}

bool hasAllowedExtension(const std::string& name) {
    static const char* kAllowed[] = {
        ".txt", ".json", ".csv", ".log", ".ini", ".dat", ".md", ".cfg",
    };

    const std::size_t dot = name.find_last_of('.');
    if (dot == std::string::npos) return false;

    std::string extension = name.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });

    for (const char* allowed : kAllowed)
        if (extension == allowed) return true;

    return false;
}

std::filesystem::path dataPath(lua_State* L, int index) {
    const char* raw = luaL_checkstring(L, index);

    if (!safeRelative(raw)) {
        luaL_error(L, "'%s' is not a valid file name (no .. and no absolute paths)", raw);
        return {};
    }

    const std::string name(raw);
    if (!hasAllowedExtension(name)) {
        luaL_error(L, "only .txt, .json, .csv, .log, .ini, .dat, .md and .cfg files "
                      "can be used");
        return {};
    }

    return dataRoot() / std::filesystem::path(name);
}

std::filesystem::path dataDir(lua_State* L, int index) {
    if (lua_isnoneornil(L, index)) return dataRoot();

    const char* raw = luaL_checkstring(L, index);
    if (!safeRelative(raw)) {
        luaL_error(L, "'%s' is not a valid folder name", raw);
        return {};
    }

    return dataRoot() / std::filesystem::path(raw);
}

int l_fs_read(lua_State* L) {
    const std::filesystem::path path = dataPath(L, 1);

    std::error_code code;
    if (!std::filesystem::exists(path, code)) {
        lua_pushnil(L);
        lua_pushstring(L, "no such file");
        return 2;
    }

    const auto size = std::filesystem::file_size(path, code);
    if (code) {
        lua_pushnil(L);
        lua_pushstring(L, "that file could not be read");
        return 2;
    }

    if (size > kMaxFileSize) {
        lua_pushnil(L);
        lua_pushstring(L, "that file is too big (4 MB max)");
        return 2;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        lua_pushnil(L);
        lua_pushstring(L, "that file could not be opened");
        return 2;
    }

    std::string body;
    body.resize((std::size_t)size);
    if (size > 0) stream.read(body.data(), (std::streamsize)size);

    const std::size_t read = (std::size_t)stream.gcount();
    body.resize(read);

    lua_pushlstring(L, body.data(), body.size());
    return 1;
}

int writeFile(lua_State* L, bool append) {
    const std::filesystem::path path = dataPath(L, 1);

    std::size_t length = 0;
    const char* text = luaL_checklstring(L, 2, &length);

    if (length > kMaxFileSize)
        return luaL_error(L, "that is too much data to write (4 MB max)");

    ensureDir(path.parent_path());

    std::ios::openmode mode = std::ios::binary | std::ios::out;
    mode |= append ? std::ios::app : std::ios::trunc;

    std::ofstream stream(path, mode);
    if (!stream) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "that file could not be opened for writing");
        return 2;
    }

    if (length > 0) stream.write(text, (std::streamsize)length);
    stream.close();

    lua_pushboolean(L, 1);
    return 1;
}

int l_fs_write(lua_State* L)  { return writeFile(L, false); }
int l_fs_append(lua_State* L) { return writeFile(L, true); }

int l_fs_exists(lua_State* L) {
    const std::filesystem::path path = dataPath(L, 1);

    std::error_code code;
    lua_pushboolean(L, std::filesystem::exists(path, code) ? 1 : 0);
    return 1;
}

int l_fs_size(lua_State* L) {
    const std::filesystem::path path = dataPath(L, 1);

    std::error_code code;
    const auto size = std::filesystem::file_size(path, code);

    if (code) lua_pushnil(L);
    else      lua_pushnumber(L, (double)size);
    return 1;
}

int l_fs_remove(lua_State* L) {
    const std::filesystem::path path = dataPath(L, 1);

    std::error_code code;
    const bool removed = std::filesystem::remove(path, code);

    lua_pushboolean(L, (removed && !code) ? 1 : 0);
    return 1;
}

int l_fs_mkdir(lua_State* L) {
    const std::filesystem::path path = dataDir(L, 1);

    ensureDir(path);

    std::error_code code;
    lua_pushboolean(L, std::filesystem::exists(path, code) ? 1 : 0);
    return 1;
}

int listEntries(lua_State* L, bool wantDirectories) {
    const std::filesystem::path path = dataDir(L, 1);

    ensureDir(dataRoot());
    lua_newtable(L);

    std::error_code code;
    if (!std::filesystem::exists(path, code)) return 1;

    int written = 0;
    std::filesystem::directory_iterator it(path, code);
    if (code) return 1;

    for (const auto& entry : it) {
        if (written >= kMaxListEntries) break;

        std::error_code entryCode;
        const bool isDirectory = entry.is_directory(entryCode);
        if (entryCode) continue;
        if (isDirectory != wantDirectories) continue;

        const std::string name = geode::utils::string::pathToString(entry.path().filename());
        lua_pushlstring(L, name.data(), name.size());
        lua_rawseti(L, -2, ++written);
    }

    return 1;
}

int l_fs_list(lua_State* L)      { return listEntries(L, false); }
int l_fs_list_dirs(lua_State* L) { return listEntries(L, true); }

int l_fs_folder(lua_State* L) {
    ensureDir(dataRoot());

    const std::string path = geode::utils::string::pathToString(dataRoot());
    lua_pushlstring(L, path.data(), path.size());
    return 1;
}

int l_fs_open_folder(lua_State* L) {
    ensureDir(dataRoot());
    geode::utils::file::openFolder(dataRoot());

    logFor(L, "opened its data folder");
    return 0;
}

struct NetShared {
    std::mutex  mutex;
    bool        finished = false;
    bool        ok       = false;
    int         code     = 0;
    std::string body;
};

struct NetRequest {
    int                        handle = 0;
    std::string                owner;
    std::string                url;
    std::shared_ptr<NetShared> shared;
    int                        callbackRef = -1;
    bool                       finished    = false;
    bool                       ok          = false;
    int                        code        = 0;
    std::string                body;
};

std::vector<std::unique_ptr<NetRequest>> g_requests;
int                                      g_nextRequest = 1;

NetRequest* requestByHandle(int handle) {
    for (auto& request : g_requests)
        if (request && request->handle == handle) return request.get();
    return nullptr;
}

bool checkUrl(lua_State* L, const char* url) {
    if (!url || !*url) {
        luaL_error(L, "that URL is empty");
        return false;
    }

    if (std::strlen(url) > kMaxUrlLength) {
        luaL_error(L, "that URL is too long");
        return false;
    }

    if (std::strncmp(url, "http://", 7) != 0 && std::strncmp(url, "https://", 8) != 0) {
        luaL_error(L, "only http:// and https:// URLs are allowed");
        return false;
    }

    return true;
}

int activeRequests() {
    int active = 0;
    for (const auto& request : g_requests)
        if (request && !request->finished) ++active;
    return active;
}

void readHeaders(lua_State* L, int index, std::vector<std::pair<std::string, std::string>>& out) {
    if (lua_isnoneornil(L, index)) return;
    if (!lua_istable(L, index)) return;

    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        if ((int)out.size() >= kMaxHeaders) {
            lua_pop(L, 2);
            break;
        }

        if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TSTRING)
            out.emplace_back(lua_tostring(L, -2), lua_tostring(L, -1));

        lua_pop(L, 1);
    }
}

int startRequest(lua_State* L, const std::string& method, const std::string& url,
                 const std::string& body,
                 const std::vector<std::pair<std::string, std::string>>& headers,
                 int callbackRef, int timeoutSeconds) {
    if (activeRequests() >= kMaxRequests) {
        if (callbackRef >= 0) {
            Script* script = ownerOf(L);
            if (script) script->freeRef(callbackRef);
        }
        return luaL_error(L, "too many requests are running at once (%d max)", kMaxRequests);
    }

    auto request = std::make_unique<NetRequest>();
    request->handle      = g_nextRequest++;
    request->owner       = ownerId(L);
    request->url         = url;
    request->shared      = std::make_shared<NetShared>();
    request->callbackRef = callbackRef;

    const int handle = request->handle;
    auto shared      = request->shared;

    logFor(L, method + " " + url);

    std::thread([shared, method, url, body, headers, timeoutSeconds]() {
        web::WebRequest webRequest;
        webRequest.timeout(std::chrono::seconds(timeoutSeconds));

        for (const auto& header : headers)
            webRequest.header(header.first, header.second);

        if (!body.empty()) webRequest.bodyString(body);

        web::WebResponse response = webRequest.sendSync(method, url);

        const bool  ok       = response.ok();
        const int   code     = response.code();
        std::string received = response.string().unwrapOr(std::string());

        if (received.size() > kMaxBodySize) received.resize(kMaxBodySize);

        std::lock_guard<std::mutex> lock(shared->mutex);
        shared->ok       = ok;
        shared->code     = code;
        shared->body     = std::move(received);
        shared->finished = true;
    }).detach();

    g_requests.push_back(std::move(request));

    lua_pushinteger(L, handle);
    return 1;
}

int takeCallback(lua_State* L, int index) {
    if (lua_isnoneornil(L, index)) return -1;

    luaL_checktype(L, index, LUA_TFUNCTION);
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

int clampTimeout(lua_State* L, int index) {
    int seconds = (int)luaL_optinteger(L, index, 20);

    if (seconds < 1)   seconds = 1;
    if (seconds > 120) seconds = 120;

    return seconds;
}

int l_net_get(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    if (!checkUrl(L, url)) return 0;

    const int ref = takeCallback(L, 2);

    std::vector<std::pair<std::string, std::string>> headers;
    readHeaders(L, 3, headers);

    return startRequest(L, "GET", url, std::string(), headers, ref, 20);
}

int l_net_post(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    if (!checkUrl(L, url)) return 0;

    std::size_t length = 0;
    const char* body   = luaL_checklstring(L, 2, &length);

    if (length > kMaxBodySize)
        return luaL_error(L, "that request body is too big (4 MB max)");

    const int ref = takeCallback(L, 3);

    std::vector<std::pair<std::string, std::string>> headers;
    readHeaders(L, 4, headers);

    return startRequest(L, "POST", url, std::string(body, length), headers, ref, 20);
}

int l_net_request(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "url");
    const char* url = lua_tostring(L, -1);
    if (!checkUrl(L, url)) return 0;
    const std::string urlCopy(url);
    lua_pop(L, 1);

    lua_getfield(L, 1, "method");
    std::string method = lua_isstring(L, -1) ? lua_tostring(L, -1) : "GET";
    lua_pop(L, 1);

    std::transform(method.begin(), method.end(), method.begin(),
        [](unsigned char c) { return (char)std::toupper(c); });

    if (method != "GET" && method != "POST" && method != "PUT" && method != "PATCH"
        && method != "DELETE" && method != "HEAD")
        return luaL_error(L, "unsupported method '%s'", method.c_str());

    lua_getfield(L, 1, "body");
    std::string body;
    if (lua_isstring(L, -1)) {
        std::size_t length = 0;
        const char* text   = lua_tolstring(L, -1, &length);
        if (length > kMaxBodySize) {
            lua_pop(L, 1);
            return luaL_error(L, "that request body is too big (4 MB max)");
        }
        body.assign(text, length);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "timeout");
    int timeout = lua_isnumber(L, -1) ? (int)lua_tonumber(L, -1) : 20;
    if (timeout < 1)   timeout = 1;
    if (timeout > 120) timeout = 120;
    lua_pop(L, 1);

    std::vector<std::pair<std::string, std::string>> headers;
    lua_getfield(L, 1, "headers");
    readHeaders(L, lua_gettop(L), headers);
    lua_pop(L, 1);

    lua_getfield(L, 1, "callback");
    const int ref = takeCallback(L, lua_gettop(L));
    lua_pop(L, 1);

    return startRequest(L, method, urlCopy, body, headers, ref, timeout);
}

int l_net_done(lua_State* L) {
    NetRequest* request = requestByHandle((int)luaL_checkinteger(L, 1));
    lua_pushboolean(L, (request && request->finished) ? 1 : 0);
    return 1;
}

int l_net_status(lua_State* L) {
    NetRequest* request = requestByHandle((int)luaL_checkinteger(L, 1));
    if (!request) { lua_pushnil(L); return 1; }

    if (!request->finished) lua_pushstring(L, "pending");
    else                    lua_pushstring(L, request->ok ? "ok" : "error");
    return 1;
}

int l_net_code(lua_State* L) {
    NetRequest* request = requestByHandle((int)luaL_checkinteger(L, 1));
    lua_pushinteger(L, request ? request->code : 0);
    return 1;
}

int l_net_body(lua_State* L) {
    NetRequest* request = requestByHandle((int)luaL_checkinteger(L, 1));
    if (!request || !request->finished) { lua_pushnil(L); return 1; }

    lua_pushlstring(L, request->body.data(), request->body.size());
    return 1;
}

int l_net_free(lua_State* L) {
    const int handle = (int)luaL_checkinteger(L, 1);

    for (std::size_t i = 0; i < g_requests.size(); ++i) {
        if (!g_requests[i] || g_requests[i]->handle != handle) continue;
        if (!g_requests[i]->finished) break;

        g_requests.erase(g_requests.begin() + (long)i);
        break;
    }

    return 0;
}

int l_net_pending(lua_State* L) {
    lua_pushinteger(L, activeRequests());
    return 1;
}

int l_net_encode(lua_State* L) {
    std::size_t length = 0;
    const char* text   = luaL_checklstring(L, 1, &length);

    std::string out;
    out.reserve(length * 3);

    static const char* kHex = "0123456789ABCDEF";

    for (std::size_t i = 0; i < length; ++i) {
        const unsigned char c = (unsigned char)text[i];

        const bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';

        if (safe) {
            out.push_back((char)c);
            continue;
        }

        out.push_back('%');
        out.push_back(kHex[(c >> 4) & 0xF]);
        out.push_back(kHex[c & 0xF]);
    }

    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

void pushImportCache(lua_State* L) {
    lua_pushlightuserdata(L, &kImportCacheKey);
    lua_rawget(L, LUA_REGISTRYINDEX);

    if (lua_istable(L, -1)) return;

    lua_pop(L, 1);
    lua_newtable(L);

    lua_pushlightuserdata(L, &kImportCacheKey);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

int l_script_import(lua_State* L) {
    const char* raw = luaL_checkstring(L, 1);

    if (!safeRelative(raw) || std::strchr(raw, '.'))
        return luaL_error(L, "'%s' is not a valid library name", raw);

    const std::string name(raw);

    pushImportCache(L);
    lua_getfield(L, -1, name.c_str());
    if (!lua_isnil(L, -1)) {

        lua_remove(L, -2);
        return 1;
    }
    lua_pop(L, 1);

    const std::filesystem::path path = libRoot() / (name + ".lua");

    std::error_code code;
    if (!std::filesystem::exists(path, code)) {
        lua_pop(L, 1);
        return luaL_error(L, "there is no scripts/lib/%s.lua", name.c_str());
    }

    const auto size = std::filesystem::file_size(path, code);
    if (code || size > kMaxFileSize) {
        lua_pop(L, 1);
        return luaL_error(L, "scripts/lib/%s.lua could not be read", name.c_str());
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        lua_pop(L, 1);
        return luaL_error(L, "scripts/lib/%s.lua could not be opened", name.c_str());
    }

    std::string source;
    source.resize((std::size_t)size);
    if (size > 0) stream.read(source.data(), (std::streamsize)size);
    source.resize((std::size_t)stream.gcount());

    if (!source.empty() && (unsigned char)source[0] == 0x1B) {
        lua_pop(L, 1);
        return luaL_error(L, "precompiled bytecode is not allowed, ship plain .lua source");
    }

    const std::string chunk = "@lib/" + name + ".lua";

    if (luaL_loadbufferx(L, source.data(), source.size(), chunk.c_str(), "t") != 0) {
        const char* message = lua_tostring(L, -1);
        const std::string copy = message ? message : "syntax error";
        lua_pop(L, 2);
        return luaL_error(L, "%s", copy.c_str());
    }

    lua_call(L, 0, 1);

    if (lua_isnil(L, -1)) {

        lua_pop(L, 1);
        lua_pushboolean(L, 1);
    }

    lua_pushvalue(L, -1);
    lua_setfield(L, -3, name.c_str());
    lua_remove(L, -2);
    return 1;
}

int l_script_name(lua_State* L) {
    Script* script = ownerOf(L);
    if (!script) { lua_pushnil(L); return 1; }

    const std::string name = script->fileName();
    lua_pushlstring(L, name.data(), name.size());
    return 1;
}

int l_script_id(lua_State* L) {
    Script* script = ownerOf(L);
    if (!script) { lua_pushnil(L); return 1; }

    const std::string id = script->id();
    lua_pushlstring(L, id.data(), id.size());
    return 1;
}

int l_script_folder(lua_State* L) {
    const std::string path = geode::utils::string::pathToString(Manager::get().dir());
    lua_pushlstring(L, path.data(), path.size());
    return 1;
}

int l_script_libs(lua_State* L) {
    ensureDir(libRoot());
    lua_newtable(L);

    std::error_code code;
    std::filesystem::directory_iterator it(libRoot(), code);
    if (code) return 1;

    int written = 0;
    for (const auto& entry : it) {
        if (written >= kMaxListEntries) break;

        std::error_code entryCode;
        if (entry.is_directory(entryCode) || entryCode) continue;

        const std::filesystem::path file = entry.path();
        if (geode::utils::string::pathToString(file.extension()) != ".lua") continue;

        const std::string stem = geode::utils::string::pathToString(file.stem());
        lua_pushlstring(L, stem.data(), stem.size());
        lua_rawseti(L, -2, ++written);
    }

    return 1;
}

int l_script_log(lua_State* L) {
    const char* text  = luaL_checkstring(L, 1);
    const bool  error = lua_toboolean(L, 2) != 0;

    Script* script = ownerOf(L);
    Manager::get().log(
        (script ? script->fileName() : std::string("script")) + ": " + text, error);
    return 0;
}

void registerTable(lua_State* L, const char* name, const luaL_Reg* fns) {
    lua_newtable(L);
    for (const luaL_Reg* f = fns; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, name);
}

}

void updateNet() {
    for (auto& request : g_requests) {
        if (!request || request->finished) continue;

        bool        ready = false;
        bool        ok    = false;
        int         code  = 0;
        std::string body;

        {
            std::lock_guard<std::mutex> lock(request->shared->mutex);
            if (request->shared->finished) {
                ready = true;
                ok    = request->shared->ok;
                code  = request->shared->code;
                body  = request->shared->body;
            }
        }

        if (!ready) continue;

        request->finished = true;
        request->ok       = ok;
        request->code     = code;
        request->body     = std::move(body);

        if (request->callbackRef < 0) continue;

        Script* script = scriptById(request->owner);
        if (script) {
            script->callRefString(request->callbackRef, request->body);
            script->freeRef(request->callbackRef);
        }

        request->callbackRef = -1;
    }
}

void releaseScriptNet(const std::string& owner) {
    for (auto& request : g_requests) {
        if (!request || request->owner != owner) continue;
        request->callbackRef = -1;
    }

    g_requests.erase(
        std::remove_if(g_requests.begin(), g_requests.end(),
            [&](const std::unique_ptr<NetRequest>& request) {
                return request && request->owner == owner && request->finished;
            }),
        g_requests.end());
}

void registerFsApi(lua_State* L) {
    static const luaL_Reg kFs[] = {
        { "read",        l_fs_read        },
        { "write",       l_fs_write       },
        { "append",      l_fs_append      },
        { "exists",      l_fs_exists      },
        { "size",        l_fs_size        },
        { "remove",      l_fs_remove      },
        { "mkdir",       l_fs_mkdir       },
        { "list",        l_fs_list        },
        { "list_dirs",   l_fs_list_dirs   },
        { "folder",      l_fs_folder      },
        { "open_folder", l_fs_open_folder },
        { nullptr,       nullptr          },
    };

    static const luaL_Reg kNet[] = {
        { "get",     l_net_get     },
        { "post",    l_net_post    },
        { "request", l_net_request },
        { "done",    l_net_done    },
        { "status",  l_net_status  },
        { "code",    l_net_code    },
        { "body",    l_net_body    },
        { "free",    l_net_free    },
        { "pending", l_net_pending },
        { "encode",  l_net_encode  },
        { nullptr,   nullptr       },
    };

    static const luaL_Reg kScript[] = {
        { "import", l_script_import },
        { "name",   l_script_name   },
        { "id",     l_script_id     },
        { "folder", l_script_folder },
        { "libs",   l_script_libs   },
        { "log",    l_script_log    },
        { nullptr,  nullptr         },
    };

    registerTable(L, "fs", kFs);
    registerTable(L, "net", kNet);
    registerTable(L, "script", kScript);
}

}
