# rc1-popin-test: the plants that popped in near the camera

`fish-oot-rc1-popin-test.bps`: rc1's test build (`fishmod/build.sh --hw --texpack --colpack --meshpack --gcc-all
--testmode`) with one change, in `fishmod/tools/impostor.py`.

**What was wrong.** Each plant kind has a far tier drawn as one card with a picture of the whole plant. The card is cut
to a triangle around the picture (`card_outline.triangle`), which gives its corners as fractions of the texture;
`impostor.py` took them as texels. The card came out a W-th of its size (W the picture's width, 16 to 64): a triangle a
few units across at the picture's top left corner, which draws nothing. 15 of the 17 kinds with a single card had it
(vine_strand and bank_roots have pictures that can't be cut, and kept their whole quad). The released rc1 ROM has the
same triangles.

So a plant at its single-card tier wasn't on screen at all, and appeared when it came near enough for its model. Above
water that tier starts 500 units out or more, and the plants there are small on screen. With the camera under water
the tiers are at half their distances: every plant past about 250 units was missing, which is nearer than Link, on
the lake's bed, in iron boots, anywhere there are plants ahead. It only looked like a direction: the single card is
only used within 20 degrees of level, and it shows where the plants are (toward the shore from the pier).

**How it was found.** Adult Link on the bed off the pier, the camera behind him looking to the shore
(`fishmod/scripts/popin_pier.inc.c`): `popin_check.py` on the game's memory found every plant ahead in the set, all of
them past 250 units at tier 3 (the single card), and none of them in the picture; with `FISHMOD_BENCH_NOIMP` they were
there; the dissolve and the 1-cycle material made no difference; the card's vertices did.

**After.** The same view shows the bushes and grass beyond Link as dark shapes in the haze, as with the cards off. The
camera tour's underwater stops show the far beds; its above-water stops are as before.

**The cost.** These cards drew nothing before, so they cost the RDP nothing: now they're drawn. They're small and
far (that's what the tier is for), but the bench's numbers will move a little, mostly under water. The LOD
calibration's distances (`fishmod_lod.h`, `fishmod_lod_tune.h`) were measured with the cards invisible: "leaving the
kind out changes no view" was true sooner than it should have been, so those distances are worth measuring again.
(`fishmod_lod_tune.h` has 31 rows for 32 kinds, so the scatter isn't using it at all at the moment.)
