# frames27

**`fish-oot-frames27.bps`** (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`):
frames26 with:

- **The water fog from above in the game's own fog**: frames26's CPU work (the bed's vertex colours, the props'
  copied vertices) is gone. Seen from above, what's under the surface gets the camera-under-water fog by the share of
  the view's ray inside the water: the RSP's own fog with its two numbers scaled for that depth and the water's colour,
  per pixel by distance as the game's fog is. The lake bed is cut into 8 depth bands (meshpack; 577 triangles in the
  cut pieces became 2,101, and about 220 fog changes, a pipe sync each); each prop under the water sets its fog for
  its depth (a pipe sync when it changes). Above the water nothing changes. The Lens of Truth turns it off.
- **Underwater plants bend as a rope**: held at the foot, floated up at the top, they give way where Link meets them
  and curve back toward upright above that (the top moves 0.3 of it); six bands where the model fits, else four.
  Reeds and other plants standing out of the water keep the stalk's bend (from the foot, the top furthest).
- **The Lens of Truth on the D-pad** stays on (the magic meter turned it off unless a C button held it).
- **Shoreline plants** need water at their feet and no cliff or wall rising near them.

Compare with frames25 (frames26's CPU fog cost should be gone; the RDP pays for the bands' triangles and syncs).
