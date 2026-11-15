#!/bin/bash
# Restore an archived run directory from the 'run-data' branch (fetches only that run's parts).
# Usage: tools/storage/restore_run.sh RUNDIR   (e.g. runs6/gebgrow_s1)
set -eu
repo=$(git rev-parse --show-toplevel);p=$(realpath -m "$1");p=${p#"$repo"/}
url=$(git -C "$repo" remote get-url origin);tmp=$(mktemp -d)
git init -q "$tmp/g";cd "$tmp/g";git remote add origin "$url"
git fetch -q --depth=1 --filter=blob:none origin run-data
for f in $(git ls-tree --name-only FETCH_HEAD "data/$p/"|sort);do git show "FETCH_HEAD:$f">"$tmp/${f##*/}";done
cd "$tmp";sha256sum -c --quiet SHA256SUMS
mkdir -p "$(dirname "$repo/$p")";rm -f "$repo/$p/ARCHIVED"
cat "$tmp"/run.tar.xz.part*|xz -dc|tar -C "$(dirname "$repo/$p")" -xf -
rm -rf "$tmp";echo "restored $p"
