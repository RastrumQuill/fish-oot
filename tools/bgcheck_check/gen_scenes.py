#!/usr/bin/env python3
"""Writes a C file with the collision of every scene (and some objects, for moving platforms) of an extracted version,
for bgcheck_check (the collision test, run.sh): each scene's CollisionHeader and its lists, as the game's own data
files define them, and a table of them with their scene ids.

Usage: gen_scenes.py EXTRACTED_DIR OUT.c
  EXTRACTED_DIR: extracted/<version> (after `make setup VERSION=<version>`)
"""
import os
import re
import sys

COL_SUFFIX = re.compile(r"(Col|PolyList|VtxList|SurfaceTypes|WaterBoxes|BgCamList|BgCamFuncData|CamDataList|CamPosData)$")
BLOCK = re.compile(r'^(SurfaceType|CollisionPoly|Vec3s|WaterBox|BgCamInfo|CollisionHeader) (\w+)(\[[^\]]*\])? = \{\n'
                   r'#include "([^"]+)"\n\};', re.M)

# objects with moving-platform collision used as the test's platforms (any that exist in the version)
PLATFORM_OBJECTS = [
    "object_spot06_objects", "object_bdan_objects", "object_hidan_objects", "object_mizu_objects",
    "object_ydan_objects", "object_ddan_objects", "object_bwall", "object_mori_objects", "object_gnd",
    "object_spot15_obj", "object_haka_objects", "object_jya_obj", "object_ice_objects", "object_spot12_obj",
]


def blocks(path, want_all_col):
    src = open(path).read()
    out = []
    for m in BLOCK.finditer(src):
        typ, name, arr, inc = m.groups()
        if typ in ("Vec3s",) and not COL_SUFFIX.search(name):
            continue
        if not want_all_col and not COL_SUFFIX.search(name) and typ != "CollisionHeader":
            continue
        out.append((typ, name, arr or "", inc))
    return out


def scene_ids(root):
    """scene file name (ydan_scene) -> SCENE_ enum name, from include/tables/scene_table.h"""
    ids = {}
    table = os.path.join(root, "include", "tables", "scene_table.h")
    for m in re.finditer(r"DEFINE_SCENE\((\w+), *\w+, *(SCENE_\w+)", open(table).read()):
        ids[m.group(1)] = m.group(2)
    return ids


def main():
    ext, out_path = sys.argv[1], sys.argv[2]
    root = os.path.abspath(os.path.join(ext, "..", ".."))
    ids = scene_ids(root)
    lines = ['#include "ultra64.h"', '#include "array_count.h"', '#include "actor.h"', '#include "bgcheck.h"',
             '#include "camera.h"', '#include "scene.h"', '#include "bgcheck_check.h"', ""]
    scenes = []
    for dirpath, _, files in sorted(os.walk(os.path.join(ext, "assets", "scenes"))):
        for f in sorted(files):
            if not f.endswith("_scene.c"):
                continue
            name = f[:-2]
            if name not in ids:
                continue
            bl = blocks(os.path.join(dirpath, f), False)
            headers = [b for b in bl if b[0] == "CollisionHeader"]
            if len(headers) != 1:
                continue
            for typ, n, arr, inc in bl:
                lines.append(f"static {typ} {n}{arr} = {{\n#include \"{inc}\"\n}};")
            scenes.append((name, ids[name], headers[0][1]))
    platforms = []
    for obj in PLATFORM_OBJECTS:
        p = os.path.join(ext, "assets", "objects", obj, obj + ".c")
        if not os.path.exists(p):
            continue
        bl = blocks(p, False)
        for typ, n, arr, inc in bl:
            lines.append(f"static {typ} {n}{arr} = {{\n#include \"{inc}\"\n}};")
        platforms += [b[1] for b in bl if b[0] == "CollisionHeader"]
    lines.append("")
    lines.append("BgCheckCheckScene gBgCheckCheckScenes[] = {")
    for name, sid, col in scenes:
        lines.append(f'    {{ "{name}", {sid}, &{col} }},')
    lines.append("};")
    lines.append(f"s32 gBgCheckCheckSceneCount = {len(scenes)};")
    lines.append("CollisionHeader* gBgCheckCheckPlatforms[] = {")
    for col in platforms:
        lines.append(f"    &{col},")
    lines.append("};")
    lines.append(f"s32 gBgCheckCheckPlatformCount = {len(platforms)};")
    open(out_path, "w").write("\n".join(lines) + "\n")
    print(f"{len(scenes)} scenes, {len(platforms)} platform collisions")


if __name__ == "__main__":
    main()
