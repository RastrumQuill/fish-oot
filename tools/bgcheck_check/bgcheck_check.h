#ifndef BGCHECK_CHECK_H
#define BGCHECK_CHECK_H

#include "ultra64.h"
#include "bgcheck.h"

typedef struct BgCheckCheckScene {
    const char* name;
    s32 sceneId;
    CollisionHeader* col;
} BgCheckCheckScene;

extern BgCheckCheckScene gBgCheckCheckScenes[];
extern s32 gBgCheckCheckSceneCount;
extern CollisionHeader* gBgCheckCheckPlatforms[];
extern s32 gBgCheckCheckPlatformCount;

#endif
