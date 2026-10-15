/* selftest.mjs —— 本地/线上同一套断言
   用法：
     node selftest.mjs --dir=.                            # 起本地静态服务器再测
     node selftest.mjs --dir=. --edge="C:\...\msedge.exe"  # 指定浏览器（默认找 Edge）
     node selftest.mjs --url=https://...                  # 直接测线上
   覆盖：无 JS 渲染 / 入口门 / 对比度 / wasm 谱面一致性 / 底部导航高亮 /
        canvas 背板 / aria-pressed / skip-link / 元信息 / 控制台异常。 */
import fs from "node:fs";
import path from "node:path";
import http from "node:http";
import crypto from "node:crypto";
import { spawn } from "node:child_process";

const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const DIR = arg("dir", "");
const PORT_HTTP = Number(arg("port", "8899"));
const PORT_CDP = Number(arg("cdp", "9334"));
const EDGE = arg("edge", process.env.SELFTEST_EDGE || "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const MIME = { ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".css": "text/css", ".svg": "image/svg+xml", ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".png": "image/png", ".webp": "image/webp", ".wasm": "application/wasm", ".mp3": "audio/mpeg", ".woff2": "font/woff2", ".txt": "text/plain; charset=utf-8", ".xml": "application/xml" };

let server = null;
let URL_ = arg("url", "");
if (DIR) {
  /* 带 Range 支持：GitHub Pages 支持分段请求，本地服务器不支持的话
     <audio> 的 seek 会被判为不可用，测出来的行为和线上不一致 */
  server = http.createServer((req, res) => {
    const p = decodeURIComponent(req.url.split("?")[0]);
    let f = path.join(DIR, p === "/" ? "index.html" : p);
    if (!fs.existsSync(f) || fs.statSync(f).isDirectory()) { res.writeHead(404, { "content-type": "text/plain" }); res.end("404"); return; }
    const type = MIME[path.extname(f).toLowerCase()] || "application/octet-stream";
    const size = fs.statSync(f).size;
    const range = req.headers.range;
    if (range) {
      const m = /bytes=(\d*)-(\d*)/.exec(range);
      let start = m && m[1] ? parseInt(m[1], 10) : 0;
      let end = m && m[2] ? parseInt(m[2], 10) : size - 1;
      if (!isFinite(start) || start < 0) start = 0;
      if (!isFinite(end) || end >= size) end = size - 1;
      if (start > end) { res.writeHead(416, { "content-range": "bytes */" + size }); res.end(); return; }
      res.writeHead(206, { "content-type": type, "accept-ranges": "bytes", "content-range": `bytes ${start}-${end}/${size}`, "content-length": end - start + 1 });
      fs.createReadStream(f, { start, end }).pipe(res);
      return;
    }
    res.writeHead(200, { "content-type": type, "accept-ranges": "bytes", "content-length": size });
    fs.createReadStream(f).pipe(res);
  });
  await new Promise((r) => server.listen(PORT_HTTP, "127.0.0.1", r));
  URL_ = `http://127.0.0.1:${PORT_HTTP}/`;
}

const profile = path.join(process.env.TEMP || ".", "selftest-edge-" + PORT_CDP);
const edge = spawn(EDGE, ["--headless=new", `--remote-debugging-port=${PORT_CDP}`, `--user-data-dir=${profile}`,
  "--no-first-run", "--no-default-browser-check", "--disable-gpu", "--hide-scrollbars", "--mute-audio",
  "--autoplay-policy=no-user-gesture-required",
  "--window-size=1440,900", "about:blank"], { stdio: "ignore" });

async function target() {
  for (let i = 0; i < 40; i++) {
    try { const list = await (await fetch(`http://127.0.0.1:${PORT_CDP}/json/list`)).json(); const p = list.find((t) => t.type === "page"); if (p) return p; } catch {}
    await sleep(250);
  }
  throw new Error("edge devtools unreachable");
}
const page = await target();
const ws = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((res, rej) => { ws.onopen = res; ws.onerror = rej; });
let mid = 0; const pend = new Map(); const consoleMsgs = [];
ws.onmessage = (ev) => {
  const m = JSON.parse(ev.data);
  if (m.id && pend.has(m.id)) { const { res, rej } = pend.get(m.id); pend.delete(m.id); m.error ? rej(new Error(JSON.stringify(m.error))) : res(m.result); }
  else if (m.method === "Runtime.exceptionThrown") consoleMsgs.push("EXCEPTION: " + (m.params.exceptionDetails?.exception?.description || "").slice(0, 200));
  else if (m.method === "Runtime.consoleAPICalled" && m.params.type === "error") consoleMsgs.push("console.error: " + (m.params.args || []).map((a) => a.value ?? a.description ?? "").join(" ").slice(0, 200));
};
const send = (method, params = {}) => new Promise((res, rej) => {
  const id = ++mid; pend.set(id, { res, rej }); ws.send(JSON.stringify({ id, method, params }));
  setTimeout(() => { if (pend.has(id)) { pend.delete(id); rej(new Error("timeout " + method)); } }, 30000);
});
const evaluate = async (expr) => {
  const r = await send("Runtime.evaluate", { expression: expr, returnByValue: true, awaitPromise: true });
  if (r.exceptionDetails) throw new Error(r.exceptionDetails.exception?.description || "eval failed");
  return r.result.value;
};
const shot = () => send("Page.captureScreenshot", { format: "jpeg", quality: 60 });
const load = async (q = "") => { await send("Page.navigate", { url: URL_ + q }); await sleep(2500); };
/* 点「进入主页」必须用真事件：`element.click()` 不是可信手势，Chrome 的自动播放策略会
   把 play() 挡掉（实测 headless 下 currentTime 一直是 0、AudioContext 根本没建）。
   真用户点的就是真事件，自检也该这么点。 */
const enter = async () => {
  const b = await evaluate(`(() => { const e = document.getElementById('enterBtn'); if (!e) return null; const r = e.getBoundingClientRect(); return { x: Math.round(r.x + r.width / 2), y: Math.round(r.y + r.height / 2) }; })()`);
  if (!b) return false;
  await send("Input.dispatchMouseEvent", { type: "mouseMoved", x: b.x, y: b.y, button: "none" });
  await send("Input.dispatchMouseEvent", { type: "mousePressed", x: b.x, y: b.y, button: "left", clickCount: 1 });
  await send("Input.dispatchMouseEvent", { type: "mouseReleased", x: b.x, y: b.y, button: "left", clickCount: 1 });
  return true;
};

const results = [];
const check = (name, ok, detail = "") => { results.push({ name, ok: !!ok, detail }); console.log(`${ok ? "PASS" : "FAIL"}  ${name}${detail ? "  — " + detail : ""}`); };

await send("Page.enable"); await send("Runtime.enable"); await send("Log.enable"); await send("Network.enable");

/* ---------- 1. 无 JS ---------- */
await send("Emulation.setScriptExecutionDisabled", { value: true });
await load();
const nojs = await evaluate(`(() => {
  const cs = (el) => getComputedStyle(el);
  const scale = (t) => { const m = /matrix\\(([-\\d.]+)/.exec(t); return m ? Math.abs(parseFloat(m[1])) : (t === 'none' ? 1 : NaN); };
  const bars = [...document.querySelectorAll('.bar-fill')].map((el) => scale(cs(el).transform));
  const reveals = [...document.querySelectorAll('.reveal')].map((el) => parseFloat(cs(el).opacity));
  const text = document.body.innerText.trim();
  const page = document.getElementById('page'), landing = document.getElementById('landing');
  const ns = document.querySelector('noscript');
  return {
    pageOpacity: cs(page).opacity, landingDisplay: cs(landing).display, bodyOverflow: cs(document.body).overflow,
    bars, revealHidden: reveals.filter((o) => o < 0.99).length, revealCount: reveals.length,
    stats: [...document.querySelectorAll('.stat-num')].map((el) => el.textContent.trim()),
    clockDisplay: cs(document.querySelector('.hero-clock')).display,
    textLen: text.length, hasAbout: text.includes('关于我'), hasContact: text.includes('联系我'),
    noscriptText: ns ? ns.innerText.trim() : '', skillMax: text.includes('C++')
  };
})()`);
check("nojs: #page 可见(opacity=1)", nojs.pageOpacity === "1", "opacity=" + nojs.pageOpacity);
check("nojs: 入场门 display:none", nojs.landingDisplay === "none", "display=" + nojs.landingDisplay);
check("nojs: 页面可滚动", nojs.bodyOverflow !== "hidden", "overflow=" + nojs.bodyOverflow);
check("nojs: 统计数字为终值", nojs.stats.join(",") === "5,3,9,8", nojs.stats.join(","));
check("nojs: 技能条有长度", nojs.bars.length === 5 && nojs.bars.every((v) => v > 0.2 && v <= 1), JSON.stringify(nojs.bars));
check("nojs: .reveal 全部可见", nojs.revealHidden === 0, `${nojs.revealCount} 个，隐藏 ${nojs.revealHidden}`);
check("nojs: 正文长度 > 2500", nojs.textLen > 2500, "len=" + nojs.textLen);
check("nojs: 首尾章节都在", nojs.hasAbout && nojs.hasContact);
check("nojs: noscript 提示语", /JavaScript/.test(nojs.noscriptText), nojs.noscriptText.slice(0, 40));
check("nojs: 实时时钟(JS 专属)隐藏", nojs.clockDisplay === "none", "display=" + nojs.clockDisplay);

/* ---------- 2. 有 JS ---------- */
await send("Emulation.setScriptExecutionDisabled", { value: false });
await load();
const gate = await evaluate(`(() => { const l = document.getElementById('landing'); return { display: getComputedStyle(l).display, btn: !!document.getElementById('enterBtn') }; })()`);
check("js: 入口门存在", gate.display !== "none" && gate.btn, JSON.stringify(gate));
await enter();
await sleep(2000);
const st = await evaluate(`(() => {
  const cs = (el) => getComputedStyle(el);
  const hex = (c) => { const m = /rgba?\\(([^)]+)\\)/.exec(c); if (!m) return null; const p = m[1].split(',').map((x) => parseFloat(x)); return { r: p[0], g: p[1], b: p[2], a: p.length > 3 ? p[3] : 1 }; };
  const lum = (c) => { const f = (v) => { v /= 255; return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4); }; return 0.2126 * f(c.r) + 0.7152 * f(c.g) + 0.0722 * f(c.b); };
  const bgOf = (el) => { let n = el; while (n && n !== document.documentElement) { const c = hex(cs(n).backgroundColor); if (c && c.a > 0.5) return c; n = n.parentElement; } return { r: 246, g: 247, b: 252, a: 1 }; };
  const over = (c, b) => ({ r: c.r * c.a + b.r * (1 - c.a), g: c.g * c.a + b.g * (1 - c.a), b: c.b * c.a + b.b * (1 - c.a), a: 1 });
  const ratio = (el) => { const f0 = hex(cs(el).color), b = bgOf(el); const f = over(f0, b); const l1 = lum(f), l2 = lum(b); const hi = Math.max(l1, l2), lo = Math.min(l1, l2); return Math.round(((hi + 0.05) / (lo + 0.05)) * 100) / 100; };
  const sels = ['.stat-k', '.fact-k', '.anime-tag', '.card-tech', '.skill-level', '.tools-title', '.contact-k', '.tl-org'];
  const ratios = {};
  sels.forEach((s) => { const el = document.querySelector(s); if (el) ratios[s] = ratio(el); });
  const footer = document.querySelector('.footer span');
  if (footer) ratios['.footer span'] = ratio(footer);
  const cvs = document.getElementById('rhythmCanvas');
  const rect = cvs.getBoundingClientRect();
  return {
    pageOpacity: cs(document.getElementById('page')).opacity,
    pageVisibleClass: document.getElementById('page').classList.contains('visible'),
    stillEntry: document.documentElement.classList.contains('entry'),
    ratios, dpr: window.devicePixelRatio,
    canvasCss: [Math.round(rect.width), Math.round(rect.height)], canvasBacking: [cvs.width, cvs.height],
    parity: window.__rhythmParity, laneSource: window.__rhythmLaneSource, kernelVersion: window.__rhythmKernelVersion,
    chartLen: (window.__RHYTHM_CHART || '').length, chartChars: /^[0-6]+$/.test(window.__RHYTHM_CHART || ''),
    badge: document.getElementById('kernelBadge') ? document.getElementById('kernelBadge').textContent.trim() : '',
    musicPressed: document.getElementById('musicBtn') ? document.getElementById('musicBtn').getAttribute('aria-pressed') : null,
    skipHref: document.querySelector('.skip') ? document.querySelector('.skip').getAttribute('href') : null,
    skipTarget: (() => { const s = document.querySelector('.skip'); return s ? !!document.querySelector(s.getAttribute('href')) : false; })(),
    canonical: !!document.querySelector('link[rel=canonical]'),
    canonicalHref: (document.querySelector('link[rel=canonical]') || {}).href || '',
    ogImage: (document.querySelector('meta[property="og:image"]') || {}).content || '',
    ogImageW: (document.querySelector('meta[property="og:image:width"]') || {}).content || '',
    ogImageH: (document.querySelector('meta[property="og:image:height"]') || {}).content || '',
    twitter: (document.querySelector('meta[name="twitter:card"]') || {}).content || '',
    enBlock: (() => { const el = document.querySelector('.en-abstract'); return el ? { lang: el.getAttribute('lang'), len: el.textContent.trim().length } : null; })(),
    preload: !!document.querySelector('link[rel=preload][as=image]'),
    leftovers: (document.documentElement.outerHTML.match(/__[A-Z_]+__/g) || []).slice(0, 5),
    title: document.title, lang: document.documentElement.lang
  };
})()`);
check("js: 进站后 #page 可见", st.pageOpacity === "1" && st.pageVisibleClass, "opacity=" + st.pageOpacity);
check("js: 解除滚动锁", st.stillEntry === false);
check("js: wasm 谱面一致(parity=ok)", st.parity === "ok", String(st.parity));
check("js: 轨道来自 wasm", st.laneSource === "wasm", String(st.laneSource));
check("js: 内核版本号=3（含判定与起音检测导出）", st.kernelVersion === 3, String(st.kernelVersion));
check("js: 内联谱面 256 个合法轨道", st.chartLen === 256 && st.chartChars, "len=" + st.chartLen);
check("js: 内核徽章文案", /WASM/.test(st.badge), st.badge);
check("js: canvas 背板=Css×dpr", st.canvasBacking[0] === Math.round(st.canvasCss[0] * st.dpr) && st.canvasBacking[1] === Math.round(st.canvasCss[1] * st.dpr), `css=${st.canvasCss} backing=${st.canvasBacking} dpr=${st.dpr}`);
check("js: 音乐按钮有 aria-pressed", st.musicPressed === "false" || st.musicPressed === "true", String(st.musicPressed));
check("js: skip-link 指向存在的锚点", st.skipHref === "#about" && st.skipTarget, String(st.skipHref));
check("js: canonical", st.canonical && st.canonicalHref.startsWith("https://"), st.canonicalHref);
check("js: og:image 指向 1200×630 分享卡片", /og-image\.jpg$/.test(st.ogImage) && st.ogImageW === "1200" && st.ogImageH === "630", st.ogImage + ` ${st.ogImageW}×${st.ogImageH}`);
check("js: twitter:card", st.twitter === "summary_large_image", st.twitter);
check("js: 英文摘要 lang=en 且有内容", st.enBlock && st.enBlock.lang === "en" && st.enBlock.len > 200, JSON.stringify(st.enBlock));
check("js: 预加载封面", st.preload);
check("js: 无占位符残留", st.leftovers.length === 0, st.leftovers.join(","));
const worst = Object.entries(st.ratios).sort((a, b) => a[1] - b[1]);
check("js: 小字对比度 >= 4.5:1", worst.every(([, v]) => v >= 4.5), worst.map(([k, v]) => `${k}:${v}`).join(" "));
console.log("     对比度明细：" + JSON.stringify(st.ratios));

/* 滚动到底部：导航高亮 + 回顶（注意 smooth 滚动要等它停下） */
await evaluate(`window.scrollTo(0, document.documentElement.scrollHeight); true`);
let last = -1, now = -2;
for (let i = 0; i < 20 && last !== now; i++) { last = now; await shot(); await sleep(400); now = await evaluate(`Math.round(window.scrollY)`); }
await shot(); await sleep(400); await shot();
const nav = await evaluate(`(() => {
  const links = [...document.querySelectorAll('.nav a[href^="#"]')];
  const active = links.filter((a) => /active|current|on\\b/.test(a.className)).map((a) => a.getAttribute('href'));
  return { scrollY: Math.round(window.scrollY), max: Math.round(document.documentElement.scrollHeight - window.innerHeight), active, contactTop: Math.round(document.getElementById('contact').getBoundingClientRect().top) };
})()`);
check("js: 滚到底高亮最后一节", nav.active.includes("#contact"), `scrollY=${nav.scrollY}/${nav.max} active=${JSON.stringify(nav.active)} contactTop=${nav.contactTop}`);

/* 音游在跑：等一会儿看 COMBO 是否递增。
   注意要滚到 canvas 本身，不能只滚到 #rhythm 段落顶部 —— 段落里现在有横幅与说明，
   滚到顶部时 canvas 还在视口外，按设计会自动暂停，谱面就不再推进了。 */
await evaluate(`document.getElementById('rhythmCanvas').scrollIntoView({block:'center'}); true`);
let combo = 0;
for (let i = 0; i < 12; i++) { await shot(); await sleep(700); combo = await evaluate(`parseInt((document.getElementById('comboNum')||{textContent:'0'}).textContent,10)||0`); if (combo > 3) break; }
check("js: 音游 COMBO 递增", combo > 3, "combo=" + combo);

/* ---------- 3. 可玩模式：判定与算分由 wasm 内核给 ---------- */
const jp = await evaluate(`window.__rhythmJudgeParity`);
check("play: 判定/算分内核自检(judgeParity=ok)", jp === "ok", String(jp));

const probe = await evaluate(`(() => {
  const d = window.__rhythmJudgeDelta, f = window.__rhythmJudgeFallback, fa = window.__rhythmAccuracyFallback;
  const ks = [-200, -91, -90, -89, -46, -45, -44, 0, 44, 45, 46, 89, 90, 91, 200];
  const acc = [[0,0,0],[64,0,64],[0,64,64],[32,32,64],[1,0,3],[8,0,9]];
  return {
    kernel: ks.map(d).join(""),
    fallback: ks.map(f).join(""),
    accFallback: acc.map((a) => fa(a[0], a[1], a[2])).join(","),
    accExpect: (window.__RHYTHM_ACC_EXPECT || []).join(",")
  };
})()`);
check("play: 判定窗口边界 ±45/±90", probe.kernel === "001112222211100", probe.kernel);
check("play: JS 回退镜像与内核/生成器同答案",
  probe.fallback === probe.kernel && probe.accFallback === probe.accExpect,
  `${probe.kernel} | acc ${probe.accFallback} vs ${probe.accExpect}`);

/* 点开始挑战 → 进 play，统计行与「回到自动演奏」出现 */
await evaluate(`document.getElementById('rhythmPlay').click(); true`);
await sleep(350);
const playUI = await evaluate(`(() => ({
  mode: (window.__rhythmStats || {}).mode,
  statsHidden: document.getElementById('rhythmStats').hidden,
  stopHidden: document.getElementById('rhythmStop').hidden,
  tag: document.getElementById('rhythmTag').textContent,
  pressed: document.getElementById('rhythmPlay').getAttribute('aria-pressed')
}))()`);
check("play: 进挑战模式且控制条就位", playUI.mode === "play" && playUI.statsHidden === false && playUI.stopHidden === false && playUI.pressed === "true", JSON.stringify(playUI));

/* 等到最近音符差 <= 45ms，按对应键位，断言拿到非 MISS 判定 */
let hitRes = null;
for (let i = 0; i < 60; i++) {
  const lane = await evaluate(`window.__rhythmLaneOfNext()`);
  const d = lane === null ? null : await evaluate(`window.__rhythmNextDelta(${lane})`);
  if (lane !== null && d !== null && Math.abs(d) <= 45) {
    await evaluate(`document.dispatchEvent(new KeyboardEvent('keydown', { key: 'dfghjkl'[${lane}], bubbles: true })); true`);
    hitRes = await evaluate(`JSON.parse(JSON.stringify(window.__rhythmStats))`);
    break;
  }
  await shot(); await sleep(25);
}
check("play: 按键命中计分且来源=wasm",
  hitRes && (hitRes.perfect + hitRes.good) >= 1 && hitRes.judgeSource === "wasm",
  hitRes ? `P=${hitRes.perfect} G=${hitRes.good} M=${hitRes.miss} src=${hitRes.judgeSource}` : "没抓到判定窗口");

/* 放任音符漏过 → MISS 递增 */
const missBefore = hitRes ? hitRes.miss : 0;
let missAfter = missBefore;
for (let i = 0; i < 60; i++) {
  await shot(); await sleep(60);
  missAfter = await evaluate(`(window.__rhythmStats || {}).miss || 0`);
  if (missAfter > missBefore) break;
}
check("play: 漏掉音符判 MISS", missAfter > missBefore, `miss ${missBefore} → ${missAfter}`);

/* 统计行里的准确率必须就是内核那个千分比 */
const accDom = await evaluate(`(() => {
  const s = window.__rhythmStats || {};
  return { dom: (document.getElementById('rAcc') || {}).textContent, kern: window.__rhythmAccuracyPermille(), stats: s.accPermille };
})()`);
check("play: DOM 准确率=内核千分比", accDom.dom === (accDom.kern / 10).toFixed(1) + "%" && accDom.kern === accDom.stats,
  `dom=${accDom.dom} kern=${accDom.kern} stats=${accDom.stats}`);

/* 结算 + 最好成绩落盘 */
await evaluate(`window.__rhythmEndRound(); true`);
await sleep(250);
const done = await evaluate(`(() => ({
  hidden: document.getElementById('rhythmResult').hidden,
  text: document.getElementById('rhythmResult').textContent.trim().slice(0, 120),
  best: localStorage.getItem('myintro.rhythm.best.v1'),
  bestText: document.getElementById('rBest').textContent
}))()`);
check("play: 结算面板 + 最好成绩写入", done.hidden === false && !!done.best && /WASM/.test(done.text),
  done.text.slice(0, 70));

/* 回到自动演奏 */
const back = await evaluate(`(() => { document.getElementById('rhythmStop').click(); return { stats: JSON.parse(JSON.stringify(window.__rhythmStats)), tag: document.getElementById('rhythmTag').textContent }; })()`);
check("play: 回到自动演奏", back.stats.mode === "auto" && back.tag === "AUTO PLAY", JSON.stringify(back.tag + " mode=" + back.stats.mode));

/* 帧率（强制出帧） */
const fps = await evaluate(`new Promise((res) => { let n = 0; const t0 = performance.now(); const tick = () => { n++; if (performance.now() - t0 < 1200) requestAnimationFrame(tick); else res(Math.round(n / ((performance.now() - t0) / 1000))); }; requestAnimationFrame(tick); })`);
check("js: rAF 帧率 >= 40", fps >= 40, fps + " fps");

/* ---------- 音频通路（音乐断断续续的修法）----------
   本地静态服务器上要求严格：图必须是 44100、一次 waiting/stalled 都不许有。
   线上受 CDN 影响会有正常的缓冲抖动，此时有意义的是「用户听感不断」：
   要么零等待，要么已经自动降级到原生通路，且 currentTime 仍在跟着真实时间走。 */
const LOCAL_RUN = !!DIR;
const audioSnap = () => evaluate(`(() => { const m = document.querySelector('#bgMusic'); return {
  mode: window.__audioMode ? window.__audioMode() : 'n/a',
  ctx: window.__audioCtxInfo ? window.__audioCtxInfo() : 'n/a',
  watch: window.__audioWatch ? { w: window.__audioWatch.waiting, s: window.__audioWatch.stalled, f: window.__audioWatch.frozen } : null,
  t: m ? m.currentTime : -1, paused: m ? m.paused : null
}; })()`);
const audioNow = await audioSnap();
check("audio: 走 Web Audio 图且采样率=文件(44100)",
  (audioNow.mode === "graph" && audioNow.ctx === "44100/running") || (!LOCAL_RUN && audioNow.mode.indexOf("plain") === 0),
  `mode=${audioNow.mode} ctx=${audioNow.ctx}${LOCAL_RUN ? " (本地：必须是图)" : " (线上：已降级也算过)"}`);
await sleep(1600);
const audioThen = await audioSnap();
const advanced = audioThen.t - audioNow.t;
const clean = audioThen.watch && audioThen.watch.w === 0 && audioThen.watch.s === 0 && audioThen.watch.f === 0;
check("audio: 播放不中断（零等待，或抖动后已自动降级且进度仍在推进）",
  audioThen.paused === false && advanced >= 1.3 && (LOCAL_RUN ? clean : (clean || audioThen.mode.indexOf("plain") === 0)),
  `advanced=${advanced.toFixed(2)}s/1.6s watch=${JSON.stringify(audioThen.watch)} mode=${audioThen.mode}`);

/* 播放中强制降级：进度要接上，不能回到开头 */
const beforeT = await evaluate(`document.querySelector('#bgMusic').currentTime`);
await evaluate(`window.__audioForceFallback(); true`);
await sleep(2600);
const fb = await evaluate(`(() => { const m = document.querySelector('#bgMusic'); return {
  mode: window.__audioMode(), ctx: window.__audioCtxInfo(), t: m.currentTime, paused: m.paused,
  n: document.querySelectorAll('audio').length, id: m.id,
  landed: window.__audioSeekLanded || null, seekable: window.__audioSeekable
}; })()`);
check("audio: 抖动时降级到原生通路并退出图", fb.mode === "plain-fallback" && fb.ctx === "none", `mode=${fb.mode} ctx=${fb.ctx}`);
check("audio: 降级把进度接上（seek 到位）", fb.seekable === true && fb.landed !== null && Math.abs(fb.landed - beforeT) < 1.5, `landed=${fb.landed} before=${beforeT.toFixed(2)}`);
check("audio: 降级后仍在放、页面只剩一个 audio", fb.paused === false && fb.t >= beforeT - 0.4 && fb.n === 1 && fb.id === "bgMusic", `t ${beforeT.toFixed(2)} → ${fb.t.toFixed(2)} n=${fb.n}`);

/* ?audio=plain：一点都不碰 Web Audio */
await load("?audio=plain");
await enter();
await sleep(2200);
const plain = await evaluate(`(() => { const m = document.querySelector('#bgMusic'); return {
  mode: window.__audioMode(), ctx: window.__audioCtxInfo(), paused: m.paused, t: m.currentTime
}; })()`);
check("audio: ?audio=plain 不建图也照样放", plain.mode === "plain-forced" && plain.ctx === "none" && plain.paused === false && plain.t > 0.3,
  `mode=${plain.mode} ctx=${plain.ctx} t=${plain.t.toFixed(2)}`);

/* ---------- 点「进入主页」→ 真的出声，花了多久 ----------
   用户报过「进去之后要等几秒才播放」：原因是 preload="none" + 图建在 play() 前面。
   现在入口门摆着的时候就预热首包，并且先 play() 再建图。 */
await load();
/* 等主脚本真的跑起来（它会挂出 __audioMode），否则按钮上还没有事件，点了不算数 */
for (let i = 0; i < 40; i++) {
  if (await evaluate(`typeof window.__audioMode === 'function' && !!document.getElementById('enterBtn')`)) break;
  await sleep(250);
}
/* 预热是异步的：线上 160KB 有时要 1–3 秒才回来，轮询到状态出现为止 */
let warm = { warm: null, preload: null };
for (let i = 0; i < 40; i++) {
  warm = await evaluate(`({ warm: window.__musicWarm || null, preload: document.querySelector('#bgMusic').getAttribute('preload') })`);
  if (warm.warm) break;
  await sleep(250);
}
check("audio: 入口门期间预热音频首包（省流量模式除外）", warm.warm === 206 || warm.warm === "skipped",
  `warm=${warm.warm} preload=${warm.preload}`);
const soundPromise = evaluate(`window.__soundMs = new Promise((res) => {
  const m = document.querySelector('#bgMusic'); const t0 = performance.now();
  const iv = setInterval(function () {
    if (m.currentTime > 0.15) { clearInterval(iv); res(Math.round(performance.now() - t0)); }
  }, 40);
  setTimeout(function () { clearInterval(iv); res(-1); }, 6000);
})`).catch(() => {});
await enter();
const soundMs = await evaluate(`window.__soundMs`);
check("audio: 点进入主页到出声 <= 1.2s", soundMs >= 0 && soundMs <= 1200, `${soundMs} ms`);

/* 谱面跟着这首歌走：时间轴 = 生成器内联的实测起音点；判定时钟 = audio.currentTime */
await evaluate(`document.getElementById('rhythmCanvas').scrollIntoView({block:'center'}); true`);
for (let i = 0; i < 40; i++) {
  if (await evaluate(`window.__rhythmClockProbe().timeline > 0`)) break;
  await sleep(150);
}
const clk = await evaluate(`(() => { const p = window.__rhythmClockProbe(); return { src: p.src, song: p.song, audio: p.audio, onsets: p.onsets, songMs: p.songMs, timeline: p.timeline, nextIsOnset: p.nextIsOnset }; })()`);
check("sync: 谱面时间轴来自内联的实测起音点（657 个 / 曲长 256.6s）",
  clk.onsets === 657 && clk.songMs === 256600 && clk.timeline > 0,
  `onsets=${clk.onsets} songMs=${clk.songMs} timeline=${clk.timeline}`);
check("sync: 音轨在播时判定时钟就是 audio.currentTime（同一时刻同一读数）",
  clk.src === "audio" && Math.abs(clk.song - clk.audio) < 5,
  `src=${clk.src} song=${clk.song.toFixed(1)} audio=${clk.audio.toFixed(1)}`);
check("sync: 下一个音符的落点确实等于某个实测起音点", clk.nextIsOnset === true, `nextIsOnset=${clk.nextIsOnset}`);
await evaluate(`document.querySelector('#bgMusic').pause(); true`);
await sleep(150);
const pausedClk = await evaluate(`(() => { const p = window.__rhythmClockProbe(); return { src: p.src, song: p.song, audio: p.audio }; })()`);
check("sync: 暂停后退回虚拟时钟，且接着暂停那一刻的音频位置",
  pausedClk.src === "virtual" && Math.abs(pausedClk.song - pausedClk.audio) < 1500,
  `src=${pausedClk.src} song=${Math.round(pausedClk.song)} audio=${Math.round(pausedClk.audio)}`);

/* 刷新后最好成绩仍在（localStorage 持久化） */
await load();
await enter();
await sleep(2200);
const persisted = await evaluate(`(() => ({
  best: document.getElementById('rBest').textContent,
  stored: !!localStorage.getItem('myintro.rhythm.best.v1'),
  mode: (window.__rhythmStats || {}).mode
}))()`);
check("play: 刷新后最好成绩仍在且默认自动演奏",
  persisted.stored && /%/.test(persisted.best) && persisted.mode === "auto",
  `best=${persisted.best} mode=${persisted.mode}`);

/* ---------- 谱面 seed：第三份实现（Node 自己算一遍）---------- */
/* 内核 = C++/wasm，页面的 JS 镜像 = 浏览器，这里是 Node —— 三份独立实现比同一串数字 */
const nodeChart = (seed, lanes, n) => {
  let s = (seed >>> 0) || 0x9e3779b9, last = -1, out = [];
  const nxt = () => { s = (s ^ (s << 13)) >>> 0; s = (s ^ (s >>> 17)) >>> 0; s = (s ^ (s << 5)) >>> 0; return s; };
  for (let i = 0; i < n; i++) {
    let l = nxt() % lanes;
    if (l === last) l = (l + 1 + (nxt() % (lanes - 1))) % lanes;
    last = l; out.push(l);
  }
  return out;
};
const fnv = (str) => { let h = 2166136261; for (let i = 0; i < str.length; i++) { h ^= str.charCodeAt(i); h = Math.imul(h, 16777619) >>> 0; } return h >>> 0; };
const KSEED = 0x5eed2026;   /* rhythm_core.hpp 里的 kSeed */

await load("?enter=1");
await enter();
await sleep(2200);
const seedDefault = await evaluate(`(() => { window.__rhythmApplySeed(''); return window.__rhythmSeedState(); })()`);
check("seed: 默认 seed 的 16 音符与 Node 独立实现一致",
  seedDefault.parity === "ok" && seedDefault.len === 256 && seedDefault.head === nodeChart(KSEED, 7, 16).join(""),
  `parity=${seedDefault.parity} head=${seedDefault.head} node=${nodeChart(KSEED, 7, 16).join("")}`);

const seedCustom = await evaluate(`(() => { window.__rhythmApplySeed('zhehong'); const s = window.__rhythmSeedState();
  return Object.assign(s, { info: (document.getElementById('seedInfo') || {}).textContent || "" }); })()`);
check("seed: 任意文字 seed（FNV 折成 32 位）内核与镜像仍逐位一致",
  seedCustom.parity === "ok" && seedCustom.source === "wasm" && seedCustom.seed === fnv("zhehong") &&
  seedCustom.head === nodeChart(fnv("zhehong"), 7, 16).join(""),
  `seed=0x${seedCustom.seed.toString(16)} head=${seedCustom.head}`);
check("seed: 换 seed 确实换了谱面（对 Node 也是新的）",
  seedCustom.head !== nodeChart(KSEED, 7, 16).join("") && seedCustom.head !== nodeChart(fnv("other"), 7, 16).join(""),
  seedCustom.head);
check("seed: 页面上写出核对结论", /一致/.test(seedCustom.info), seedCustom.info.slice(0, 72));

await load("?enter=1&seed=zhehong");
await enter();
await sleep(2200);
const seedQuery = await evaluate(`(() => { const s = window.__rhythmSeedState(); return Object.assign(s, { input: document.getElementById('seedInput').value }); })()`);
check("seed: ?seed=zhehong 直接进来就生效（输入框也回填）",
  seedQuery.seed === fnv("zhehong") && seedQuery.parity === "ok" && seedQuery.input === "zhehong",
  `seed=0x${seedQuery.seed.toString(16)} input=${seedQuery.input} parity=${seedQuery.parity}`);

/* ---------- kernel.html：内核控制台 ---------- */
await load("kernel.html");
await sleep(2600);
const kc = await evaluate(`(() => { const c = window.__kernelConsole || {}; return {
  loaded: c.loaded, judgeBad: c.judgeBad, accBad: c.accBad, probe: c.probeParity, parity: c.seedParity,
  head: c.chartHead, ver: c.kernelVersion, bytes: c.wasmBytes, sha: (c.sha256 || "").length,
  rows: document.querySelectorAll('#judgeTable tbody tr').length,
  arows: document.querySelectorAll('#accTable tbody tr').length,
  badRows: document.querySelectorAll('.tbl tr[data-bad="1"]').length,
  okCells: document.querySelectorAll('.tbl td.ok').length
}; })()`);
check("kernel 页: 内核加载、三张表跑完（19 判定 + 12 算分 + 探针全对）",
  kc.loaded === true && kc.judgeBad === 0 && kc.accBad === 0 && kc.probe === "ok" &&
  kc.rows === 19 && kc.arows === 12 && kc.badRows === 0 && kc.okCells === 31,
  JSON.stringify(kc));
/* wasm 字节数与 SHA-256 不能写死：本地 emsdk 与 CI 的 emcc 版本不同，产物会差几个字节
   （实测本地 732 B / CI 724 B）。改成拿页面里真正内联的那段 base64 现算：
   页面自己的 sha256 必须等于这段字节的 sha256，任何 emcc 版本都成立。 */
const kcWasm = await evaluate(`(() => ({ b64: window.__RHYTHM_WASM || "", sha: (window.__kernelConsole || {}).sha256 || "" }))()`);
const kcBytes = Buffer.from(kcWasm.b64, "base64");
const kcSha = crypto.createHash("sha256").update(kcBytes).digest("hex");
check("kernel 页: 默认 seed 谱面与 Node 一致 + kernel_version/SHA-256",
  kc.parity === "ok" && kc.head === nodeChart(KSEED, 7, 16).join("") && kc.ver === 3 && kc.sha === 64 &&
  kc.bytes === kcBytes.length && kcWasm.sha === kcSha,
  `head=${kc.head} ver=${kc.ver} wasm=${kc.bytes}B(内联 ${kcBytes.length}B) sha与内联一致=${kcWasm.sha === kcSha}`);

const kc2 = await evaluate(`(() => { document.getElementById('seedInput').value = 'myintro';
  document.getElementById('seedApply').click(); const c = window.__kernelConsole;
  return { parity: c.seedParity, head: c.chartHead, seed: c.seed, q: location.search,
    label: document.getElementById('seedLabel').textContent }; })()`);
check("kernel 页: 换 seed 后内核/镜像仍一致且地址栏同步",
  kc2.parity === "ok" && kc2.seed === fnv("myintro") && kc2.head === nodeChart(fnv("myintro"), 7, 16).join("") &&
  /seed=myintro/.test(kc2.q) && /0x/.test(kc2.label),
  `q=${kc2.q} head=${kc2.head}`);

/* ---------- nojs.html：无 JS 对照页 ---------- */
await load("nojs.html");
await sleep(3400);
const np = await evaluate(`(() => { const f = document.querySelectorAll('iframe'); const p = window.__nojsProbe || null;
  return { frames: f.length, sandbox: f[0] ? f[0].getAttribute('sandbox') : 'missing',
    src0: f[0] ? f[0].getAttribute('src') : '', src1: f[1] ? f[1].getAttribute('src') : '',
    probe: p, text: (document.getElementById('probe') || {}).textContent || "" }; })()`);
check("nojs 页: 两个 iframe，左边 sandbox 不含 allow-scripts",
  np.frames === 2 && np.sandbox !== null && !/allow-scripts/.test(np.sandbox) &&
  np.src0 === "index.html" && /enter=1/.test(np.src1),
  `sandbox="${np.sandbox}" src0=${np.src0} src1=${np.src1}`);
check("nojs 页: 左框跨源隔离（父页读不到）+ 右框已跑脚本进入主页",
  !!np.probe && np.probe.sandboxOpaque === true && np.probe.sandboxHasAllowScripts === false &&
  np.probe.jsFrameEntered === true && np.probe.jsFrameText > 1500,
  JSON.stringify(np.probe));

/* ---------- 主题：★ 开关 / 记忆 / 首帧不闪 ---------- */
await load("?enter=1");
const th1 = await evaluate(`new Promise(function (res) {
  var b = document.getElementById('themeBtn');
  var before = { cls: document.documentElement.className, stored: localStorage.getItem('myintro.theme') };
  window.__applyTheme('light', false);   /* 先把状态归零，保证下面点一下必定是"切到暗色" */
  b.click();
  setTimeout(function () {
    res({ before: before, cls: document.documentElement.className, theme: window.__theme,
      meta: (document.querySelector('meta[name="theme-color"]') || {}).content,
      bg: getComputedStyle(document.body).backgroundColor,
      stored: localStorage.getItem('myintro.theme'), pressed: b.getAttribute('aria-pressed') });
  }, 80);
})`);
check("theme: 初始主题与 localStorage 一致（没有记忆时是亮色）",
  /theme-mygo/.test(th1.before.cls) === (th1.before.stored === "mygo"),
  `${th1.before.cls} stored=${th1.before.stored}`);
check("theme: 点★切到 MyGO 舞台（类名 + meta + 底色一起换）",
  /theme-mygo/.test(th1.cls) && th1.theme === "mygo" && th1.meta === "#07080f" && th1.bg === "rgb(7, 8, 15)" && th1.pressed === "true",
  `${th1.cls} ${th1.theme} ${th1.meta} ${th1.bg} pressed=${th1.pressed}`);
check("theme: 选择写进 localStorage", th1.stored === "mygo", String(th1.stored));

/* 刷新后：head 里的引导脚本必须在解析阶段就把类名挂上，否则会先闪一下亮色 */
await send("Page.addScriptToEvaluateOnNewDocument", { source: `window.__themeTrace = [];
new MutationObserver(function (ms) {
  for (var i = 0; i < ms.length; i++) {
    var t = ms[i].target;
    window.__themeTrace.push([document.readyState, (t && t.tagName) || '?', (t && t.className) || '']);
  }
}).observe(document, { attributes: true, attributeFilter: ['class'], subtree: true });` });
await load("?enter=1");
const th2 = await evaluate(`({ cls: document.documentElement.className, theme: window.__theme, trace: window.__themeTrace, bg: getComputedStyle(document.body).backgroundColor })`);
check("theme: 刷新后记住选择（不带 ?theme= 也是暗色）",
  /theme-mygo/.test(th2.cls) && th2.theme === "mygo" && th2.bg === "rgb(7, 8, 15)", `${th2.cls} ${th2.theme} ${th2.bg}`);
const firstMygo = (th2.trace || []).find((r) => r[1] === "HTML" && /theme-mygo/.test(r[2]));
check("theme: 类名在 head 解析阶段就挂好（首帧不闪）",
  !!firstMygo && firstMygo[0] === "loading", JSON.stringify(firstMygo || (th2.trace || []).slice(0, 3)));
/* 收尾：把记忆清掉，免得下次跑这条断言时起点不是亮色 */
await evaluate(`localStorage.removeItem('myintro.theme'); window.__applyTheme('light', false); true`);

/* ---------- 生成物不该留下任何 __PLACEHOLDER__ ---------- */
if (DIR) {
  const files = ["index.html", "kernel.html", "nojs.html", "404.html"].filter((f) => fs.existsSync(path.join(DIR, f)));
  const leftover = files.map((f) => [f, [...new Set(fs.readFileSync(path.join(DIR, f), "utf8").match(/__[A-Z_]+__/g) || [])]])
    .filter(([, m]) => m.length);
  check("生成物: 四个页面都没有占位符残留", leftover.length === 0 && files.length === 4,
    leftover.length ? leftover.map(([f, m]) => f + ":" + m.join(",")).join(" ") : files.length + " 个文件干净");
}

check("js: 控制台无异常", consoleMsgs.length === 0, consoleMsgs.slice(0, 3).join(" | "));

const fails = results.filter((r) => !r.ok);
console.log(`\n=== ${results.length - fails.length}/${results.length} PASS ===`);
if (fails.length) console.log("FAILED: " + fails.map((f) => f.name).join("; "));
try { ws.close(); } catch {}
edge.kill();
if (server) server.close();
process.exit(fails.length ? 1 : 0);
