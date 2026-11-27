#!/usr/bin/env bash
# LL1c verdict table (docs/LIFE.md): one row per run from docs/results/ll1c/RUN.txt and the run's log (reseeds).
# Columns: freeze share (t), against E (t), toxic share intact / frozen, toxic share by age 0-99 and 400+, progress
# against epoch 1000 (t), reseeds. Usage: tools/w/ll1c_table.sh [RESULTS_DIR]
set -u;R="${1:-docs/results/ll1c}"
echo "| Run | Freeze: frozen share (t) | Against E (t) | Toxic share intact / frozen | Intact toxic, age 0-99 -> 400+ | Against epoch 1000 (t) | Reseeds |"
echo "|---|---|---|---|---|---|---|"
for a in E L LM LL2;do for s in 1 2 3 4 5 6;do f="$R/w1k_${a}_s$s.txt";r=runs8/w1k_${a}_s$s;[ -f "$f" ]||continue
  awk -v n="${a} s$s" -v rs="$(grep -v '^#' $r/log.txt 2>/dev/null|tail -1|cut -f9)" -F'\t' '
    $1=="freeze"{fz=sprintf("%.3f (%s)",$3,$5)} $1=="vs_E"{ve=sprintf("%.3f (%s)",$3,$5)} $1=="vs_own"{vo=sprintf("%.3f (%s)",$3,$5)}
    $1=="curve"{ci=$3;split($4,b," ");a0=b[1];a4=b[5];sub(/^0:/,"",a0);sub(/\/.*/,"",a0);sub(/^400:/,"",a4);sub(/\/.*/,"",a4)}
    $1=="curve_frozen"{cf=$3}
    END{printf "| %s | %s | %s | %s / %s | %s -> %s | %s | %s |\n",n,fz?fz:"-",ve?ve:"-",ci,cf?cf:"-",a0,a4,vo,rs}' "$f";done;done
