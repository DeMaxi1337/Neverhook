#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <algorithm>
#include <cstdio>
#include <ctime>

#include <sys/stat.h>
#include <sys/types.h>

using namespace geode::prelude;

namespace nh::lua {

namespace {

char kOwnerKey     = 0;
char kCallbacksKey = 0;

constexpr int kStartupBudgetMs  = 2000;
constexpr int kCallbackBudgetMs = 100;
constexpr int kHookInterval     = 20000;

const char* kBannedGlobals[] = {
    "dofile", "loadfile", "load", "loadstring", "require", "module",
    "package", "io", "debug", "newproxy", "gcinfo", "setfenv", "getfenv",
};

const char* kAllowedOsFields[] = { "clock", "time", "date", "difftime" };

std::string formatFileTime(const std::filesystem::path& p) {
    std::time_t tt = 0;

#ifdef _WIN32
    struct _stat64 st{};
    if (_wstat64(p.c_str(), &st) != 0) return "--.-- --:--";
    tt = static_cast<std::time_t>(st.st_mtime);
#else
    struct stat st{};
    if (::stat(p.c_str(), &st) != 0) return "--.-- --:--";
    tt = st.st_mtime;
#endif

    std::tm tm{};
#ifdef _WIN32
    if (localtime_s(&tm, &tt) != 0) return "--.-- --:--";
#else
    if (!localtime_r(&tt, &tm)) return "--.-- --:--";
#endif

    char buf[32];
    if (std::strftime(buf, sizeof(buf), "%d.%m %H:%M", &tm) == 0)
        return "--.-- --:--";

    return buf;
}

void countHook(lua_State* L, lua_Debug*) {
    Script* s = ownerOf(L);
    if (s && s->overBudget()) {
        lua_pushstring(L, "script aborted: exceeded its time budget "
                          "(infinite loop?)");
        lua_error(L);
    }
}

int traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    luaL_traceback(L, L, msg ? msg : "(non-string error)", 1);
    return 1;
}

}

const char* eventName(Event e) {
    switch (e) {
    case Event::Frame:      return "frame";
    case Event::Draw:       return "draw";
    case Event::LevelStart: return "level_start";
    case Event::LevelEnd:   return "level_end";
    case Event::Death:      return "death";
    case Event::Reset:      return "reset";
    case Event::Checkpoint: return "checkpoint";
    case Event::Attempt:    return "attempt";
    case Event::Percent:    return "percent";
    default:                return "?";
    }
}

bool eventFromName(const char* name, Event& out) {
    if (!name) return false;
    for (int i = 0; i < (int)Event::Count; ++i) {
        const auto e = (Event)i;
        if (std::strcmp(name, eventName(e)) == 0) { out = e; return true; }
    }
    return false;
}

void* callbacksRegistryKey() { return &kCallbacksKey; }

void runWidgetCallback(const std::string& owner, int fnRef) {
    for (auto& s : Manager::get().scripts()) {
        if (!s || !s->running() || s->id() != owner) continue;
        s->callRef(fnRef);
        return;
    }
}

void runWidgetChanged(const std::string& owner, int fnRef) {
    if (fnRef < 0) return;
    runWidgetCallback(owner, fnRef);
}

Script* ownerOf(lua_State* L) {
    lua_pushlightuserdata(L, &kOwnerKey);
    lua_rawget(L, LUA_REGISTRYINDEX);
    auto* s = static_cast<Script*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return s;
}

Script::Script(std::filesystem::path path) : m_path(std::move(path)) {
    m_fileName = geode::utils::string::pathToString(m_path.filename());
    m_id       = geode::utils::string::pathToString(m_path.stem());
    refreshTimestamp();
}

Script::~Script() { stop(); }

void Script::refreshTimestamp() { m_modified = formatFileTime(m_path); }

bool Script::overBudget() const {
    if (m_budgetMs <= 0) return false;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_callStart).count();
    return elapsed > m_budgetMs;
}

void Script::beginBudget(int ms) {
    m_budgetMs  = ms;
    m_callStart = std::chrono::steady_clock::now();
}

