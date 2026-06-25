#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "../Config.hpp"

using namespace geode::prelude;

namespace {
    const ccColor3B COL_SOLID       = { 0,   0,   255 };
    const ccColor3B COL_HAZARD      = { 255, 0,   0   };
    const ccColor3B COL_PASSABLE    = { 0,   255, 255 };
    const ccColor3B COL_INTERACT    = { 0,   255, 0   };
    const ccColor3B COL_PLAYER      = { 255, 0,   0   };
    const ccColor3B COL_PLAYER_ROT  = { 170, 0,   0   };

    const float OUTLINE = 0.35f;

    enum class Kind { Skip, Solid, Hazard, Passable, Interact };

    bool typeIn(GameObject* o, std::initializer_list<GameObjectType> list) {
        for (auto t : list)
            if (o->m_objectType == t)
                return true;
        return false;
    }

    Kind classify(GameObject* o) {
        if (typeIn(o, { GameObjectType::Hazard, GameObjectType::AnimatedHazard }))
            return Kind::Hazard;
        if (o->m_isPassable || typeIn(o, { GameObjectType::Breakable }))
            return Kind::Passable;
        if (typeIn(o, { GameObjectType::Solid, GameObjectType::Slope }))
            return Kind::Solid;
        if (typeIn(o, {
                GameObjectType::UserCoin, GameObjectType::Collectible, GameObjectType::SecretCoin,
                GameObjectType::YellowJumpPad, GameObjectType::PinkJumpPad, GameObjectType::RedJumpPad,
                GameObjectType::GravityPad, GameObjectType::SpiderPad,
                GameObjectType::YellowJumpRing, GameObjectType::PinkJumpRing, GameObjectType::RedJumpRing,
                GameObjectType::GravityRing, GameObjectType::GreenRing, GameObjectType::DropRing,
                GameObjectType::CustomRing, GameObjectType::DashRing, GameObjectType::GravityDashRing,
                GameObjectType::SpiderOrb, GameObjectType::TeleportOrb, GameObjectType::Modifier }))
            return Kind::Interact;
        return Kind::Skip;
    }

    ccColor4F strokeColour(Kind k) {
        switch (k) {
            case Kind::Hazard:   return ccc4FFromccc3B(COL_HAZARD);
            case Kind::Solid:    return ccc4FFromccc3B(COL_SOLID);
            case Kind::Passable: return ccc4FFromccc3B(COL_PASSABLE);
            case Kind::Interact: return ccc4FFromccc3B(COL_INTERACT);
            default:             return ccc4FFromccc3B(ccWHITE);
        }
    }
}

class NHHitboxNode : public CCDrawNode {
public:
    static NHHitboxNode* create() {
        auto ret = new (std::nothrow) NHHitboxNode();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCDrawNode::init())
            return false;
        setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
        return true;
    }

    void rebuild() {
        clear();

        auto game = GJBaseGameLayer::get();
        if (!game)
            return;

        auto& cfg = Config::get();
        bool dead = game->m_player1 && game->m_player1->m_isDead;
        if (!cfg.showHitboxes && !(cfg.showHitboxesOnDeath && dead))
            return;

        walkSections(game);

        if (game->m_player1)
            strokePlayer(game->m_player1);
        if (game->m_player2 && game->m_player2->isRunning())
            strokePlayer(game->m_player2);
    }

