
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/utils/web.hpp>

#if __has_include(<Geode/ui/ColorPickPopup.hpp>)
#include <Geode/ui/ColorPickPopup.hpp>
#define NH_HAS_COLOR_POPUP 1
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace nh::lua {
namespace {

Script* findScript(const std::string& owner) {
    for (auto& s : Manager::get().scripts())
        if (s && s->running() && s->id() == owner) return s.get();
    return nullptr;
}

void deliverString(const std::string& owner, int ref, const std::string& value) {
    if (ref < 0) return;
    if (Script* s = findScript(owner)) {
        s->callRefString(ref, value);
        s->freeRef(ref);
    }
}

void deliverInt(const std::string& owner, int ref, int value) {
    if (ref < 0) return;
    if (Script* s = findScript(owner)) {
        s->callRefInt(ref, value);
        s->freeRef(ref);
    }
}

void deliverColor(const std::string& owner, int ref, float r, float g, float b) {
    if (ref < 0) return;
    if (Script* s = findScript(owner)) s->callRefColor(ref, r, g, b);
}

std::string ownerId(lua_State* L) {
    Script* s = ownerOf(L);
    return s ? s->id() : std::string();
}

int takeCallback(lua_State* L, int index) {
    if (lua_isnoneornil(L, index)) return -1;

    luaL_checktype(L, index, LUA_TFUNCTION);
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

class NHTextPopup : public geode::Popup {
protected:
    geode::TextInput* m_input = nullptr;
    std::string       m_owner;
    int               m_ref = -1;

    void build(std::string title, std::string owner, int ref) {
        m_owner = std::move(owner);
        m_ref   = ref;

        this->setTitle(title);

        const CCSize size = m_size;

        m_input = geode::TextInput::create(220.f, "Text");
        m_input->setPosition(size.width / 2.f, size.height / 2.f);
        m_mainLayer->addChild(m_input);

        auto* button = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("OK"), this, menu_selector(NHTextPopup::onConfirm));

        auto* menu = CCMenu::create();
        menu->setPosition(size.width / 2.f, 32.f);
        menu->addChild(button);
        m_mainLayer->addChild(menu);
    }

    void onConfirm(CCObject*) {
        const int ref = m_ref;
        m_ref = -1;

        deliverString(m_owner, ref, m_input ? m_input->getString() : std::string());
        this->removeFromParent();
    }

public:
    static NHTextPopup* create(std::string title, std::string owner, int ref) {
        auto* popup = new NHTextPopup();
        if (popup->init(280.f, 160.f)) {
            popup->autorelease();
            popup->build(std::move(title), std::move(owner), ref);
            return popup;
        }
        delete popup;
        return nullptr;
    }
};

class NHChoicePopup : public geode::Popup {
protected:
    std::string m_owner;
    int         m_ref = -1;

    void build(std::string title, std::vector<std::string> items,
               std::string owner, int ref) {
        m_owner = std::move(owner);
        m_ref   = ref;

        this->setTitle(title);

        const CCSize size = m_size;

        auto* menu = CCMenu::create();
        menu->setPosition(size.width / 2.f, size.height / 2.f - 12.f);
        m_mainLayer->addChild(menu);

        const int count = (int)items.size();
        float     y     = (float)(count - 1) * 15.f;

        for (int i = 0; i < count; ++i) {
            auto* button = CCMenuItemSpriteExtra::create(
                ButtonSprite::create(items[i].c_str(), 160, true, "bigFont.fnt", "GJ_button_01.png", 28.f, 0.5f),
                this, menu_selector(NHChoicePopup::onPick));

            button->setTag(i + 1);
            button->setPosition(0.f, y);
            menu->addChild(button);

            y -= 30.f;
        }
    }

    void onPick(CCObject* sender) {
        const int ref = m_ref;
        m_ref = -1;

        deliverInt(m_owner, ref, sender ? sender->getTag() : 0);
        this->removeFromParent();
    }

public:
    static NHChoicePopup* create(std::string title, std::vector<std::string> items,
                                 std::string owner, int ref) {
        const float height = 90.f + (float)items.size() * 30.f;

        auto* popup = new NHChoicePopup();
        if (popup->init(300.f, height)) {
            popup->autorelease();
            popup->build(std::move(title), std::move(items), std::move(owner), ref);
            return popup;
        }
        delete popup;
        return nullptr;
    }
};

int l_gd_alert(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    const char* body  = luaL_checkstring(L, 2);

    FLAlertLayer::create(title, body, "OK")->show();
    return 0;
}

int l_gd_confirm(lua_State* L) {
    const std::string title = luaL_checkstring(L, 1);
    const std::string body  = luaL_checkstring(L, 2);
    const int         ref   = takeCallback(L, 3);
    const std::string owner = ownerId(L);

    geode::createQuickPopup(
        title.c_str(), body,
        "No", "Yes",
        [owner, ref](auto*, bool yes) {
            deliverInt(owner, ref, yes ? 1 : 0);
        });
    return 0;
}

int l_gd_text_input(lua_State* L) {
    const std::string title = luaL_checkstring(L, 1);
    const int         ref   = takeCallback(L, 2);

    if (auto* popup = NHTextPopup::create(title, ownerId(L), ref)) popup->show();
    return 0;
}

int l_gd_dropdown(lua_State* L) {
    const std::string title = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    std::vector<std::string> items;

    const int count = (int)lua_objlen(L, 2);
    for (int i = 1; i <= count && i <= 10; ++i) {
        lua_rawgeti(L, 2, i);
        if (const char* text = lua_tostring(L, -1)) items.push_back(text);
        lua_pop(L, 1);
    }

    if (items.empty()) return luaL_error(L, "gd.dropdown needs at least one item");

    const int ref = takeCallback(L, 3);

    if (auto* popup = NHChoicePopup::create(title, items, ownerId(L), ref)) popup->show();
    return 0;
}

int l_gd_color_picker(lua_State* L) {
#ifdef NH_HAS_COLOR_POPUP
    const auto clamp255 = [](double v) -> GLubyte {
        if (v < 0.0)   v = 0.0;
        if (v > 255.0) v = 255.0;
        return (GLubyte)v;
    };

    const ccColor4B start = {
        clamp255(luaL_optnumber(L, 1, 255.0)),
        clamp255(luaL_optnumber(L, 2, 255.0)),
        clamp255(luaL_optnumber(L, 3, 255.0)),
        255,
    };

    const int         ref   = takeCallback(L, 4);
    const std::string owner = ownerId(L);

    auto* popup = geode::ColorPickPopup::create(start);
    if (!popup) return 0;

    popup->setCallback([owner, ref](ccColor4B const& color) {
        deliverColor(owner, ref, color.r, color.g, color.b);
    });
    popup->show();
    return 0;
#else
    return luaL_error(L, "the native colour picker is not available in this build");
#endif
}

int l_client_notify(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);

    if (auto* notification = geode::Notification::create(text, geode::NotificationIcon::Info, 2.f))
        notification->show();

    return 0;
}

int l_client_clipboard_get(lua_State* L) {
    const std::string text = geode::utils::clipboard::read();
    lua_pushlstring(L, text.data(), text.size());
    return 1;
}

int l_client_clipboard_set(lua_State* L) {
    std::size_t length = 0;
    const char* text   = luaL_checklstring(L, 1, &length);

    if (length > 65536) return luaL_error(L, "that text is too long for the clipboard");

    geode::utils::clipboard::write(std::string(text, length));
    return 0;
}

int l_client_open_url(lua_State* L) {
    const std::string url = luaL_checkstring(L, 1);

    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0)
        return luaL_error(L, "only http:// and https:// URLs can be opened");

    Script* s = ownerOf(L);
    Manager::get().log((s ? s->fileName() + ": " : std::string()) + "opening " + url);

    geode::utils::web::openLinkInBrowser(url);
    return 0;
}

