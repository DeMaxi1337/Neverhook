
#include "LuaEngine.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>

#if __has_include(<Geode/ui/SceneManager.hpp>)
    #include <Geode/ui/SceneManager.hpp>
    #define NH_HAS_SCENE_MANAGER 1
#endif

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <concepts>
#include <cstring>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace nh::lua {
namespace {

constexpr int kMaxNodes    = 512;
constexpr int kMaxChildren = 256;
constexpr int kMaxTextLen  = 512;

char kNodeMetaKey = 0;

struct NodeSlot {
    int         handle = 0;
    std::string owner;
    CCNode*     node    = nullptr;
    bool        created = false;
};

std::vector<NodeSlot> g_nodes;
int                   g_nextNode = 1;
CCNode*               g_world    = nullptr;

class LuaCallbackTarget : public CCObject {
public:
    static LuaCallbackTarget* create(const std::string& owner, int ref) {
        auto* target = new LuaCallbackTarget();
        target->m_owner = owner;
        target->m_ref   = ref;
        target->autorelease();
        return target;
    }

    void onCallback(CCObject*) { runWidgetCallback(m_owner, m_ref); }

    const std::string& owner() const { return m_owner; }

private:
    std::string m_owner;
    int         m_ref = -1;
};

std::vector<LuaCallbackTarget*> g_targets;

std::string ownerId(lua_State* L) {
    Script* script = ownerOf(L);
    return script ? script->id() : std::string();
}

NodeSlot* slotOf(int handle) {
    for (auto& slot : g_nodes)
        if (slot.handle == handle) return &slot;
    return nullptr;
}

int pushNode(lua_State* L, CCNode* node, bool created) {
    if (!node) {
        lua_pushnil(L);
        return 1;
    }

    for (const auto& slot : g_nodes) {
        if (slot.node == node) {
            lua_pushinteger(L, slot.handle);
            return 1;
        }
    }

    if ((int)g_nodes.size() >= kMaxNodes) {

        return luaL_error(L, "too many node handles are alive (%d max), "
                             "free some with node.forget()", kMaxNodes);
    }

    NodeSlot slot;
    slot.handle  = g_nextNode++;
    slot.owner   = ownerId(L);
    slot.node    = node;
    slot.created = created;

    if (created) node->retain();
    g_nodes.push_back(slot);

    lua_pushinteger(L, slot.handle);
    return 1;
}

CCNode* nodeArg(lua_State* L, int index) {
    const int handle = (int)luaL_checkinteger(L, index);

    NodeSlot* slot = slotOf(handle);
    if (!slot || !slot->node) {
        luaL_error(L, "that node handle is not valid any more");
        return nullptr;
    }

    return slot->node;
}

CCNode* parentArg(lua_State* L, int index) {
    if (lua_isnoneornil(L, index)) {
        auto* director = CCDirector::sharedDirector();
        return director ? director->getRunningScene() : nullptr;
    }

    return nodeArg(L, index);
}

void dropSlot(NodeSlot& slot) {
    if (!slot.node) return;

    if (slot.created) {
        if (slot.node->getParent()) slot.node->removeFromParent();
        slot.node->release();
    }

    slot.node = nullptr;
}

bool safeName(const char* name) {
    return name && *name
        && !std::strchr(name, '/')
        && !std::strchr(name, '\\')
        && !std::strstr(name, "..")
        && !std::strchr(name, ':');
}

template <class T>
concept HasNodeId = requires(T* node) { node->getID(); };

template <class T>
concept HasSetNodeId = requires(T* node) { node->setID(std::string("x")); };

template <class T>
concept HasChildById = requires(T* node) { node->getChildByID(std::string("x")); };

template <class T>
concept HasQuerySelector = requires(T* node) { node->querySelector(std::string("x")); };

template <class T>
std::string nodeIdOf(T* node) {
    if constexpr (HasNodeId<T>) return node->getID();
    else                        return std::string();
}

template <class T>
void setNodeId(T* node, const std::string& id) {
    if constexpr (HasSetNodeId<T>) node->setID(id);
}

template <class T>
CCNode* childById(T* node, const std::string& id) {
    if constexpr (HasChildById<T>) return node->getChildByID(id);
    else                          return nullptr;
}

template <class T>
CCNode* queryFor(T* node, const std::string& selector) {
    if constexpr (HasQuerySelector<T>) return node->querySelector(selector);
    else                              return nullptr;
}

int l_node_scene(lua_State* L) {
    auto* director = CCDirector::sharedDirector();
    return pushNode(L, director ? director->getRunningScene() : nullptr, false);
}

int l_node_play_layer(lua_State* L) {
    return pushNode(L, PlayLayer::get(), false);
}

int l_node_editor(lua_State* L) {
    return pushNode(L, LevelEditorLayer::get(), false);
}

int l_node_scene_name(lua_State* L) {
    if (PlayLayer::get())        { lua_pushstring(L, "play");   return 1; }
    if (LevelEditorLayer::get()) { lua_pushstring(L, "editor"); return 1; }

    auto* director = CCDirector::sharedDirector();
    CCScene* scene = director ? director->getRunningScene() : nullptr;
    if (!scene) { lua_pushstring(L, "other"); return 1; }

    lua_pushstring(L, scene->getChildrenCount() > 0 ? "menu" : "other");
    return 1;
}

int l_node_find(lua_State* L) {
    CCNode* parent = parentArg(L, 1);
    const char* id = luaL_checkstring(L, 2);

    if (!parent) { lua_pushnil(L); return 1; }
    return pushNode(L, childById(parent, std::string(id)), false);
}

int l_node_query(lua_State* L) {
    CCNode* parent        = parentArg(L, 1);
    const char* selector  = luaL_checkstring(L, 2);

    if (!parent) { lua_pushnil(L); return 1; }
    return pushNode(L, queryFor(parent, std::string(selector)), false);
}

int l_node_child_count(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    lua_pushinteger(L, node ? (int)node->getChildrenCount() : 0);
    return 1;
}

int l_node_child_at(lua_State* L) {
    CCNode*   node  = nodeArg(L, 1);
    const int index = (int)luaL_checkinteger(L, 2);

    if (!node || index < 1 || index > (int)node->getChildrenCount()) {
        lua_pushnil(L);
        return 1;
    }

    return pushNode(L, node->getChildren()
        ? static_cast<CCNode*>(node->getChildren()->objectAtIndex((unsigned)(index - 1)))
        : nullptr, false);
}

int l_node_children(lua_State* L) {
    CCNode* node = nodeArg(L, 1);

    lua_newtable(L);
    if (!node || !node->getChildren()) return 1;

    int written = 0;
    for (auto* child : CCArrayExt<CCNode*>(node->getChildren())) {
        if (written >= kMaxChildren) break;
        if (!child) continue;

        pushNode(L, child, false);
        lua_rawseti(L, -2, ++written);
    }

    return 1;
}

int l_node_parent(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    return pushNode(L, node ? node->getParent() : nullptr, false);
}

int l_node_id(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) { lua_pushnil(L); return 1; }

