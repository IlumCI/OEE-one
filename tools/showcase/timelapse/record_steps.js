const { chromium } = require('playwright');
const [,, PAGE, W, H, OUT, EVERY] = process.argv;
(async () => {
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: +W, height: +H }, deviceScaleFactor: 1 });
  await p.goto('file://' + PAGE);
  await p.evaluate(() => document.fonts.ready);
  await p.waitForTimeout(500);
  const n = await p.evaluate(() => window.FRAMES);
  for (let i = 0; i < n; i++) {
    await p.evaluate(() => window.step());
    if (i % (+EVERY || 1) === 0) await p.screenshot({ path: `${OUT}/f${String(i).padStart(4, '0')}.png` });
  }
  console.log('frames', n);
  await b.close();
})();
