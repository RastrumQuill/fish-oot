// Part of fishmod_debug.c (#included there, sharing its statics): the auto-angler (FISHMOD_AUTO_ANGLER).
#ifdef FISHMOD_AUTO_ANGLER
    // after the script: play the fishing game from the rod's state (for testing the whole loop)
    if (!sScriptActive && sPlacedPlayer && (sScriptStepIndex >= sScript.numSteps)) {
        static s32 sAnglerTimer;
        static s32 sAnglerLastState;
        static s32 sAnglerLastBiting;
        static s32 sAnglerCasts;
        static s32 sAnglerBites;
        static s32 sAnglerLanded;
        static s32 sAnglerFloatTimer;
        u16 button = 0;
        s8 stickY = 0;

        sAnglerTimer++;
        // tallies, to compare builds by how the fishing goes
        if ((gFishmodRodInfo.state == 4) && (sAnglerLastState != 4)) {
            sAnglerCasts++;
        }
        if (gFishmodRodInfo.biting && !sAnglerLastBiting) {
            sAnglerBites++;
        }
        if ((gFishmodRodInfo.state == 7) && (sAnglerLastState != 7)) {
            sAnglerLanded++;
        }
        sAnglerFloatTimer = (gFishmodRodInfo.state == 4) ? sAnglerFloatTimer + 1 : 0;
        sAnglerLastState = gFishmodRodInfo.state;
        sAnglerLastBiting = gFishmodRodInfo.biting;
        Fishmod_DebugPrintf(11, "angler t%d casts %d bites %d landed %d", sAnglerTimer / 20, sAnglerCasts,
                            sAnglerBites, sAnglerLanded);
        switch (gFishmodRodInfo.state) {
            case 0: // rod away
                if ((sAnglerTimer % 40) == 0) {
                    button = BTN_CLEFT;
                }
                break;
            case 1: // held: cast every so often
#ifdef FISHMOD_ANGLER_RIG
                FISHMOD_SET_RIG(FISHMOD_ANGLER_RIG); // (the tackle on the line: a lure, the sinker)
#endif
#ifdef FISHMOD_ANGLER_BAIT
                // (a test of the bite and the fight: a bait on the hook every cast, as a bait bottle puts it)
                if (gFishmodRodInfo.bait == FISHMOD_BAIT_NONE) {
                    gFishmodRodInfo.baitRequest = FISHMOD_ANGLER_BAIT;
                }
#endif
                if ((sAnglerTimer % 60) == 0) {
                    button = BTN_CLEFT;
                }
                break;
            case 2: // wind-up: hold the rod button to about 37% power (the old 60% cast, before casts got longer)
#ifndef FISHMOD_ANGLER_POWER
#define FISHMOD_ANGLER_POWER 0.33f
#endif
                if (gFishmodRodInfo.castPower < FISHMOD_ANGLER_POWER) {
                    button = BTN_CLEFT;
                }
                break;
            case 4: // float: strike on a real bite; nothing for 25 seconds, reel in and cast again
                if (gFishmodRodInfo.biting && ((sAnglerTimer % 4) == 0)) {
                    button = BTN_B;
                } else if (sAnglerFloatTimer > 500) {
                    button = BTN_A;
                }
#if defined(FISHMOD_ANGLER_RIG) && (FISHMOD_ANGLER_RIG != 0) && (FISHMOD_ANGLER_RIG != 3)
                // a lure worked stop-and-go: a second's reeling, then three still
                else if ((sAnglerFloatTimer > 120) && ((sAnglerFloatTimer % 80) < 20)) {
                    button = BTN_A;
                }
#endif
                break;
            case 5: // on the ground: reel back
                button = BTN_A;
                break;
            case 6: // fight: reel while the tension is comfortable
#ifdef FISHMOD_ANGLER_SLOW_FIGHT
                {
                    // (the fight drawn out, for looking at its indicators: only enough reeling to keep the line from
                    // going slack for the first 15 seconds of each)
                    static s32 sFightFrames;
                    static s32 sFightLastTimer;

                    sFightFrames = (sAnglerTimer == sFightLastTimer + 1) ? sFightFrames + 1 : 0;
                    sFightLastTimer = sAnglerTimer;
                    if (sFightFrames < 300) {
                        if (gFishmodRodInfo.tension < 30.0f) {
                            button = BTN_A;
                        }
                        break;
                    }
                }
#endif
                if (gFishmodRodInfo.landOpen) {
                    // the landing: A only while the window's open (a press outside it puts the next one off)
                    button = ((sAnglerTimer & 1) == 0) ? BTN_A : 0;
                } else if (gFishmodRodInfo.tension < 60.0f) {
                    button = BTN_A;
                }
                if (gFishmodRodInfo.leaping) {
                    stickY = -70; // (the rod down for a leap)
                }
                break;
            case 7: // landed: A through the catch textbox (its cursor starts on keeping it)
                if ((gFishmodRodInfo.choice != 0) && ((sAnglerTimer % 8) == 0)) {
                    button = BTN_A;
                }
                break;
        }
        input->prev = input->cur;
        input->cur.button = button;
        input->cur.stick_x = 0;
        input->cur.stick_y = stickY;
        buttonDiff = input->prev.button ^ input->cur.button;
        input->press.button = input->cur.button & buttonDiff;
        input->rel.button = input->prev.button & buttonDiff;
        PadUtils_UpdateRelXY(input);
        input->press.stick_x = 0;
        input->press.stick_y = 0;
        return;
    }
#endif