    const std::string id = nodeIdOf(node);
    lua_pushlstring(L, id.data(), id.size());
    return 1;
}

int l_node_set_id(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const char* id = luaL_checkstring(L, 2);

    if (node) setNodeId(node, std::string(id));
    return 0;
}

int l_node_label(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    const char* font = luaL_optstring(L, 2, "bigFont.fnt");

    if (std::strlen(text) > kMaxTextLen)
        return luaL_error(L, "that label text is too long");

    if (!safeName(font))
        return luaL_error(L, "only plain font names are allowed");

    return pushNode(L, CCLabelBMFont::create(text, font), true);
}

int l_node_sprite(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    if (!safeName(name))
        return luaL_error(L, "only plain sprite names are allowed, no paths");

    CCSprite* sprite = CCSprite::createWithSpriteFrameName(name);
    if (!sprite) sprite = CCSprite::create(name);

    return pushNode(L, sprite, true);
}

int l_node_sprite_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const std::string file = path ? path : "";

    if (file.empty() || file.size() > 512)
        return luaL_error(L, "node.sprite_file() needs a path from draw.image_path()");

    const std::string root =
        geode::utils::string::pathToString(Manager::get().imagesDir());

    if (file.find("..") != std::string::npos || file.compare(0, root.size(), root) != 0)
        return luaL_error(L, "node.sprite_file() only accepts files from the images folder");

    CCTexture2D* tex = CCTextureCache::sharedTextureCache()->addImage(file.c_str(), false);
    if (!tex) return luaL_error(L, "that image could not be loaded");

    CCSprite* sprite = CCSprite::createWithTexture(tex);
    if (!sprite) return luaL_error(L, "that image could not be loaded");

    return pushNode(L, sprite, true);
}