int l_client_set_timer(lua_State* L) {
    const double interval = luaL_checknumber(L, 1);
    const int    ref      = takeCallback(L, 2);

    Script* s = ownerOf(L);
    if (!s || ref < 0) { lua_pushnil(L); return 1; }

    lua_pushinteger(L, s->addTimer(ref, interval, true));
    return 1;
}

int l_client_delay_call(lua_State* L) {
    const double delay = luaL_checknumber(L, 1);
    const int    ref   = takeCallback(L, 2);

    Script* s = ownerOf(L);
    if (!s || ref < 0) { lua_pushnil(L); return 1; }

    lua_pushinteger(L, s->addTimer(ref, delay, false));
    return 1;
}

int l_client_stop_timer(lua_State* L) {
    Script* s = ownerOf(L);
    if (s) s->removeTimer((int)luaL_checkinteger(L, 1));
    return 0;
}

int l_client_mouse_pos(lua_State* L) {
    const ImVec2 pos = ImGui::GetIO().MousePos;
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2;
}

int l_client_mouse_down(lua_State* L) {
    int button = (int)luaL_optinteger(L, 1, 1) - 1;
    if (button < 0) button = 0;
    if (button > 4) button = 4;

    lua_pushboolean(L, ImGui::IsMouseDown(button));
    return 1;
}

