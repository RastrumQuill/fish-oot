#include "console_logo_state.h"
#include "attributes.h"
#include "setup_state.h"
#include "printf.h"
#include "translation.h"
#include "save.h"
#include "fishmod.h"
#include "play_state.h"
#include "cic6105.h"

void Setup_InitImpl(SetupState* this) {
    PRINTF(T("ゼルダ共通データ初期化\n", "Zelda common data initialization\n"));
    SaveContext_Init();
    this->state.running = false;
#if FISHMOD_DEBUG
    if (!Fishmod_DebugNormalBoot()) {
        Fishmod_DebugBoot();
        // the title screen, skipped here, is what turns on the audio's RSP tasks: without this every script ran with
        // the sound mixed on the CPU but never on the RSP (silent, and the RSP's audio time missing from the benches)
        CIC6105_EnableAudio();
        SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);
        return;
    }
#endif
    SET_NEXT_GAMESTATE(&this->state, ConsoleLogo_Init, ConsoleLogoState);
}

void Setup_Destroy(UNUSED GameState* thisx) {
}

void Setup_Init(GameState* thisx) {
    SetupState* this = (SetupState*)thisx;

    this->state.destroy = Setup_Destroy;
    Setup_InitImpl(this);
}
