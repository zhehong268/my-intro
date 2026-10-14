/* make-og.mjs —— 用无头 Edge 把 tools/og-card.html 渲染成根目录的 og-image.png（1200×630）
   产物是提交进仓库的构建输入：CI 只负责 cp 到 _site，不重新生成。
   用法：node tools/make-og.mjs [--edge=<msedge.exe>] */
import fs from "node:fs";
import path from "node:path";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "..");
const OUT = path.join(ROOT, "og-image.jpg");
const CARD = path.join(HERE, "og-card.html");
const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const EDGE = arg("edge", process.env.SELFTEST_EDGE || "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
const PORT = Number(arg("cdp", "9337"));
const W = 1200, H = 630;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const edge = spawn(EDGE, ["--headless=new", `--remote-debugging-port=${PORT}`,
  `--user-data-dir=${path.join(process.env.TEMP || ".", "og-render")}`, "--no-first-run", "--no-default-browser-check",
  "--disable-gpu", "--hide-scrollbars", "--force-device-scale-factor=1", `--window-size=${W},${H}`, "about:blank"], { stdio: "ignore" });

let page = null;
for (let i = 0; i < 40 && !page; i++) {
  try { page = (await (await fetch(`http://127.0.0.1:${PORT}/json/list`)).json()).find((t) => t.type === "page"); } catch {}
  if (!page) await sleep(250);
}
if (!page) throw new Error("edge devtools unreachable");
const ws = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((res, rej) => { ws.onopen = res; ws.onerror = rej; });
let mid = 0; const pend = new Map();
ws.onmessage = (ev) => { const m = JSON.parse(ev.data); if (m.id && pend.has(m.id)) { const p = pend.get(m.id); pend.delete(m.id); m.error ? p.rej(new Error(JSON.stringify(m.error))) : p.res(m.result); } };
const send = (method, params = {}) => new Promise((res, rej) => { const id = ++mid; pend.set(id, { res, rej }); ws.send(JSON.stringify({ id, method, params })); setTimeout(() => { if (pend.has(id)) { pend.delete(id); rej(new Error("timeout " + method)); } }, 30000); });
const evaluate = async (e) => (await send("Runtime.evaluate", { expression: e, returnByValue: true, awaitPromise: true })).result.value;

await send("Page.enable");
await send("Emulation.setDeviceMetricsOverride", { width: W, height: H, deviceScaleFactor: 1, mobile: false });
await send("Page.navigate", { url: "file:///" + CARD.replace(/\\/g, "/") });
await sleep(1200);
const ready = await evaluate(`(() => { const i = document.images[0]; return { n: document.images.length, complete: i ? i.complete : false, w: i ? i.naturalWidth : 0, h: i ? i.naturalHeight : 0, bodyW: document.body.scrollWidth, bodyH: document.body.scrollHeight }; })()`);
console.log("card ready:", JSON.stringify(ready));
if (!ready.complete || !ready.w) throw new Error("avatar 没加载出来，别出图");
const shot = await send("Page.captureScreenshot", { format: "jpeg", quality: 88, captureBeyondViewport: false });
fs.writeFileSync(OUT, Buffer.from(shot.data, "base64"));
const size = fs.statSync(OUT).size;
console.log("wrote", OUT, size, "bytes");
ws.close(); edge.kill();
if (size < 20000) throw new Error("og-image.jpg 太小，可能渲染失败");
