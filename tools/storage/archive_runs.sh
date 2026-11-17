#!/bin/bash
# Archive finished runs to the 'run-data' branch on origin and free the container (docs/COMPUTE.md
# step 5). Usage: tools/storage/archive_runs.sh TMPDIR BATCH_MB RUNDIR...
# Each RUNDIR (finished, not running) becomes
#   data/<runs dir>/<run>/run.tar.xz.partNNN  (xz -3, 90 MB parts) + SHA256SUMS + files.txt
# Batches of about BATCH_MB are committed in a throwaway blob-less clone (only the branch's trees are
# fetched, so local disk holds one batch) and pushed; the branch tip holds every archived run.
# After a successful push the run's files are replaced by a stub: finished, exit_code, ARCHIVED.
# ANY=1: also directories without a 'finished' marker (completed tool outputs, stopped runs).
# KEEP=1: upload a copy and keep the local files (paused/resumable or running runs); re-archiving a
# kept run replaces its earlier copy on the branch.
set -u
T=$1;B=$2;shift 2
repo=$(git rev-parse --show-toplevel);url=$(git -C "$repo" remote get-url origin)
R=;pending=();acc=0
start(){
R=$(mktemp -d "$T/rd.XXXX");git init -q "$R";cd "$R"||exit 1
git config user.name ilumCI;git config user.email ilumbackup@gmail.com
git remote add origin "$url";git symbolic-ref HEAD refs/heads/run-data
if git fetch -q --depth=1 --filter=blob:none origin run-data 2>/dev/null;then git update-ref refs/heads/run-data FETCH_HEAD;git read-tree FETCH_HEAD
else printf '# run-data\nArchived OEE-one run directories; restore with tools/storage/restore_run.sh on the main branch.\n'>README.md;git add README.md;fi
cd "$repo"||exit 1
}
flush(){
[ ${#pending[@]} -eq 0 ]&&return 0
(cd "$R"&&git add data&&git commit -qm "archive runs"&&for i in 1 2 3 4;do git push -q origin run-data&&exit 0;sleep $((i*15));done;exit 1)||{ echo "PUSH FAIL (kept $R)";exit 1;}
for d in "${pending[@]}";do
p="${d#"$repo"/}"
[ -n "${KEEP:-}" ]&&{ echo "copied $p";continue;}
find "$d" -mindepth 1 -maxdepth 1 ! -name finished ! -name exit_code -exec rm -rf {} +
echo "run-data:data/$p">"$d/ARCHIVED";echo "archived $p"
done
rm -rf "$R";pending=();acc=0;start
}
start
for d in "$@";do
d=$(realpath "$d");p="${d#"$repo"/}"
[ -f "$d/finished" ]||[ -n "${ANY:-}" ]||{ echo "skip $p (not finished)";continue;}
[ -f "$d/ARCHIVED" ]&&continue
[ -z "${KEEP:-}" ]&&[ -f "$d/pid" ]&&grep -qa oee_geb "/proc/$(cat "$d/pid")/cmdline" 2>/dev/null&&{ echo "skip $p (running)";continue;}
# plain presence is fine: xz in the archive compresses it
o="$R/data/$p";(cd "$R"&&git rm -rq --cached --ignore-unmatch "data/$p" >/dev/null 2>&1);rm -rf "$o";mkdir -p "$o"
(cd "$d"&&find . -type f|sort>"$o/files.txt")
tar -C "$(dirname "$d")" -cf - "$(basename "$d")"|xz -3 -T2 -c|split -b 90M -d -a 3 - "$o/run.tar.xz.part"
(cd "$o"&&sha256sum run.tar.xz.part*>SHA256SUMS)
if ! cat "$o"/run.tar.xz.part*|xz -dc|tar -tf - >/dev/null;then echo "VERIFY FAIL $p";rm -rf "$o";continue;fi
pending+=("$d");acc=$((acc+$(du -sm "$o"|cut -f1)))
[ $acc -ge "$B" ]&&flush
done
flush
rm -rf "$R"
