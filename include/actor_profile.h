#ifndef ACTOR_PROFILE_H
#define ACTOR_PROFILE_H

#include "ultra64.h"

struct Actor;
struct PlayState;

#define DEFINE_ACTOR(_0, enum, _2, _3) enum,
#define DEFINE_ACTOR_INTERNAL(_0, enum, _2, _3) enum,
#define DEFINE_ACTOR_UNSET(enum) enum,

typedef enum ActorID {
    #include "tables/actor_table.h"
    /* 0x01D7 */ ACTOR_ID_MAX // originally "ACTOR_DLF_MAX"
} ActorID;

#undef DEFINE_ACTOR
#undef DEFINE_ACTOR_INTERNAL
#undef DEFINE_ACTOR_UNSET

typedef enum ActorCategory {
    /* 0x00 */ ACTORCAT_SWITCH,
    /* 0x01 */ ACTORCAT_BG,
    /* 0x02 */ ACTORCAT_PLAYER,
    /* 0x03 */ ACTORCAT_EXPLOSIVE,
    /* 0x04 */ ACTORCAT_NPC,
    /* 0x05 */ ACTORCAT_ENEMY,
    /* 0x06 */ ACTORCAT_PROP,
    /* 0x07 */ ACTORCAT_ITEMACTION,
    /* 0x08 */ ACTORCAT_MISC,
    /* 0x09 */ ACTORCAT_BOSS,
    /* 0x0A */ ACTORCAT_DOOR,
    /* 0x0B */ ACTORCAT_CHEST,
    // fish-oot (frames12): the mod's fish, and then their manager (which draws them, after their culling is worked
    // out), each in a list of its own: the Z-target search (sAttentionCategorySearchOrder) walks neither, and it
    // read every fish's flags every frame to skip them (as NPCs)
    /* 0x0C */ ACTORCAT_FISH,
    /* 0x0D */ ACTORCAT_FISHMGR,
    /* 0x0E */ ACTORCAT_MAX
} ActorCategory;

typedef void (*ActorFunc)(struct Actor*, struct PlayState*);

typedef struct ActorProfile {
    /* 0x00 */ s16 id;
    /* 0x02 */ u8 category; // Classifies actor and determines when it will update or draw
    /* 0x04 */ u32 flags;
    /* 0x08 */ s16 objectId;
    /* 0x0C */ u32 instanceSize;
    /* 0x10 */ ActorFunc init; // Constructor
    /* 0x14 */ ActorFunc destroy; // Destructor
    /* 0x18 */ ActorFunc update; // Update Function
    /* 0x1C */ ActorFunc draw; // Draw function
} ActorProfile; // size = 0x20

#endif
