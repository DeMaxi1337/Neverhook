
#include "LuaEngine.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace nh::lua {
namespace {

constexpr std::size_t kUtilMaxTextLen = 1u * 1024u * 1024u;
constexpr int         kUtilMaxTableSize = 100000;
constexpr int         kUtilMaxCopyDepth = 16;

double clampd(double value, double low, double high) {
    if (value < low)  return low;
    if (value > high) return high;
    return value;
}

int l_str_split(lua_State* L) {
    std::size_t textLen = 0;
    const char* text = luaL_checklstring(L, 1, &textLen);

    std::size_t sepLen = 0;
    const char* sep = luaL_optlstring(L, 2, ",", &sepLen);

    if (sepLen == 0)
        return luaL_error(L, "the separator cannot be empty");

    lua_newtable(L);

    std::size_t start = 0;
    int         index = 1;
    const std::string haystack(text, textLen);
    const std::string needle(sep, sepLen);

    while (index <= kUtilMaxTableSize) {
        const std::size_t at = haystack.find(needle, start);

        if (at == std::string::npos) {
            lua_pushlstring(L, haystack.data() + start, haystack.size() - start);
            lua_rawseti(L, -2, index);
            break;
        }

        lua_pushlstring(L, haystack.data() + start, at - start);
        lua_rawseti(L, -2, index++);
        start = at + needle.size();
    }

    return 1;
}

int l_str_join(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    const char* sep = luaL_optstring(L, 2, ",");

    const int count = (int)lua_objlen(L, 1);
    std::string out;

    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(L, 1, i);

        if (i > 1) out += sep;

        if (lua_isstring(L, -1)) {
            std::size_t len = 0;
            const char* piece = lua_tolstring(L, -1, &len);
            out.append(piece, len);
        }
        lua_pop(L, 1);

        if (out.size() > kUtilMaxTextLen)
            return luaL_error(L, "the joined string got too large");
    }

    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int l_str_trim(lua_State* L) {
    std::size_t len = 0;
    const char* text = luaL_checklstring(L, 1, &len);

    std::size_t first = 0;
    while (first < len && (unsigned char)text[first] <= ' ') ++first;

    std::size_t last = len;
    while (last > first && (unsigned char)text[last - 1] <= ' ') --last;

    lua_pushlstring(L, text + first, last - first);
    return 1;
}

int l_str_starts_with(lua_State* L) {
    std::size_t len = 0, prefixLen = 0;
    const char* text   = luaL_checklstring(L, 1, &len);
    const char* prefix = luaL_checklstring(L, 2, &prefixLen);

    lua_pushboolean(L,
        prefixLen <= len && std::memcmp(text, prefix, prefixLen) == 0 ? 1 : 0);
    return 1;
}

int l_str_ends_with(lua_State* L) {
    std::size_t len = 0, suffixLen = 0;
    const char* text   = luaL_checklstring(L, 1, &len);
    const char* suffix = luaL_checklstring(L, 2, &suffixLen);

    lua_pushboolean(L,
        suffixLen <= len
        && std::memcmp(text + (len - suffixLen), suffix, suffixLen) == 0 ? 1 : 0);
    return 1;
}

int l_str_contains(lua_State* L) {
    const std::string text   = luaL_checkstring(L, 1);
    const std::string needle = luaL_checkstring(L, 2);

    lua_pushboolean(L, text.find(needle) != std::string::npos ? 1 : 0);
    return 1;
}

int l_str_replace(lua_State* L) {
    const std::string text = luaL_checkstring(L, 1);
    const std::string from = luaL_checkstring(L, 2);
    const std::string to   = luaL_optstring(L, 3, "");

    if (from.empty())
        return luaL_error(L, "the search string cannot be empty");

    std::string out;
    std::size_t start = 0;

    while (true) {
        const std::size_t at = text.find(from, start);

        if (at == std::string::npos) {
            out.append(text, start, std::string::npos);
            break;
        }

        out.append(text, start, at - start);
        out += to;
        start = at + from.size();

        if (out.size() > kUtilMaxTextLen)
            return luaL_error(L, "the result string got too large");
    }

    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int l_str_lower(lua_State* L) {
    std::string text = luaL_checkstring(L, 1);
    for (char& c : text) c = (char)std::tolower((unsigned char)c);

    lua_pushlstring(L, text.data(), text.size());
    return 1;
}

int l_str_upper(lua_State* L) {
    std::string text = luaL_checkstring(L, 1);
    for (char& c : text) c = (char)std::toupper((unsigned char)c);

    lua_pushlstring(L, text.data(), text.size());
    return 1;
}

int l_str_pad(lua_State* L) {
    const std::string text  = luaL_checkstring(L, 1);
    const int         width = (int)luaL_checkinteger(L, 2);
    const char*       fill  = luaL_optstring(L, 3, " ");
    const bool        left  = lua_toboolean(L, 4) != 0;

    if (width < 0 || width > 4096)
        return luaL_error(L, "the width must be between 0 and 4096");

    const char pad = fill[0] ? fill[0] : ' ';

    std::string out = text;
    while ((int)out.size() < width) {
        if (left) out.insert(out.begin(), pad);
        else      out.push_back(pad);
    }

    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int l_str_number(lua_State* L) {
    const double value    = luaL_checknumber(L, 1);
    const int    decimals = (int)luaL_optinteger(L, 2, 0);
    const char*  sep      = luaL_optstring(L, 3, " ");

    if (decimals < 0 || decimals > 9)
        return luaL_error(L, "decimals must be between 0 and 9");

    char format[16];
    std::snprintf(format, sizeof(format), "%%.%df", decimals);

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), format, value);

    std::string digits = buffer;
    std::string tail;

    const std::size_t dot = digits.find('.');
    if (dot != std::string::npos) {
        tail   = digits.substr(dot);
        digits = digits.substr(0, dot);
    }

    std::string sign;
    if (!digits.empty() && (digits[0] == '-' || digits[0] == '+')) {
        sign   = digits.substr(0, 1);
        digits = digits.substr(1);
    }

    std::string grouped;
    int since = 0;
    for (std::size_t i = digits.size(); i-- > 0;) {
        grouped.insert(grouped.begin(), digits[i]);
        if (++since == 3 && i > 0) {
            grouped.insert(0, sep);
            since = 0;
        }
    }

    const std::string out = sign + grouped + tail;
    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int l_str_time(lua_State* L) {
    double seconds = luaL_checknumber(L, 1);
    if (seconds < 0.0) seconds = 0.0;

    const bool showMs = lua_isnoneornil(L, 2) ? true : lua_toboolean(L, 2) != 0;

    const int total   = (int)seconds;
    const int hours   = total / 3600;
    const int minutes = (total % 3600) / 60;
    const int secs    = total % 60;
    const int hundred = (int)((seconds - (double)total) * 100.0);

    char buffer[64];
    if (hours > 0)
        std::snprintf(buffer, sizeof(buffer), showMs ? "%d:%02d:%02d.%02d" : "%d:%02d:%02d",
                      hours, minutes, secs, hundred);
    else
        std::snprintf(buffer, sizeof(buffer), showMs ? "%d:%02d.%02d" : "%d:%02d",
                      minutes, secs, hundred);

    lua_pushstring(L, buffer);
    return 1;
}

void deepCopy(lua_State* L, int index, int depth) {
    if (depth > kUtilMaxCopyDepth) {
        luaL_error(L, "the table is nested too deeply to copy");
        return;
    }

    if (index < 0 && index > LUA_REGISTRYINDEX) index = lua_gettop(L) + index + 1;

    lua_newtable(L);
    const int target = lua_gettop(L);

    lua_pushnil(L);
    while (lua_next(L, index) != 0) {

        if (lua_istable(L, -1)) {
            lua_pushvalue(L, -2);
            deepCopy(L, lua_gettop(L) - 1, depth + 1);
            lua_rawset(L, target);
            lua_pop(L, 1);
        }
        else {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_rawset(L, target);
            lua_pop(L, 1);
        }
    }
}

int l_tbl_copy(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    deepCopy(L, 1, 0);
    return 1;
}

int l_tbl_count(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    int count = 0;
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        ++count;
        lua_pop(L, 1);
    }

    lua_pushinteger(L, count);
    return 1;
}

int l_tbl_keys(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_newtable(L);
    int index = 1;

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        lua_pushvalue(L, -2);
        lua_rawseti(L, -4, index++);
        lua_pop(L, 1);
    }