int l_client_mouse_wheel(lua_State* L) {
    lua_pushnumber(L, ImGui::GetIO().MouseWheel);
    return 1;
}

constexpr int         kMaxDepth    = 32;
constexpr std::size_t kMaxJsonSize = 4u * 1024u * 1024u;

void jsonEscape(std::string& out, const char* text, std::size_t length) {
    out += '"';
    for (std::size_t i = 0; i < length; ++i) {
        const unsigned char c = (unsigned char)text[i];
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        default:
            if (c < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
                out += buffer;
            }
            else {
                out += (char)c;
            }
        }
    }
    out += '"';
}

void jsonNumber(std::string& out, double value) {
    if (value == (double)(long long)value) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
        out += buffer;
        return;
    }

    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.14g", value);
    out += buffer;
}

bool looksLikeArray(lua_State* L, int index) {
    const int length = (int)lua_objlen(L, index);

    int counted = 0;
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        if (lua_type(L, -2) != LUA_TNUMBER) { lua_pop(L, 2); return false; }

        const double key = lua_tonumber(L, -2);
        if (key < 1.0 || key != (double)(int)key) { lua_pop(L, 2); return false; }

        ++counted;
        lua_pop(L, 1);
    }
    return counted == length;
}

bool jsonWrite(lua_State* L, int index, std::string& out, int depth) {
    if (depth > kMaxDepth) return false;

    switch (lua_type(L, index)) {
    case LUA_TNIL:
        out += "null";
        return true;

    case LUA_TBOOLEAN:
        out += lua_toboolean(L, index) ? "true" : "false";
        return true;

    case LUA_TNUMBER:
        jsonNumber(out, lua_tonumber(L, index));
        return true;

    case LUA_TSTRING: {
        std::size_t length = 0;
        const char* text   = lua_tolstring(L, index, &length);
        jsonEscape(out, text, length);
        return true;
    }

    case LUA_TTABLE: {
        const int table = index < 0 ? lua_gettop(L) + index + 1 : index;

        if (looksLikeArray(L, table)) {
            out += '[';
            const int length = (int)lua_objlen(L, table);
            for (int i = 1; i <= length; ++i) {
                if (i > 1) out += ',';

                lua_rawgeti(L, table, i);
                const bool ok = jsonWrite(L, -1, out, depth + 1);
                lua_pop(L, 1);

                if (!ok) return false;
            }
            out += ']';
            return true;
        }

        out += '{';
        bool first = true;

        lua_pushnil(L);
        while (lua_next(L, table) != 0) {
            const int keyType = lua_type(L, -2);
            if (keyType != LUA_TSTRING && keyType != LUA_TNUMBER) {
                lua_pop(L, 1);
                continue;
            }

            if (!first) out += ',';
            first = false;

            lua_pushvalue(L, -2);
            std::size_t length = 0;
            const char* key    = lua_tolstring(L, -1, &length);
            jsonEscape(out, key ? key : "", length);
            lua_pop(L, 1);

            out += ':';

            if (!jsonWrite(L, -1, out, depth + 1)) {
                lua_pop(L, 2);
                return false;
            }
            lua_pop(L, 1);
        }

        out += '}';
        return true;
    }

    default:
        return false;
    }
}

