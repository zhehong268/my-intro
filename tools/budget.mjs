/* budget.mjs —— 首屏体积门禁
   规则：从 index.html 里抓出所有本地引用，减去「首次加载不会下载」的两类
   （music.mp3 是 preload="none" + 用户手势后才加载；og-image.jpg 只出现在
   <meta content> 里，给爬虫和聊天工具看，浏览器正常访问不会取），剩下的就是
   首屏字节数。超预算直接非零退出，让 CI 红掉。
   用法：node tools/budget.mjs [--root=<仓库目录>] */
import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";
import { fileURLToPath } from "node:url";

const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const ROOT = path.resolve(arg("root", path.join(path.dirname(fileURLToPath(import.meta.url)), "..")));

/* 预算：首屏 500 KB 原始字节 / HTML 压缩后 60 KB / 交互后按需加载的音频 4.5 MB
   （首屏从 450 KB 上调到 500 KB：2026-10 引入自托管字体子集，三个 woff2 共约 52 KB，
   是主动的视觉升级而不是无意变重——标题换霞鹜文楷、等宽换 JetBrains Mono，
   按页面实际用字子集化。省掉的回退链在 Windows 上只能落到 Consolas/雅黑，
   这个字节数花得值。门禁照旧硬失败：再有人无意加一张大图照样红。） */
const BUDGET = { firstLoad: 500 * 1024, htmlGzip: 60 * 1024, music: 4.5 * 1024 * 1024 };
/* 入口门一出现，页面会自己 fetch 音频的头 160 KB 预热（main.cpp 的 warmMusic()，
   Range: bytes=0-163839）。它不在 HTML 的引用列表里，抓不到，但不能当它不存在 ——
   这笔账记在这里，算进首屏口径。省流量模式下会被跳过，所以这是上限不是必花。 */
const WARMUP = 160 * 1024;
/* 首次加载不会下载的文件，以及原因 */
const NOT_FIRST_LOAD = {
  "music.mp3": "preload=none，首次交互后才加载",
  "og-image.jpg": "只出现在 meta content 里，浏览器访问页面不会取",
  "404.html": "只有 404 路径才会返回",
};

const html = fs.readFileSync(path.join(ROOT, "index.html"), "utf8");
const refs = new Set();
/* 只数真正会被下载的引用：src="..."、<link href="...">、CSS 里的 url(...)。
   页面之间的 <a href="kernel.html"> 是点击才加载，不算首屏——这是这个门禁
   最容易误判的地方（工具页一挂到页脚，首屏就虚增 90 KB）。 */
for (const m of html.matchAll(/\bsrc="([^"]+)"/g)) refs.add(m[1]);
for (const m of html.matchAll(/<link\b[^>]*\bhref="([^"]+)"/g)) refs.add(m[1]);
for (const m of html.matchAll(/url\(\s*[\x22\x27]?([^)\x22\x27\s]+)/g)) refs.add(m[1]);
const external = (u) => /^(https?:|mailto:|tel:|data:|#|\/\/|\/)/.test(u);

let firstLoad = 0, music = 0, bad = 0;
const rows = [];
for (const r of [...refs].sort()) {
  if (external(r)) continue;
  const p = path.join(ROOT, r);
  if (!fs.existsSync(p)) { console.log("::error::引用存在但文件不存在: " + r); bad = 1; continue; }
  const size = fs.statSync(p).size;
  if (r === "music.mp3") { music = size; rows.push([r, size, "按需"]); continue; }
  if (NOT_FIRST_LOAD[r]) { rows.push([r, size, "不计"]); continue; }
  firstLoad += size;
  rows.push([r, size, "首屏"]);
}
const htmlSize = fs.statSync(path.join(ROOT, "index.html")).size;
const htmlGz = zlib.gzipSync(Buffer.from(html), { level: 9 }).length;
rows.push(["index.html", htmlSize, "首屏"]);
firstLoad += htmlSize;

const kb = (n) => (n / 1024).toFixed(1) + " KB";
console.log("文件".padEnd(18) + "原始".padStart(12) + "  计入");
for (const [n, s, tag] of rows) console.log(n.padEnd(18) + kb(s).padStart(12) + "  " + tag);
console.log("-".repeat(40));
console.log("首屏合计        " + kb(firstLoad).padStart(12) + "  预算 " + kb(BUDGET.firstLoad));
console.log("含音频预热      " + kb(firstLoad + WARMUP).padStart(12) + "  预热 " + kb(WARMUP));
console.log("index.html gzip " + kb(htmlGz).padStart(12) + "  预算 " + kb(BUDGET.htmlGzip));
console.log("music.mp3       " + kb(music).padStart(12) + "  预算 " + kb(BUDGET.music));

/* 工具页各自独立预算：它们不是首屏的一部分，但也不该越做越重 */
const TOOL_BUDGET = { raw: 160 * 1024, gzip: 45 * 1024 };
for (const name of ["kernel.html", "nojs.html"]) {
  const p = path.join(ROOT, name);
  if (!fs.existsSync(p)) { console.log("::error::缺少工具页: " + name); bad = 1; continue; }
  const raw = fs.statSync(p).size;
  const gz = zlib.gzipSync(fs.readFileSync(p), { level: 9 }).length;
  console.log(name.padEnd(16) + kb(raw).padStart(12) + "  工具页（gzip " + kb(gz) + "，预算 " + kb(TOOL_BUDGET.raw) + " / " + kb(TOOL_BUDGET.gzip) + "）");
  if (raw > TOOL_BUDGET.raw || gz > TOOL_BUDGET.gzip) { console.log("::error::" + name + " 超预算"); bad = 1; }
}

if (firstLoad + WARMUP > BUDGET.firstLoad) { console.log("::error::首屏（含音频预热）超预算"); bad = 1; }
if (htmlGz > BUDGET.htmlGzip) { console.log("::error::HTML 压缩后超预算"); bad = 1; }
if (music > BUDGET.music) { console.log("::error::音频超预算"); bad = 1; }
console.log(bad ? "budget gate FAILED" : "budget gate OK");
process.exit(bad);
