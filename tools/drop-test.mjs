/* drop-test.mjs —— 「拖一首歌进来，页面当场量起音点」这条路的端到端测试
   本地跑（要 Chrome/Edge + CDP，CI 里没有浏览器）：
     node tools/drop-test.mjs [--dir=.] [--port=8124] [--cdp=9342] [--out=.] [--keep]

   它做的事：
   1. 现场合成一个 10 秒、每 500ms 一击的 WAV（44.1kHz 单声道 16bit，20 个起音点）；
   2. 起一个支持 Range 的本地静态服务器，开无头 Edge 打开 index.html?enter=1；
   3. 断言页面里的 wasm 自检值 == native 生成器内联的值（同一套算法的证据）；
   4. 用 CDP 的 DOM.setFileInputFiles 把 WAV 塞进 #dropFile（真·文件选择路径）；
   5. 断言谱面换成了这首歌、起音点个数对得上、时钟切到 audio.currentTime、时钟标签跟着改名；
   6. 出图 drop-zone.png / drop-canvas.png。
*/
import fs from "node:fs";
import http from "node:http";
import path from "node:path";
import { spawn } from "node:child_process";

const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const ROOT = path.resolve(arg("dir", "."));
const OUT = path.resolve(arg("out", ROOT));
const PORT = Number(arg("port", "8124")), CDP = Number(arg("cdp", "9342"));
const EDGE = arg("edge", "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
const THEME = arg("theme", "");
const MIME = { ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".css": "text/css", ".svg": "image/svg+xml", ".png": "image/png", ".jpg": "image/jpeg", ".webp": "image/webp", ".mp3": "audio/mpeg", ".wav": "audio/wav", ".woff2": "font/woff2", ".wasm": "application/wasm", ".txt": "text/plain; charset=utf-8" };
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let bad = 0;
const check = (name, ok, extra = "") => {
  console.log((ok ? "PASS  " : "FAIL  ") + name + (extra ? "  — " + extra : ""));
  if (!ok) bad = 1;
};

/* ---------- 1. 合成一个能被当成「歌」的 WAV ---------- */
const RATE = 44100, SECONDS = 10, CLICK_MS = 500;
const taps = Math.round((SECONDS * 1000) / CLICK_MS); // 20
const wavPath = path.join(OUT, "click-track.wav");
{
  const n = RATE * SECONDS, pcm = Buffer.alloc(n * 2);
  let seed = 12345;
  for (let i = 0; i < n; i++) {
    const t = i / RATE, phase = t % (CLICK_MS / 1000);
    let v = phase < 0.06 ? Math.sin(2 * Math.PI * 880 * t) * Math.exp(-phase * 40) * 0.8 : 0;
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    v += ((seed / 0x7fffffff) - 0.5) * 0.002; // 一点点底噪，别让谱通量全零
    pcm.writeInt16LE(Math.max(-32767, Math.min(32767, Math.round(v * 32767))), i * 2);
  }
  const head = Buffer.alloc(44);
  head.write("RIFF", 0); head.writeUInt32LE(36 + pcm.length, 4); head.write("WAVE", 8);
  head.write("fmt ", 12); head.writeUInt32LE(16, 16); head.writeUInt16LE(1, 20); head.writeUInt16LE(1, 22);
  head.writeUInt32LE(RATE, 24); head.writeUInt32LE(RATE * 2, 28); head.writeUInt16LE(2, 32); head.writeUInt16LE(16, 34);
  head.write("data", 36); head.writeUInt32LE(pcm.length, 40);
  fs.writeFileSync(wavPath, Buffer.concat([head, pcm]));
  console.log(`合成测试音频：${path.basename(wavPath)}（${SECONDS}s / ${taps} 个 500ms 一击）`);
}

/* ---------- 2. 静态服务器（支持 Range） ---------- */
const server = http.createServer((req, res) => {
  const u = decodeURIComponent(req.url.split("?")[0]);
  const file = path.join(ROOT, u === "/" ? "index.html" : u.replace(/^\/+/, ""));
  if (!file.startsWith(ROOT) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) { res.writeHead(404); res.end("nope"); return; }
  const buf = fs.readFileSync(file), type = MIME[path.extname(file).toLowerCase()] || "application/octet-stream";
  const range = req.headers.range;
  if (range) {
    const m = /bytes=(\d*)-(\d*)/.exec(range);
    const start = m[1] ? Number(m[1]) : 0, end = m[2] ? Number(m[2]) : buf.length - 1;
    res.writeHead(206, { "content-type": type, "accept-ranges": "bytes", "content-range": `bytes ${start}-${end}/${buf.length}`, "content-length": end - start + 1 });
    res.end(buf.subarray(start, end + 1));
  } else {
    res.writeHead(200, { "content-type": type, "accept-ranges": "bytes", "content-length": buf.length });
    res.end(buf);
  }
});
await new Promise((r) => server.listen(PORT, r));

/* ---------- 3. 无头浏览器 + CDP ---------- */
const edge = spawn(EDGE, ["--headless=new", `--remote-debugging-port=${CDP}`, `--user-data-dir=${path.join(process.env.TEMP || ".", "drop-test")}`,
  "--no-first-run", "--no-default-browser-check", "--disable-gpu", "--hide-scrollbars",
  "--autoplay-policy=no-user-gesture-required", "--mute-audio", "about:blank"], { stdio: "ignore" });
let page = null;
for (let i = 0; i < 40 && !page; i++) { try { page = (await (await fetch(`http://127.0.0.1:${CDP}/json/list`)).json()).find((t) => t.type === "page"); } catch {} if (!page) await sleep(250); }
if (!page) { console.log("FAIL  没连上无头浏览器（CDP " + CDP + "）"); server.close(); edge.kill(); process.exit(1); }
const ws = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((res, rej) => { ws.onopen = res; ws.onerror = rej; });
let mid = 0; const pend = new Map();
ws.onmessage = (e) => { const m = JSON.parse(e.data); if (m.id && pend.has(m.id)) { const p = pend.get(m.id); pend.delete(m.id); m.error ? p.rej(new Error(JSON.stringify(m.error))) : p.res(m.result); } };
const send = (method, params = {}) => new Promise((res, rej) => { const id = ++mid; pend.set(id, { res, rej }); ws.send(JSON.stringify({ id, method, params })); setTimeout(() => { if (pend.has(id)) { pend.delete(id); rej(new Error("timeout " + method)); } }, 40000); });
const ev = async (e) => (await send("Runtime.evaluate", { expression: e, returnByValue: true, awaitPromise: true })).result.value;

await send("Page.enable"); await send("DOM.enable"); await send("Runtime.enable");
await send("Emulation.setDeviceMetricsOverride", { width: 1440, height: 900, deviceScaleFactor: 1, mobile: false });
await send("Page.navigate", { url: `http://127.0.0.1:${PORT}/index.html?enter=1${THEME ? "&theme=" + THEME : ""}` });

let parity = null;
for (let i = 0; i < 60 && parity === null; i++) { parity = await ev("window.__onsetParity || null"); if (parity === null) await sleep(250); }
check("页面里的 wasm 起音自检与 native 生成器内联的值一致", parity === "ok", String(parity));
check("内核版本是 3（含起音检测）", (await ev("window.__rhythmKernelVersion || 0")) === 3, String(await ev("window.__rhythmKernelVersion")));

const before = await ev("JSON.stringify(window.__onsetState ? window.__onsetState() : null)");
console.log("        拖之前：" + before);

/* ---------- 4. 把 WAV 塞进真的文件选择路径 ---------- */
const doc = await send("DOM.getDocument", { depth: -1 });
const node = await send("DOM.querySelector", { nodeId: doc.root.nodeId, selector: "#dropFile" });
check("找得到 #dropFile", !!node.nodeId, "nodeId=" + node.nodeId);
await send("DOM.setFileInputFiles", { files: [wavPath], nodeId: node.nodeId });

let after = null;
for (let i = 0; i < 80; i++) {
  after = JSON.parse((await ev("JSON.stringify(window.__onsetState ? window.__onsetState() : null)")) || "null");
  if (after && after.source && after.source.indexOf("本站") < 0) break;
  await sleep(250);
}
console.log("        拖之后：" + JSON.stringify(after));
check("谱面来源换成了这首歌", !!after && /click-track/.test(String(after.source)), String(after && after.source));
check("起音点个数在 15~25 之间（20 击，首尾可能各差一个）", !!after && after.onsets >= 15 && after.onsets <= 25, String(after && after.onsets));
check("曲长按这首歌算（约 10000ms）", !!after && Math.abs(after.songMs - 10000) < 400, String(after && after.songMs));
check("audio 元素换成了 blob: 地址（放的就是这首歌）", /^blob:/.test(String(await ev("(document.querySelector('audio')||{}).src || ''"))), String(await ev("(document.querySelector('audio')||{}).src")).slice(0, 40));
check("时钟标签跟着改名", /跟随\s*click-track/.test(String(await ev("(document.getElementById('rhythmClock')||{}).textContent"))), String(await ev("(document.getElementById('rhythmClock')||{}).textContent")));
check("状态行说的是当前谱面", /当前谱面/.test(String(await ev("(document.getElementById('dropInfo')||{}).textContent"))), String(await ev("(document.getElementById('dropInfo')||{}).textContent")).slice(0, 60));
check("重置按钮露出来了", (await ev("!(document.getElementById('dropReset')||{}).hidden")) === true);

/* 让这首「歌」真的播起来，再看判定时钟是不是 audio.currentTime */
await ev("(function(){var a=document.querySelector('audio');if(a){a.play&&a.play();}return 1})()");
await sleep(1500);
const probe = JSON.parse((await ev("JSON.stringify(window.__rhythmClockProbe ? window.__rhythmClockProbe() : null)")) || "null");
console.log("        时钟探针：" + JSON.stringify(probe));
check("判定时钟切到 audio.currentTime（同一时刻同一读数）", !!probe && probe.src === "audio" && Math.abs(probe.song - probe.audio) < 1, probe ? probe.src + " song=" + probe.song + " audio=" + probe.audio : "null");
check("新谱面确实是用这首歌的起音点排的", !!probe && probe.timeline > 0 && probe.nextIsOnset === true, probe ? "timeline=" + probe.timeline + " nextIsOnset=" + probe.nextIsOnset : "null");

/* ---------- 5. 出图 ---------- */
await ev("document.getElementById('dropZone').scrollIntoView({block:'center',behavior:'instant'})");
await sleep(1200);
const tag = THEME ? "-" + THEME : "";
let shot = await send("Page.captureScreenshot", { format: "png", captureBeyondViewport: false });
fs.writeFileSync(path.join(OUT, `drop-zone${tag}.png`), Buffer.from(shot.data, "base64"));
await ev("document.getElementById('rhythm').scrollIntoView({block:'start',behavior:'instant'})");
await sleep(2200);
shot = await send("Page.captureScreenshot", { format: "png", captureBeyondViewport: false });
fs.writeFileSync(path.join(OUT, `drop-canvas${tag}.png`), Buffer.from(shot.data, "base64"));
console.log(`        → drop-zone${tag}.png / drop-canvas${tag}.png`);

if (!args.includes("--keep")) fs.rmSync(wavPath, { force: true });
console.log(bad ? "drop test FAILED" : "drop test OK");
ws.close(); edge.kill(); server.close();
process.exit(bad);
