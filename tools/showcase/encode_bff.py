#!/usr/bin/env python3
"""Encode BFF soup dumps (oee_bff -dump-every) into the showcase footage stream.

Usage: encode_bff.py DUMP_DIR FIRST_EPOCH LAST_EPOCH OUT_JSON [--programs 1296]
Each frame: the first N programs of the soup (program slots persist across epochs), each
byte mapped to a 4-bit class:
  0 zero byte, 1-2 no-op (two shades by byte parity), 3 '[', 4 ']', 5 '+', 6 '-',
  7 '.', 8 ',', 9 '<', 10 '>', 11 '{', 12 '}'.
Frames are packed two classes per byte, concatenated, gzip-compressed and base64-encoded;
the page decompresses them with DecompressionStream('gzip').
"""
import argparse
import base64
import glob
import gzip
import json
import os

import numpy as np

CMDS = {ord('['): 3, ord(']'): 4, ord('+'): 5, ord('-'): 6, ord('.'): 7, ord(','): 8,
        ord('<'): 9, ord('>'): 10, ord('{'): 11, ord('}'): 12}
LUT = np.array([0 if b == 0 else CMDS.get(b, 1 + (b & 1)) for b in range(256)], np.uint8)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dumps")
    ap.add_argument("first", type=int)
    ap.add_argument("last", type=int)
    ap.add_argument("out")
    ap.add_argument("--programs", type=int, default=1296)
    ap.add_argument("--log")
    a = ap.parse_args()
    files = []
    for f in sorted(glob.glob(os.path.join(a.dumps, "soup_*.bin"))):
        e = int(os.path.basename(f)[5:15])
        if a.first <= e <= a.last:
            files.append((e, f))
    n = a.programs * 64
    raw = bytearray()
    for _, f in files:
        cls = LUT[np.fromfile(f, np.uint8, count=n)]
        raw += (cls[0::2] << 4 | cls[1::2]).tobytes()
    entropy = {}
    if a.log:
        for line in open(a.log).read().splitlines()[1:]:
            c = line.split(",")
            entropy[int(c[0])] = float(c[3])
    meta = dict(programs=a.programs, epochs=[e for e, _ in files], entropy=entropy,
                data=base64.b64encode(gzip.compress(bytes(raw), 9)).decode())
    json.dump(meta, open(a.out, "w"))
    print(f"{len(files)} frames, {len(raw)} raw bytes, {len(meta['data'])} base64 bytes")


if __name__ == "__main__":
    main()
