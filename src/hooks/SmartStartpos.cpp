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

    GameObject* nhClosest(std::vector<GameObject*> const& list, float x) {
        GameObject* result = nullptr;
        for (auto o : list) {
            if (o->m_positionX - 10.f > x) break;
            if (o->m_positionX - 10.f < x) result = o;
        }
        return result;
    }
}

class $modify(NHSmartStartPos, PlayLayer) {
    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();

        if (!Config::get().smartStartpos || !m_objects) return;

        std::vector<StartPosObject*> starts;
        std::vector<GameObject*> portals;

        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            if (auto sp = typeinfo_cast<StartPosObject*>(obj)) {
                starts.push_back(sp);
            } else {
                switch (obj->m_objectID) {
                case 200: case 201: case 202: case 203: case 1334:
                case 12: case 13: case 47: case 111: case 660: case 745: case 1331: case 1933:
                case 99: case 101:
                case 45: case 46:
                case 286: case 287:
                case 10: case 11:
                case 2903: case 2904:
                    portals.push_back(obj);
                    break;
                default: break;
                }
            }
        }

        if (starts.empty()) return;

        std::sort(portals.begin(), portals.end(), [](GameObject* a, GameObject* b) {
            return a->m_positionX < b->m_positionX;
        });

        LevelSettingsObject* ls = m_levelSettings;

        for (auto sp : starts) {
            auto st = sp->m_startSettings;
            if (!st) continue;
            float x = sp->m_positionX;

            GameObject* lastSpeed   = nullptr;
            GameObject* lastMode    = nullptr;
            GameObject* lastSize    = nullptr;
            GameObject* lastMirror  = nullptr;
            GameObject* lastDual    = nullptr;
            GameObject* lastGravity = nullptr;
            GameObject* lastReverse = nullptr;

            for (auto o : portals) {
                if (o->m_positionX - 10.f > x) break;
                int id = o->m_objectID;
                if (id == 200 || id == 201 || id == 202 || id == 203 || id == 1334) lastSpeed = o;
                else if (id == 12 || id == 13 || id == 47 || id == 111 || id == 660 || id == 745 || id == 1331 || id == 1933) lastMode = o;
                else if (id == 99 || id == 101) lastSize = o;
                else if (id == 45 || id == 46) lastMirror = o;
                else if (id == 286 || id == 287) lastDual = o;
                else if (id == 10 || id == 11) lastGravity = o;
                else if (id == 2903 || id == 2904) lastReverse = o;
            }

            st->m_startSpeed      = lastSpeed   ? nhStartSpeed(lastSpeed->m_objectID) : (ls ? ls->m_startSpeed : Speed::Normal);
            st->m_startMode       = lastMode    ? nhStartMode(lastMode->m_objectID)   : (ls ? ls->m_startMode : 0);
            st->m_startMini       = lastSize    ? (lastSize->m_objectID == 101)        : (ls ? ls->m_startMini : false);
            st->m_mirrorMode      = lastMirror  ? (lastMirror->m_objectID == 45)      : (ls ? ls->m_mirrorMode : false);
            st->m_startDual       = lastDual    ? (lastDual->m_objectID == 286)       : (ls ? ls->m_startDual : false);
            st->m_isFlipped       = lastGravity ? (lastGravity->m_objectID == 11)      : (ls ? ls->m_isFlipped : false);
            st->m_reverseGameplay = lastReverse ? (lastReverse->m_objectID == 2903)    : (ls ? ls->m_reverseGameplay : false);

            if (ls) {
                st->m_twoPlayerMode  = ls->m_twoPlayerMode;
                st->m_rotateGameplay = ls->m_rotateGameplay;
                st->m_noTimePenalty  = ls->m_noTimePenalty;
                st->m_spawnGroup     = ls->m_spawnGroup;
            }
        }
    }
};
