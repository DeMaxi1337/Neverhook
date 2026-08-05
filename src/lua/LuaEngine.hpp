#pragma once

#include "LuaHooks.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct lua_State;

namespace nh::lua {

enum class Event {
    Frame,
    Draw,
    LevelStart,
    LevelEnd,
    Death,
    Reset,
    Checkpoint,
    Attempt,
    Percent,
    Count
};

const char* eventName(Event e);
bool        eventFromName(const char* name, Event& out);

void pollLevelEvents();

enum class WidgetType {
    Toggle, Slider, SliderInt, Combo, Button, Label, Separator,
    InputText,
    ColorPicker,
    GroupBegin,
    GroupEnd
};

struct TextStyle {
    bool  colored  = false;
    bool  gradient = false;
    bool  animated = false;
    bool  rainbow  = false;
    float speed    = 1.f;
    float colorA[4] = { 1.f, 1.f, 1.f, 1.f };
    float colorB[4] = { 1.f, 1.f, 1.f, 1.f };
};

struct MenuWidget {
    WidgetType               type   = WidgetType::Label;
    std::string              id;
    std::string              label;
    bool                     bValue = false;
    float                    fValue = 0.f;
    int                      iValue = 0;
    float                    minVal = 0.f;
    float                    maxVal = 1.f;
    std::vector<std::string> items;
    int                      fnRef  = -1;
    int                      changedRef = -1;
    std::string              sValue;
    float                    color[4] = { 1.f, 1.f, 1.f, 1.f };
    TextStyle                style;

    std::shared_ptr<bool>    bBox;
    std::string              bindName;

    bool* boolPtr() { return bBox ? bBox.get() : &bValue; }
    bool  boolValue() const { return bBox ? *bBox : bValue; }
    void  setBool(bool v) { if (bBox) *bBox = v; else bValue = v; }
};

struct MenuTab {
    std::string             owner;
    std::string             title;
    std::vector<MenuWidget> widgets;
};

struct LogLine {
    std::string text;
    bool        error = false;
};

struct Timer {
    int    id       = 0;
    int    fnRef    = -1;
    double interval = 0.0;
    double nextAt   = 0.0;
    bool   repeat   = false;
    bool   dead     = false;
};

class Script {
public:
    explicit Script(std::filesystem::path path);
    ~Script();

    Script(const Script&)            = delete;
    Script& operator=(const Script&) = delete;

    bool start();
    void stop();

    void dispatch(Event e);

    bool dispatchHook(HookId id, const HookEvent& event);
    int  hookListenerCount(HookId id) const;
    void resetHookFrame();

    void callRef(int ref);
    void callRefString(int ref, const std::string& value);
    void callRefColor(int ref, float r, float g, float b);
    void callRefInt(int ref, int value);
    void freeRef(int ref);

    int  addTimer(int fnRef, double seconds, bool repeat);
    void removeTimer(int id);
    void updateTimers(double now);

    const std::string& id()       const { return m_id; }
    const std::string& fileName() const { return m_fileName; }
    const std::string& modified() const { return m_modified; }
    const std::string& lastError()const { return m_lastError; }
    bool               running()  const { return m_L != nullptr; }
    lua_State*         state()    const { return m_L; }
    const std::filesystem::path& path() const { return m_path; }

    void refreshTimestamp();

    bool overBudget() const;

private:
    bool openSandbox();
    bool pcall(int nargs, int nresults, const char* what);
    void beginBudget(int ms);
    bool prepareRef(int ref);
    void finishCall(const char* what);
    void afterCallbackError();

    std::filesystem::path m_path;
    std::string           m_id;
    std::string           m_fileName;
    std::string           m_modified;
    std::string           m_lastError;
    lua_State*            m_L = nullptr;

    std::vector<Timer>    m_timers;
    int                   m_nextTimerId = 1;

    struct HookBudget {
        double usedMs   = 0.0;
        int    overruns = 0;
        bool   disabled = false;
        bool   warned   = false;
    };

    std::vector<HookBudget> m_hookBudgets;

    std::chrono::steady_clock::time_point m_callStart{};
    int                                   m_budgetMs = 0;
};

class Manager {
public:
    static Manager& get();

    void refresh();
    void dispatch(Event e);
    void update();
    void stopAll();

    std::vector<std::unique_ptr<Script>>& scripts() { return m_scripts; }
    std::vector<MenuTab>&                 tabs()    { return m_tabs; }
    std::vector<LogLine>&                 console() { return m_console; }

    void log(const std::string& text, bool error = false);
    void clearConsole() { m_console.clear(); }

    int      addTab(const std::string& owner, const std::string& title);
    void     removeTabsOf(const std::string& owner);
    MenuTab* tabAt(int index);

    std::shared_ptr<bool> makeBindBox(bool initial);

    std::filesystem::path dir() const;
    std::filesystem::path soundsDir() const;
    std::filesystem::path imagesDir() const;
    void                  openFolder();
    bool                  createScript(const std::string& name);

    double now() const;

private:
    Manager() = default;

    std::vector<std::unique_ptr<Script>> m_scripts;
    std::vector<MenuTab>                 m_tabs;
    std::vector<LogLine>                 m_console;
    std::vector<std::shared_ptr<bool>>   m_bindBoxes;
};

void registerApi(lua_State* L, Script* owner);
void registerGameApi(lua_State* L);
void registerMediaApi(lua_State* L);
void registerUiApi(lua_State* L);
void registerImguiApi(lua_State* L);
void registerUtilApi(lua_State* L);
void registerNodeApi(lua_State* L);
void registerFsApi(lua_State* L);
void registerWorldApi(lua_State* L);
void registerTaskApi(lua_State* L);
void registerClassApi(lua_State* L);

void releaseScriptMedia(const std::string& owner);

void releaseScriptNodes(const std::string& owner);

void updateNodes();
void releaseScriptNet(const std::string& owner);
void releaseScriptTasks(const std::string& owner);

void updateMedia();

void updateNet();
void updateTasks();

void closeImguiScopes();

Script* ownerOf(lua_State* L);

void* callbacksRegistryKey();

void runWidgetCallback(const std::string& owner, int fnRef);

void runWidgetChanged(const std::string& owner, int fnRef);

}