private:
    bool drawable(GameObject* o) {
        if (!o || o->m_isDecoration || o->m_isDecoration2 || o->m_unk3ee)
            return false;
        return classify(o) != Kind::Skip;
    }

    cocos2d::CCRect rectOf(GameObject* o) {
        if (o->m_isObjectRectDirty) {
            bool d = o->m_isObjectRectDirty;
            bool b = o->m_boxOffsetCalculated;
            auto r = o->getObjectRect();
            o->m_isObjectRectDirty = d;
            o->m_boxOffsetCalculated = b;
            return r;
        }
        return o->m_objectRect;
    }

    void strokeObject(GameObject* o) {
        auto col = strokeColour(classify(o));
        ccColor4F hollow = { col.r, col.g, col.b, 0.f };

        if (o->m_objectRadius != 0) {
            drawCircle(
                ccp(o->m_positionX, o->m_positionY),
                o->m_objectRadius * std::max<float>(o->m_scaleX, o->m_scaleY),
                hollow, OUTLINE, col, 60);
            return;
        }

        auto r = rectOf(o);
        CCPoint quad[4] = {
            ccp(r.getMinX(), r.getMinY()),
            ccp(r.getMaxX(), r.getMinY()),
            ccp(r.getMaxX(), r.getMaxY()),
            ccp(r.getMinX(), r.getMaxY()),
        };
        if (o->m_orientedBox) {
            quad[0] = o->m_orientedBox->m_corners[0];
            quad[1] = o->m_orientedBox->m_corners[1];
            quad[2] = o->m_orientedBox->m_corners[2];
            quad[3] = o->m_orientedBox->m_corners[3];
        }
        drawPolygon(quad, 4, hollow, OUTLINE, col);
    }

    void strokeRect(cocos2d::CCRect const& r, ccColor4F const& col) {
        ccColor4F hollow = { col.r, col.g, col.b, 0.f };
        CCPoint quad[4] = {
            ccp(r.getMinX(), r.getMinY()),
            ccp(r.getMaxX(), r.getMinY()),
            ccp(r.getMaxX(), r.getMaxY()),
            ccp(r.getMinX(), r.getMaxY()),
        };
        drawPolygon(quad, 4, hollow, OUTLINE, col);
    }

    void strokePlayer(PlayerObject* p) {
        auto reg   = ccc4FFromccc3B(COL_PLAYER);
        auto rot   = ccc4FFromccc3B(COL_PLAYER_ROT);
        auto inner = ccc4FFromccc3B(COL_SOLID);

        if (auto ob = p->m_orientedBox) {
            ccColor4F hollow = { rot.r, rot.g, rot.b, 0.f };
            CCPoint quad[4] = {
                ob->m_corners[0], ob->m_corners[1], ob->m_corners[2], ob->m_corners[3],
            };
            drawPolygon(quad, 4, hollow, OUTLINE, rot);
        }

        strokeRect(p->getObjectRect(p->m_vehicleSize, p->m_vehicleSize), reg);
        strokeRect(p->getObjectRect(0.3f, 0.3f), inner);
    }

    void walkSections(GJBaseGameLayer* game) {
        int colCount = game->m_sections.empty() ? -1 : (int)game->m_sections.size();

        for (int i = game->m_leftSectionIndex; i <= game->m_rightSectionIndex && i < colCount; ++i) {
            if (i < 0) continue;
            auto column = game->m_sections[i];
            if (!column) continue;

            int rowCount = (int)column->size();
            for (int j = game->m_bottomSectionIndex; j <= game->m_topSectionIndex && j < rowCount; ++j) {
                if (j < 0) continue;
                auto section = column->at(j);
                if (!section) continue;

                int n = (int)section->size();
                if (i < (int)game->m_sectionSizes.size() && game->m_sectionSizes[i]
                    && j < (int)game->m_sectionSizes[i]->size()) {
                    n = std::min(n, game->m_sectionSizes[i]->at(j));
                }

                for (int k = 0; k < n; ++k) {
                    auto obj = section->at(k);
                    if (!obj) continue;
                    if (obj == game->m_player1CollisionBlock) continue;
                    if (obj == game->m_player2CollisionBlock) continue;
                    if (obj == game->m_anticheatSpike) continue;
                    if (drawable(obj)) strokeObject(obj);
                }
            }
        }
    }
};

class $modify(NHHitboxHook, PlayLayer) {
    struct Fields {
        NHHitboxNode* node = nullptr;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!m_fields->node && m_objectLayer) {
            m_fields->node = NHHitboxNode::create();
            if (m_fields->node)
                m_objectLayer->addChild(m_fields->node, 1000);
        }

        if (m_fields->node)
            m_fields->node->rebuild();
    }
};
