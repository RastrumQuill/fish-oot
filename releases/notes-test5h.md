# Test 5h: Lake Hylia within a 20 fps budget

## The patches
- **`fish-oot-lake-tour-5h.bps`**: the Lake Hylia tour with the frame counter (top right). A for the next stop, B
  back, stick to look round. The counter on the console is the one that counts.
- **`fish-oot-kokiri-test5h.bps`**: the Kokiri / Lake Hylia test build, as before.
- **`fish-oot-hwbench-5h.bps`**: the overnight bench (needs a fresh save; send the `.sav`). The same 22 stops and modes
  as 5g, on this build. It checks the frame-time estimates below against the console.

## The goal
A solid 20 fps from views like the game's own: fishing from the shore, swimming, diving, and walking the bed in iron
boots, looking all the way round. A cost model fitted to bench 4 (each prop costs per scanline per card it covers)
says every lake section now holds it (spots it was never tuned on: 0-1% over 50 ms, each spot judged by its worst of
eight directions; 51% of views were over before). It's an estimate: the bench above is the real test.

## What changed
- **Tall plants are accents:** kelp is one card that turns only as far as needed to never show edge-on (at most 45
  degrees off facing you), 180 tall; tall weed 2 cards, 140 tall; ribbon weed 3 cards. Kelp, tall weed and vine
  strands are a third as many. Ribbon weed keeps its beds' edges; the middles became grass and broadleaf.
- **Fewer cards, same plants:** pondweed 4 -> 2, bush 6 -> 4, broadleaf 3 -> 2, grass patches 8 -> 4.
- **Moss mats** (the castle garden's hedge, recoloured): low lumpy mats lying on the bed, tilted to its slope, their
  edge sunk in the sand. They also shade the bed under them like plants do.
- **Thinning where the console needs it:** the densest bed cells are capped, and wherever a spot still goes over,
  the plant costing that view most became a moss mat (or a mat went). The West weed flat and North beach shallows
  are visibly thinner than before; elsewhere it's hard to see.
- **Vine curtains** capped with spatterdock pads (from above: leaves, the vines are their roots); the flat dark top
  is gone.
- **Under water:** the skybox isn't drawn (it can't be seen there: an exact A/B changed no pixel), about 5 ms a
  frame back. The tall plants stop at 900 and the haze is a little nearer.

## Worth looking at
- The West weed flat and North beach shallows: still lush enough?
- The moss mats: do they read as moss or as flat green patches?
- Kelp from different sides (it should no longer spin).
- The counter anywhere you can make it drop under 20.