    return 1;
}

int l_tbl_values(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_newtable(L);
    int index = 1;

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        lua_pushvalue(L, -1);
        lua_rawseti(L, -4, index++);
        lua_pop(L, 1);
    }

    return 1;
}

int l_tbl_index_of(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    const int count = (int)lua_objlen(L, 1);
    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(L, 1, i);
        const bool same = lua_equal(L, 2, -1) != 0;
        lua_pop(L, 1);

        if (same) {
            lua_pushinteger(L, i);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

int l_tbl_contains(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    const int count = (int)lua_objlen(L, 1);
    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(L, 1, i);
        const bool same = lua_equal(L, 2, -1) != 0;
        lua_pop(L, 1);

        if (same) {
            lua_pushboolean(L, 1);
            return 1;
        }
    }

    lua_pushboolean(L, 0);
    return 1;
}

int l_tbl_reverse(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    const int count = (int)lua_objlen(L, 1);
    lua_newtable(L);

    for (int i = count; i >= 1; --i) {
        lua_rawgeti(L, 1, i);
        lua_rawseti(L, -2, count - i + 1);
    }

    return 1;
}

int l_tbl_slice(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    const int count = (int)lua_objlen(L, 1);
    int first = (int)luaL_optinteger(L, 2, 1);
    int last  = (int)luaL_optinteger(L, 3, count);

    if (first < 1)     first = 1;
    if (last  > count) last  = count;

    lua_newtable(L);
    int index = 1;

    for (int i = first; i <= last; ++i) {
        lua_rawgeti(L, 1, i);
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

int l_tbl_merge(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_newtable(L);
    const int target = lua_gettop(L);

    for (int source = 1; source <= 2; ++source) {
        lua_pushnil(L);
        while (lua_next(L, source) != 0) {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_rawset(L, target);
            lua_pop(L, 1);
        }
    }

    return 1;
}

int l_col_hsv(lua_State* L) {
    const double h = clampd(luaL_checknumber(L, 1), 0.0, 360.0) / 60.0;
    const double s = clampd(luaL_checknumber(L, 2), 0.0, 1.0);
    const double v = clampd(luaL_checknumber(L, 3), 0.0, 1.0);

    const int    sector = (int)std::floor(h) % 6;
    const double f      = h - std::floor(h);

    const double p = v * (1.0 - s);
    const double q = v * (1.0 - s * f);
    const double t = v * (1.0 - s * (1.0 - f));

    double r = v, g = t, b = p;
    switch (sector) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }

    lua_pushnumber(L, r * 255.0);
    lua_pushnumber(L, g * 255.0);
    lua_pushnumber(L, b * 255.0);
    lua_pushnumber(L, luaL_optnumber(L, 4, 255.0));
    return 4;
}

int l_col_to_hsv(lua_State* L) {
    const double r = clampd(luaL_checknumber(L, 1), 0.0, 255.0) / 255.0;
    const double g = clampd(luaL_checknumber(L, 2), 0.0, 255.0) / 255.0;
    const double b = clampd(luaL_checknumber(L, 3), 0.0, 255.0) / 255.0;

    const double max = std::max(r, std::max(g, b));
    const double min = std::min(r, std::min(g, b));
    const double delta = max - min;

    double hue = 0.0;
    if (delta > 0.00001) {
        if (max == r)      hue = 60.0 * std::fmod((g - b) / delta, 6.0);
        else if (max == g) hue = 60.0 * (((b - r) / delta) + 2.0);
        else               hue = 60.0 * (((r - g) / delta) + 4.0);
    }

    if (hue < 0.0) hue += 360.0;

    lua_pushnumber(L, hue);
    lua_pushnumber(L, max <= 0.0 ? 0.0 : delta / max);
    lua_pushnumber(L, max);
    return 3;
}

int l_col_lerp(lua_State* L) {
    const double t = clampd(luaL_checknumber(L, 9), 0.0, 1.0);

    for (int i = 0; i < 4; ++i) {
        const double from = luaL_optnumber(L, 1 + i, 255.0);
        const double to   = luaL_optnumber(L, 5 + i, 255.0);
        lua_pushnumber(L, from + (to - from) * t);
    }

    return 4;
}

int l_col_rainbow(lua_State* L) {
    const double speed  = clampd(luaL_optnumber(L, 1, 1.0), 0.05, 20.0);
    const double offset = luaL_optnumber(L, 2, 0.0);
    const double alpha  = luaL_optnumber(L, 3, 255.0);

    const double phase = std::fmod(Manager::get().now() * speed * 60.0 + offset, 360.0);

    lua_pushnumber(L, phase);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, alpha);

    const int base = lua_gettop(L) - 4;
    lua_pushcfunction(L, l_col_hsv);
    lua_insert(L, base + 1);
    lua_call(L, 4, 4);
    return 4;
}

int l_col_hex(lua_State* L) {
    std::string hex = luaL_checkstring(L, 1);

    if (!hex.empty() && hex[0] == '#') hex.erase(hex.begin());

    if (hex.size() != 6 && hex.size() != 8)
        return luaL_error(L, "a hex colour looks like RRGGBB or RRGGBBAA");

    int values[4] = { 255, 255, 255, 255 };

    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        int part = 0;

        for (int k = 0; k < 2; ++k) {
            const char c = hex[i + (std::size_t)k];
            int digit;

            if (c >= '0' && c <= '9')      digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else return luaL_error(L, "'%s' is not a hex colour", hex.c_str());

            part = part * 16 + digit;
        }

        values[i / 2] = part;
    }

    for (int i = 0; i < 4; ++i) lua_pushinteger(L, values[i]);
    return 4;
}

