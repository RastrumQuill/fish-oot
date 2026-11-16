#ifndef FISHMOD_LOD_POP_H
#define FISHMOD_LOD_POP_H

/*
 * The props' tier distances as the pop-in test set them (fishmod/build.sh --script pop_ab,
 * fishmod/tools/pop_ab_capture.py; releases/notes-rc1-headroom.md): each kind's change over 150 units of travel at
 * every tour stop under LPIPS 2.0 (x1000, less the control's), where it could be brought there by distance alone.
 *
 * FISHMOD_UNDER_LOD_SCALE: the tiers' distances with the camera under water, as a share of those above it. It was
 * 0.5 while the far single cards drew nothing (impostor.py's bug): with them drawn, a plant became its card at half
 * the distance it was made for, twice the size on screen, and the swap showed.
 *
 * FISHMOD_LOD_POP(X): X(kind, card, half, near): a kind's single-card distance times `card`, the distance its
 * half-size texture starts at times `half`, and the distance its near drawing ends at (PROP_FAR_DIST: the cut cards
 * and the near edge mode) times `near`. Kinds not listed: 1, 1 and 1.
 * - reed_clump, cattails: thin stalks, mostly edge, and the edge mode changes at PROP_FAR_DIST (500 units: near, the
 *   coverage alone and the VI smooths it; far, blended with the picture behind): a clump 60 pixels tall changed colour
 *   there. Near until 1200 now, just short of where their single cards start (1234 and 1429). The half-size texture
 *   and the card's distance were tried first and weren't it.
 * (0.75 under water was tried: grass_tuft and vine_strand stayed over the line.)
 */
#define FISHMOD_UNDER_LOD_SCALE 1.0f

#define FISHMOD_LOD_POP(X) \
    X(REED_CLUMP, 1.0f, 1.0f, 2.4f) \
    X(CATTAILS, 1.0f, 1.0f, 2.4f)

#endif
