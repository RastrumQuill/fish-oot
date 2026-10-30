"""The habitat prop kinds (fishmod/data/prop_kinds.json): the one table the habitat tools and the game share.

    import prop_kinds as pk
    pk.GAME            # the game's kinds in its enum order (names)
    pk.GENERATED       # [(name, texture, format)]: the kinds gen_habitat.py builds models for
    pk.VANILLA         # the kinds drawn with object_fish's own models
    pk.flagged("tri")  # the kinds with a flag
    pk.HEIGHT          # {name: height at scale 1}

gen_habitat.py writes the game's side (fishmod_props_kinds.h) from it, so the tools no longer read the header.
"""
import json
import os

_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "prop_kinds.json")

with open(_PATH) as _f:
    KINDS = json.load(_f)["kinds"]
BY_NAME = {k["name"]: k for k in KINDS}

GAME = [k["name"] for k in KINDS if "placed_as" not in k]
GENERATED = [(k["name"], k["texture"], k["format"]) for k in KINDS if "texture" in k]
VANILLA = [k["name"] for k in KINDS if k.get("vanilla")]


def flagged(flag):
    return {k["name"] for k in KINDS if flag in k["flags"]}


def field(name):
    """{kind: value} for every kind that has the field."""
    return {k["name"]: (tuple(k[name]) if isinstance(k[name], list) else k[name]) for k in KINDS if name in k}


HEIGHT = field("height")