int l_col_to_hex(lua_State* L) {
    const int r = (int)clampd(luaL_checknumber(L, 1), 0.0, 255.0);
    const int g = (int)clampd(luaL_checknumber(L, 2), 0.0, 255.0);
    const int b = (int)clampd(luaL_checknumber(L, 3), 0.0, 255.0);

    char buffer[16];
    if (lua_isnoneornil(L, 4)) {
        std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", r, g, b);
    }
    else {
        const int a = (int)clampd(luaL_checknumber(L, 4), 0.0, 255.0);
        std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X%02X", r, g, b, a);
    }

    lua_pushstring(L, buffer);
    return 1;
}

int l_vec_add(lua_State* L) {
    lua_pushnumber(L, luaL_checknumber(L, 1) + luaL_checknumber(L, 3));
    lua_pushnumber(L, luaL_checknumber(L, 2) + luaL_checknumber(L, 4));
    return 2;
}

int l_vec_sub(lua_State* L) {
    lua_pushnumber(L, luaL_checknumber(L, 1) - luaL_checknumber(L, 3));
    lua_pushnumber(L, luaL_checknumber(L, 2) - luaL_checknumber(L, 4));
    return 2;
}

int l_vec_scale(lua_State* L) {
    const double factor = luaL_checknumber(L, 3);
    lua_pushnumber(L, luaL_checknumber(L, 1) * factor);
    lua_pushnumber(L, luaL_checknumber(L, 2) * factor);
    return 2;
}