bool Script::openSandbox() {
    m_L = luaL_newstate();
    if (!m_L) {
        m_lastError = "out of memory creating the Lua state";
        return false;
    }

    static const luaL_Reg kLibs[] = {
        { "",              luaopen_base   },
        { LUA_TABLIBNAME,  luaopen_table  },
        { LUA_STRLIBNAME,  luaopen_string },
        { LUA_MATHLIBNAME, luaopen_math   },
        { LUA_OSLIBNAME,   luaopen_os     },
        { LUA_BITLIBNAME,  luaopen_bit    },
        { nullptr,         nullptr        },
    };
    for (const luaL_Reg* lib = kLibs; lib->func; ++lib) {
        lua_pushcfunction(m_L, lib->func);
        lua_pushstring(m_L, lib->name);
        lua_call(m_L, 1, 0);
    }

    for (const char* name : kBannedGlobals) {
        lua_pushnil(m_L);
        lua_setglobal(m_L, name);
    }

    lua_getglobal(m_L, "os");
    if (lua_istable(m_L, -1)) {
        lua_newtable(m_L);
        for (const char* field : kAllowedOsFields) {
            lua_getfield(m_L, -2, field);
            lua_setfield(m_L, -2, field);
        }
        lua_setglobal(m_L, "os");
    }
    lua_pop(m_L, 1);

    lua_pushlightuserdata(m_L, &kOwnerKey);
    lua_pushlightuserdata(m_L, this);
    lua_rawset(m_L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(m_L, &kCallbacksKey);
    lua_newtable(m_L);
    for (int i = 0; i < (int)Event::Count; ++i) {
        lua_newtable(m_L);
        lua_setfield(m_L, -2, eventName((Event)i));
    }
    lua_rawset(m_L, LUA_REGISTRYINDEX);

    lua_sethook(m_L, countHook, LUA_MASKCOUNT, kHookInterval);

    registerApi(m_L, this);
    return true;
}

bool Script::pcall(int nargs, int nresults, const char* what) {
    const int base = lua_gettop(m_L) - nargs;
    lua_pushcfunction(m_L, traceback);
    lua_insert(m_L, base);

    const int rc = lua_pcall(m_L, nargs, nresults, base);
    lua_remove(m_L, base);

    if (rc != 0) {
        const char* msg = lua_tostring(m_L, -1);
        m_lastError = msg ? msg : "unknown error";
        lua_pop(m_L, 1);
        closeImguiScopes();
        Manager::get().log(m_fileName + " [" + what + "]: " + m_lastError, true);
        return false;
    }
    return true;
}

bool Script::start() {
    stop();
    m_lastError.clear();

    std::error_code ec;
    if (!std::filesystem::exists(m_path, ec) || ec) {
        m_lastError = "file not found";
        Manager::get().log(m_fileName + ": file not found", true);
        return false;
    }

    const auto data = file::readString(m_path);
    if (!data) {
        m_lastError = "could not read the file";
        Manager::get().log(m_fileName + ": could not read the file", true);
        return false;
    }
    const std::string& src = data.unwrap();

    if (!src.empty() && src[0] == 0x1B) {
        m_lastError = "precompiled bytecode is not allowed, ship plain .lua source";
        Manager::get().log(m_fileName + ": " + m_lastError, true);
        return false;
    }

    if (!openSandbox()) {
        Manager::get().log(m_fileName + ": " + m_lastError, true);
        return false;
    }

    const std::string chunkName = "@" + m_fileName;

    if (luaL_loadbufferx(m_L, src.data(), src.size(), chunkName.c_str(), "t") != 0) {
        const char* msg = lua_tostring(m_L, -1);
        m_lastError = msg ? msg : "syntax error";
        Manager::get().log(m_fileName + ": " + m_lastError, true);
        stop();
        return false;
    }

    beginBudget(kStartupBudgetMs);
    if (!pcall(0, 0, "load")) {
        stop();
        return false;
    }

    Manager::get().log(m_fileName + " loaded");
    return true;
}

void Script::stop() {
    closeImguiScopes();
    if (!m_L) return;

    lua_close(m_L);
    m_L = nullptr;

    m_timers.clear();
    m_nextTimerId = 1;

    Manager::get().removeTabsOf(m_id);

    releaseScriptMedia(m_id);
    releaseScriptNodes(m_id);
    releaseScriptNet(m_id);
    releaseScriptTasks(m_id);
}

bool Script::prepareRef(int ref) {
    if (!m_L || ref < 0) return false;

    lua_rawgeti(m_L, LUA_REGISTRYINDEX, ref);
    if (!lua_isfunction(m_L, -1)) {
        lua_pop(m_L, 1);
        return false;
    }

    beginBudget(kCallbackBudgetMs);
    return true;
}

void Script::afterCallbackError() {
    Manager::get().log(m_fileName + " stopped after an error", true);
    stop();
}

void Script::callRef(int ref) {
    if (!prepareRef(ref)) return;
    if (!pcall(0, 0, "callback")) afterCallbackError();
}

void Script::callRefString(int ref, const std::string& value) {
    if (!prepareRef(ref)) return;

    lua_pushlstring(m_L, value.data(), value.size());
    if (!pcall(1, 0, "callback")) afterCallbackError();
}

void Script::callRefColor(int ref, float r, float g, float b) {
    if (!prepareRef(ref)) return;

    lua_pushnumber(m_L, r);
    lua_pushnumber(m_L, g);
    lua_pushnumber(m_L, b);
    if (!pcall(3, 0, "callback")) afterCallbackError();
}

void Script::callRefInt(int ref, int value) {
    if (!prepareRef(ref)) return;

    lua_pushinteger(m_L, value);
    if (!pcall(1, 0, "callback")) afterCallbackError();
}

void Script::freeRef(int ref) {
    if (!m_L || ref < 0) return;
    luaL_unref(m_L, LUA_REGISTRYINDEX, ref);
}

int Script::addTimer(int fnRef, double seconds, bool repeat) {
    if (m_timers.size() >= 256) {
        Manager::get().log(m_fileName + ": too many timers (256 max)", true);
        return -1;
    }
    if (seconds < 0.0) seconds = 0.0;

    Timer t;
    t.id       = m_nextTimerId++;
    t.fnRef    = fnRef;
    t.interval = seconds;
    t.nextAt   = Manager::get().now() + seconds;
    t.repeat   = repeat;

    m_timers.push_back(t);
    return t.id;
}

void Script::removeTimer(int id) {
    for (auto& t : m_timers)
        if (t.id == id) t.dead = true;
}

void Script::updateTimers(double now) {
    if (!m_L || m_timers.empty()) return;

    for (std::size_t i = 0; i < m_timers.size(); ++i) {
        if (m_timers[i].dead || now < m_timers[i].nextAt) continue;

        const int ref = m_timers[i].fnRef;

        if (m_timers[i].repeat) m_timers[i].nextAt = now + m_timers[i].interval;
        else                    m_timers[i].dead   = true;

        callRef(ref);
        if (!m_L) return;
    }

    m_timers.erase(
        std::remove_if(m_timers.begin(), m_timers.end(),
            [](const Timer& t) { return t.dead; }),
        m_timers.end());
}

void Script::dispatch(Event e) {
    if (!m_L) return;

    lua_pushlightuserdata(m_L, &kCallbacksKey);
    lua_rawget(m_L, LUA_REGISTRYINDEX);
    lua_getfield(m_L, -1, eventName(e));

    if (!lua_istable(m_L, -1)) {
        lua_pop(m_L, 2);
        return;
    }

    const int count = (int)lua_objlen(m_L, -1);
    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(m_L, -1, i);
        if (!lua_isfunction(m_L, -1)) { lua_pop(m_L, 1); continue; }

        beginBudget(kCallbackBudgetMs);
        if (!pcall(0, 0, eventName(e))) {

            lua_pop(m_L, 2);
            Manager::get().log(m_fileName + " stopped after an error", true);
            stop();
            return;
        }
    }

    lua_pop(m_L, 2);
}