int l_json_encode(lua_State* L) {
    std::string out;
    out.reserve(256);

    if (!jsonWrite(L, 1, out, 0)) {
        lua_pushnil(L);
        lua_pushstring(L, "this value cannot be encoded as JSON");
        return 2;
    }

    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

struct JsonReader {
    const char* text = nullptr;
    std::size_t size = 0;
    std::size_t at   = 0;
    bool        bad  = false;

    void skip() {
        while (at < size) {
            const char c = text[at];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++at;
            else break;
        }
    }

    char peek() const { return at < size ? text[at] : '\0'; }
};

bool jsonRead(lua_State* L, JsonReader& r, int depth);

void pushUtf8(std::string& out, unsigned int code) {
    if (code < 0x80) {
        out += (char)code;
    }
    else if (code < 0x800) {
        out += (char)(0xC0 | (code >> 6));
        out += (char)(0x80 | (code & 0x3F));
    }
    else {
        out += (char)(0xE0 | (code >> 12));
        out += (char)(0x80 | ((code >> 6) & 0x3F));
        out += (char)(0x80 | (code & 0x3F));
    }
}

bool jsonReadString(JsonReader& r, std::string& out) {
    if (r.peek() != '"') return false;
    ++r.at;

    while (r.at < r.size) {
        const char c = r.text[r.at++];

        if (c == '"') return true;

        if (c != '\\') {
            out += c;
            continue;
        }

        if (r.at >= r.size) return false;
        const char escape = r.text[r.at++];

        switch (escape) {
        case '"':  out += '"';  break;
        case '\\': out += '\\'; break;
        case '/':  out += '/';  break;
        case 'n':  out += '\n'; break;
        case 'r':  out += '\r'; break;
        case 't':  out += '\t'; break;
        case 'b':  out += '\b'; break;
        case 'f':  out += '\f'; break;
        case 'u': {
            if (r.at + 4 > r.size) return false;

            char digits[5] = { r.text[r.at], r.text[r.at + 1],
                               r.text[r.at + 2], r.text[r.at + 3], '\0' };
            r.at += 4;

            char*              end  = nullptr;
            const unsigned int code = (unsigned int)std::strtoul(digits, &end, 16);
            if (end != digits + 4) return false;

            pushUtf8(out, (code >= 0xD800 && code <= 0xDFFF) ? 0xFFFD : code);
            break;
        }
        default:
            return false;
        }
    }
    return false;
}

bool jsonReadArray(lua_State* L, JsonReader& r, int depth) {
    ++r.at;
    lua_newtable(L);

    r.skip();
    if (r.peek() == ']') { ++r.at; return true; }

    int index = 1;
    while (true) {
        if (!jsonRead(L, r, depth + 1)) return false;
        lua_rawseti(L, -2, index++);

        r.skip();
        const char c = r.peek();

        if (c == ',') { ++r.at; r.skip(); continue; }
        if (c == ']') { ++r.at; return true; }

        return false;
    }
}

bool jsonReadObject(lua_State* L, JsonReader& r, int depth) {
    ++r.at;
    lua_newtable(L);

    r.skip();
    if (r.peek() == '}') { ++r.at; return true; }

    while (true) {
        r.skip();

        std::string key;
        if (!jsonReadString(r, key)) return false;

        r.skip();
        if (r.peek() != ':') return false;
        ++r.at;
        r.skip();

        lua_pushlstring(L, key.data(), key.size());
        if (!jsonRead(L, r, depth + 1)) { lua_pop(L, 1); return false; }
        lua_rawset(L, -3);

        r.skip();
        const char c = r.peek();

        if (c == ',') { ++r.at; continue; }
        if (c == '}') { ++r.at; return true; }

        return false;
    }
}

bool jsonRead(lua_State* L, JsonReader& r, int depth) {
    if (depth > 64) return false;

    r.skip();
    const char c = r.peek();

    switch (c) {
    case '{': return jsonReadObject(L, r, depth);
    case '[': return jsonReadArray(L, r, depth);

    case '"': {
        std::string text;
        if (!jsonReadString(r, text)) return false;
        lua_pushlstring(L, text.data(), text.size());
        return true;
    }

    case 't':
        if (r.size - r.at < 4 || std::strncmp(r.text + r.at, "true", 4) != 0) return false;
        r.at += 4;
        lua_pushboolean(L, 1);
        return true;

    case 'f':
        if (r.size - r.at < 5 || std::strncmp(r.text + r.at, "false", 5) != 0) return false;
        r.at += 5;
        lua_pushboolean(L, 0);
        return true;

    case 'n':
        if (r.size - r.at < 4 || std::strncmp(r.text + r.at, "null", 4) != 0) return false;
        r.at += 4;
        lua_pushnil(L);
        return true;

    default: {
        char*        end   = nullptr;
        const double value = std::strtod(r.text + r.at, &end);

        if (!end || end == r.text + r.at) return false;

        r.at = (std::size_t)(end - r.text);
        lua_pushnumber(L, value);
        return true;
    }
    }
}

int l_json_decode(lua_State* L) {
    std::size_t length = 0;
    const char* text   = luaL_checklstring(L, 1, &length);

    if (length > kMaxJsonSize) {
        lua_pushnil(L);
        lua_pushstring(L, "that JSON document is too large");
        return 2;
    }

    JsonReader reader;
    reader.text = text;
    reader.size = length;

    const int base = lua_gettop(L);

    if (!jsonRead(L, reader, 0)) {
        lua_settop(L, base);
        lua_pushnil(L);
        lua_pushstring(L, "invalid JSON");
        return 2;
    }

    reader.skip();
    if (reader.at != reader.size) {
        lua_settop(L, base);
        lua_pushnil(L);
        lua_pushstring(L, "invalid JSON: trailing characters");
        return 2;
    }

    return 1;
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

void registerUiApi(lua_State* L) {
    static const luaL_Reg kGdUi[] = {
        { "alert",        l_gd_alert        },
        { "confirm",      l_gd_confirm      },
        { "text_input",   l_gd_text_input   },
        { "dropdown",     l_gd_dropdown     },
        { "color_picker", l_gd_color_picker },
        { nullptr,        nullptr           },
    };
    addFunctions(L, "gd", kGdUi);

    static const luaL_Reg kClientExtras[] = {
        { "notify",        l_client_notify        },
        { "clipboard_get", l_client_clipboard_get },
        { "clipboard_set", l_client_clipboard_set },
        { "open_url",      l_client_open_url      },
        { "set_timer",     l_client_set_timer     },
        { "delay_call",    l_client_delay_call    },
        { "stop_timer",    l_client_stop_timer    },
        { "mouse_pos",     l_client_mouse_pos     },
        { "mouse_down",    l_client_mouse_down    },
        { "mouse_wheel",   l_client_mouse_wheel   },
        { nullptr,         nullptr                },
    };
    addFunctions(L, "client", kClientExtras);

    static const luaL_Reg kJson[] = {
        { "encode", l_json_encode },
        { "decode", l_json_decode },
        { nullptr,  nullptr       },
    };

    lua_newtable(L);
    for (const luaL_Reg* f = kJson; f->name; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }
    lua_setglobal(L, "json");
}

}
