# OEE-one

Open-ended evolution as the route to artificial superintelligence.

- [docs/DESIGN.md](docs/DESIGN.md): architecture, prior-system synthesis, transformer
  scaffold policy.
- [docs/PLAN.md](docs/PLAN.md): implementation plan.
- [docs/GEB_SPEC.md](docs/GEB_SPEC.md): specification of Geb v08b3 as ported.

## Milestone M1: Geb-class L2

`third_party/geb_v08b3/` holds Alastair Channon's original Geb v08b3 source (MIT, unmodified),
used as the reference oracle. `src/geb/` is a C++20 port that reproduces the original's
outputs byte-for-byte (`tests/golden/`).

### Build and test

```
cmake --preset release && cmake --build --preset release && ctest --preset release
cmake --preset asan    && cmake --build --preset asan    && ctest --preset asan
tools/geb_reference/build.sh            # optional: build the original as an oracle
```

### Run

```
build/rel/oee_geb_resets -seed 1 -stats -rulestat -stopat 6000000 \
    -components components -second-shadow -presence presence
```

Original options: `-seed -length -neurons -stats -rulestat -stopat`. Extensions:
`-variant resets|no_resets`, `-components FILE`, `-presence FILE`, `-second-shadow`
(independent null shadow, does not alter the run), `-random-selection` (GebR),
`-outdir DIR`.

### Evolutionary activity analysis

```
python3 tools/activity/geb_activity.py RUN --variant resets --grouping pb --source presence [--null] --plot
python3 tools/activity/report.py OUT RUN [RUN ...]
```

Implements Bedau-Packard activity statistics and Channon's component activity
normalisation (GPEM 2006), with a second-shadow null control.
