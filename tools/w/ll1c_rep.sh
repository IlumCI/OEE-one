#!/usr/bin/env bash
# LL1c replication summary (docs/LIFE.md): per arm over seeds 1-6, the counts of freeze t < -3 and of wins against E
# (t > 2), the pooled share against E (mean of per-seed shares, one-sample t over seeds against 0.5), and LL2 against L
# head to head in each seed (both learning; B = L learns from energy only, -rew_b 0). Usage: tools/w/ll1c_rep.sh BIN [SEEDS=16 THREADS=2]
set -u;bin="$1";n="${SEEDS:-16}";T="${THREADS:-2}";R=docs/results/ll1c
for a in L LL2;do awk -v a=$a -F'\t' 'FNR==1{f=0}$1=="freeze"{if($5<-3)fz++;nf++}$1=="vs_E"{x[++k]=$3;if($5>2)w++}
  END{m=0;for(i=1;i<=k;++i)m+=x[i];m/=k;s=0;for(i=1;i<=k;++i)s+=(x[i]-m)^2;sd=k>1?sqrt(s/(k-1)):0;t=sd>0?(m-0.5)/(sd/sqrt(k)):0;
    printf "%s: freeze t<-3 in %d of %d; beats E (t>2) in %d of %d; pooled share against E %.3f (sd %.3f, t %.2f over %d seeds)\n",a,fz,nf,w,k,m,sd,t,k}' $R/w1k_${a}_s[1-6].txt;done
W=$(tools/w/wflags.sh runs8/w1k_LL2_s1)
for s in 1 2 3 4 5 6;do x=runs8/w1k_LL2_s$s/archive/egg_2000.bin;y=runs8/w1k_L_s$s/archive/egg_2000.bin;[ -f $x ]&&[ -f $y ]||continue
  echo -e "LL2_vs_L\ts$s\t$("$bin" -tourney $x $y $W -rew_b 0 -seeds $n -threads $T|awk '/^mean_lshareA/{printf "%s\t%s\t%s\t%s",$2,$4,$6,$8}')";done
