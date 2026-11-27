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
#ifdef FISHMOD_ANGLER_FIGHT_TEST
        gFishmodFightTest = FISHMOD_ANGLER_FIGHT_TEST; // (what the hooked fish are made to do: FIGHT_TEST_*)
#endif
#if defined(FISHMOD_ANGLER_QUICK) && FISHMOD_TESTMODE
        // (more fights to the minute: the nibbles close together and the fights short, by the feel tuner's numbers)
        gFishmodFeel[FEEL_NIBBLE_GAP] = 0.2f;
        gFishmodFeel[FEEL_FIGHT_LENGTH] = 0.5f;
#endif
#ifdef FISHMOD_ANGLER_BAITKEEP
        // The bait's keeping, tested (readings in gFishmodFT[FT_KEEP + n], the last a 1 when it's through):
        //   a worm on the hook; the pond lure on (0: bait, 1: set aside); a worm bottle used with the lure on (2: bait,
        //   3: set aside, 4: rig, 5: bottles changed); to another scene and the rod out there (6: bait); the lure on
        //   again (7, 8) and back (9: bait, 10: set aside); the lure off (11: bait, 12: set aside)
        {
            static s32 sKeepPhase;
            static s32 sKeepWait;
            static s32 sKeepBottles;
            static s16 sKeepScene = -1;
            PlayState* keepPlay = (PlayState*)gameState;
            s32* out = &gFishmodFT[FT_KEEP];
            s32 bottles = 0;
            s32 k;

            for (k = 0; k < FISHMOD_BOTTLES; k++) {
                bottles = bottles * 31 + INV_SLOT(FISHMOD_BOTTLE_SLOT(k)) * 256 + FISHMOD_SAVE->bottleFish[k].lengthTenths;
            }
            if (sKeepScene < 0) {
                sKeepScene = keepPlay->sceneId;
            }
            button = 0;
            if (gFishmodRodInfo.state == 0) {
                sKeepWait = 0;
                if ((keepPlay->transitionTrigger == TRANS_TRIGGER_OFF) && (keepPlay->transitionMode == TRANS_MODE_OFF) &&
                    ((sAnglerTimer % 40) == 0)) {
                    button = BTN_CLEFT; // the rod out
                }
            } else if ((gFishmodRodInfo.state == 1) && (++sKeepWait >= 25)) {
                sKeepWait = 0;
                switch (sKeepPhase++) {
                    case 0:
                        FISHMOD_SET_RIG(FISHMOD_RIG_BOBBER);
                        gFishmodRodInfo.baitRequest = FISHMOD_BAIT_WORM;
                        break;
                    case 1:
                        FISHMOD_SET_RIG(FISHMOD_RIG_LURE_POND);
                        break;
                    case 2:
                        out[0] = gFishmodRodInfo.bait;
                        out[1] = gFishmodRodInfo.spareBait;
                        sKeepBottles = bottles;
                        Fishmod_UseItem(keepPlay, ITEM_BOTTLE_WORM, 0);
                        break;
                    case 3:
                        out[2] = gFishmodRodInfo.bait;
                        out[3] = gFishmodRodInfo.spareBait;
                        out[4] = FISHMOD_RIG();
                        out[5] = (bottles != sKeepBottles);
                        keepPlay->nextEntranceIndex = ENTR_HYRULE_FIELD_4;
                        keepPlay->transitionTrigger = TRANS_TRIGGER_START;
                        keepPlay->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
                        break;
                    case 4:
                        if (keepPlay->sceneId == sKeepScene) {
                            sKeepPhase--; // (not there yet)
                            break;
                        }
                        out[6] = gFishmodRodInfo.bait;
                        FISHMOD_SET_RIG(FISHMOD_RIG_LURE_POND);
                        break;
                    case 5:
                        out[7] = gFishmodRodInfo.bait;
                        out[8] = gFishmodRodInfo.spareBait;
                        keepPlay->nextEntranceIndex = ENTR_LAKE_HYLIA_0;
                        keepPlay->transitionTrigger = TRANS_TRIGGER_START;
                        keepPlay->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
                        break;
                    case 6:
                        if (keepPlay->sceneId != sKeepScene) {
                            sKeepPhase--;
                            break;
                        }
                        out[9] = gFishmodRodInfo.bait;
                        out[10] = gFishmodRodInfo.spareBait;
                        FISHMOD_SET_RIG(FISHMOD_RIG_BOBBER);
                        break;
                    case 7:
                        out[11] = gFishmodRodInfo.bait;
                        out[12] = gFishmodRodInfo.spareBait;
                        out[15] = 1;
                        break;
                    default:
                        sKeepPhase = 8;
                        break;
                }
            }
            goto angler_input;
        }
#endif
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
#ifdef FISHMOD_ANGLER_MISS_EVERY
                // (the first bite of every so many let go by: a missed bite)
                if (gFishmodRodInfo.biting && ((sAnglerBites % FISHMOD_ANGLER_MISS_EVERY) == 1)) {
                    break;
                }
#endif
#ifdef FISHMOD_ANGLER_CALL
                // (a fish called to the bait, as test mode's L + D-pad Right: the fights don't wait on luck)
                if (!gFishmodRodInfo.biting && !gFishmodRodInfo.nibbling && (sAnglerFloatTimer > 40) &&
                    ((sAnglerFloatTimer % 60) == 0)) {
                    gFishmodTestCallFish = 1;
                }
#endif
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
                } else if (gFishmodRodInfo.landing) {
                    // (waiting for the window: a press of A outside it puts the next one off, and the angler's
                    // reeling did, every time the tension let it: no fish of its was ever landed)
                    button = 0;
                } else if (gFishmodRodInfo.tension < 60.0f) {
                    button = BTN_A;
                }
#ifndef FISHMOD_ANGLER_NO_DIP // (the rod left up: the hook may be thrown)
                if (gFishmodRodInfo.leaping) {
                    stickY = -70; // (the rod down for a leap)
                }
#endif
                break;
            case 7: // landed: A through the catch textbox (its cursor starts on keeping it)
                if ((gFishmodRodInfo.choice != 0) && ((sAnglerTimer % 8) == 0)) {
                    button = BTN_A;
                }
                break;
        }
#ifdef FISHMOD_ANGLER_BAITKEEP
    angler_input:
#endif
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
