#!/bin/bash
# Lossless packing of finished runs (docs/COMPUTE.md step 5).
#   pack_runs.sh pack DIR...    presence -> presence.xz for each DIR with a 'finished' marker and no
#                               live simulator; verified by decompress-and-compare before the plain
#                               file is removed. Readers (tools/activity) open either form.
#   pack_runs.sh unpack DIR...  presence.xz -> presence (needed before resuming a run).
# XZ_THREADS (default 2) threads per file.
set -u
mode=$1;shift
for d in "$@";do
d=${d%/}
if [ "$mode" = pack ];then
[ -f "$d/finished" ]||{ echo "skip $d (not finished)";continue;}
[ -f "$d/pid" ]&&grep -qa oee_geb "/proc/$(cat "$d/pid")/cmdline" 2>/dev/null&&{ echo "skip $d (running)";continue;}
[ -f "$d/presence" ]||continue
[ -f "$d/presence.xz" ]&&{ echo "skip $d (both forms present)";continue;}
xz -6 -T"${XZ_THREADS:-2}" -k -c "$d/presence" > "$d/presence.xz.tmp"||{ rm -f "$d/presence.xz.tmp";echo "FAIL $d";continue;}
if xz -dc "$d/presence.xz.tmp"|cmp -s - "$d/presence";then
mv "$d/presence.xz.tmp" "$d/presence.xz"&&rm "$d/presence"&&echo "packed $d $(stat -c %s "$d/presence.xz")"
else rm -f "$d/presence.xz.tmp";echo "VERIFY FAIL $d";fi
elif [ "$mode" = unpack ];then
[ -f "$d/presence.xz" ]||continue
xz -dc "$d/presence.xz" > "$d/presence.tmp"&&mv "$d/presence.tmp" "$d/presence"&&rm "$d/presence.xz"&&echo "unpacked $d"
fi
done