int l_vec_length(lua_State* L) {
    const double x = luaL_checknumber(L, 1);
    const double y = luaL_checknumber(L, 2);

    lua_pushnumber(L, std::sqrt(x * x + y * y));
    return 1;
}

int l_vec_distance(lua_State* L) {
    const double dx = luaL_checknumber(L, 3) - luaL_checknumber(L, 1);
    const double dy = luaL_checknumber(L, 4) - luaL_checknumber(L, 2);

    lua_pushnumber(L, std::sqrt(dx * dx + dy * dy));
    return 1;
}

int l_vec_normalize(lua_State* L) {
    const double x = luaL_checknumber(L, 1);
    const double y = luaL_checknumber(L, 2);

    const double length = std::sqrt(x * x + y * y);
    if (length < 0.000001) {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 2;
    }

    lua_pushnumber(L, x / length);
    lua_pushnumber(L, y / length);
    return 2;
}

int l_vec_dot(lua_State* L) {
    lua_pushnumber(L,
        luaL_checknumber(L, 1) * luaL_checknumber(L, 3)
      + luaL_checknumber(L, 2) * luaL_checknumber(L, 4));
    return 1;
}

int l_vec_angle(lua_State* L) {
    const double dx = luaL_checknumber(L, 3) - luaL_checknumber(L, 1);
    const double dy = luaL_checknumber(L, 4) - luaL_checknumber(L, 2);

    lua_pushnumber(L, std::atan2(dy, dx) * 180.0 / 3.14159265358979323846);
    return 1;
}

