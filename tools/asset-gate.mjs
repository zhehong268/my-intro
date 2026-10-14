/* asset-gate.mjs —— 资产门：index.html 里引用到的每个本地文件都必须真的存在于发布目录
   扫三类引用：
     1. src="..." / href="..."            —— 常规标签
     2. url(...) 与 url('...')            —— CSS 背景图（带引号和不带引号都认）
     3. 任何 文件名.媒体后缀 形式的字符串  —— og:image / twitter:image 藏在 <meta content> 里，
                                             前两类抓不到，只能靠后缀兜住
   第 3 类会把整条 URL 都匹配进来，所以：以本站 canonical 地址开头的、或以 / 开头的，
   都折算成文件名来检查（发布目录是平铺的）；其它域名的绝对 URL 才当作外部资源跳过。
   用法：node tools/asset-gate.mjs [--dir=_site] [--html=index.html] */
import fs from "node:fs";
import path from "node:path";

const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const DIR = arg("dir", "_site");
const HTML = arg("html", "index.html");

const html = fs.readFileSync(HTML, "utf8");
const canonical = (html.match(/<link rel="canonical" href="([^"]+)"/) || [, ""])[1];
const refs = new Set();
for (const m of html.matchAll(/(?:src|href)="([^"]+)"/g)) refs.add(m[1]);
for (const m of html.matchAll(/url\(\s*[\x22\x27]?([^)\x22\x27\s]+)/g)) refs.add(m[1]);
for (const m of html.matchAll(/[\w./:-]+\.(?:png|jpe?g|webp|svg|gif|mp3|wav|ogg|ico)\b/g)) {
  let r = m[0];
  const own = (u) => canonical && u.startsWith(canonical);
  if (own(r) || r.startsWith("/")) r = r.split("/").pop(); // 本站绝对地址 / 根路径 → 折算成文件名
  refs.add(r);
}

const external = (u) => /^(https?:|mailto:|tel:|data:|#|\/\/|\/)/.test(u);
let bad = 0, checked = 0;
for (const r of [...refs].sort()) {
  if (external(r)) continue;
  checked++;
  if (!fs.existsSync(path.join(DIR, r))) { console.log("::error::引用了但发布目录里没有: " + r); bad = 1; }
}
console.log(bad ? "asset gate FAILED" : "asset gate OK - " + checked + " 个本地引用都在 " + DIR + "/ 里");
process.exit(bad);
