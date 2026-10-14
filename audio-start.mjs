/* audio-start.mjs —— 量「点进入主页 → 真的出声」这段有几秒，以及时间花在哪
   用法： node audio-start.mjs [url] [下行 kbps]   （kbps 传 0 表示不限速） */
import { spawn } from "node:child_process";
import path from "node:path";

const URL_ = process.argv[2] || "https://zhehong268.github.io/my-intro/";
const KBPS = Number(process.argv[3] || 0);
const CDP = Number(process.argv[4] || 9357);
/* PowerShell 会把空字符串参数整个丢掉，所以空查询用 "-" 占位 */
const Q = process.argv[5] && process.argv[5] !== "-" ? process.argv[5] : "";
/* 点门前等多久：800ms 模拟「一看到按钮就点」，3500ms 模拟真人看完封面再点 */
const CLICK_DELAY = Number(process.argv[6] || 800);
const EDGE = "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe";
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const edge = spawn(EDGE, ["--headless=new", `--remote-debugging-port=${CDP}`, `--user-data-dir=${path.join(process.env.TEMP || ".", "audio-start-" + CDP)}`,
  "--no-first-run", "--no-default-browser-check", "--disable-gpu", "--hide-scrollbars", "--window-size=1440,900", "about:blank"], { stdio: "ignore" });
let page = null, listRaw = "";
for (let i = 0; i < 60 && !page; i++) {
  try { listRaw = await (await fetch(`http://127.0.0.1:${CDP}/json/list`)).text(); page = JSON.parse(listRaw).find((t) => t.type === "page"); } catch (e) { listRaw = String(e.message); }
  if (!page) await sleep(250);
}
if (!page) { console.log("Edge 起不来，/json/list =", listRaw.slice(0, 300)); process.exit(3); }
const ws = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((res, rej) => { ws.onopen = res; ws.onerror = rej; });
let mid = 0; const pend = new Map(); const net = [];
ws.onmessage = (e) => {
  const m = JSON.parse(e.data);
  if (m.id && pend.has(m.id)) { const p = pend.get(m.id); pend.delete(m.id); m.error ? p.rej(new Error(JSON.stringify(m.error))) : p.res(m.result); }
  if (m.method === "Network.requestWillBeSent" && /music\.mp3/.test(m.params.request.url)) {
    net.push({ reqId: m.params.requestId, ev: "请求发出", extra: (m.params.request.headers && m.params.request.headers.Range) || "无 Range", ts: m.params.wallTime });
  }
  if (m.method === "Network.responseReceived" && /music\.mp3/.test(m.params.response.url)) {
    const n = net.find((x) => x.reqId === m.params.requestId); if (n) { n.ev = "响应头"; n.extra = m.params.response.status + " " + (m.params.response.headers["content-range"] || "整文件"); }
  }
  if (m.method === "Network.loadingFinished") {
    const n = net.find((x) => x.reqId === m.params.requestId); if (n) { n.done = (m.params.encodedDataLength / 1024).toFixed(0) + " KB"; n.doneTs = m.params.timestamp; }
  }
};
const send = (method, params = {}) => new Promise((res, rej) => { const id = ++mid; pend.set(id, { res, rej }); ws.send(JSON.stringify({ id, method, params })); setTimeout(() => { if (pend.has(id)) { pend.delete(id); rej(new Error("timeout " + method)); } }, 40000); });
const ev = async (e) => (await send("Runtime.evaluate", { expression: e, returnByValue: true, awaitPromise: true })).result.value;

await send("Page.enable"); await send("Runtime.enable"); await send("Network.enable");
await send("Emulation.setDeviceMetricsOverride", { width: 1440, height: 900, deviceScaleFactor: 1, mobile: false });
if (KBPS > 0) await send("Network.emulateNetworkConditions", { offline: false, latency: 60, downloadThroughput: Math.round(KBPS * 1024 / 8), uploadThroughput: Math.round(KBPS * 1024 / 8 / 3) });

