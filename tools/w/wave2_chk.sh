#!/usr/bin/env bash
# Adapter for w3_when.sh (CHK=tools/w/wave2_chk.sh): scores a wave-2 arm runs8/w3c<arm>_sN against C (runs8/w3c_sN)
# with tools/w/wave2_eval.sh. Usage: tools/w/wave2_chk.sh BIN ARM_RUN EPOCH [seeds] [threads]
set -u;bin="$1";a="$2";e="$3";n="${4:-16}";T="${5:-1}";c=$(echo "$a"|sed 's/w3c[a-z]*_s/w3c_s/')
exec "$(dirname "$0")"/wave2_eval.sh "$bin" "$a" "$c" "$e" "$n" "$T"
