# Brand images (site/brand/)

- `oee-one-avatar.png` (1024x1024): a real nation. The densest large bonded colony in `l3v6L40_s1`
  (division of labour, L40, ~3.94M steps): 32 members packed into a 7x7-cell area, 54 bonds (most
  members at the 4-bond cap), 19 warriors (red cores) and 13 breeders (dark cores). Neighbouring
  colonies faint. Positions and bonds are as in the simulation (`nation.html`, `render_one.js`).
- `oee-one-avatar-glyph.png` (1024x1024): the most common evolved body in `l3v7_s1` at 3.5M steps (sprite 28253,
  21 of 1417 agents), drawn as a 5x5 pixel glyph with its red arm, on the showcase lattice. No text.
- `oee-one-avatar-world.png` (1024x1024): the 48x48 growing world at 3.5M steps, with the borders of
  every earlier world size (20 ... 44) drawn as growth rings.
- `oee-one-banner.png` (1500x500): world stages at 0.25M, 1M, 2M, 3M, 3.5M (20x20 -> 48x48), same cell
  size, growing left to right; OEE·ONE wordmark (Silkscreen).

Rebuild: `node render.js brand.html OUTDIR` (brand.html embeds the colonies-mode replays of the stages).
