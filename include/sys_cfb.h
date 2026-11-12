#ifndef SYS_CFB_H
#define SYS_CFB_H

#include "ultra64.h"

void SysCfb_Init(s32 n64dd);
void* SysCfb_GetFbPtr(s32 idx);
s32 SysCfb_GetFbCount(void); // fish-oot: 2, or 3 with the Expansion Pak
void* SysCfb_GetFbEnd(void);

#endif
