// Part of fishmod_debug.c (#included there, sharing its statics): the animation studio (FISHMOD_ANIM_STUDIO).
#ifdef FISHMOD_ANIM_STUDIO
/*
 * Animation studio: each species in turn as a row of three fish on the photo backdrop (full rig, reduced rig, and a
 * small one on the reduced rig), held in place while a fixed program puts them through cruising, fast swimming,
 * turns, startles, a hooked thrash and hovering, seen from above and then from a three-quarter view. The corner
 * marker (gFishmodPhoto.marker) tells fishmod/tools/anim_capture.py which species and view is on screen.
 */
#ifdef FISHMOD_STUDIO_ORBIT
// one species seen from all round (for finding angles it vanishes from): above, three-quarter, nose-on, from
// behind, from below, low from the side
static u8 sStudioSpecies[] = { FISHMOD_STUDIO_ORBIT };
#define STUDIO_VIEWS 6
#else
static u8 sStudioSpecies[] = {
    FISHMOD_SPECIES_GREENGILL,    FISHMOD_SPECIES_HYRULE_BASS, FISHMOD_SPECIES_ORDON_CATFISH,
    FISHMOD_SPECIES_HYLIAN_LOACH, FISHMOD_SPECIES_SANKE_CARP,  FISHMOD_SPECIES_HYRULE_MINNOW,
};
#define STUDIO_VIEWS 2
#endif
#define STUDIO_FISH 3

typedef struct StudioPhase {
    s16 frames;
    s16 swimMag;
    s16 phaseStep; // hundredths
    s16 yawSwing;  // amplitude of a yaw sweep
    s16 yawRate;
    s16 jaw;
    s16 roll;
    s8 startleEvery; // frames between startles, 0 = none
} StudioPhase;

static StudioPhase sStudioProgram[] = {
    { 70, 1500, 25, 0, 0, 0, 0, 0 },               // cruise
    { 50, 2200, 45, 0, 0, 0, 0, 0 },               // fast
    { 90, 1500, 30, 0x2800, 0x500, 0, 0, 0 },      // turning left and right
    { 60, 1500, 25, 0, 0, 0, 0, 30 },              // startles
    { 70, 2800, 60, 0x1000, 0x1800, 0x1800, 0x800, 0 }, // hooked thrash
    { 50, 350, 10, 0, 0, 0, 0, 0 },                // hovering
};

static Vec3f sStudioPos = { 1033.0f, 900.0f, 172.0f };
static s32 sStudioSegment = -1; // species * STUDIO_VIEWS + view
static s32 sStudioFrame;
static s16 sStudioCamId = SUB_CAM_ID_DONE;
static f32 sStudioSpacing;
static Vec3f sStudioAt;

static void Fishmod_StudioSpawn(PlayState* play, s32 species) {
    const FishmodSpecies* sp = &gFishmodSpecies[species];
    f32 typical = sp->lengthTypical;
    f32 small = CLAMP_MAX(typical * 0.6f, 7.0f);
    s32 i;

    for (i = 0; i < STUDIO_FISH; i++) {
        if (gFishmodStudio.fish[i] != NULL) {
            Actor_Kill(gFishmodStudio.fish[i]);
            gFishmodStudio.fish[i] = NULL;
        }
    }
    // side by side across X, all facing +Z (up the screen from above)
    sStudioSpacing = typical * 1.64f * 1.3f + 6.0f;
    for (i = 0; i < STUDIO_FISH; i++) {
        gFishmodStudio.spawnLength = (i == 2) ? small : typical;
#ifdef FISHMOD_STUDIO_ORBIT
        {
            // to compare: dark and cool; bright, warm and mirrored; plain colours (koi: three different patterns)
            static const u8 sLooks[] = { 0x00, 0x7F, 0xDB };

            gFishmodStudio.spawnLook = sLooks[i % 3];
        }
#else
        gFishmodStudio.spawnLook = 0x1B;
#endif
        gFishmodStudio.rig[i] = (i == 0) ? 1 : 2;
        gFishmodStudio.fish[i] =
            Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, sStudioPos.x + (i - 1) * sStudioSpacing, sStudioPos.y,
                        sStudioPos.z, 0, 0, 0, FISHMOD_PARAMS(FISHMOD_TYPE_FISH, species));
        if (gFishmodStudio.fish[i] != NULL) {
            gFishmodStudio.fish[i]->room = -1;
        }
    }
    gFishmodStudio.spawnLength = 0.0f;
}

