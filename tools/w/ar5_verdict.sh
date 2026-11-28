#!/usr/bin/env bash
# AR5 verdict (docs/ARTIFACTS.md, protocol fixed 2026-11-28 11:10 UTC) from DIR/w2a7_{E,L,LT}_s*.txt (ar5_eval.sh +
# viab_any.sh). Per learning arm over its scored seeds (n; the bar is ceil(2n/3), i.e. 2 of 3, 4 of 6):
#  pass: freeze t<-3, beats E t>2, tokens per opening below E's (same seed); LT also noteach t<-3, beats L t>2.
#  kill: freeze |t|<2 in every scored seed of L and LT.
#  extension: a t in the grey zone (freeze/noteach t in [-3,-2], vs_E/vs_L t in [2,3]) is deciding when counting the
#  grey values as passes or as failures gives different verdicts; then the arm goes to 6 seeds before a verdict.
# Usage: tools/w/ar5_verdict.sh [DIR]
d="${1:-docs/results/ar5}"
awk 'function g(A,k){return (k in A)&&A[k]!=""?A[k]:"-"}
function cr(nm,x,dr,s){if(x=="-")return;x+=0
  if(dr<0){C[nm]+=x<-3;LO[nm]+=x<-3;HI[nm]+=x<=-2;if(x>=-3&&x<=-2)ext=ext" "nm"_s"s}
  else{C[nm]+=x>2;LO[nm]+=x>3;HI[nm]+=x>=2;if(x>=2&&x<=3)ext=ext" "nm"_s"s}}
function ok(A,lt){return A["freeze"]>=b&&A["vsE"]>=b&&A["tok"]>=b&&(!lt||(A["noteach"]>=b&&A["vsL"]>=b))}
{n=split(FILENAME,p,"/");f=p[n];sub(/\.txt$/,"",f);split(f,q,"_");a=q[2];s=substr(q[3],2)+0;R[a,s]=1;if(s>ms)ms=s}
$1=="freeze"{F[a,s]=$5;FS_[a,s]=$3}$1=="noteach"{N[a,s]=$5;NS[a,s]=$3}$1=="vs_E"{VE[a,s]=$5;VES[a,s]=$3}
$1=="vs_L"{VL[a,s]=$5;VLS[a,s]=$3}$1~/_solo$/{D[$1,a,s]=$3" ("$5")";HD[$1]=1}$1=="vs_own"{O[a,s]=$3" ("$5")"}
$1=="knowledge"{for(i=3;i<NF;++i)if($i=="tokens_per_opening")K[a,s]=$(i+1)}
$1~/^w2a7_.*:[0-9]+:[0-9]+/{split($1,z,":");V[a,s]=z[3]}
END{print "| run | freeze share (t) | noteach share (t) | vs E share (t) | vs L share (t) | tokens/opening (E) | vs own e1000 (t) | max reseeds in 200 |"
print "|---|---|---|---|---|---|---|---|"
na=split("E L LT",ar," ")
for(i=1;i<=na;++i)for(s=1;s<=ms;++s){a=ar[i];if(!((a,s) in R))continue;k=a SUBSEP s
  printf "| %s s%d | %s (%s) | %s (%s) | %s (%s) | %s (%s) | %s (%s) | %s | %s |\n",a,s,g(FS_,k),g(F,k),g(NS,k),g(N,k),g(VES,k),g(VE,k),g(VLS,k),g(VL,k),g(K,k),g(K,"E" SUBSEP s),g(O,k),g(V,k)}
if(length(HD)){printf "\nDiagnostics, not criteria: solo log ratio of post-founder biomass, A over B (t)\n| run | freeze_solo | noteach_solo | vs_E_solo | vs_L_solo |\n|---|---|---|---|---|\n"
  for(i=2;i<=na;++i)for(s=1;s<=ms;++s){a=ar[i];if(!((a,s) in R))continue;o="| "a" s"s" |";split("freeze_solo noteach_solo vs_E_solo vs_L_solo",dn," ");for(j=1;j<=4;++j)o=o" "((dn[j],a,s) in D?D[dn[j],a,s]:"-")" |";print o}}
kill=1;nk=0
for(i=2;i<=na;++i){a=ar[i];lt=a=="LT";n=0;ext="";split("",C);split("",LO);split("",HI)
  for(s=1;s<=ms;++s){k=a SUBSEP s;if(g(F,k)=="-")continue;++n;++nk;t=F[k]+0;if(t>=2||t<=-2)kill=0
    cr("freeze",F[k],-1,s);cr("vsE",g(VE,k),1,s);if(lt){cr("noteach",g(N,k),-1,s);cr("vsL",g(VL,k),1,s)}
    x=g(K,k)!="-"&&g(K,"E" SUBSEP s)!="-"&&K[k]+0<K["E",s]+0;C["tok"]+=x;LO["tok"]+=x;HI["tok"]+=x}
  b=int((2*n+2)/3);printf "\n%s: %d seeds scored, bar %d; freeze %d, beats E %d, tokens/opening below E %d",a,n,b,C["freeze"],C["vsE"],C["tok"]
  if(lt)printf ", noteach %d, beats L %d",C["noteach"],C["vsL"]
  v=ok(C,lt)?"PASS":"FAIL";if(n<3)v="PENDING (fewer than 3 seeds)";else if(n<6&&ok(LO,lt)!=ok(HI,lt))v="EXTEND to 6 seeds before a verdict (provisional "v")"
  printf "\n  %s%s\n",v,ext!=""?"; grey zone:"ext:""}
printf "\nkill (freeze |t|<2 in every scored seed of L and LT, %d scored): %s\n",nk,nk==0?"PENDING":kill?"MET":"not met"}' "$d"/w2a7_*_s*.txt
