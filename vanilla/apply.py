#!/usr/bin/env python3
"""Adds the vanilla bench (vanilla/README.md) to an unmodified decomp tree: its files, and the hooks it needs in the
game's own (each edit checks its anchor, and stops if one isn't there exactly once).

Usage: vanilla/apply.py TREE   (a checkout of the decomp import, commit 2a34508)
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


PENDING = {}  # path -> new text: written only once every edit has found its anchor


def edit(tree, path, pairs, include=False):
    p = os.path.join(tree, path)
    s = PENDING.get(p) or open(p).read()
    for old, new in pairs:
        n = s.count(old)
        if n != 1:
            sys.exit(f"{path}: the anchor appears {n} times: {old[:60]!r} (nothing written)")
        s = s.replace(old, new, 1)
    if include:
        s = '#include "vbench.h"\n' + s
    PENDING[p] = s


SECTS = [
    ("Object_UpdateEntries(&this->objectCtx);", "VB_SECT_OBJECTS"),
    ("Room_ProcessRoomRequest(this, &this->roomCtx);", "VB_SECT_ROOMREQ"),
    ("CollisionCheck_AT(this, &this->colChkCtx);", "VB_SECT_COL_AT"),
    ("CollisionCheck_OC(this, &this->colChkCtx);", "VB_SECT_COL_OC"),
    ("CollisionCheck_Damage(this, &this->colChkCtx);", "VB_SECT_COL_DAMAGE"),
    ("CollisionCheck_ClearContext(this, &this->colChkCtx);", "VB_SECT_COL_CLEAR"),
    ("Actor_UpdateAll(this, &this->actorCtx);", "VB_SECT_ACTORS"),
    ("Cutscene_UpdateManual(this, &this->csCtx);", "VB_SECT_CUTSCENE"),
    ("Cutscene_UpdateScripted(this, &this->csCtx);", "VB_SECT_CUTSCENE"),
    ("Effect_UpdateAll(this);", "VB_SECT_EFFECTS"),
    ("EffectSs_UpdateAll(this);", "VB_SECT_EFFECTS"),
    ("func_80095AA0(this, &this->roomCtx.curRoom, &input[1], 0);", "VB_SECT_ROOMFX"),
    ("func_80095AA0(this, &this->roomCtx.prevRoom, &input[1], 1);", "VB_SECT_ROOMFX"),
    ("Skybox_Update(&this->skyboxCtx);", "VB_SECT_SKYBOX"),
    ("KaleidoScopeCall_Update(this);", "VB_SECT_MESSAGE"),
    ("GameOver_Update(this);", "VB_SECT_MESSAGE"),
    ("Message_Update(this);", "VB_SECT_MESSAGE"),
    ("Interface_Update(this);", "VB_SECT_INTERFACE"),
    ("            AnimTaskQueue_Update(this, &this->animTaskQueue);", "VB_SECT_ANIMTASK"),  # (not Play_Init's)
    ("SfxSource_UpdateAll(this);", "VB_SECT_SFXSRC"),
    ("Camera_Update(this->cameraPtrs[i]);", "VB_SECT_CAMERA"),
    ("Camera_Update(this->cameraPtrs[this->nextCamId]);", "VB_SECT_CAMERA"),
]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    tree = sys.argv[1]
    edit(tree, "spec/spec", [('    include "$(BUILD_DIR)/src/code/z_play.o"\n',
                              '    include "$(BUILD_DIR)/src/code/z_play.o"\n    include "$(BUILD_DIR)/src/code/vbench.o"\n')])
    edit(tree, "src/code/title_setup.c", [
        ('#include "save.h"\n', '#include "save.h"\n#include "play_state.h"\n'),
        ("    this->state.running = false;\n",
         "    this->state.running = false;\n    VBench_Boot();\n    SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);\n"
         "    return;\n"),
    ], include=True)
    edit(tree, "src/code/game.c", [
        ("    PadMgr_RequestPadData(&gPadMgr, gameState->input, true);\n",
         "    PadMgr_RequestPadData(&gPadMgr, gameState->input, true);\n    VBench_Input(gameState->input);\n"),
        ("#if DEBUG_FEATURES\n    sLastButtonPressed", "    VBench_DrawText(&newDList);\n\n#if DEBUG_FEATURES\n    sLastButtonPressed"),
    ], include=True)
    play = [("Rand_Seed((u32)osGetTime());", "Rand_Seed(0x46495348); // (vbench: fixed, as the mod's scripted runs)")]
    play += [("    Input* input = this->state.input;\n    s32 isPaused;\n    STACK_PAD(s32);\n",
              "    Input* input = this->state.input;\n    s32 isPaused;\n    STACK_PAD(s32);\n\n"
              "    VB_SECT(VB_SECT_FM_DEBUG, VBench_Update(this));\n")]
    play += [(call, call[:len(call) - len(call.lstrip())] + f"VB_SECT({slot}, {call.strip()[:-1]});")
             for call, slot in SECTS]
    play += [("    Environment_Update(this, &this->envCtx, &this->lightCtx, &this->pauseCtx, &this->msgCtx, &this->gameOverCtx,\n"
              "                       this->state.gfxCtx);",
              "    VB_SECT(VB_SECT_ENV, Environment_Update(this, &this->envCtx, &this->lightCtx, &this->pauseCtx, &this->msgCtx,\n"
              "                                            &this->gameOverCtx, this->state.gfxCtx));")]
    play += [("        Play_Update(this);\n    }\n\n    PLAY_LOG(4583);\n\n    Play_Draw(this);\n",
              "        OSTime vbStart = osGetTime();\n\n        Play_Update(this);\n"
              "        gVBenchCpuTime[0] += osGetTime() - vbStart;\n    }\n\n    PLAY_LOG(4583);\n\n"
              "    {\n        OSTime vbStart = osGetTime();\n\n        Play_Draw(this);\n"
              "        gVBenchCpuTime[1] += osGetTime() - vbStart;\n    }\n")]
    edit(tree, "src/code/z_play.c", play, include=True)
    edit(tree, "src/code/z_actor.c", [
        ("            DynaPoly_UpdateContext(play, &play->colCtx.dyna);",
         "            VB_SECT(VB_SECT_DYNAPOLY, DynaPoly_UpdateContext(play, &play->colCtx.dyna));"),
        ("                    actor->update(actor, play);\n                    DynaPoly_UnsetAllInteractFlags",
         "                    {\n                        OSTime vbStart = osGetTime();\n                        u32 vbCycles;\n\n"
         "                        actor->update(actor, play);\n                        vbCycles = (u32)(osGetTime() - vbStart);\n"
         "                        if ((u32)actor->id < VB_ACTOR_IDS) {\n"
         "                            gVBenchActorCycles[actor->id] += vbCycles;\n                        }\n"
         "                        gVBenchCatCycles[i] += vbCycles;\n                    }\n"
         "                    DynaPoly_UnsetAllInteractFlags"),
    ], include=True)
    edit(tree, "src/libultra/os/exceptasm.s", [
        ("    MFC0(   t1, C0_COMPARE)\n    MTC0(   t1, C0_COMPARE)\n    /* Post counter message */",
         "    MFC0(   t1, C0_COMPARE)\n    MTC0(   t1, C0_COMPARE)\n"
         "    /* vbench: where the interrupted thread was, for the PC sampler (timerintr.c) */\n"
         "    lw      t2, THREAD_PC(k0)\n    la      t3, gVBenchCounterPc\n    sw      t2, (t3)\n"
         "    /* Post counter message */"),
    ])
    edit(tree, "src/libultra/os/timerintr.c", [
        ("        if (timer->mq != NULL) {\n            osSendMesg(timer->mq, timer->msg, OS_MESG_NOBLOCK);",
         "        {\n            extern void (*gVBenchTimerHook)(OSTimer* timer);\n\n"
         "            if (gVBenchTimerHook != NULL) {\n                gVBenchTimerHook(timer);\n            }\n        }\n"
         "        if (timer->mq != NULL) {\n            osSendMesg(timer->mq, timer->msg, OS_MESG_NOBLOCK);"),
        ("#if LIBULTRA_VERSION >= LIBULTRA_VERSION_K\n    if (time < 468) {",
         "#if 1 // (vbench: version K's minimum, else a timer can be missed until the count wraps, about 91 s)\n"
         "    if (time < 468) {"),
    ])
    # the bench's files
    shutil.copy(os.path.join(HERE, "vbench.c"), os.path.join(tree, "src", "code", "vbench.c"))
    shutil.copy(os.path.join(HERE, "vbench.h"), os.path.join(tree, "include", "vbench.h"))
    shutil.copy(os.path.join(ROOT, "fishmod", "scripts", "tour.inc.c"),
                os.path.join(tree, "src", "code", "vbench_tour.inc.c"))
    os.makedirs(os.path.join(tree, "n64prof"), exist_ok=True)
    for f in ("n64prof.c", "n64prof.h"):
        shutil.copy(os.path.join(ROOT, "n64prof", f), os.path.join(tree, "n64prof", f))

    for p, text in PENDING.items():
        open(p, "w").write(text)
    print(f"vbench applied to {tree}")


if __name__ == "__main__":
    main()
