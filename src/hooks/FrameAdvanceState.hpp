#pragma once
#include "../Config.hpp"

namespace nh {
    // --- input state (set by the keyboard hook / on-screen control) ---
    inline bool  faPressed     = false; // step key pressed this frame (non-repeat)
    inline bool  faDown        = false; // step key currently held
    inline float faHoldDelay   = 0.0f;  // time held before auto-repeat starts
    inline int   faHoldAdvance = 0;     // auto-repeat tick counter
    inline bool  faKeyWaiting  = false; // rebinding: waiting for next key
}
