#!/usr/bin/env python3
"""Encode spatial BFF snapshots (oee_bff -grid W H -dump-every K) for the showcase map scene.

Usage: encode_bff2d.py DUMP_DIR W H OUT_JSON [--min-copies 8] [--min-commands 6]
One byte per program and frame:
  bits 0-2: family 1..6 if the program's instruction skeleton (commands kept, other bytes
            blanked) occurs at least MIN_COPIES times in the grid and has at least
            MIN_COMMANDS instructions, else 0;
  bits 3-4: a static shade (0..3) from the program's bytes, for the background texture.
Frames are concatenated, gzip-compressed and base64-encoded.
"""
import argparse
import base64
import glob
import gzip
import json
import os
import zlib
from collections import Counter

import numpy as np

CMD = np.zeros(256, bool)
for c in b"[]+-.,<>{}":
    CMD[c] = True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dumps")
    ap.add_argument("w", type=int)
    ap.add_argument("h", type=int)
    ap.add_argument("out")
    ap.add_argument("--min-copies", type=int, default=8)
    ap.add_argument("--min-commands", type=int, default=6)
    a = ap.parse_args()
    files = sorted(glob.glob(os.path.join(a.dumps, "soup_*.bin")))
    raw = bytearray()
    epochs = []
    for f in files:
        soup = np.fromfile(f, np.uint8).reshape(a.w * a.h, 64)
        skel = np.where(CMD[soup], soup, 0).astype(np.uint8)
        keys = [r.tobytes() for r in skel]
        cnt = Counter(keys)
        ncmd = CMD[soup].sum(1)
        shade = (soup.astype(np.uint32).sum(1) % 4).astype(np.uint8)
        out = np.empty(len(keys), np.uint8)
        for i, k in enumerate(keys):
            fam = 1 + zlib.crc32(k) % 6 if cnt[k] >= a.min_copies and ncmd[i] >= a.min_commands else 0
            out[i] = fam | (shade[i] << 3)
        raw += out.tobytes()
        epochs.append(int(os.path.basename(f)[5:15]))
    meta = dict(w=a.w, h=a.h, epochs=epochs, data=base64.b64encode(gzip.compress(bytes(raw), 9)).decode())
    json.dump(meta, open(a.out, "w"))
    print(f"{len(files)} frames, {len(raw)} raw bytes, {len(meta['data'])} base64 bytes")


if __name__ == "__main__":
    main()
