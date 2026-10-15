/* font-coverage.mjs —— 子集字体覆盖检查
   门禁要防的是一件很隐蔽的事：展示字体（霞鹜文楷）只子集了「当时写页面时用到的字」，
   以后改了标题/引言却忘了重新跑 tools/subset-fonts.py，那个字会悄悄回退成系统宋体，
   页面上只是某个字看着怪，肉眼很容易漏。这里把「页面真正用到的字」和
   assets/fonts/display-chars.txt、mono-chars.txt 对一遍。
   用法：node tools/font-coverage.mjs [--dir=.]
   exit 0 = 覆盖完整；exit 1 = 有字没被收进子集（会回退） */
import fs from "node:fs";
import path from "node:path";

const arg = (k, d) => { const a = process.argv.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const DIR = arg("dir", ".");
const FONTS = path.join(DIR, "assets/fonts");
const PAGES = ["index.html", "kernel.html", "nojs.html", "404.html"];
const isCJK = (c) => /[\u3000-\u9fff\uff00-\uffef]/.test(c);

/* 展示字体挂在哪些类上（与 main.cpp 的 var(--display) 用法一致） */
const DISPLAY_CLASSES = ["landing-title", "landing-sub", "section-title", "hero-name"];
/* 等宽字体挂在哪些类上：类名里带 mono 的，加上几个手写的 */
const MONO_EXTRA = ["eyebrow", "brand-sub", "landing-latin", "enter-hint", "combo-num", "combo-label", "hero-clock", "stat-num"];

const strip = (s) => s.replace(/<[^>]*>/g, "").replace(/&amp;/g, "&").replace(/&nbsp;/g, " ").replace(/&[a-z]+;/g, " ").trim();

function collect(html, classes, { cjkOnly = false, nonCjkOnly = false } = {}) {
  const hits = new Map(); // char -> 出现它的元素文字（最多留一个例子）
  for (const cls of classes) {
    const re = new RegExp(`class="(?:[^"]*\\s)?${cls}(?:\\s[^"]*)?"[^>]*>([\\s\\S]*?)<`, "g");
    let m;
    while ((m = re.exec(html))) {
      const text = strip(m[1]);
      for (const ch of text.replace(/\s+/g, "")) {
        if (ch === "") continue;
        if (cjkOnly && !isCJK(ch)) continue;
        if (nonCjkOnly && isCJK(ch)) continue;
        if (!hits.has(ch)) hits.set(ch, cls + "：" + text.slice(0, 24));
      }
    }
  }
  return hits;
}

const read = (p) => fs.readFileSync(p, "utf8");
const setOf = (p) => new Set(read(p).replace(/\s+/g, "").split("").filter(Boolean));

if (!fs.existsSync(path.join(FONTS, "display-chars.txt"))) {
  console.log("font coverage: 没有子集字体（assets/fonts/display-chars.txt 不存在），跳过");
  process.exit(0);
}

const displaySet = setOf(path.join(FONTS, "display-chars.txt"));
const monoAll = setOf(path.join(FONTS, "mono-chars.txt"));
const monoSet = new Set([...monoAll].filter((c) => !isCJK(c)));

/* subset-fonts.py 的 is_cjk() 把 —…· 当成「类 CJK」，等宽子集故意不收它们，
   让它们跟着 CJK 一起回退。这三个标点落在等宽行里（页脚 C++ · Deployed、
   引用的 —— 署名、内核徽章的 检测中…）只是字形略有差别，不算缺陷，所以只提醒。 */
const MONO_KNOWN_FALLBACK = new Set(["—", "…", "·"]);

let bad = 0, warn = 0;
for (const f of PAGES) {
  const file = path.join(DIR, f);
  if (!fs.existsSync(file)) continue;
  const html = read(file);

  const dUsed = collect(html, DISPLAY_CLASSES);
  const dMiss = [...dUsed].filter(([ch]) => !displaySet.has(ch));

  const mClasses = [...new Set([...html.matchAll(/class="([^"]*)"/g)].flatMap((m) => m[1].split(/\s+/)).filter((c) => /mono/.test(c)))];
  const mUsed = collect(html, [...mClasses, ...MONO_EXTRA], { nonCjkOnly: true });
  const mMiss = [...mUsed].filter(([ch]) => !monoSet.has(ch));

  console.log(`${f}: 展示字体用字 ${dUsed.size}（子集 ${displaySet.size}）· 等宽非 CJK 用字 ${mUsed.size}（子集 ${monoSet.size}）`);
  for (const [ch, where] of dMiss) { console.log(`  MISSING display "${ch}"  ← ${where}`); bad++; }
  for (const [ch, where] of mMiss) {
    if (MONO_KNOWN_FALLBACK.has(ch)) { console.log(`  ~ 等宽按设计回退 "${ch}"  ← ${where}`); warn++; }
    else { console.log(`  MISSING mono "${ch}"  ← ${where}`); bad++; }
  }
}

if (bad) { console.log(`font coverage FAILED: ${bad} 个字没进子集且不在「按设计回退」名单里 —— 重跑 python tools/subset-fonts.py，或改文案时顺手重新子集`); process.exit(1); }
console.log(`font coverage OK: 展示字体用字全覆盖${warn ? `（${warn} 处等宽标点按设计回退系统字体）` : ""}`);
