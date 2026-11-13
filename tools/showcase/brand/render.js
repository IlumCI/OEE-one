const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch();
  for (const [mode, w, h] of [['glyph',1024,1024],['world',1024,1024],['banner',1500,500]]) {
    const p = await b.newPage({ viewport: { width: w, height: h }, deviceScaleFactor: 1 });
    await p.goto('file://' + process.argv[2] + '#' + mode);
    await p.waitForFunction(() => document.title === 'done', null, { timeout: 30000 });
    await p.screenshot({ path: `${process.argv[3]}/${mode}.png` });
    await p.close();
  }
  await b.close();
})();
