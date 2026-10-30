// Part of fishmod_debug.c (#included there, sharing its statics): the auto-angler (FISHMOD_AUTO_ANGLER).
#ifdef FISHMOD_AUTO_ANGLER
    // after the script: play the fishing game from the rod's state (for testing the whole loop)
    if (!sScriptActive && sPlacedPlayer && (sScriptStepIndex >= (s32)ARRAY_COUNT(sScriptSteps))) {
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
                if ((sAnglerTimer % 60) == 0) {
                    button = BTN_CLEFT;
                }
                break;
            case 2: // wind-up: hold the rod button to about 37% power (the old 60% cast, before casts got longer)
                if (gFishmodRodInfo.castPower < 0.33f) {
                    button = BTN_CLEFT;
                }
                break;
            case 4: // float: strike on a real bite; nothing for 25 seconds, reel in and cast again
                if (gFishmodRodInfo.biting && ((sAnglerTimer % 4) == 0)) {
                    button = BTN_B;
                } else if (sAnglerFloatTimer > 500) {
                    button = BTN_A;
                }
                break;
            case 5: // on the ground: reel back
                button = BTN_A;
                break;
            case 6: // fight: reel while the tension is comfortable
                if (gFishmodRodInfo.tension < 60.0f) {
                    button = BTN_A;
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
