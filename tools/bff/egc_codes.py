#!/usr/bin/env python3
"""Genetic-code statistics of a BFF soup checkpoint (docs/EGC.md).

Usage: egc_codes.py CK [CK ...]
For each checkpoint: number of distinct codon tables, share of the most common table, Shannon entropy
of the table distribution (bits), share of programs whose table is the standard code, and the number
of tables held by at least 1% of the soup ("dialects").
"""
import math,struct,sys
from collections import Counter
TAPE,CODON=64,10
STD=b"[]+-.,<>{}"
def stats(path):
    raw=open(path,"rb").read()
    n=struct.unpack("<6Q",raw[:48])[1]
    soup=raw[48:48+n*TAPE]
    c=Counter(soup[i*TAPE:i*TAPE+CODON] for i in range(n))
    top=c.most_common(1)[0][1]/n
    H=-sum(v/n*math.log2(v/n) for v in c.values())
    return dict(programs=n,distinct=len(c),top_share=round(top,4),entropy_bits=round(H,3),
                standard_share=round(c.get(STD,0)/n,4),dialects=sum(1 for v in c.values() if v>=0.01*n))
for p in sys.argv[1:]:
    print(p,stats(p))
