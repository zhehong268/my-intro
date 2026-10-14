/* warm-probe.mjs —— 只看一件事：入口门摆着的时候，预热 fetch 到底什么时候发出去
   用法：node warm-probe.mjs [url] [cdpPort] */
import { spawn } from "node:child_process";
import path from "node:path";
const URL_ = process.argv[2] || "https://zhehong268.github.io/my-intro/";
const PORT = Number(process.argv[3] || 9365);
const EDGE = process.env.SELFTEST_EDGE || "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe";
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const edge = spawn(EDGE, ["--headless=new", `--remote-debugging-port=${PORT}`,
  `--user-data-dir=${path.join(process.env.TEMP || ".", "warm-probe-" + PORT)}`, "--no-first-run",
  "--disable-gpu", "--hide-scrollbars", "--window-size=1440,900", "about:blank"], { stdio: "ignore" });
let page = null;
for (let i = 0; i < 40 && !page; i++) {
  try { page = (await (await fetch(`http://127.0.0.1:${PORT}/json/list`)).json()).find((t) => t.type === "page"); } catch {}
  if (!page) await sleep(250);
}
const ws = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((res, rej) => { ws.onopen = res; ws.onerror = rej; });
let mid = 0; const pend = new Map();
ws.onmessage = (e) => { const m = JSON.parse(e.data); if (m.id && pend.has(m.id)) { const p = pend.get(m.id); pend.delete(m.id); m.error ? p.rej(new Error(JSON.stringify(m.error))) : p.res(m.result); } };
const send = (method, params = {}) => new Promise((res, rej) => { const id = ++mid; pend.set(id, { res, rej }); ws.send(JSON.stringify({ id, method, params })); setTimeout(() => { if (pend.has(id)) { pend.delete(id); rej(new Error("timeout " + method)); } }, 20000); });
const ev = async (x) => (await send("Runtime.evaluate", { expression: x, returnByValue: true, awaitPromise: true })).result.value;

await send("Page.enable"); await send("Runtime.enable");
await send("Emulation.setDeviceMetricsOverride", { width: 1440, height: 900, deviceScaleFactor: 1, mobile: false });
await send("Page.navigate", { url: URL_ });
for (let i = 0; i < 25; i++) {
  await sleep(400);
  const s = await ev(`(() => {
    const nav = performance.getEntriesByType('navigation')[0] || {};
    const m = performance.getEntriesByType('resource').filter(function (e) { return /music\\.mp3/.test(e.name); })
      .map(function (e) { return e.initiatorType + '@' + Math.round(e.startTime) + 'ms/' + e.transferSize + 'B'; });
    return { now: Math.round(performance.now()), dcl: Math.round(nav.domContentLoadedEventEnd || 0), load: Math.round(nav.loadEventEnd || 0),
      warm: window.__musicWarm || null, music: m, hasGate: !!document.getElementById('enterBtn') };
  })()`);
  console.log(JSON.stringify(s));
}
ws.close(); edge.kill();
