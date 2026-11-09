#!/usr/bin/env python3
"""Encode replay JSON (tools/replay) into the compact footage stream used by the showcase.

Usage: encode.py REPLAY_JSON TEMPLATE_HTML OUT_HTML [BFF_JSON]
Per frame: u16 count, then per agent u16 slot, u16 x64, u16 y64,
u16 (dir | (pen+1) << 9 | nodes << 11), u16 genome signature. Slots are reused one
frame after an agent dies, so a slot never jumps between two agents across a frame.
"""
import base64
import json
import struct
import sys

src, template, out = sys.argv[1:4]
bff = open(sys.argv[4]).read() if len(sys.argv) > 4 else json.dumps(dict(programs=0, epochs=[], entropy={}, data=""))
d = json.load(open(src))
slot, free, pending, buf, prev = {}, [], [], bytearray(), set()
for fr in d["frames"]:
    ids = {a[0] for a in fr}
    free += pending
    pending = [slot.pop(i) for i in prev - ids]
    free.sort()
    buf += struct.pack("<H", len(fr))
    for i, x, y, dr, pen, nn, sp in fr:
        if i not in slot:
            slot[i] = free.pop(0) if free else len(slot) + len(free) + len(pending)
        buf += struct.pack("<HHHHH", slot[i], x, y, dr | ((pen + 1) << 9) | (min(nn, 31) << 11), sp)
    prev = ids
html = open(template).read()
html = html.replace("__FRAMES__", base64.b64encode(bytes(buf)).decode())
html = html.replace("__META__", json.dumps(dict(L=d["L"], t0=d["t0"])))
html = html.replace("__BFF__", bff.replace("</", "<\\/"))
head = ('<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">\n'
        '<meta name="description" content="Self-evolving agent civilizations. Real footage from an '
        'open-ended evolution simulation.">\n')
title_end = html.index("</style>") + len("</style>")
open(out, "w").write(head + html[:title_end] + "\n</head>\n<body>\n" + html[title_end:] + "\n</body>\n</html>\n")
