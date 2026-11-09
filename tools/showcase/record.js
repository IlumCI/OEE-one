const { chromium } = require('playwright');
const [,, W, H, FPS, SECS, OUT] = process.argv;
(async () => {
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: +W, height: +H }, deviceScaleFactor: 1 });
  await p.clock.install({ time: 0 });
  await p.goto('file://' + __dirname + '/wrap.html' + (process.env.HASH || ''));
  await p.evaluate(() => document.fonts.ready);
  await p.waitForTimeout(800);              // footage decompression is asynchronous
  await p.clock.runFor(1500);               // let trails settle
  const n = Math.round(FPS * SECS), step = 1000 / FPS;
  for (let i = 0; i < n; i++) {
    await p.clock.runFor(step);
    await p.screenshot({ path: `${OUT}/f${String(i).padStart(4, '0')}.png` });
  }
  await b.close();
})();