int l_node_button_sprite(lua_State* L) {
    const char* text    = luaL_checkstring(L, 1);
    const char* texture = luaL_optstring(L, 2, "GJ_button_01.png");
    const int   width   = (int)luaL_optinteger(L, 3, 160);

    if (std::strlen(text) > 128)
        return luaL_error(L, "that button caption is too long");

    if (!safeName(texture))
        return luaL_error(L, "only plain texture names are allowed");

    return pushNode(L, ButtonSprite::create(text, width, true, "bigFont.fnt",
                                            texture, 28.f, 0.5f), true);
}

int l_node_button(lua_State* L) {
    CCNode* sprite = nodeArg(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    if (!sprite) { lua_pushnil(L); return 1; }

    lua_pushvalue(L, 2);
    const int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    LuaCallbackTarget* target = LuaCallbackTarget::create(ownerId(L), ref);
    target->retain();
    g_targets.push_back(target);

    auto* item = CCMenuItemSpriteExtra::create(
        sprite, target, menu_selector(LuaCallbackTarget::onCallback));

    return pushNode(L, item, true);
}

int l_node_menu(lua_State* L) {
    CCMenu* menu = CCMenu::create();

    if (menu) {
        menu->setPosition(0.f, 0.f);
        menu->setContentSize(CCSizeMake(0.f, 0.f));
    }

    return pushNode(L, menu, true);
}

int l_node_layer_color(lua_State* L) {
    const int r = (int)luaL_optinteger(L, 1, 0);
    const int g = (int)luaL_optinteger(L, 2, 0);
    const int b = (int)luaL_optinteger(L, 3, 0);
    const int a = (int)luaL_optinteger(L, 4, 255);

    auto* layer = CCLayerColor::create(ccc4(
        (GLubyte)r, (GLubyte)g, (GLubyte)b, (GLubyte)a));

    if (layer && !lua_isnoneornil(L, 5)) {
        const float w = (float)luaL_checknumber(L, 5);
        const float h = (float)luaL_checknumber(L, 6);
        layer->setContentSize(CCSizeMake(w, h));
    }

    return pushNode(L, layer, true);
}

int l_node_scale9(lua_State* L) {
    const char* texture = luaL_optstring(L, 1, "GJ_square01.png");

    if (!safeName(texture))
        return luaL_error(L, "only plain texture names are allowed");

    auto* sprite = cocos2d::extension::CCScale9Sprite::create(texture);

    if (sprite && !lua_isnoneornil(L, 2)) {
        const float w = (float)luaL_checknumber(L, 2);
        const float h = (float)luaL_checknumber(L, 3);
        sprite->setContentSize(CCSizeMake(w, h));
    }

    return pushNode(L, sprite, true);
}

int l_node_empty(lua_State* L) {
    return pushNode(L, CCNode::create(), true);
}

int l_node_add(lua_State* L) {
    CCNode* parent = parentArg(L, 1);
    CCNode* child  = nodeArg(L, 2);
    const int z    = (int)luaL_optinteger(L, 3, 0);

    if (!parent || !child) return 0;

    if (typeinfo_cast<CCSpriteBatchNode*>(parent))
        return luaL_error(L, "that parent is a sprite batch, it only accepts the "
                             "game's own sprites, add to node.object_layer() instead");

    for (const auto& slot : g_nodes) {
        if (slot.node != child || slot.created) continue;

        return luaL_error(L, "node.add(parent, child) can only move nodes your script "
                             "created, not the game's own nodes");
    }

    if (parent == child)
        return luaL_error(L, "a node cannot be added to itself");

    if (child->getParent()) child->removeFromParent();
    parent->addChild(child, z);
    return 0;
}

int l_node_remove(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (node && node->getParent()) node->removeFromParent();
    return 0;
}

int l_node_remove_children(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (node) node->removeAllChildren();
    return 0;
}

int l_node_forget(lua_State* L) {
    const int handle = (int)luaL_checkinteger(L, 1);

    for (std::size_t i = 0; i < g_nodes.size(); ++i) {
        if (g_nodes[i].handle != handle) continue;

        if (g_nodes[i].node && g_nodes[i].created) g_nodes[i].node->release();
        g_nodes.erase(g_nodes.begin() + (long)i);
        break;
    }

    return 0;
}

int l_node_keep_across_scenes(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

#ifdef NH_HAS_SCENE_MANAGER
    geode::SceneManager::get()->keepAcrossScenes(node);
    return 0;
#else
    return luaL_error(L, "this build cannot keep nodes across scenes");
#endif
}

int l_node_pos(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    lua_pushnumber(L, node->getPositionX());
    lua_pushnumber(L, node->getPositionY());
    return 2;
}

int l_node_set_pos(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float x = (float)luaL_checknumber(L, 2);
    const float y = (float)luaL_checknumber(L, 3);

    if (node) node->setPosition(x, y);
    return 0;
}

int l_node_move(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float dx = (float)luaL_checknumber(L, 2);
    const float dy = (float)luaL_checknumber(L, 3);

    if (node) node->setPosition(node->getPositionX() + dx, node->getPositionY() + dy);
    return 0;
}

int l_node_world_pos(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    const CCPoint point = node->convertToWorldSpace(CCPointMake(0.f, 0.f));
    lua_pushnumber(L, point.x);
    lua_pushnumber(L, point.y);
    return 2;
}

int l_node_size(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    const CCSize size = node->getContentSize();
    lua_pushnumber(L, size.width);
    lua_pushnumber(L, size.height);
    return 2;
}

int l_node_set_size(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float w = (float)luaL_checknumber(L, 2);
    const float h = (float)luaL_checknumber(L, 3);

    if (node) node->setContentSize(CCSizeMake(w, h));
    return 0;
}

int l_node_scale(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    lua_pushnumber(L, node->getScaleX());
    lua_pushnumber(L, node->getScaleY());
    return 2;
}

int l_node_set_scale(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float sx = (float)luaL_checknumber(L, 2);
    const float sy = (float)luaL_optnumber(L, 3, sx);

    if (node) {
        node->setScaleX(sx);
        node->setScaleY(sy);
    }
    return 0;
}

int l_node_rotation(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    lua_pushnumber(L, node ? node->getRotation() : 0.f);
    return 1;
}

int l_node_set_rotation(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (node) node->setRotation((float)luaL_checknumber(L, 2));
    return 0;
}

int l_node_set_anchor(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float x = (float)luaL_checknumber(L, 2);
    const float y = (float)luaL_checknumber(L, 3);

    if (node) node->setAnchorPoint(CCPointMake(x, y));
    return 0;
}

int l_node_zorder(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    lua_pushinteger(L, node ? node->getZOrder() : 0);
    return 1;
}

int l_node_set_zorder(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (node) node->setZOrder((int)luaL_checkinteger(L, 2));
    return 0;
}

int l_node_visible(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    lua_pushboolean(L, node && node->isVisible() ? 1 : 0);
    return 1;
}

int l_node_set_visible(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (node) node->setVisible(lua_toboolean(L, 2) != 0);
    return 0;
}

CCRGBAProtocol* rgbaOf(CCNode* node) {
    return node ? typeinfo_cast<CCRGBAProtocol*>(node) : nullptr;
}

int l_node_opacity(lua_State* L) {
    CCRGBAProtocol* rgba = rgbaOf(nodeArg(L, 1));
    lua_pushinteger(L, rgba ? (int)rgba->getOpacity() : 255);
    return 1;
}

int l_node_set_opacity(lua_State* L) {
    CCRGBAProtocol* rgba = rgbaOf(nodeArg(L, 1));

    int value = (int)luaL_checkinteger(L, 2);
    if (value < 0)   value = 0;
    if (value > 255) value = 255;

    if (rgba) rgba->setOpacity((GLubyte)value);
    return 0;
}

int l_node_color(lua_State* L) {
    CCRGBAProtocol* rgba = rgbaOf(nodeArg(L, 1));
    if (!rgba) return 0;

    const ccColor3B color = rgba->getColor();
    lua_pushinteger(L, color.r);
    lua_pushinteger(L, color.g);
    lua_pushinteger(L, color.b);
    return 3;
}

int l_node_set_color(lua_State* L) {
    CCRGBAProtocol* rgba = rgbaOf(nodeArg(L, 1));

    const int r = (int)luaL_checkinteger(L, 2);
    const int g = (int)luaL_checkinteger(L, 3);
    const int b = (int)luaL_checkinteger(L, 4);

    if (rgba) rgba->setColor(ccc3((GLubyte)r, (GLubyte)g, (GLubyte)b));
    return 0;
}

int l_node_set_text(lua_State* L) {
    CCNode*     node = nodeArg(L, 1);
    const char* text = luaL_checkstring(L, 2);

    if (std::strlen(text) > kMaxTextLen)
        return luaL_error(L, "that text is too long");

    if (auto* bm = typeinfo_cast<CCLabelBMFont*>(node)) {
        bm->setString(text);
        return 0;
    }

    if (auto* ttf = typeinfo_cast<CCLabelTTF*>(node)) {
        ttf->setString(text);
        return 0;
    }

    if (auto* button = typeinfo_cast<ButtonSprite*>(node)) {
        button->setString(text);
        return 0;
    }

    return luaL_error(L, "that node has no text");
}

int l_node_text(lua_State* L) {
    CCNode* node = nodeArg(L, 1);

    if (auto* bm = typeinfo_cast<CCLabelBMFont*>(node)) {
        lua_pushstring(L, bm->getString());
        return 1;
    }

    if (auto* ttf = typeinfo_cast<CCLabelTTF*>(node)) {
        lua_pushstring(L, ttf->getString());
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

CCActionInterval* applyEase(lua_State* L, CCActionInterval* action, const char* name) {
    if (!action || !name || !*name) return action;

    if (std::strcmp(name, "linear") == 0)  return action;
    if (std::strcmp(name, "in") == 0)      return CCEaseIn::create(action, 2.f);
    if (std::strcmp(name, "out") == 0)     return CCEaseOut::create(action, 2.f);
    if (std::strcmp(name, "in_out") == 0)  return CCEaseInOut::create(action, 2.f);
    if (std::strcmp(name, "sine_in") == 0) return CCEaseSineIn::create(action);
    if (std::strcmp(name, "sine_out") == 0)return CCEaseSineOut::create(action);
    if (std::strcmp(name, "elastic") == 0) return CCEaseElasticOut::create(action, 0.3f);
    if (std::strcmp(name, "bounce") == 0)  return CCEaseBounceOut::create(action);
    if (std::strcmp(name, "back") == 0)    return CCEaseBackOut::create(action);

    luaL_error(L, "unknown easing curve '%s'", name);
    return action;
}

double durationArg(lua_State* L, int index) {
    double duration = luaL_checknumber(L, index);

    if (duration < 0.0)   duration = 0.0;
    if (duration > 600.0) duration = 600.0;

    return duration;
}

int l_node_move_to(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float x = (float)luaL_checknumber(L, 2);
    const float y = (float)luaL_checknumber(L, 3);
    const double duration = durationArg(L, 4);

    if (!node) return 0;

    CCActionInterval* action = CCMoveTo::create((float)duration, CCPointMake(x, y));
    node->runAction(applyEase(L, action, luaL_optstring(L, 5, "linear")));
    return 0;
}

int l_node_move_by(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float dx = (float)luaL_checknumber(L, 2);
    const float dy = (float)luaL_checknumber(L, 3);
    const double duration = durationArg(L, 4);

    if (!node) return 0;

    CCActionInterval* action = CCMoveBy::create((float)duration, CCPointMake(dx, dy));
    node->runAction(applyEase(L, action, luaL_optstring(L, 5, "linear")));
    return 0;
}

int l_node_scale_to(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float scale = (float)luaL_checknumber(L, 2);
    const double duration = durationArg(L, 3);

    if (!node) return 0;

    CCActionInterval* action = CCScaleTo::create((float)duration, scale);
    node->runAction(applyEase(L, action, luaL_optstring(L, 4, "linear")));
    return 0;
}

int l_node_rotate_to(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float degrees = (float)luaL_checknumber(L, 2);
    const double duration = durationArg(L, 3);

    if (!node) return 0;

    CCActionInterval* action = CCRotateTo::create((float)duration, degrees);
    node->runAction(applyEase(L, action, luaL_optstring(L, 4, "linear")));
    return 0;
}

int l_node_rotate_by(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float degrees = (float)luaL_checknumber(L, 2);
    const double duration = durationArg(L, 3);

    if (!node) return 0;

    CCActionInterval* action = CCRotateBy::create((float)duration, degrees);
    node->runAction(applyEase(L, action, luaL_optstring(L, 4, "linear")));
    return 0;
}

int l_node_fade_to(lua_State* L) {
    CCNode* node = nodeArg(L, 1);

    int opacity = (int)luaL_checkinteger(L, 2);
    if (opacity < 0)   opacity = 0;
    if (opacity > 255) opacity = 255;

    const double duration = durationArg(L, 3);
    if (!node) return 0;

    node->runAction(CCFadeTo::create((float)duration, (GLubyte)opacity));
    return 0;
}

int l_node_tint_to(lua_State* L) {
    CCNode* node = nodeArg(L, 1);

    const int r = (int)luaL_checkinteger(L, 2);
    const int g = (int)luaL_checkinteger(L, 3);
    const int b = (int)luaL_checkinteger(L, 4);
    const double duration = durationArg(L, 5);

    if (!node) return 0;

    node->runAction(CCTintTo::create((float)duration,
        (GLubyte)r, (GLubyte)g, (GLubyte)b));
    return 0;
}

int l_node_pulse(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const float scale = (float)luaL_optnumber(L, 2, 1.1);
    const double duration = durationArg(L, 3);

    if (!node) return 0;

    const float base = node->getScale();

    auto* up   = CCScaleTo::create((float)duration * 0.5f, scale);
    auto* down = CCScaleTo::create((float)duration * 0.5f, base);
    auto* seq  = CCSequence::create(up, down, nullptr);

    node->runAction(CCRepeatForever::create(seq));
    return 0;
}

int l_node_delay_then(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    const double duration = durationArg(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    if (!node) return 0;

    lua_pushvalue(L, 3);
    const int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    LuaCallbackTarget* target = LuaCallbackTarget::create(ownerId(L), ref);
    target->retain();
    g_targets.push_back(target);

    auto* delay = CCDelayTime::create((float)duration);
    auto* call  = CCCallFuncO::create(
        target, callfuncO_selector(LuaCallbackTarget::onCallback), nullptr);

    node->runAction(CCSequence::create(delay, call, nullptr));
    return 0;
}

int l_node_stop_actions(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (node) node->stopAllActions();
    return 0;
}

int l_node_count(lua_State* L) {
    lua_pushinteger(L, (int)g_nodes.size());
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

}

void releaseScriptNodes(const std::string& owner) {
    for (auto& slot : g_nodes) {
        if (slot.owner != owner) continue;
        dropSlot(slot);
    }

    g_nodes.erase(
        std::remove_if(g_nodes.begin(), g_nodes.end(),
            [&](const NodeSlot& slot) { return slot.owner == owner; }),
        g_nodes.end());

    for (auto* target : g_targets) {
        if (!target || target->owner() != owner) continue;
        target->release();
    }

    g_targets.erase(
        std::remove_if(g_targets.begin(), g_targets.end(),
            [&](LuaCallbackTarget* target) {
                return !target || target->owner() == owner;
            }),
        g_targets.end());
}

void updateNodes() {
    auto*    director = CCDirector::sharedDirector();
    CCScene* scene    = director ? director->getRunningScene() : nullptr;

    static CCScene* lastScene = nullptr;
    if (scene == lastScene) return;

    lastScene = scene;

    if (g_world) {
        if (g_world->getParent()) g_world->removeFromParent();
        g_world->release();
        g_world = nullptr;
    }

    for (auto& slot : g_nodes)
        if (!slot.created) slot.node = nullptr;

    g_nodes.erase(
        std::remove_if(g_nodes.begin(), g_nodes.end(),
            [](const NodeSlot& slot) { return slot.node == nullptr; }),
        g_nodes.end());
}

int l_node_player(lua_State* L) {
    GJBaseGameLayer* gl = GJBaseGameLayer::get();
    return pushNode(L, gl ? gl->m_player1 : nullptr, false);
}

int l_node_player2(lua_State* L) {
    GJBaseGameLayer* gl = GJBaseGameLayer::get();
    return pushNode(L, gl ? gl->m_player2 : nullptr, false);
}

class WorldContainer : public CCNode {
public:
    static WorldContainer* create() {
        auto* container = new WorldContainer();

        if (!container->init()) {
            delete container;
            return nullptr;
        }

        container->autorelease();
        container->scheduleUpdate();
        return container;
    }

    void update(float) override {
        GJBaseGameLayer* gl = GJBaseGameLayer::get();
        if (!gl) return;

        CCNode* src = gl->m_objectLayer;
        if (!src || src == this) return;
        if (typeinfo_cast<CCSpriteBatchNode*>(src)) return;

        setPosition(src->getPosition());
        setScale(src->getScale());
        setRotation(src->getRotation());
    }
};

CCNode* worldParent() {
    GJBaseGameLayer* gl = GJBaseGameLayer::get();
    if (!gl) return nullptr;

    if (g_world) {
        if (g_world->getParent() == gl) return g_world;

        if (g_world->getParent()) g_world->removeFromParent();
        g_world->release();
        g_world = nullptr;
    }

    WorldContainer* container = WorldContainer::create();
    if (!container) return nullptr;

    setNodeId(container, std::string("nh-lua-world"));

    CCNode* src = gl->m_objectLayer;
    if (src && !typeinfo_cast<CCSpriteBatchNode*>(src)) {
        container->setPosition(src->getPosition());
        container->setScale(src->getScale());
    }

    gl->addChild(container, 500);
    container->retain();
    g_world = container;

    return container;
}

int l_node_valid(lua_State* L) {
    const int handle = (int)luaL_checkinteger(L, 1);
    NodeSlot* slot   = slotOf(handle);

    lua_pushboolean(L, (slot && slot->node) ? 1 : 0);
    return 1;
}

int l_node_object_layer(lua_State* L) {
    return pushNode(L, worldParent(), false);
}

int l_node_shadow(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return luaL_error(L, "that node handle is not valid any more");

    CCSprite* sprite = typeinfo_cast<CCSprite*>(node);
    if (!sprite) return luaL_error(L, "node.shadow() needs a sprite");

    if (typeinfo_cast<CCSpriteBatchNode*>(sprite->getParent()))
        return luaL_error(L, "that sprite lives in a sprite batch and cannot hold a shadow");

    const double dx      = luaL_optnumber(L, 2, 3.0);
    const double dy      = luaL_optnumber(L, 3, -3.0);
    double       opacity = luaL_optnumber(L, 4, 120.0);
    double       spread  = luaL_optnumber(L, 5, 1.05);

    if (opacity < 0.0)   opacity = 0.0;
    if (opacity > 255.0) opacity = 255.0;
    if (spread < 0.1)    spread = 0.1;
    if (spread > 4.0)    spread = 4.0;

    CCSprite* shadow = CCSprite::createWithTexture(sprite->getTexture(), sprite->getTextureRect());
    if (!shadow) return luaL_error(L, "that sprite cannot cast a shadow");

    const CCSize size = sprite->getContentSize();

    shadow->setAnchorPoint({ 0.5f, 0.5f });
    shadow->setPosition({ (float)(size.width * 0.5 + dx), (float)(size.height * 0.5 + dy) });
    shadow->setColor({ 0, 0, 0 });
    shadow->setOpacity((GLubyte)opacity);
    shadow->setScale((float)spread);
    shadow->setFlipX(sprite->isFlipX());
    shadow->setFlipY(sprite->isFlipY());

    sprite->addChild(shadow, -1);

    return pushNode(L, shadow, true);
}

int l_node_smooth_to(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    const double x = luaL_checknumber(L, 2);
    const double y = luaL_checknumber(L, 3);
    double factor  = luaL_optnumber(L, 4, 0.15);

    if (factor < 0.0) factor = 0.0;
    if (factor > 1.0) factor = 1.0;

    const CCPoint at = node->getPosition();
    node->setPosition({
        (float)(at.x + (x - at.x) * factor),
        (float)(at.y + (y - at.y) * factor),
    });
    return 0;
}

int l_node_smooth_rotate(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    const double target = luaL_checknumber(L, 2);
    double factor       = luaL_optnumber(L, 3, 0.15);

    if (factor < 0.0) factor = 0.0;
    if (factor > 1.0) factor = 1.0;

    double delta = target - node->getRotation();
    while (delta > 180.0)  delta -= 360.0;
    while (delta < -180.0) delta += 360.0;

    node->setRotation((float)(node->getRotation() + delta * factor));
    return 0;
}

int l_node_smooth_scale(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    const double target = luaL_checknumber(L, 2);
    double factor       = luaL_optnumber(L, 3, 0.15);

    if (factor < 0.0) factor = 0.0;
    if (factor > 1.0) factor = 1.0;

    const double at = node->getScale();
    node->setScale((float)(at + (target - at) * factor));
    return 0;
}

int l_node_set_flip(lua_State* L) {
    CCNode* node = nodeArg(L, 1);
    if (!node) return 0;

    CCSprite* sprite = typeinfo_cast<CCSprite*>(node);
    if (!sprite) return 0;

    sprite->setFlipX(lua_toboolean(L, 2) != 0);
    sprite->setFlipY(lua_toboolean(L, 3) != 0);
    return 0;
}

void registerNodeApi(lua_State* L) {
    static const luaL_Reg kNode[] = {

        { "scene",              l_node_scene              },
        { "valid",              l_node_valid              },
        { "player",             l_node_player             },
        { "player2",            l_node_player2            },
        { "object_layer",       l_node_object_layer       },
        { "shadow",             l_node_shadow             },
        { "smooth_to",          l_node_smooth_to          },
        { "smooth_rotate",      l_node_smooth_rotate      },
        { "smooth_scale",       l_node_smooth_scale       },
        { "set_flip",           l_node_set_flip           },
        { "sprite_file",        l_node_sprite_file        },
        { "play_layer",         l_node_play_layer         },
        { "editor",             l_node_editor             },
        { "scene_name",         l_node_scene_name         },
        { "find",               l_node_find               },
        { "query",              l_node_query              },
        { "child_count",        l_node_child_count        },
        { "child_at",           l_node_child_at           },
        { "children",           l_node_children           },
        { "parent",             l_node_parent             },
        { "id",                 l_node_id                 },
        { "set_id",             l_node_set_id             },

        { "label",              l_node_label              },
        { "sprite",             l_node_sprite             },
        { "button_sprite",      l_node_button_sprite      },
        { "button",             l_node_button             },
        { "menu",               l_node_menu               },
        { "layer_color",        l_node_layer_color        },
        { "scale9",             l_node_scale9             },
        { "empty",              l_node_empty              },

        { "add",                l_node_add                },
        { "remove",             l_node_remove             },
        { "remove_children",    l_node_remove_children    },
        { "forget",             l_node_forget             },
        { "keep_across_scenes", l_node_keep_across_scenes },
        { "count",              l_node_count              },

        { "pos",                l_node_pos                },
        { "set_pos",            l_node_set_pos            },
        { "move",               l_node_move               },
        { "world_pos",          l_node_world_pos          },
        { "size",               l_node_size               },
        { "set_size",           l_node_set_size           },
        { "scale",              l_node_scale              },
        { "set_scale",          l_node_set_scale          },
        { "rotation",           l_node_rotation           },
        { "set_rotation",       l_node_set_rotation       },
        { "set_anchor",         l_node_set_anchor         },
        { "zorder",             l_node_zorder             },
        { "set_zorder",         l_node_set_zorder         },
        { "visible",            l_node_visible            },
        { "set_visible",        l_node_set_visible        },
        { "opacity",            l_node_opacity            },
        { "set_opacity",        l_node_set_opacity        },
        { "color",              l_node_color              },
        { "set_color",          l_node_set_color          },
        { "text",               l_node_text               },
        { "set_text",           l_node_set_text           },

        { "move_to",            l_node_move_to            },
        { "move_by",            l_node_move_by            },
        { "scale_to",           l_node_scale_to           },
        { "rotate_to",          l_node_rotate_to          },
        { "rotate_by",          l_node_rotate_by          },
        { "fade_to",            l_node_fade_to            },
        { "tint_to",            l_node_tint_to            },
        { "pulse",              l_node_pulse              },
        { "delay_then",         l_node_delay_then         },
        { "stop_actions",       l_node_stop_actions       },
        { nullptr,              nullptr                   },
    };

    registerTable(L, "node", kNode);

    (void)kNodeMetaKey;
}

}
