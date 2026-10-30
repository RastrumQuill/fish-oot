// Part of fishmod_debug.c (#included there, sharing its statics): the photo booth for the wiki renders (FISHMOD_PHOTO).
#ifdef FISHMOD_PHOTO
/*
 * Photo booth for wiki renders: shows each species in turn, from the side and from a three-quarter view,
 * high above the test spot on a plain backdrop. fishmod/tools/photo_capture.py screenshots each photo
 * using the marker color drawn in the corner.
 */
static u8 sPhotoSpecies[] = {
    FISHMOD_SPECIES_GREENGILL,    FISHMOD_SPECIES_HYRULE_BASS, FISHMOD_SPECIES_ORDON_CATFISH,
    FISHMOD_SPECIES_HYLIAN_LOACH, FISHMOD_SPECIES_SANKE_CARP,  FISHMOD_SPECIES_HYRULE_MINNOW,
};
#define PHOTO_POSES 2
#define PHOTO_FRAMES 100

static s32 sPhotoIndex = -1;
static s32 sPhotoTimer;
static Actor* sPhotoFish;
// Size line-up after the portraits: every species at its typical size, smallest at the top.
static u8 sLineupOrder[] = {
    FISHMOD_SPECIES_HYRULE_MINNOW, FISHMOD_SPECIES_GREENGILL,  FISHMOD_SPECIES_HYLIAN_LOACH,  FISHMOD_SPECIES_HYRULE_BASS,
    FISHMOD_SPECIES_SANKE_CARP, FISHMOD_SPECIES_ORDON_CATFISH,
};
static Actor* sLineupFish[ARRAY_COUNT(sLineupOrder)];
static f32 sMeasuredExtent[FISHMOD_SPECIES_MAX];
static Vec3f sMeasuredOffset[FISHMOD_SPECIES_MAX]; // drawn center minus home position
static s16 sPhotoCamId = SUB_CAM_ID_DONE;
static Vec3f sPhotoEye;
static Vec3f sPhotoAt;

