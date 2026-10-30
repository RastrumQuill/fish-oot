// The scenes with collision for their habitat props (fishmod/tools/gen_collision.py):
// FISHMOD_COLLISION(sceneId, segment name, extra collision memory, lookup grid cells across x and z)
// The extra memory makes room for the props' polygons in BgCheck's lists (the debug builds report how much of it is
// used, SRAM FISHMOD_SRAM_COLLISION); the finer grid keeps each cell's list short with them added. 0 cells: the
// scene's own grid.
FISHMOD_COLLISION(SCENE_LAKE_HYLIA, fishmod_col_spot06, 0x10000, 32)
