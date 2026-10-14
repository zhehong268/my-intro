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
import { spawn } from "node:child_process";

const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const DIR = arg("dir", "");
const PORT_HTTP = Number(arg("port", "8899"));
const PORT_CDP = Number(arg("cdp", "9334"));
const EDGE = arg("edge", process.env.SELFTEST_EDGE || "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const MIME = { ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".css": "text/css", ".svg": "image/svg+xml", ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".png": "image/png", ".webp": "image/webp", ".wasm": "application/wasm", ".mp3": "audio/mpeg", ".txt": "text/plain; charset=utf-8", ".xml": "application/xml" };

let server = null;
let URL_ = arg("url", "");
if (DIR) {
  server = http.createServer((req, res) => {
    const p = decodeURIComponent(req.url.split("?")[0]);
    let f = path.join(DIR, p === "/" ? "index.html" : p);
    if (!fs.existsSync(f) || fs.statSync(f).isDirectory()) { res.writeHead(404, { "content-type": "text/plain" }); res.end("404"); return; }
    res.writeHead(200, { "content-type": MIME[path.extname(f).toLowerCase()] || "application/octet-stream" });
    res.end(fs.readFileSync(f));
  });
  await new Promise((r) => server.listen(PORT_HTTP, "127.0.0.1", r));
  URL_ = `http://127.0.0.1:${PORT_HTTP}/`;
}

const profile = path.join(process.env.TEMP || ".", "selftest-edge-" + PORT_CDP);
const edge = spawn(EDGE, ["--headless=new", `--remote-debugging-port=${PORT_CDP}`, `--user-data-dir=${profile}`,
  "--no-first-run", "--no-default-browser-check", "--disable-gpu", "--hide-scrollbars", "--mute-audio",
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
const load = async () => { await send("Page.navigate", { url: URL_ }); await sleep(2500); };

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
await evaluate(`document.getElementById('enterBtn').click(); true`);
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
check("js: 内核版本号=2（含判定导出）", st.kernelVersion === 2, String(st.kernelVersion));
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

/* 音游在跑：等一会儿看 COMBO 是否递增 */
await evaluate(`document.getElementById('rhythm').scrollIntoView(); true`);
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

/* 刷新后最好成绩仍在（localStorage 持久化） */
await load();
await evaluate(`document.getElementById('enterBtn').click(); true`);
await sleep(2200);
const persisted = await evaluate(`(() => ({
  best: document.getElementById('rBest').textContent,
  stored: !!localStorage.getItem('myintro.rhythm.best.v1'),
  mode: (window.__rhythmStats || {}).mode
}))()`);
check("play: 刷新后最好成绩仍在且默认自动演奏",
  persisted.stored && /%/.test(persisted.best) && persisted.mode === "auto",
  `best=${persisted.best} mode=${persisted.mode}`);

check("js: 控制台无异常", consoleMsgs.length === 0, consoleMsgs.slice(0, 3).join(" | "));

const fails = results.filter((r) => !r.ok);
console.log(`\n=== ${results.length - fails.length}/${results.length} PASS ===`);
if (fails.length) console.log("FAILED: " + fails.map((f) => f.name).join("; "));
try { ws.close(); } catch {}
edge.kill();
if (server) server.close();
process.exit(fails.length ? 1 : 0);
