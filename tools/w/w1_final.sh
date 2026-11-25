#!/usr/bin/env bash
# Full W1 analysis once runs finish (docs/WORLD.md §6, W1 protocol). For each run:
#   era progress (e vs e-250, 16 seeds) -> RUN/era_250.tsv and OUT/<run>_era.tsv
#   cyclic share of the champion matrix (every 250 epochs) -> OUT/<run>_ciao.txt
#   memory ablations (amnesia, shuffle) of the last champion -> OUT/<run>_memory.tsv
#   era progress at 1000-epoch steps in a held-out arena (48x48, 4 patches, regrowth 1) -> OUT/<run>_heldout.tsv
# Usage: tools/w/w1_final.sh BIN OUTDIR RUN...   (then: tools/w/w1_report.py RUN...)
set -u;bin="$1";out="$2";shift 2;mkdir -p "$out";W="-S 64 -regrow 2 -nact 7 -ticks 2000"
for r in "$@";do n=$(basename "$r")
  tools/w/w1_eval.sh "$bin" "$r" 250 16 4 > "$r/era_250.tsv";cp "$r/era_250.tsv" "$out/${n}_era.tsv"
  python3 tools/w/ciao.py "$bin" "$r" --every 250 --seeds 16 --threads 4 > "$out/${n}_ciao.txt"
  last=$(ls "$r"/archive/egg_*.bin|sed 's/.*egg_\([0-9]*\).bin/\1/'|sort -n|tail -1);g="$r/archive/egg_$last.bin"
  { printf "ablation\tepoch\tishareA\tse\tt\tlshareA\tlse\tlt\n"
    for ab in amnesia shuffle;do "$bin" -tourney "$g" "$g" -seeds 16 -threads 4 $W -ablate $ab|
      awk -v a=$ab -v e=$last '/^mean_ishareA/{i=$2"\t"$4"\t"$6}/^mean_lshareA/{printf "%s\t%s\t%s\t%s\t%s\t%s\n",a,e,i,$2,$4,$6}';done; } > "$out/${n}_memory.tsv"
  tools/w/w1_eval.sh "$bin" "$r" 1000 16 4 -L 48 -patches 4 -regrow 1 -S 64 -nact 7 -ticks 2000 > "$out/${n}_heldout.tsv"
done
