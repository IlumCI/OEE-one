# Geb version 8 (v08b3), vendored reference

- Author: Alastair Channon. License: MIT (see `LICENSE`; the notice also appears in each
  source file header).
- Source: http://www.channon.net/alastair/geb/geb_v08/geb_v08b3.src.tar.gz
- SHA-256 of the downloaded archive (an uncompressed tar despite the `.tar.gz` name):
  `387bffa0568b1806ebb2987374b3f9e16d418d3317314e43a847da6c1d2abcf5`
- All files in this directory except `LICENSE` and this README are unmodified.

## Role in OEE-one
This is the read-only reference oracle for milestone M1. The C++20 port in `src/geb/`
must reproduce this code's trajectories and component activity statistics exactly for
fixed seeds before any modernisation. Do not edit files here; record deviations in
`docs/GEB_SPEC.md`.

## Building the reference
`tools/geb_reference/build.sh` builds `geb_resets` and `geb_no_resets` out of tree with
the original flags (`-O3 -ffloat-store`). Headless operation: with no X display, `main`
steps `ga_main_loop()` until `-stopat N` and exits with code 100.

Verified with g++ 13.3 (Ubuntu 24.04): compiles cleanly; two runs with `-seed 1 -stats
-rulestat -stopat 200` produce byte-identical stdout, `rulestat` and `SHADOWrulestat`.