int l_vec_rotate(lua_State* L) {
    const double x       = luaL_checknumber(L, 1);
    const double y       = luaL_checknumber(L, 2);
    const double degrees = luaL_checknumber(L, 3);

    const double radians = degrees * 3.14159265358979323846 / 180.0;
    const double sinv    = std::sin(radians);
    const double cosv    = std::cos(radians);

    lua_pushnumber(L, x * cosv - y * sinv);
    lua_pushnumber(L, x * sinv + y * cosv);
    return 2;
}

int l_utils_sign(lua_State* L) {
    const double value = luaL_checknumber(L, 1);
    lua_pushinteger(L, value > 0.0 ? 1 : (value < 0.0 ? -1 : 0));
    return 1;
}

int l_utils_map(lua_State* L) {
    const double value   = luaL_checknumber(L, 1);
    const double inMin   = luaL_checknumber(L, 2);
    const double inMax   = luaL_checknumber(L, 3);
    const double outMin  = luaL_checknumber(L, 4);
    const double outMax  = luaL_checknumber(L, 5);

    if (std::fabs(inMax - inMin) < 0.000001) {
        lua_pushnumber(L, outMin);
        return 1;
    }

    const double t = clampd((value - inMin) / (inMax - inMin), 0.0, 1.0);
    lua_pushnumber(L, outMin + (outMax - outMin) * t);
    return 1;
}

int l_utils_approach(lua_State* L) {
    const double current = luaL_checknumber(L, 1);
    const double target  = luaL_checknumber(L, 2);
    const double step    = std::fabs(luaL_checknumber(L, 3));

    if (std::fabs(target - current) <= step) {
        lua_pushnumber(L, target);
        return 1;
    }

    lua_pushnumber(L, current + (target > current ? step : -step));
    return 1;
}