static void Fishmod_StudioUpdate(PlayState* play) {
    s32 programFrames = 0;
    s32 t;
    s32 i;
    StudioPhase* phase = NULL;
    Vec3f at;
    Vec3f eye;
    f32 dist;

    if (sStudioCamId == SUB_CAM_ID_DONE) {
        sStudioCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sStudioCamId, CAM_STAT_ACTIVE);
        gFishmodPhoto.active = true;
        gFishmodStudio.active = true;
    }
    play->envCtx.skyboxDisabled = true;
    play->envCtx.sunMoonDisabled = true;
    play->envCtx.adjAmbientColor[0] = play->envCtx.adjAmbientColor[1] = play->envCtx.adjAmbientColor[2] = 70;
    play->envCtx.adjLight1Color[0] = play->envCtx.adjLight1Color[1] = play->envCtx.adjLight1Color[2] = 60;
    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);

    for (i = 0; i < ARRAY_COUNT(sStudioProgram); i++) {
        programFrames += sStudioProgram[i].frames;
    }
    if ((sStudioSegment < 0) || (++sStudioFrame >= programFrames)) {
        sStudioFrame = 0;
        sStudioSegment++;
        if (sStudioSegment >= (s32)ARRAY_COUNT(sStudioSpecies) * STUDIO_VIEWS) {
            sStudioSegment = 0;
        }
        if ((sStudioSegment % STUDIO_VIEWS) == 0) {
            Fishmod_StudioSpawn(play, sStudioSpecies[sStudioSegment / STUDIO_VIEWS]);
        }
        gFishmodPhoto.marker = sStudioSegment + 1;
    }

    // where in the program
    t = sStudioFrame;
    for (i = 0; i < ARRAY_COUNT(sStudioProgram); i++) {
        if (t < sStudioProgram[i].frames) {
            phase = &sStudioProgram[i];
            break;
        }
        t -= sStudioProgram[i].frames;
    }
    gFishmodStudio.swimMag = phase->swimMag;
    gFishmodStudio.swimPhaseStep = phase->phaseStep * 0.01f;
    gFishmodStudio.yaw = (s16)(Math_SinS(t * phase->yawRate) * phase->yawSwing);
    gFishmodStudio.pitch = 0;
    gFishmodStudio.roll = (s16)(Math_SinS(t * 0x1300) * phase->roll);
    gFishmodStudio.jawRot = phase->jaw;
    gFishmodStudio.startle = 0;
    if ((phase->startleEvery != 0) && ((t % phase->startleEvery) == 5)) {
        gFishmodStudio.startle = ((t / phase->startleEvery) & 1) ? -8 : 8;
    }

    // camera: aimed at the middle fish's drawn center (the models' origins aren't at their middles), taken once it
    // has been drawn and held for the segment; from above (a touch behind, so the view has an up direction), then
    // three-quarter from the front
    if ((sStudioFrame == 3) || ((sStudioSegment % STUDIO_VIEWS) == 0) && (sStudioFrame == 0)) {
        sStudioAt = (sStudioFrame == 3) ? gFishmodPhoto.center : sStudioPos;
    }
    at = sStudioAt;
    // a narrow lens from farther away: nearly flat, so the fish at the sides are seen from straight above too
    dist = sStudioSpacing * 1.7f * 2.4f;
    if ((sStudioSegment % STUDIO_VIEWS) == 0) {
        eye.x = at.x;
        eye.y = at.y + dist;
        eye.z = at.z - dist * 0.08f;
    } else if ((sStudioSegment % STUDIO_VIEWS) == 1) {
        dist *= 1.3f;
        eye.x = at.x + dist * 0.45f;
        eye.y = at.y + dist * 0.45f;
        eye.z = at.z + dist * 0.75f;
    } else {
        // (FISHMOD_STUDIO_ORBIT) nose-on, from behind, from below, low from the side
        static const f32 sDir[4][3] = { { 0.05f, 0.1f, 1.0f }, { 0.05f, 0.1f, -1.0f }, { 0.08f, -1.0f, 0.1f },
                                        { 1.0f, -0.2f, 0.05f } };
        const f32* d = sDir[(sStudioSegment % STUDIO_VIEWS) - 2];

        // close up nose-on (into the mouth) and from the side
        dist *= (((sStudioSegment % STUDIO_VIEWS) == 2) || ((sStudioSegment % STUDIO_VIEWS) == 5)) ? 0.3f : 1.3f;
        eye.x = at.x + d[0] * dist;
        eye.y = at.y + d[1] * dist;
        eye.z = at.z + d[2] * dist;
        if (((sStudioSegment % STUDIO_VIEWS) == 5) && (gFishmodStudio.fish[0] != NULL)) { // the full-rig fish, from the side
            at = gFishmodStudio.fish[0]->world.pos;
            eye.x = at.x - dist;
            eye.y = at.y - 0.2f * dist;
            eye.z = at.z + 0.05f * dist;
        }
    }
    Play_SetCameraAtEye(play, sStudioCamId, &at, &eye);
    Play_SetCameraFov(play, sStudioCamId, 25.0f);
}
#endif