Manager& Manager::get() {
    static Manager inst;
    return inst;
}

std::filesystem::path Manager::dir() const {
    return Mod::get()->getSaveDir() / "scripts";
}

std::filesystem::path Manager::soundsDir() const { return dir() / "sounds"; }
std::filesystem::path Manager::imagesDir() const { return dir() / "images"; }

double Manager::now() const {
    static const auto origin = std::chrono::steady_clock::now();
    const auto        delta  = std::chrono::steady_clock::now() - origin;
    return std::chrono::duration<double>(delta).count();
}

std::shared_ptr<bool> Manager::makeBindBox(bool initial) {
    m_bindBoxes.push_back(std::make_shared<bool>(initial));
    return m_bindBoxes.back();
}

void Manager::log(const std::string& text, bool error) {
    m_console.push_back({ text, error });
    if (m_console.size() > 200)
        m_console.erase(m_console.begin(), m_console.begin() + 50);

    if (error) log::warn("[lua] {}", text);
    else       log::info("[lua] {}", text);
}

void Manager::refresh() {
    const auto folder = dir();

    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    if (ec) {
        log("could not create the scripts folder", true);
        return;
    }

    std::error_code mediaEc;
    std::filesystem::create_directories(soundsDir(), mediaEc);
    std::filesystem::create_directories(imagesDir(), mediaEc);

    std::vector<std::unique_ptr<Script>> kept;
    std::vector<std::filesystem::path>   found;

    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec) || ec) continue;
        if (entry.path().extension() != ".lua") continue;
        found.push_back(entry.path());
    }

    std::sort(found.begin(), found.end());

    for (const auto& p : found) {
        auto it = std::find_if(m_scripts.begin(), m_scripts.end(),
            [&](const std::unique_ptr<Script>& s) { return s && s->path() == p; });

        if (it != m_scripts.end()) {
            (*it)->refreshTimestamp();
            kept.push_back(std::move(*it));
        }
        else {
            kept.push_back(std::make_unique<Script>(p));
        }
    }

    for (auto& s : m_scripts)
        if (s) s->stop();

    m_scripts = std::move(kept);
}

