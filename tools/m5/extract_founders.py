#!/usr/bin/env python3
"""Founder genomes for the M5 coupling: the most frequent exact programs (raw 64 bytes) in
BFF soup checkpoints whose replicators held (docs/M3.md).

Usage: extract_founders.py OUT.txt TOP CHECKPOINT [CHECKPOINT ...]
Writes one genome per line as hex, with a comment line per source.
"""
import sys
from collections import Counter

import numpy as np


def main():
    out, top, cks = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
    lines = []
    for ck in cks:
        raw = open(ck, "rb").read()
        magic = int.from_bytes(raw[:8], "little")
        hdr = 48 if magic == 0x4f45454246460002 else 40
        soup = np.frombuffer(raw[hdr:], np.uint8).reshape(-1, 64)
        cnt = Counter(r.tobytes() for r in soup)
        lines.append(f"# {ck}: {len(soup)} programs")
        for prog, c in cnt.most_common(top):
            lines.append(prog.hex())
            print(f"{ck}: {c} copies")
    open(out, "w").write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
