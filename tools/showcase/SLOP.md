# Slop rules for OEE·ONE films

Distilled 2026-11-23 from: motionmaxxing (github.com/Tejashmakwana/motionmaxxing, SKILL.md + references/slop.md),
opus.pro "12 tells", wildsite "what looks like AI slop", 925studios "AI slop design tells".
charliehills "29 tells" is paywalled (1 of 29 seen: the highlighter pill). Root cause per all sources:
defaults nobody decided. A film passes only if every item below is either absent or justified in NOTE.md.

## Never ship
1. Page chrome / HUD: names in corners, tracked-caps labels, counters, timecodes, step readouts, progress bars, brackets.
2. Deck layouts: text block left + media right, kicker over headline, title-then-content.
3. Stat slides: big number over small label; count-up tween to a value.
4. Slogan copy: "Not just X. It's Y.", one-word triplets, "the future of", taglines.
5. Fades on everything; dissolves between scenes; fade to black at the end.
6. Logo + tagline end card held 2 s+; logo scaling up from zero.
7. Flat void as the ground for every scene; small object floating on black.
8. Premium pastiche: near-black + glow + sparks + bloom with no physical light source.
9. A technique per beat (showreel) instead of one idea.
10. Letter-typed headline with caret (only allowed inside a product's own input).
11. Equal beat lengths; identical entrance for every element; uniform stagger.
12. Dead frames: more than 3 consecutive frames with nothing moving.
13. Sound as wallpaper: flat loop at one level, a click on every cut and word.
14. Invented numbers; claims not in docs or run data.
15. A film nobody looked at frame by frame.

## Laws
- Every beat buys one belief; one hero per frame; one world (a surface with light), not a void.
- Show it, then say it; words are few and arrive whole, at speech rhythm.
- Cut on motion; the last frame of a beat hands an object to the next.
- Arrivals decelerate (from oversize, never from zero); exits accelerate and are faster.
- One accent colour with one meaning; colour as an event, not spread evenly.
- Sound: 1-3 hits tied to visible causes, designed silence, music shaped to the edit.
- Ending: cut mid-motion, end hold 0.6-1.4 s.

## Audit of brag v1 (2026-11-22) against this list
Fails: tracked-caps plates on every line (1), "Live run · today" corner tag and STEP counter (1), left text + right
world in "Now" and ladder-left + line-right in the goal (2), Class 3b line with 18→106 count-up (3), taglines (4),
opacity/blur entrances on every element (5, 11), wordmark held 2.17 s at the end (6), void ground in 3 of 6 scenes (7),
click on each lineage name and a hit on every cut over a flat corporate loop (13), feature-tour structure with
near-equal beats (11). Passes: no invented numbers (14), every frame reviewed (15).

## Claude-specific slop (the house style this agent falls into)
Sources: Anthropic prompting guide (Opus 4.8, "Design and frontend defaults"), Anthropic frontend-aesthetics
cookbook (via theadpharm), generativelabs, Kyle Chayka / New Yorker (via pxlnv), linkandth.ink "22 Claude cliches".
- Default look: warm cream ground (~#F4F1EA), serif display (Georgia, Fraunces, Playfair), italic accent words,
  terracotta/amber accent, tracked-out subheads, ticker-like bars, dashboards of rounded rectangles.
  Underlying grammar: quiet field + dark ink + one saturated warm accent.
- Its dark twin (what brag v1 and the brain test frames are): near-black ground + one warm amber/gold accent +
  glow + monospace or pixel caps in tracked plates. Same grammar inverted. Counts as Claude slop.
- "Escape" defaults are also slop: Space Grotesk as the upgrade, purple/indigo gradients, glassmorphism,
  animated blobs, shadcn cards, one orchestrated load with staggered reveals, blur-in titles.
- Telling it "don't use cream" swaps in another fixed palette (Anthropic's own docs). A colour swap does not escape
  the grammar. Fix: a concrete positive spec (hexes, faces, radius, motion timings, density) derived from the subject,
  or propose 4 distinct directions with one-line rationale and let the owner pick one.
- Tool defaults are not neutral: the Hyperframes motion guide's suggested title entrance (blur 8px -> 0 in 0.2 s,
  per-word stagger) is a house default too; using it unchanged reads as generated.
- On-screen copy tells (from Claude prose): contrastive binary "not X but Y" / "X, not Y" (most frequent),
  aphoristic one-line enders, colon reveals, mirrored-clause symmetry, stakes-raising, significance-signalling,
  candor flags ("the honest answer"), spaced-dash asides, rule-of-three lists. Brag v1 used two of these
  ("Open-ended evolution. The route to superintelligence." as an aphoristic ender; the planned "evolution, not
  imitation" binary).
