# Timelapse (site/media/oee-one-timelapse*.{gif,mp4})

1. Growth stages: `replay runs6/l3v7_s1/ck.archive.<t> 48 2 colonies > stages/<t>.bin` for every archive
   (250k, then each growth step 500k ... 3.5M: one stage per world size, 20x20 -> 48x48).
2. A standalone page (same palette and caption style as the showcase) draws each stage centred and
   un-tiled, so the world visibly grows; frames are captured with `record_steps.js PAGE W H OUT 1`.
3. ffmpeg: every scene trimmed to before its loop point (origin 0-12.5 s, map 1.5-10.5 s, nations
   0-17.5 s; the recorded scene clips loop), all at 3x; growth at 16 fps; 0.6 s crossfades;
   GIF = 960x540, 7 fps, 32-colour palette (kept under 15 MB).
