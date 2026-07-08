#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <vector>
#include "../Config.hpp"

using namespace geode::prelude;

namespace {
    Speed nhStartSpeed(int id) {
        switch (id) {
        case 200:  return Speed::Slow;
        case 202:  return Speed::Fast;
        case 203:  return Speed::Faster;
        case 1334: return Speed::Fastest;
        default:   return Speed::Normal;
        }
    }

    int nhStartMode(int id) {
        switch (id) {
        case 13:   return 1;
        case 47:   return 2;
        case 111:  return 3;
        case 660:  return 4;
        case 745:  return 5;
        case 1331: return 6;
        case 1933: return 7;
        default:   return 0;
        }
    }

    GameObject* nhLast(std::vector<GameObject*> const& list, float x) {
        GameObject* result = nullptr;
        for (auto o : list) {
            if (o->m_positionX > x) break;
            result = o;
        }
        return result;
    }
}

class $modify(NHSmartStartPos, PlayLayer) {
    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();

        if (!Config::get().smartStartpos) return;

        std::vector<StartPosObject*> starts;
        std::vector<GameObject*> speed, mode, size, mirror, dual, gravity;

        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            if (auto sp = typeinfo_cast<StartPosObject*>(obj)) {
                starts.push_back(sp);
                continue;
            }
            switch (obj->m_objectID) {
            case 200: case 201: case 202: case 203: case 1334: speed.push_back(obj); break;
            case 12: case 13: case 47: case 111: case 660: case 745: case 1331: case 1933: mode.push_back(obj); break;
            case 99:  case 101: size.push_back(obj); break;
            case 45:  case 46:  mirror.push_back(obj); break;
            case 286: case 287: dual.push_back(obj); break;
            case 10:  case 11:  case 2926: gravity.push_back(obj); break;
            default: break;
            }
        }

        auto cmp = [](GameObject* a, GameObject* b) { return a->m_positionX < b->m_positionX; };
        std::sort(speed.begin(), speed.end(), cmp);
        std::sort(mode.begin(), mode.end(), cmp);
        std::sort(size.begin(), size.end(), cmp);
        std::sort(mirror.begin(), mirror.end(), cmp);
        std::sort(dual.begin(), dual.end(), cmp);
        std::sort(gravity.begin(), gravity.end(), cmp);

        for (auto sp : starts) {
            auto st = sp->m_startSettings;
            if (!st) continue;
            float x = sp->m_positionX;

            if (auto o = nhLast(speed, x))  st->m_startSpeed = nhStartSpeed(o->m_objectID);
            if (auto o = nhLast(mode, x))   st->m_startMode = nhStartMode(o->m_objectID);
            if (auto o = nhLast(size, x))   st->m_startMini = o->m_objectID == 101;
            if (auto o = nhLast(mirror, x)) st->m_mirrorMode = o->m_objectID == 45;
            if (auto o = nhLast(dual, x))   st->m_startDual = o->m_objectID == 286;

            bool flipped = false;
            for (auto o : gravity) {
                if (o->m_positionX > x) break;
                if (o->m_objectID == 10) flipped = false;
                else if (o->m_objectID == 11) flipped = true;
                else if (o->m_objectID == 2926) flipped = !flipped;
            }
            st->m_isFlipped = flipped;
        }
    }
};