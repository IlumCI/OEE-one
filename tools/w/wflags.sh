#!/usr/bin/env bash
# World and brain flags of a run, read from RUNDIR/cmd.txt (training-only flags dropped), so tournaments play in the
# training world. -climate_every c becomes -climate_ticks c*H.
# Usage: tools/w/wflags.sh RUNDIR
set -u
sed 's/#.*//' "$1/cmd.txt"|tr -s ' \t' '\n\n'|awk 'BEGIN{n=split("-L -S -regrow -base -patches -nact -season -view -signals -rot -sig_cost -matter -m_steps -m_decay -mk_cost -wr_cost -feed_amt -rep_cost -wall_cost -spawn -mut -own -wall_decay -kin_sig -lamarck -nodes -node_pay -node_per -node_ripe -m_cost -m_give -graze -pass -irr -irr_decay -claim -land -farms -farm_store -farm_every -farm_free -irr_full -farm_reset -home_cue -life -life_repro -life_mod -life_eta -life_lam -life_beta -life_rew -life_teach -life_obs -give -give_amt -art -art_glyphs -art_pay -art_rest -art_era -art_family -art_lock -layers -sig -D -founders",a," ");for(i=1;i<=n;++i)k[a[i]]=1;H=200}
  p=="-H"{H=$0}p=="-climate_every"{c=$0}k[p]{o=o p" "$0" "}{p=$0}END{if(c!="")o=o"-climate_ticks "c*H" ";sub(/ $/,"",o);print o}'
