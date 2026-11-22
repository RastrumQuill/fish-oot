#ifndef FISHMOD_FEEL_H
#define FISHMOD_FEEL_H

/*
 * The fishing's feel: the numbers the fight and the bite are made of, in one list. Test builds read them from a
 * table that the desktop tuner (fishmod/pc/feel.bat) changes over the SummerCart64's USB while the game runs
 * (src/code/fishmod_feel.inc.c); other builds use the defaults as constants. fishmod/pc/feel.py reads this file for the
 * names, defaults and ranges: one line each, in this form.
 *
 * FEEL(ID, "name", default, min, max, "what it does")
 */
#define FISHMOD_FEEL_LIST(FEEL)                                                                                        \
    FEEL(FEEL_TENSION_BASE, "tension_base", 14.0f, 0.0f, 50.0f, "the gauge's floor on a tight line")                  \
    FEEL(FEEL_TAUT_GAIN, "taut_gain", 3.0f, 0.0f, 10.0f, "how much a tight line adds a unit, times the load")        \
    FEEL(FEEL_TAUT_CAP, "taut_cap", 5.0f, 1.0f, 20.0f, "the tight line's units that count, at most")                  \
    FEEL(FEEL_STRENGTH_GAIN, "strength_gain", 0.25f, 0.0f, 1.0f, "how much the species' strength adds, times load")  \
    FEEL(FEEL_RUN_ADD, "run_add", 10.0f, 0.0f, 40.0f, "what a run adds to the gauge, times the load")                 \
    FEEL(FEEL_OPPOSE_RELIEF, "oppose_relief", 12.0f, 0.0f, 40.0f, "what holding the stick against the run takes off") \
    FEEL(FEEL_LOAD_PER_LB, "load_per_lb", 0.5f, 0.05f, 2.0f, "load = sqrt(pounds x this): how much weight is felt")  \
    FEEL(FEEL_LOAD_MIN, "load_min", 0.35f, 0.05f, 1.0f, "the load of the smallest fish")                              \
    FEEL(FEEL_LOAD_MAX, "load_max", 1.8f, 1.0f, 4.0f, "the load of the biggest fish")                                 \
    FEEL(FEEL_GAUGE_RATE, "gauge_rate", 0.25f, 0.02f, 1.0f, "how quickly the gauge follows (a share a frame)")      \
    FEEL(FEEL_GAUGE_STEP, "gauge_step", 4.0f, 0.5f, 20.0f, "the most the gauge moves a frame")                        \
    FEEL(FEEL_REEL_GAIN, "reel_gain", 18.0f, 0.0f, 40.0f, "what reeling adds to the gauge, per unit of reel speed")   \
    FEEL(FEEL_REEL_SPEED, "reel_speed", 1.2f, 0.2f, 5.0f, "line reeled a frame with A")                               \
    FEEL(FEEL_REEL_FAST, "reel_fast", 2.2f, 0.2f, 8.0f, "line reeled a frame with A and R")                           \
    FEEL(FEEL_RUN_PULL, "run_pull", 2.2f, 1.0f, 5.0f, "how much harder the fish pulls on a run")                      \
    FEEL(FEEL_TIRE_OPPOSE, "tire_oppose", 2.5f, 1.0f, 6.0f, "how much faster the fish tires with the stick against") \
    FEEL(FEEL_DRAG_AT, "drag_at", 85.0f, 50.0f, 99.0f, "the gauge where the drag pays out line")                      \
    FEEL(FEEL_SNAP_FRAMES, "snap_frames", 12.0f, 1.0f, 60.0f, "frames at 100 before the line snaps")                  \
    FEEL(FEEL_SLACK_FRAMES, "slack_frames", 90.0f, 10.0f, 300.0f, "frames slack before the hook falls out")           \
    FEEL(FEEL_NIBBLE_GAP, "nibble_gap", 1.0f, 0.2f, 4.0f, "every species' time between taps, times this")             \
    FEEL(FEEL_BITE_WINDOW, "bite_window", 1.0f, 0.2f, 4.0f, "every species' time to set the hook, times this")  \
    FEEL(FEEL_RUMBLE_NIBBLE, "rumble_nibble", 1.0f, 0.0f, 2.0f, "the rumble of a nibble's tap, times this (0 off)") \
    FEEL(FEEL_RUMBLE_BITE, "rumble_bite", 1.0f, 0.0f, 2.0f, "the bite's, the hook's and a snap's jolts, times this") \
    FEEL(FEEL_RUMBLE_FIGHT, "rumble_fight", 1.0f, 0.0f, 2.0f, "the fight's rumble (the line, runs, leaps), times this")

#define FISHMOD_FEEL_ENUM(id, name, def, lo, hi, what) id,
typedef enum FishmodFeelId {
    FISHMOD_FEEL_LIST(FISHMOD_FEEL_ENUM)
    FEEL_MAX
} FishmodFeelId;
#undef FISHMOD_FEEL_ENUM

#if FISHMOD_TESTMODE
extern f32 gFishmodFeel[FEEL_MAX];
#define FEEL(id) (gFishmodFeel[id])
// once a frame (Fishmod_PlayUpdate): the tuner's lines read, the table changed, the changes said back
void Fishmod_FeelUpdate(void);
#else
// (the defaults, as constants)
#define FISHMOD_FEEL_DEFAULT(id, name, def, lo, hi, what) static const f32 id##_DEFAULT = def;
FISHMOD_FEEL_LIST(FISHMOD_FEEL_DEFAULT)
#undef FISHMOD_FEEL_DEFAULT
#define FEEL(id) (id##_DEFAULT)
#endif

#endif