void Manager::dispatch(Event e) {
    if (m_scripts.empty()) return;
    for (auto& s : m_scripts)
        if (s && s->running()) s->dispatch(e);
}

void Manager::update() {
    if (m_scripts.empty()) return;
    pollLevelEvents();
    updateNodes();
    updateMedia();
    updateNet();
    resetHookFrames();

    const double t = now();
    for (auto& s : m_scripts)
        if (s && s->running()) s->updateTimers(t);

    updateTasks();
}

void Manager::stopAll() {
    for (auto& s : m_scripts)
        if (s) s->stop();
}

int Manager::addTab(const std::string& owner, const std::string& title) {
    m_tabs.push_back({ owner, title, {} });
    return (int)m_tabs.size() - 1;
}

void Manager::removeTabsOf(const std::string& owner) {
    m_tabs.erase(
        std::remove_if(m_tabs.begin(), m_tabs.end(),
            [&](const MenuTab& t) { return t.owner == owner; }),
        m_tabs.end());
}

MenuTab* Manager::tabAt(int index) {
    if (index < 0 || index >= (int)m_tabs.size()) return nullptr;
    return &m_tabs[index];
}

void Manager::openFolder() {
    std::error_code ec;
    std::filesystem::create_directories(dir(), ec);
    file::openFolder(dir());
}

bool Manager::createScript(const std::string& name) {
    std::string clean;
    for (char c : name) {
        if (std::isalnum((unsigned char)c) || c == '_' || c == '-' || c == ' ')
            clean += c;
    }
    while (!clean.empty() && clean.back() == ' ') clean.pop_back();
    if (clean.empty()) {
        log("invalid script name", true);
        return false;
    }

    const auto path = dir() / (clean + ".lua");

    std::error_code ec;
    std::filesystem::create_directories(dir(), ec);
    if (std::filesystem::exists(path, ec)) {
        log("a script with that name already exists", true);
        return false;
    }

    static const char* kTemplate =
        "-- {name}\n"
        "-- Neverhook Lua API: see the documentation link in the Scripts tab.\n"
        "\n"
        "local tab = menu.tab(\"{name}\")\n"
        "tab:toggle(\"enabled\", \"Enabled\", false)\n"
        "\n"
        "callbacks.add(\"frame\", function()\n"
        "    if not tab:get(\"enabled\") then return end\n"
        "    -- your code here\n"
        "end)\n";

    std::string body = kTemplate;
    for (std::size_t at = body.find("{name}"); at != std::string::npos;
         at = body.find("{name}"))
        body.replace(at, 6, clean);

    const auto res = file::writeString(path, body);
    if (!res) {
        log("could not create the file", true);
        return false;
    }

    refresh();
    log(clean + ".lua created");
    return true;
}

}
