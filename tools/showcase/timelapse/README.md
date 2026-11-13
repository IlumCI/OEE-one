# Timelapse (site/media/oee-one-timelapse*.{gif,mp4})

1. Growth stages: `replay runs6/l3v7_s1/ck.archive.<t> 48 2 colonies > stages/<t>.bin` for every archive
   (250k ... 3.5M; world 20x20 -> 48x48).
2. A standalone page (same palette and caption style as the showcase) draws each stage centred and
   un-tiled, so the world visibly grows; frames are captured with `record_steps.js PAGE W H OUT 1`.
3. ffmpeg: origin and map scenes at 4x, agents and nations at 3x, growth at 20 fps, 0.6 s crossfades;
   GIF = 960x540, 10 fps, 32-colour palette.
