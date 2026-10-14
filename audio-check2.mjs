/* audio-check2.mjs —— 音频通路四问（需要 danger-full-access：无头 Edge 走具名管道）
   1) 默认带参数进入 → 走 Web Audio 图，ctx 采样率应等于文件的 44100
   2) ?audio=plain → 完全不建图（ctx none）
   3) 播放中强制降级 → 换成原生元素后音乐仍在放、进度继续走
   4) 手机宽度（视觉化画布 display:none）→ 也不建图
   用法：node audio-check2.mjs            全部跑一遍
        node audio-check2.mjs --dir=. --edge="C:\...\msedge.exe" */
import fs from "node:fs";
import path from "node:path";
import http from "node:http";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const DIR = path.resolve(arg("dir", HERE));
const EDGE = arg("edge", process.env.SELFTEST_EDGE || "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
const PORT = Number(arg("port", "8137")), CDP = Number(arg("cdp", "9441"));
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const MIME = { ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".mjs": "text/javascript", ".css": "text/css", ".svg": "image/svg+xml", ".png": "image/png", ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".webp": "image/webp", ".mp3": "audio/mpeg", ".txt": "text/plain; charset=utf-8", ".xml": "application/xml", ".wasm": "application/wasm" };
/* 带 Range 支持：GitHub Pages 支持分段请求，本地服务器也必须支持，
   否则 <audio> 的 seek 会被浏览器判为不可用，测出来的行为和线上不一致 */
const server = http.createServer((req, res) => {
  const p = decodeURIComponent(req.url.split("?")[0]);
  const f = path.join(DIR, p === "/" ? "index.html" : p);
  if (!f.startsWith(DIR) || !fs.existsSync(f) || fs.statSync(f).isDirectory()) { res.writeHead(404); return res.end("nope"); }
  const type = MIME[path.extname(f).toLowerCase()] || "application/octet-stream";
  const size = fs.statSync(f).size;
  const range = req.headers.range;
  if (range) {
    const m = /bytes=(\d*)-(\d*)/.exec(range);
    let start = m && m[1] ? parseInt(m[1], 10) : 0;
    let end = m && m[2] ? parseInt(m[2], 10) : size - 1;
    if (!isFinite(start) || start < 0) start = 0;
    if (!isFinite(end) || end >= size) end = size - 1;
    if (start > end) { res.writeHead(416, { "content-range": "bytes */" + size }); return res.end(); }
    res.writeHead(206, { "content-type": type, "accept-ranges": "bytes", "content-range": `bytes ${start}-${end}/${size}`, "content-length": end - start + 1, "cache-control": "no-store" });
    fs.createReadStream(f, { start, end }).pipe(res);
    return;
  }
  res.writeHead(200, { "content-type": type, "accept-ranges": "bytes", "content-length": size, "cache-control": "no-store" });
  fs.createReadStream(f).pipe(res);
});
await new Promise((r) => server.listen(PORT, "127.0.0.1", r));

const edge = spawn(EDGE, ["--headless=new", `--remote-debugging-port=${CDP}`,
  `--user-data-dir=${path.join(process.env.TEMP || ".", "audio-chk")}`, "--no-first-run", "--no-default-browser-check",
  "--disable-gpu", "--hide-scrollbars", "--mute-audio", "--autoplay-policy=no-user-gesture-required", "about:blank"], { stdio: "ignore" });
let page = null;
for (let i = 0; i < 40 && !page; i++) {
  try { page = (await (await fetch(`http://127.0.0.1:${CDP}/json/list`)).json()).find((t) => t.type === "page"); } catch {}
  if (!page) await sleep(250);
}
const ws = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((res, rej) => { ws.onopen = res; ws.onerror = rej; });
let mid = 0; const pend = new Map();
ws.onmessage = (ev) => { const m = JSON.parse(ev.data); if (m.id && pend.has(m.id)) { const p = pend.get(m.id); pend.delete(m.id); m.error ? p.rej(new Error(JSON.stringify(m.error))) : p.res(m.result); } };
const send = (method, params = {}) => new Promise((res, rej) => { const id = ++mid; pend.set(id, { res, rej }); ws.send(JSON.stringify({ id, method, params })); setTimeout(() => { if (pend.has(id)) { pend.delete(id); rej(new Error("timeout " + method)); } }, 30000); });
const ev = async (e) => (await send("Runtime.evaluate", { expression: e, returnByValue: true, awaitPromise: true })).result.value;
await send("Page.enable"); await send("Runtime.enable");

const results = [];
const check = (name, ok, detail) => { results.push({ name, ok: !!ok, detail }); console.log((ok ? "PASS  " : "FAIL  ") + name + (detail ? "  — " + detail : "")); };

async function load(url, width = 1440, height = 900) {
  await send("Emulation.setDeviceMetricsOverride", { width, height, deviceScaleFactor: 1, mobile: width < 500 });
  await send("Page.navigate", { url });
  await sleep(2600);
  /* 入场 */
  await ev(`(() => { const b = document.querySelector('#enterBtn'); if (b) b.click(); return 1; })()`);
  await sleep(2600);
}

const probe = `(() => { const m = document.querySelector('#bgMusic'); return {
  mode: window.__audioMode ? window.__audioMode() : 'n/a',
  plain: window.__audioIsPlain ? window.__audioIsPlain() : null,
  ctx: window.__audioCtxInfo ? window.__audioCtxInfo() : 'n/a',
  t: m ? m.currentTime : -1, paused: m ? m.paused : null, ready: m ? m.readyState : -1,
  nAudio: document.querySelectorAll('audio').length, id: m ? m.id : null, src: m ? m.getAttribute('src') : null
}; })()`;

/* 1) 默认（桌面宽度，频谱可见）*/
await load(`http://127.0.0.1:${PORT}/index.html?enter=1`, 1440, 900);
const p1 = await ev(probe);
check("默认进入后走 Web Audio 图", p1.mode === "graph", `mode=${p1.mode}`);
check("图的采样率跟文件一致（44100），状态 running", p1.ctx === "44100/running", `ctx=${p1.ctx}`);
check("播放推进中（不是 paused）", p1.paused === false && p1.t > 0.3, `paused=${p1.paused} t=${p1.t.toFixed(2)}`);

/* 2) 强制原生通路 */
await load(`http://127.0.0.1:${PORT}/index.html?enter=1&audio=plain`, 1440, 900);
const p2 = await ev(probe);
check("?audio=plain 不建图（mode=plain-forced、ctx=none）", p2.mode === "plain-forced" && p2.ctx === "none", `mode=${p2.mode} ctx=${p2.ctx}`);

/* 3) 播放中强制降级 */
await load(`http://127.0.0.1:${PORT}/index.html?enter=1`, 1440, 900);
const before = await ev(probe);
await ev(`window.__audioForceFallback()`);
await sleep(300);
const seekTarget = await ev(`window.__audioSeekTarget`);
const seekInfo = await ev(`({ landed: window.__audioSeekLanded || null, seekable: window.__audioSeekable, dur: (document.querySelector('#bgMusic')||{}).duration })`);
await sleep(2200);
const after = await ev(probe);
check("强制降级后 mode=plain-fallback", after.mode === "plain-fallback", `mode=${after.mode}`);
check("服务器支持 Range、新元素可 seek", seekInfo.seekable === true, `seekable=${seekInfo.seekable} dur=${seekInfo.dur}`);
check("降级时 seek 到了原进度", seekTarget > 0 && Math.abs(seekTarget - before.t) < 1.5, `target=${seekTarget} before=${before.t.toFixed(2)} landed=${seekInfo.landed}`);
check("降级后不回到开头、进度继续走", after.paused === false && after.t >= before.t - 0.4, `t ${before.t.toFixed(2)} → ${after.t.toFixed(2)} paused=${after.paused}`);
check("降级后页面上只剩一个 audio 元素", after.nAudio === 1 && after.id === "bgMusic", `n=${after.nAudio} id=${after.id} src=${after.src}`);
check("降级后彻底退出 Web Audio（ctx=none）", after.ctx === "none", `ctx=${after.ctx}`);

/* 4) 手机宽度：频谱画布 display:none → 不该建图 */
await load(`http://127.0.0.1:${PORT}/index.html?enter=1`, 390, 844);
const p4 = await ev(probe);
const vizState = await ev(`(() => { const v = document.querySelector('#viz'); if (!v) return 'none'; const r = v.getBoundingClientRect(); return r.width + 'x' + r.height; })()`);
check("手机宽度下频谱不可见时不建图", p4.ctx === "none", `ctx=${p4.ctx} viz=${vizState}`);
check("不建图也照样在放音乐", p4.paused === false && p4.t > 0.3, `paused=${p4.paused} t=${p4.t.toFixed(2)}`);

const failed = results.filter((r) => !r.ok);
console.log("\n" + (results.length - failed.length) + "/" + results.length + " PASS");
ws.close(); edge.kill(); server.close();
process.exit(failed.length ? 1 : 0);