static void Fishmod_PhotoUpdate(PlayState* play) {
    const FishmodSpecies* species;
    s32 pose;
    f32 length;
    f32 dist;

    if (sPhotoCamId == SUB_CAM_ID_DONE) {
        sPhotoCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sPhotoCamId, CAM_STAT_ACTIVE);
        gFishmodPhoto.active = true;
    }

    play->envCtx.skyboxDisabled = true;
    play->envCtx.sunMoonDisabled = true;
    // studio lighting: lift the ambient and key light so colors read clearly
    play->envCtx.adjAmbientColor[0] = play->envCtx.adjAmbientColor[1] = play->envCtx.adjAmbientColor[2] = 70;
    play->envCtx.adjLight1Color[0] = play->envCtx.adjLight1Color[1] = play->envCtx.adjLight1Color[2] = 60;
    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);

    if ((sPhotoIndex < 0) || (++sPhotoTimer >= PHOTO_FRAMES)) {
        s32 numPortraits = (s32)ARRAY_COUNT(sPhotoSpecies) * PHOTO_POSES;
        s32 i;

        // remember how big each fish drew, for the line-up
        if ((sPhotoIndex >= 0) && (sPhotoIndex < numPortraits) && (gFishmodPhoto.extent > 0.0f) &&
            (sPhotoFish != NULL)) {
            s32 sp = sPhotoSpecies[sPhotoIndex / PHOTO_POSES];

            sMeasuredExtent[sp] = gFishmodPhoto.extent;
            sMeasuredOffset[sp].x = gFishmodPhoto.center.x - sPhotoFish->world.pos.x;
            sMeasuredOffset[sp].y = gFishmodPhoto.center.y - sPhotoFish->world.pos.y;
            sMeasuredOffset[sp].z = gFishmodPhoto.center.z - sPhotoFish->world.pos.z;
        }

        sPhotoTimer = 0;
        sPhotoIndex++;
        if (sPhotoIndex > numPortraits) {
            sPhotoIndex = 0;
        }
        if (sPhotoFish != NULL) {
            Actor_Kill(sPhotoFish);
            sPhotoFish = NULL;
        }
        for (i = 0; i < ARRAY_COUNT(sLineupFish); i++) {
            if (sLineupFish[i] != NULL) {
                Actor_Kill(sLineupFish[i]);
                sLineupFish[i] = NULL;
            }
        }

        if (sPhotoIndex < numPortraits) {
            sPhotoFish =
                Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, sPhotoPos.x, sPhotoPos.y, sPhotoPos.z, 0, 0, 0,
                            FISHMOD_PARAMS(FISHMOD_TYPE_FISH, sPhotoSpecies[sPhotoIndex / PHOTO_POSES]));
            if (sPhotoFish != NULL) {
                sPhotoFish->room = -1;
            }
        } else {
            f32 rowY = sPhotoPos.y;

            for (i = 0; i < ARRAY_COUNT(sLineupOrder); i++) {
                s32 sp = sLineupOrder[i];
                f32 rowHeight = sMeasuredExtent[sp] * 0.25f + 2.0f;

                rowY -= rowHeight;
                sLineupFish[i] = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, sPhotoPos.x - sMeasuredOffset[sp].x,
                                             rowY - sMeasuredOffset[sp].y, sPhotoPos.z - sMeasuredOffset[sp].z, 0, 0, 0,
                                             FISHMOD_PARAMS(FISHMOD_TYPE_FISH, sp));
                if (sLineupFish[i] != NULL) {
                    sLineupFish[i]->room = -1;
                }
                rowY -= rowHeight;
            }
        }
        gFishmodPhoto.marker = sPhotoIndex + 1;
        gFishmodPhoto.extent = 0.0f;
    }

    if (sPhotoIndex >= (s32)ARRAY_COUNT(sPhotoSpecies) * PHOTO_POSES) {
        // line-up: frame the whole column
        f32 top = sPhotoPos.y;
        f32 bottom = sPhotoPos.y;
        f32 widest = 0.0f;
        s32 i;

        for (i = 0; i < ARRAY_COUNT(sLineupOrder); i++) {
            s32 sp = sLineupOrder[i];

            bottom -= 2.0f * (sMeasuredExtent[sp] * 0.25f + 2.0f);
            widest = CLAMP_MIN(widest, sMeasuredExtent[sp]);
        }
        gFishmodPhoto.yaw = sPhotoYaw;
        gFishmodPhoto.pitch = 0;
        sPhotoAt.x = sPhotoPos.x;
        sPhotoAt.y = (top + bottom) * 0.5f;
        sPhotoAt.z = sPhotoPos.z;
        dist = CLAMP_MIN(widest * 1.25f * 1.35f, (top - bottom) * 1.05f);
        sPhotoEye.x = sPhotoAt.x;
        sPhotoEye.y = sPhotoAt.y;
        sPhotoEye.z = sPhotoAt.z + dist;
        Play_SetCameraAtEye(play, sPhotoCamId, &sPhotoAt, &sPhotoEye);
        return;
    }

    species = &gFishmodSpecies[sPhotoSpecies[sPhotoIndex / PHOTO_POSES]];
    pose = sPhotoIndex % PHOTO_POSES;

    // frame the fish from the bounds it reported in its last draw (limb origins, so pad a little)
    length = gFishmodPhoto.extent * 1.25f;
    if (length < 5.0f) {
        length = 20.0f;
    }
    dist = length * 1.35f;

    gFishmodPhoto.yaw = sPhotoYaw;
    gFishmodPhoto.pitch = 0;

    if (gFishmodPhoto.extent > 0.0f) {
        sPhotoAt = gFishmodPhoto.center;
    } else {
        sPhotoAt = sPhotoPos;
    }
    if (pose == 0) {
        sPhotoEye.x = sPhotoAt.x;
        sPhotoEye.y = sPhotoAt.y + dist * 0.05f;
        sPhotoEye.z = sPhotoAt.z + dist;
    } else {
        sPhotoEye.x = sPhotoAt.x - dist * 0.35f;
        sPhotoEye.y = sPhotoAt.y + dist * 0.7f;
        sPhotoEye.z = sPhotoAt.z + dist * 0.62f;
    }
    Play_SetCameraAtEye(play, sPhotoCamId, &sPhotoAt, &sPhotoEye);
}
#endif