await send("Page.navigate", { url: URL_ + Q });
/* 等真正的入口门出现（真实流程：先看门，再点进入） */
for (let i = 0; i < 60; i++) {
  const ok = await ev(`!!(document.getElementById('enterBtn') && document.getElementById('bgMusic'))`);
  if (ok) break;
  if (i === 59) { console.log("15 秒内没等到入口门，放弃（URL 对不对？）"); ws.close(); edge.kill(); process.exit(2); }
  await sleep(250);
}
await sleep(CLICK_DELAY);
await ev(`(() => {
  window.__marks = []; window.__seen = {};
  window.__t0 = performance.now();
  const mk = (n) => { if (!window.__seen[n]) { window.__seen[n] = 1; window.__marks.push([n, Math.round(performance.now() - window.__t0)]); } };
  window.__mk = mk;
  const m = document.querySelector('#bgMusic');
  ['play','playing','waiting','stalled','canplay','loadedmetadata','loadeddata','seeking','seeked'].forEach(function (e) { m.addEventListener(e, function () { mk(e + ' (t=' + m.currentTime.toFixed(2) + ')'); }); });
  m.addEventListener('timeupdate', function () { if (m.currentTime > 0.05) mk('timeupdate t>0 (t=' + m.currentTime.toFixed(2) + ')'); });
  new MutationObserver(function () { if (!document.documentElement.classList.contains('entry')) mk('入场遮罩撤掉'); }).observe(document.documentElement, { attributes: true, attributeFilter: ['class'] });
  document.getElementById('enterBtn').addEventListener('click', function () { window.__clickAt = Math.round(performance.now() - window.__t0); }, true);
  window.__preload = m.getAttribute('preload'); window.__src = m.getAttribute('src');
  return true;
})()`);
console.log("audio 标签:", JSON.stringify(await ev(`({ preload: window.__preload, src: window.__src, paused: document.querySelector('#bgMusic').paused })`)));
const box = await ev(`(() => { const b = document.getElementById('enterBtn').getBoundingClientRect(); return { x: Math.round(b.x + b.width / 2), y: Math.round(b.y + b.height / 2) }; })()`);
/* 真人要先把鼠标移到按钮上（pointermove 是预热触发器之一），隔一会儿才点下去 */
await send("Input.dispatchMouseEvent", { type: "mouseMoved", x: box.x - 140, y: box.y - 70, button: "none" });
await sleep(120);
await send("Input.dispatchMouseEvent", { type: "mouseMoved", x: box.x, y: box.y, button: "none" });
await sleep(CLICK_DELAY);
await send("Input.dispatchMouseEvent", { type: "mousePressed", x: box.x, y: box.y, button: "left", clickCount: 1 });
await send("Input.dispatchMouseEvent", { type: "mouseReleased", x: box.x, y: box.y, button: "left", clickCount: 1 });
const t0 = Date.now();
let firstSound = null;
while (Date.now() - t0 < 25000) {
  const s = await ev(`(() => { const m = document.querySelector('#bgMusic'); return { t: +m.currentTime.toFixed(2), rs: m.readyState, mode: window.__audioMode ? window.__audioMode() : 'n/a' }; })()`);
  if (s.t > 0.15 && firstSound === null) { firstSound = Date.now() - t0; console.log(`第一次听到声音（t>0.15）：约 ${(firstSound / 1000).toFixed(1)}s，rs=${s.rs} mode=${s.mode}`); break; }
  await sleep(120);
}
if (firstSound === null) console.log("25 秒内没出声");
console.log(`\n点击进入主页：+${await ev(`window.__clickAt`)} ms（以下相对点击前的那次页面加载）`);
for (const [n, t] of await ev(`window.__marks`)) console.log(`  ${String(t).padStart(6)} ms  ${n}`);
for (const n of net) console.log(`  net ${n.ev} ${n.extra}${n.done ? " → " + n.done : ""}`);
/* 预热有没有发生、元素的那次请求是不是命中缓存 */
console.log("预热状态 __musicWarm =", await ev(`window.__musicWarm || null`));
for (const r of await ev(`performance.getEntriesByType('resource').filter(function (e) { return /music\\.mp3/.test(e.name); }).map(function (e) { return { initiator: e.initiatorType, start: Math.round(e.startTime), dur: Math.round(e.duration), transfer: e.transferSize, decoded: e.decodedBodySize, proto: e.nextHopProtocol }; })`)) {
  console.log("  ", JSON.stringify(r));
}
ws.close(); edge.kill();
