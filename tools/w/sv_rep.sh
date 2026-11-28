#!/usr/bin/env bash
# SL1 V in LL1c summary (docs/SOCIAL.md): over seeds 1-6, counts of noteach t < -3, freeze t < -3 and wins against E
# (t > 2); pooled share against E (one-sample t over seeds); SV against L head to head (B does not teach, -teach_b 0);
# SV frozen against E (hiding); newborn (age 0-99) toxic share of SV and L. Usage: tools/w/sv_rep.sh BIN [SEEDS=16 THREADS=4]
set -u;bin="$1";n="${SEEDS:-16}";T="${THREADS:-4}";R=docs/results/ll1c
awk -F'\t' '$1=="noteach"{if($5<-3)nt++;nn++}$1=="freeze"{if($5<-3)fz++;nf++}$1=="vs_E"{x[++k]=$3;if($5>2)w++}
  END{m=0;for(i=1;i<=k;++i)m+=x[i];m/=k;s=0;for(i=1;i<=k;++i)s+=(x[i]-m)^2;sd=k>1?sqrt(s/(k-1)):0;t=sd>0?(m-0.5)/(sd/sqrt(k)):0;
    printf "SV: noteach t<-3 in %d of %d; freeze t<-3 in %d of %d; beats E (t>2) in %d of %d; pooled share against E %.3f (sd %.3f, t %.2f over %d seeds)\n",nt,nn,fz,nf,w,k,m,sd,t,k}' $R/w1k_SV_s[1-6].txt
W=$(tools/w/wflags.sh runs8/w1k_SV_s1);WL=$(tools/w/wflags.sh runs8/w1k_L_s1)
for s in 1 2 3 4 5 6;do x=runs8/w1k_SV_s$s/archive/egg_2000.bin;y=runs8/w1k_L_s$s/archive/egg_2000.bin;e=runs8/w1k_E_s$s/archive/egg_2000.bin;[ -f $x ]||continue
  h=$("$bin" -tourney $x $y $W -teach_b 0 -seeds $n -threads $T|awk '/^mean_lshareA/{printf "%s\t%s\t%s\t%s",$2,$4,$6,$8}')
  z=$("$bin" -tourney $x $e $W -life_b 0 -ablate freeze -seeds $n -threads $T|awk '/^mean_lshareA/{printf "%s (t %s)",$2,$6}')
  nb=$(awk -F'\t' '$1=="curve"{split($4,b," ");sub(/^0:/,"",b[1]);sub(/\/.*/,"",b[1]);print b[1]}' $R/w1k_SV_s$s.txt)
  nl=$(awk -F'\t' '$1=="curve"{split($4,b," ");sub(/^0:/,"",b[1]);sub(/\/.*/,"",b[1]);print b[1]}' $R/w1k_L_s$s.txt)
  echo -e "SV_vs_L\ts$s\t$h\tfrozen_SV_vs_E $z\tnewborn_toxic SV $nb L $nl";done
