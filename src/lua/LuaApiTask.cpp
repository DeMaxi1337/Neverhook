
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace nh::lua {
namespace {

constexpr int    kMaxTasks       = 64;
constexpr double kResumeBudgetMs = 100.0;
constexpr int    kHookInterval   = 20000;
constexpr double kMaxWait        = 3600.0;

char kTasksKey = 0;

struct TaskSlot {
    int         id     = 0;
    std::string owner;
    lua_State*  thread = nullptr;
    double      resumeAt   = 0.0;
    int         framesLeft = 0;
    bool        dead       = false;
    bool        started    = false;
};

std::vector<TaskSlot> g_tasks;
int                   g_nextTask = 1;

double g_resumeStart = 0.0;

std::string ownerId(lua_State* L) {
    Script* script = ownerOf(L);
    return script ? script->id() : std::string();
}

Script* scriptById(const std::string& id) {
    for (auto& script : Manager::get().scripts())
        if (script && script->id() == id) return script.get();
    return nullptr;
}

TaskSlot* slotOfThread(lua_State* thread) {
    for (auto& slot : g_tasks)
        if (slot.thread == thread) return &slot;
    return nullptr;
}

TaskSlot* slotOfId(int id) {
    for (auto& slot : g_tasks)
        if (slot.id == id) return &slot;
    return nullptr;
}

int taskCountOf(const std::string& owner) {
    int count = 0;
    for (const auto& slot : g_tasks)
        if (!slot.dead && slot.owner == owner) ++count;
    return count;
}

void pushTaskTable(lua_State* L) {
    lua_pushlightuserdata(L, &kTasksKey);
    lua_rawget(L, LUA_REGISTRYINDEX);

    if (lua_istable(L, -1)) return;

    lua_pop(L, 1);
    lua_newtable(L);

    lua_pushlightuserdata(L, &kTasksKey);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

void anchorThread(lua_State* L, int id) {

    pushTaskTable(L);
    lua_pushvalue(L, -2);
    lua_rawseti(L, -2, id);
    lua_pop(L, 1);
}

void releaseThread(lua_State* L, int id) {
    if (!L) return;

    pushTaskTable(L);
    lua_pushnil(L);
    lua_rawseti(L, -2, id);
    lua_pop(L, 1);
}

void taskHook(lua_State* L, lua_Debug*) {
    const double elapsedMs = (Manager::get().now() - g_resumeStart) * 1000.0;

    if (elapsedMs < kResumeBudgetMs) return;

    lua_sethook(L, nullptr, 0, 0);
    luaL_error(L, "task aborted: it ran too long without waiting (infinite loop?)");
}

int l_task_run(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    double delay = luaL_optnumber(L, 2, 0.0);
    if (delay < 0.0)      delay = 0.0;
    if (delay > kMaxWait) delay = kMaxWait;

    const std::string owner = ownerId(L);

    if (taskCountOf(owner) >= kMaxTasks)
        return luaL_error(L, "too many tasks are running (%d max)", kMaxTasks);

    lua_State* thread = lua_newthread(L);
    if (!thread) return luaL_error(L, "a task could not be created");

    const int id = g_nextTask++;
    anchorThread(L, id);
    lua_pop(L, 1);

    lua_pushvalue(L, 1);
    lua_xmove(L, thread, 1);

    TaskSlot slot;
    slot.id       = id;
    slot.owner    = owner;
    slot.thread   = thread;
    slot.resumeAt = Manager::get().now() + delay;

    g_tasks.push_back(slot);

    lua_pushinteger(L, id);
    return 1;
}

int l_task_wait(lua_State* L) {
    if (!slotOfThread(L))
        return luaL_error(L, "task.wait() only works inside task.run()");

    double seconds = luaL_optnumber(L, 1, 0.0);
    if (seconds < 0.0)      seconds = 0.0;
    if (seconds > kMaxWait) seconds = kMaxWait;

    lua_pushstring(L, "time");
    lua_pushnumber(L, seconds);
    return lua_yield(L, 2);
}

int l_task_wait_frames(lua_State* L) {
    if (!slotOfThread(L))
        return luaL_error(L, "task.wait_frames() only works inside task.run()");

    int frames = (int)luaL_optinteger(L, 1, 1);
    if (frames < 1)     frames = 1;
    if (frames > 100000) frames = 100000;

    lua_pushstring(L, "frames");
    lua_pushnumber(L, (double)frames);
    return lua_yield(L, 2);
}

int l_task_cancel(lua_State* L) {
    TaskSlot* slot = slotOfId((int)luaL_checkinteger(L, 1));

    if (slot) slot->dead = true;

    lua_pushboolean(L, slot != nullptr ? 1 : 0);
    return 1;
}

int l_task_cancel_all(lua_State* L) {
    const std::string owner = ownerId(L);

    for (auto& slot : g_tasks)
        if (slot.owner == owner) slot.dead = true;

    return 0;
}

int l_task_running(lua_State* L) {
    lua_pushinteger(L, taskCountOf(ownerId(L)));
    return 1;
}

int l_task_is_task(lua_State* L) {
    lua_pushboolean(L, slotOfThread(L) != nullptr ? 1 : 0);
    return 1;
}

int l_task_alive(lua_State* L) {
    TaskSlot* slot = slotOfId((int)luaL_checkinteger(L, 1));
    lua_pushboolean(L, (slot && !slot->dead) ? 1 : 0);
    return 1;
}

void registerTable(lua_State* L, const char* name, const luaL_Reg* fns) {
    lua_newtable(L);
    for (const luaL_Reg* f = fns; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, name);
}

void applyYield(TaskSlot& slot, lua_State* thread, double now) {
    const int top = lua_gettop(thread);

    std::string mode  = "time";
    double      value = 0.0;

    if (top >= 2 && lua_type(thread, top - 1) == LUA_TSTRING) {
        mode  = lua_tostring(thread, top - 1);
        value = lua_tonumber(thread, top);
    } else if (top >= 1 && lua_isnumber(thread, top)) {
        value = lua_tonumber(thread, top);
    }

    lua_settop(thread, 0);

    if (mode == "frames") {
        int frames = (int)value;
        if (frames < 1) frames = 1;

        slot.framesLeft = frames;
        slot.resumeAt   = now;
    } else {
        slot.framesLeft = 0;
        slot.resumeAt   = now + value;
    }
}

}

void updateTasks() {
    if (g_tasks.empty()) return;

    const double now = Manager::get().now();

    for (std::size_t i = 0; i < g_tasks.size(); ++i) {
        TaskSlot& slot = g_tasks[i];

        if (slot.dead || !slot.thread) continue;

        if (slot.framesLeft > 0) {
            --slot.framesLeft;
            continue;
        }

        if (now < slot.resumeAt) continue;

        Script* script = scriptById(slot.owner);
        if (!script || !script->running()) {
            slot.dead = true;
            continue;
        }

        lua_State* thread = slot.thread;
        slot.started      = true;

        g_resumeStart = Manager::get().now();
        lua_sethook(thread, taskHook, LUA_MASKCOUNT, kHookInterval);

        const int status = lua_resume(thread, 0);

        lua_sethook(thread, nullptr, 0, 0);

        if (status == LUA_YIELD) {
            applyYield(slot, thread, Manager::get().now());
            continue;
        }

        if (status != 0) {
            const char* message = lua_tostring(thread, -1);

            Manager::get().log(
                script->fileName() + " [task]: "
                    + (message ? message : "unknown error"),
                true);

            lua_settop(thread, 0);
        }

        slot.dead = true;
    }

    for (auto& slot : g_tasks) {
        if (!slot.dead || !slot.thread) continue;

        Script* script = scriptById(slot.owner);
        if (script && script->running()) releaseThread(script->state(), slot.id);

        slot.thread = nullptr;
    }

    g_tasks.erase(
        std::remove_if(g_tasks.begin(), g_tasks.end(),
            [](const TaskSlot& slot) { return slot.dead; }),
        g_tasks.end());
}

void releaseScriptTasks(const std::string& owner) {
    for (auto& slot : g_tasks) {
        if (slot.owner != owner) continue;

        slot.dead   = true;
        slot.thread = nullptr;
    }

    g_tasks.erase(
        std::remove_if(g_tasks.begin(), g_tasks.end(),
            [&](const TaskSlot& slot) { return slot.owner == owner; }),
        g_tasks.end());
}

void registerTaskApi(lua_State* L) {
    static const luaL_Reg kTask[] = {
        { "run",         l_task_run         },
        { "wait",        l_task_wait        },
        { "wait_frames", l_task_wait_frames },
        { "cancel",      l_task_cancel      },
        { "cancel_all",  l_task_cancel_all  },
        { "running",     l_task_running     },
        { "is_task",     l_task_is_task     },
        { "alive",       l_task_alive       },
        { nullptr,       nullptr            },
    };

    registerTable(L, "task", kTask);
}

}
