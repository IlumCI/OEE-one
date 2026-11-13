# Brand images (site/brand/)

- `oee-one-avatar.png` (1024x1024): the most common evolved body in `l3v7_s1` at 3.5M steps (sprite 28253,
  21 of 1417 agents), drawn as a 5x5 pixel glyph with its red arm, on the showcase lattice. No text.
- `oee-one-avatar-world.png` (1024x1024): the 48x48 growing world at 3.5M steps, with the borders of
  every earlier world size (20 ... 44) drawn as growth rings.
- `oee-one-banner.png` (1500x500): world stages at 0.25M, 1M, 2M, 3M, 3.5M (20x20 -> 48x48), same cell
  size, growing left to right; OEE·ONE wordmark (Silkscreen).

Rebuild: `node render.js brand.html OUTDIR` (brand.html embeds the colonies-mode replays of the stages).