double easeValue(const char* name, double t) {
    const double pi = 3.14159265358979323846;

    if (std::strcmp(name, "linear") == 0)      return t;
    if (std::strcmp(name, "in_quad") == 0)     return t * t;
    if (std::strcmp(name, "out_quad") == 0)    return t * (2.0 - t);
    if (std::strcmp(name, "in_out_quad") == 0)
        return t < 0.5 ? 2.0 * t * t : -1.0 + (4.0 - 2.0 * t) * t;
    if (std::strcmp(name, "in_cubic") == 0)    return t * t * t;
    if (std::strcmp(name, "out_cubic") == 0) {
        const double f = t - 1.0;
        return f * f * f + 1.0;
    }
    if (std::strcmp(name, "in_out_cubic") == 0)
        return t < 0.5 ? 4.0 * t * t * t
                       : (t - 1.0) * (2.0 * t - 2.0) * (2.0 * t - 2.0) + 1.0;
    if (std::strcmp(name, "in_sine") == 0)     return 1.0 - std::cos(t * pi / 2.0);
    if (std::strcmp(name, "out_sine") == 0)    return std::sin(t * pi / 2.0);
    if (std::strcmp(name, "in_out_sine") == 0) return -(std::cos(pi * t) - 1.0) / 2.0;
    if (std::strcmp(name, "in_expo") == 0)
        return t <= 0.0 ? 0.0 : std::pow(2.0, 10.0 * (t - 1.0));
    if (std::strcmp(name, "out_expo") == 0)
        return t >= 1.0 ? 1.0 : 1.0 - std::pow(2.0, -10.0 * t);
    if (std::strcmp(name, "in_back") == 0)     return t * t * (2.70158 * t - 1.70158);
    if (std::strcmp(name, "out_back") == 0) {
        const double f = t - 1.0;
        return 1.0 + f * f * (2.70158 * f + 1.70158);
    }
    if (std::strcmp(name, "out_elastic") == 0) {
        if (t <= 0.0 || t >= 1.0) return t;
        return std::pow(2.0, -10.0 * t) * std::sin((t * 10.0 - 0.75) * (2.0 * pi / 3.0)) + 1.0;
    }
    if (std::strcmp(name, "out_bounce") == 0) {
        const double n = 7.5625, d = 2.75;
        double x = t;

        if (x < 1.0 / d)      return n * x * x;
        if (x < 2.0 / d)    { x -= 1.5   / d; return n * x * x + 0.75;   }
        if (x < 2.5 / d)    { x -= 2.25  / d; return n * x * x + 0.9375; }
        x -= 2.625 / d;
        return n * x * x + 0.984375;
    }

    return -1.0;
}

int l_utils_ease(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    const double t   = clampd(luaL_checknumber(L, 2), 0.0, 1.0);

    const double value = easeValue(name, t);
    if (value < -0.5)
        return luaL_error(L, "unknown easing curve '%s'", name);

    lua_pushnumber(L, value);
    return 1;
}

int l_utils_random(lua_State* L) {
    const double low  = luaL_optnumber(L, 1, 0.0);
    const double high = luaL_optnumber(L, 2, 1.0);

    const double unit = (double)std::rand() / (double)RAND_MAX;
    lua_pushnumber(L, low + (high - low) * unit);
    return 1;
}

std::uint64_t fnv1a(const char* data, std::size_t size) {
    std::uint64_t hash = 1469598103934665603ull;

    for (std::size_t i = 0; i < size; ++i) {
        hash ^= (unsigned char)data[i];
        hash *= 1099511628211ull;
    }

    return hash;
}

int l_utils_hash(lua_State* L) {
    std::size_t len = 0;
    const char* text = luaL_checklstring(L, 1, &len);

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%016llx",
                  (unsigned long long)fnv1a(text, len));

    lua_pushstring(L, buffer);
    return 1;
}

constexpr char kBase64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int l_utils_base64_encode(lua_State* L) {
    std::size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);

    if (len > kUtilMaxTextLen)
        return luaL_error(L, "that string is too large to encode");

    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    for (std::size_t i = 0; i < len; i += 3) {
        const unsigned a = (unsigned char)data[i];
        const unsigned b = i + 1 < len ? (unsigned char)data[i + 1] : 0u;
        const unsigned c = i + 2 < len ? (unsigned char)data[i + 2] : 0u;

        const unsigned block = (a << 16) | (b << 8) | c;

        out += kBase64[(block >> 18) & 63u];
        out += kBase64[(block >> 12) & 63u];
        out += i + 1 < len ? kBase64[(block >> 6) & 63u] : '=';
        out += i + 2 < len ? kBase64[block & 63u]        : '=';
    }

    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+')             return 62;
    if (c == '/')             return 63;
    return -1;
}

int l_utils_base64_decode(lua_State* L) {
    std::size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);

    if (len > kUtilMaxTextLen)
        return luaL_error(L, "that string is too large to decode");

    std::string out;
    out.reserve(len / 4 * 3);

    unsigned block = 0;
    int      bits  = 0;

    for (std::size_t i = 0; i < len; ++i) {
        const char c = data[i];
        if (c == '=' || c == '\n' || c == '\r' || c == ' ') continue;

        const int value = base64Value(c);
        if (value < 0)
            return luaL_error(L, "that is not valid base64");

        block = (block << 6) | (unsigned)value;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            out += (char)((block >> bits) & 0xFFu);
        }
    }

    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int l_utils_date(lua_State* L) {
    const char* format = luaL_optstring(L, 1, "%H:%M:%S");

    if (std::strlen(format) > 64)
        return luaL_error(L, "that date format is too long");

    const std::time_t stamp = std::time(nullptr);

    std::tm parts{};
#ifdef _WIN32
    localtime_s(&parts, &stamp);
#else
    parts = *std::localtime(&stamp);
#endif

    char buffer[128];
    const std::size_t written = std::strftime(buffer, sizeof(buffer), format, &parts);

    lua_pushlstring(L, buffer, written);
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

void registerUtilApi(lua_State* L) {
    static const luaL_Reg kStr[] = {
        { "split",       l_str_split       },
        { "join",        l_str_join        },
        { "trim",        l_str_trim        },
        { "starts_with", l_str_starts_with },
        { "ends_with",   l_str_ends_with   },
        { "contains",    l_str_contains    },
        { "replace",     l_str_replace     },
        { "lower",       l_str_lower       },
        { "upper",       l_str_upper       },
        { "pad",         l_str_pad         },
        { "number",      l_str_number      },
        { "time",        l_str_time        },
        { nullptr,       nullptr           },
    };
    registerTable(L, "str", kStr);

    static const luaL_Reg kTbl[] = {
        { "copy",     l_tbl_copy     },
        { "count",    l_tbl_count    },
        { "keys",     l_tbl_keys     },
        { "values",   l_tbl_values   },
        { "index_of", l_tbl_index_of },
        { "contains", l_tbl_contains },
        { "reverse",  l_tbl_reverse  },
        { "slice",    l_tbl_slice    },
        { "merge",    l_tbl_merge    },
        { nullptr,    nullptr        },
    };
    registerTable(L, "tbl", kTbl);

    static const luaL_Reg kCol[] = {
        { "hsv",     l_col_hsv     },
        { "to_hsv",  l_col_to_hsv  },
        { "lerp",    l_col_lerp    },
        { "rainbow", l_col_rainbow },
        { "hex",     l_col_hex     },
        { "to_hex",  l_col_to_hex  },
        { nullptr,   nullptr       },
    };
    registerTable(L, "col", kCol);

    static const luaL_Reg kVec[] = {
        { "add",       l_vec_add       },
        { "sub",       l_vec_sub       },
        { "scale",     l_vec_scale     },
        { "length",    l_vec_length    },
        { "distance",  l_vec_distance  },
        { "normalize", l_vec_normalize },
        { "dot",       l_vec_dot       },
        { "angle",     l_vec_angle     },
        { "rotate",    l_vec_rotate    },
        { nullptr,     nullptr         },
    };
    registerTable(L, "vec", kVec);

    static const luaL_Reg kUtilsExtras[] = {
        { "sign",          l_utils_sign          },
        { "map",           l_utils_map           },
        { "approach",      l_utils_approach      },
        { "ease",          l_utils_ease          },
        { "random",        l_utils_random        },
        { "hash",          l_utils_hash          },
        { "base64_encode", l_utils_base64_encode },
        { "base64_decode", l_utils_base64_decode },
        { "date",          l_utils_date          },
        { nullptr,         nullptr               },
    };
    addFunctions(L, "utils", kUtilsExtras);
}

}
