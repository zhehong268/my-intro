/*
 *  my-intro — 个人主页生成器
 *  ------------------------------------------------------------------
 *  整站由这一个 C++ 文件生成：内容用数据结构描述，编译运行后输出
 *  index.html 与 favicon.svg。仓库里没有手写的 HTML。
 *
 *  改内容：只需要动下面「内容数据区」。
 *  本地预览：任意 C++17 编译器编译本文件并运行，再打开 index.html。
 *  上线：push 到 main 分支，GitHub Actions 会自动编译、生成并部署到 Pages。
 *
 *  风格：亮色清爽 + 蓝紫星空 + 卡片网格 + 强动效。
 *
 *  构建管线（两步，第二步是可选增强）：
 *    1) 本文件编译成生成器并运行 → 写出 index.html / favicon.svg /
 *       404.html / robots.txt / sitemap.xml；
 *    2) 如果当前目录有 rhythm_core.wasm（由 emcc 编译 rhythm_core.cpp 得到），
 *       生成器会把它 base64 内联进 HTML，音游的谱面就由 WebAssembly 在
 *       运行时决定；没有这个文件时退回到生成器算好的同算法谱面。
 *
 *  性能约定（请勿破坏）：
 *    - 没有 JavaScript 时整站内容必须完整可读（.js 类没有加上就不锁滚动、
 *      不藏内容、不做入场动画；计数器与技能条的初值必须是最终值）
 *    - 所有动画只用 transform / opacity，保证走 GPU 合成
 *    - 不使用 backdrop-filter / background-attachment: fixed
 *    - 倾斜、聚光、视差、磁吸全部 rAF 节流，且只在精确指针设备上启用
 *    - 音游跑在单个 canvas 的 requestAnimationFrame 时间轴上，离开视口自动暂停
 *    - 背景音乐 preload="none"，首次用户交互后才加载
 *    - 触摸设备与 prefers-reduced-motion 下自动降级
 */

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "rhythm_core.hpp"

/* ==================================================================
 *  内容数据区
 * ================================================================== */

struct Profile {
    std::string name        = "遮鸿";
    std::string latin       = "ZHE HONG";
    std::string title       = "C++ 开发者";
    std::string tagline     = "写代码，也看番。";
    std::string bio         = "在读学生，在学 C++，目前还在打基础，写点小项目练手。平时喜欢看动漫："
                              "邦多利、JOJO、鬼灭、轻音、咒术、孤独摇滚、闺泣、Fate 等。这个主页也是练习的一部分。";
    std::string avatar      = "avatar.jpg";
    std::string coverImage  = "cover.jpg";
    std::string bgImage     = "bg.webp";
    std::string stagePlay;            /* 音游进行界面的底图：仓库里放了 stage-play.* 就用它，否则退回 bgImage */
    std::string bgMusic     = "music.mp3";
    std::string welcomeText = "欢迎来到遮鸿的个人主页";
    std::string email       = "3255484226@qq.com";
    std::string qq          = "3255484226";
    std::string github      = "https://github.com/zhehong268";
    std::string site        = "https://zhehong268.github.io/my-intro/";
    std::string location    = "沈阳";
    std::string hometown    = "成都";
    std::vector<std::string> skills = {"C++", "Python", "Git", "Linux"};
};

struct Project {
    std::string title;
    std::string desc;
    std::string tech;
    std::string link;
    std::string linkText;
    std::string index;
};

struct TimelineItem {
    std::string period;
    std::string role;
    std::string org;
    std::string desc;
};

struct Skill {
    std::string name;
    std::string level;
    int percent;
};

struct Card {
    std::string icon;
    std::string title;
    std::string desc;
};

struct Anime {
    std::string title;
    std::string note;
    std::string tag;
};

struct Stat {
    int value;
    std::string label;
};

/* ==================================================================
 *  图标（内联 SVG 线性图标，无 emoji）
 * ================================================================== */

std::string svgWrap(const std::string& inner) {
    return "<svg class=\"ic\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
           "stroke-width=\"1.7\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
           + inner + "</svg>";
}

std::string icon(const std::string& name); /* 前置声明：主题脚本要用 star 图标，而它的定义在后面 */

/* ==================================================================
 *  主题开关（主页与工具页共用）
 *  首帧之前由 <head> 里的内联脚本决定 html.theme-mygo（见 kHead / toolHead），
 *  这一段只负责：把图标填进按钮、点一下换主题、记住选择、顺手同步 theme-color。
 * ================================================================== */
std::string themeRuntimeScript() {
    std::string s = "<script>\n";
    s += "window.__ICON_STAR = '" + icon("star") + "';\n";
    s += "window.__ICON_STAR_ON = '" + icon("star-on") + "';\n";
    s += R"JS((function () {
  var KEY = 'myintro.theme';
  var btn = document.getElementById('themeBtn');
  var meta = document.querySelector('meta[name="theme-color"]');
  function dark() { return document.documentElement.classList.contains('theme-mygo'); }
  function sync() {
    var d = dark();
    if (meta) meta.setAttribute('content', d ? '#07080f' : '#f6f7fc');
    if (btn) {
      btn.setAttribute('aria-pressed', d ? 'true' : 'false');
      btn.setAttribute('aria-label', d ? '切换到亮色主题' : '切换到 MyGO 舞台暗色主题');
      btn.innerHTML = d ? window.__ICON_STAR_ON : window.__ICON_STAR;
    }
    window.__theme = d ? 'mygo' : 'light';
  }
  window.__applyTheme = function (t, save) {
    document.documentElement.classList.toggle('theme-mygo', t === 'mygo');
    if (save) { try { localStorage.setItem(KEY, t === 'mygo' ? 'mygo' : 'light'); } catch (e) {} }
    sync();
    try { window.dispatchEvent(new Event('themechange')); } catch (e) {}
    return window.__theme;
  };
  if (btn) btn.addEventListener('click', function () { window.__applyTheme(dark() ? 'light' : 'mygo', true); });
  sync();
})();
)JS";
    s += "</script>\n";
    return s;
}

std::string icon(const std::string& name) {
    if (name == "mail")
        return svgWrap(R"ICON(<rect x="3" y="5" width="18" height="14" rx="2.5"/><path d="M3.6 7.6 12 13.2l8.4-5.6"/>)ICON");
    if (name == "github")
        return "<svg class=\"ic\" viewBox=\"0 0 24 24\" fill=\"currentColor\" aria-hidden=\"true\">"
               "<path d=\"M12 .5C5.37.5 0 5.87 0 12.5c0 5.3 3.44 9.8 8.21 11.39.6.11.82-.26.82-.57 "
               "0-.28-.01-1.04-.02-2.04-3.34.73-4.04-1.61-4.04-1.61-.55-1.39-1.34-1.76-1.34-1.76-1.09-.75.08-.73.08-.73 "
               "1.2.08 1.84 1.24 1.84 1.24 1.07 1.84 2.81 1.31 3.5 1 .11-.78.42-1.31.76-1.61-2.67-.3-5.47-1.34-5.47-5.95 "
               "0-1.31.47-2.38 1.24-3.22-.13-.3-.54-1.52.11-3.18 0 0 1.01-.32 3.3 1.23a11.5 11.5 0 0 1 6 0c2.29-1.55 3.3-1.23 "
               "3.3-1.23.65 1.66.24 2.88.12 3.18.77.84 1.24 1.91 1.24 3.22 0 4.62-2.81 5.64-5.49 5.94.43.37.81 1.1.81 2.22 "
               "0 1.6-.01 2.9-.01 3.29 0 .31.21.69.83.57A12.01 12.01 0 0 0 24 12.5C24 5.87 18.63.5 12 .5Z\"/></svg>";
    if (name == "qq")
        return svgWrap(R"ICON(<path d="M12 3c2.8 0 4.6 2 4.6 4.6 0 .7-.1 1.4-.3 2 1.3 1.1 2.2 2.9 2.2 4.8 0 1.6-.7 2.5-1.5 2.5-.6 0-1-.5-1.3-1.2-.4 1-1.4 1.9-2.6 2.4l.9 1.6c.3.5-.1 1.1-.7 1.1H10.7c-.6 0-1-.6-.7-1.1l.9-1.6c-1.3-.5-2.2-1.4-2.6-2.4-.3.7-.7 1.2-1.3 1.2-.8 0-1.5-.9-1.5-2.5 0-1.9.9-3.7 2.2-4.8-.2-.6-.3-1.3-.3-2C7.4 5 9.2 3 12 3Z"/>)ICON");
    if (name == "music")
        return svgWrap(R"ICON(<path d="M9 17.5V6.2l10-1.7v11"/><circle cx="6.5" cy="17.5" r="2.6"/><circle cx="16.5" cy="15.8" r="2.6"/>)ICON");
    if (name == "music-off")
        return svgWrap(R"ICON(<path d="M9 17.5V6.2l10-1.7v11"/><circle cx="6.5" cy="17.5" r="2.6"/><path d="M4 4l16 16"/>)ICON");
    if (name == "arrow-up")
        return svgWrap(R"ICON(<path d="M12 19V5.5"/><path d="M5.5 12 12 5.5 18.5 12"/>)ICON");
    if (name == "external")
        return svgWrap(R"ICON(<path d="M14.5 4.5H19.5V9.5"/><path d="M19.5 4.5 11 13"/><path d="M19 14.5v4a1.5 1.5 0 0 1-1.5 1.5h-12A1.5 1.5 0 0 1 4 18.5v-12A1.5 1.5 0 0 1 5.5 5h4"/>)ICON");
    if (name == "code")
        return svgWrap(R"ICON(<path d="M9.5 8 5.5 12l4 4"/><path d="M14.5 8l4 4-4 4"/>)ICON");
    if (name == "cpu")
        return svgWrap(R"ICON(<rect x="7" y="7" width="10" height="10" rx="2"/><path d="M4 10h3M4 14h3M17 10h3M17 14h3M10 4v3M14 4v3M10 17v3M14 17v3"/>)ICON");
    if (name == "layers")
        return svgWrap(R"ICON(<path d="M12 3 3 8l9 5 9-5-9-5Z"/><path d="M3 13l9 5 9-5"/>)ICON");
    if (name == "git")
        return svgWrap(R"ICON(<circle cx="6.5" cy="6" r="2.5"/><circle cx="6.5" cy="18" r="2.5"/><circle cx="17.5" cy="9" r="2.5"/><path d="M6.5 8.5v7"/><path d="M17.5 11.5c0 3-2.6 4-5.5 4H6.5"/>)ICON");
    if (name == "terminal")
        return svgWrap(R"ICON(<rect x="3" y="4.5" width="18" height="15" rx="2.5"/><path d="M7.5 10 10 12.5 7.5 15"/><path d="M12.5 15.5H16.5"/>)ICON");
    if (name == "book")
        return svgWrap(R"ICON(<path d="M4.5 5.5A1.5 1.5 0 0 1 6 4h13v16H6a1.5 1.5 0 0 1-1.5-1.5v-13Z"/><path d="M9 4v16"/>)ICON");
    if (name == "activity")
        return svgWrap(R"ICON(<path d="M3 12.5h3.5L9.5 5l4.5 14 2.6-6.5H21"/>)ICON");
    if (name == "film")
        return svgWrap(R"ICON(<rect x="3" y="4" width="18" height="16" rx="2.5"/><path d="M8 4v16M16 4v16M3 9h5M3 15h5M16 9h5M16 15h5"/>)ICON");
    if (name == "sparkle")
        return svgWrap(R"ICON(<path d="M12 3.5l1.9 5.6 5.6 1.9-5.6 1.9L12 18.5l-1.9-5.6L4.5 11l5.6-1.9L12 3.5Z"/>)ICON");
    /* ★ / ☆：主题开关的标记（自绘五角星，跟官方 logo 无关） */
    if (name == "star")
        return svgWrap(R"ICON(<path d="M12 3.9 14.4 9l5.6.82-4.05 3.95 1 5.6L12 16.7l-4.95 2.67 1-5.6L4 9.82 9.6 9 12 3.9Z"/>)ICON");
    if (name == "star-on")
        return "<svg class=\"ic\" viewBox=\"0 0 24 24\" fill=\"currentColor\" aria-hidden=\"true\"><path d=\"M12 3.9 14.4 9l5.6.82-4.05 3.95 1 5.6L12 16.7l-4.95 2.67 1-5.6L4 9.82 9.6 9 12 3.9Z\"/></svg>";
    if (name == "user")
        return svgWrap(R"ICON(<circle cx="12" cy="8" r="3.8"/><path d="M4.5 20.5c0-3.9 3.4-5.8 7.5-5.8s7.5 1.9 7.5 5.8"/>)ICON");
    if (name == "pin")
        return svgWrap(R"ICON(<path d="M12 21s6.8-6.3 6.8-10.7A6.8 6.8 0 0 0 5.2 10.3C5.2 14.7 12 21 12 21Z"/><circle cx="12" cy="10.3" r="2.4"/>)ICON");
    if (name == "target")
        return svgWrap(R"ICON(<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="3.4"/><path d="M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3"/>)ICON");
    if (name == "menu")
        return svgWrap(R"ICON(<path d="M4 7h16"/><path d="M4 12h16"/><path d="M4 17h16"/>)ICON");
    if (name == "close")
        return svgWrap(R"ICON(<path d="M6 6l12 12"/><path d="M18 6 6 18"/>)ICON");
    return "";
}

/* ==================================================================
 *  小组件
 * ================================================================== */

std::string chips(const std::vector<std::string>& items, const std::string& cls) {
    std::string out;
    for (const auto& s : items)
        out += "<span class=\"" + cls + "\">" + s + "</span>";
    return out;
}

std::string buildProjects(const std::vector<Project>& list) {
    std::string out;
    for (const auto& p : list) {
        std::string linkText = p.linkText.empty() ? "查看" : p.linkText;
        std::string target   = (p.link.rfind("http", 0) == 0) ? " target=\"_blank\" rel=\"noopener\"" : "";
        out += "<article class=\"card tilt reveal\">"
               "<span class=\"card-index mono\">" + p.index + "</span>"
               "<h3 class=\"card-title\">" + p.title + "</h3>"
               "<p class=\"card-desc\">" + p.desc + "</p>"
               "<p class=\"card-tech mono\">" + p.tech + "</p>"
               "<a class=\"link-arrow\" href=\"" + p.link + "\"" + target + ">" + linkText + icon("external") + "</a>"
               "</article>";
    }
    return out;
}

std::string buildTimeline(const std::vector<TimelineItem>& list) {
    std::string out = "<ol class=\"timeline\">";
    for (size_t i = 0; i < list.size(); ++i) {
        const auto& t = list[i];
        out += "<li class=\"tl-item reveal\" style=\"--d:" + std::to_string(i * 90) + "ms\">"
               "<span class=\"tl-dot\"></span>"
               "<p class=\"tl-period mono\">" + t.period + "</p>"
               "<h3 class=\"tl-role\">" + t.role + "</h3>"
               "<p class=\"tl-org\">" + t.org + "</p>"
               "<p class=\"tl-desc\">" + t.desc + "</p>"
               "</li>";
    }
    out += "</ol>";
    return out;
}

std::string buildSkills(const std::vector<Skill>& list) {
    std::string out;
    for (size_t i = 0; i < list.size(); ++i) {
        const auto& s = list[i];
        out += "<div class=\"skill reveal\" style=\"--d:" + std::to_string(i * 70) + "ms\">"
               "<div class=\"skill-head\"><span class=\"skill-name\">" + s.name + "</span>"
               "<span class=\"skill-level mono\">" + s.level + "</span></div>"
               "<div class=\"bar\"><span class=\"bar-fill\" style=\"--w:" +
               std::to_string(s.percent / 100.0) + "\"></span></div>"
               "</div>";
    }
    return out;
}

std::string buildCards(const std::vector<Card>& list) {
    std::string out;
    for (size_t i = 0; i < list.size(); ++i) {
        const auto& c = list[i];
        out += "<article class=\"card card-sm tilt reveal\" style=\"--d:" + std::to_string(i * 70) + "ms\">"
               "<span class=\"card-icon\">" + icon(c.icon) + "</span>"
               "<h3 class=\"card-title\">" + c.title + "</h3>"
               "<p class=\"card-desc\">" + c.desc + "</p>"
               "</article>";
    }
    return out;
}

std::string buildAnime(const std::vector<Anime>& list) {
    std::string out;
    for (size_t i = 0; i < list.size(); ++i) {
        const auto& a = list[i];
        int n = static_cast<int>(i) + 1;
        std::string num = (n < 10 ? "0" : "") + std::to_string(n);
        out += "<article class=\"anime tilt reveal\" style=\"--d:" + std::to_string(i * 55) + "ms\">"
               "<span class=\"anime-num mono\">" + num + "</span>"
               "<div class=\"anime-body\">"
               "<h3 class=\"anime-title\">" + a.title + "</h3>"
               "<p class=\"anime-note\">" + a.note + "</p>"
               "</div>"
               "<span class=\"anime-tag mono\">" + a.tag + "</span>"
               "</article>";
    }
    return out;
}

std::string buildStats(const std::vector<Stat>& list) {
    std::string out;
    for (size_t i = 0; i < list.size(); ++i) {
        const auto& s = list[i];
        out += "<div class=\"stat reveal\" style=\"--d:" + std::to_string(i * 80) + "ms\">"
               /* 初值就是最终值：没有 JS 时也显示 5/3/9/8，而不是 0 */
               "<span class=\"stat-num mono\" data-count=\"" + std::to_string(s.value) + "\">" +
               std::to_string(s.value) + "</span>"
               "<span class=\"stat-k mono\">" + s.label + "</span>"
               "</div>";
    }
    return out;
}

std::string buildTools(const std::vector<std::string>& list) {
    std::string out;
    for (const auto& t : list)
        out += "<li class=\"tool mono\"><span class=\"tool-dot\"></span>" + t + "</li>";
    return out;
}

/* ==================================================================
 *  样式（亮色）
 * ================================================================== */

const char* kCSS = R"CSS(
/* 自托管字体（OFL 授权，子集化后仅 53 KB，明细见 assets/fonts/README.md）：
   展示字体用霞鹜文楷 Medium（只子集了标题/引言实际用到的 60 字），
   等宽用 JetBrains Mono Regular/Bold（子集化到页面实际出现的 96 个字符）。
   font-display:swap：字体没到位前用回退字体渲染，不阻塞首屏。 */
@font-face{font-family:'LxgwWenKai';src:url('assets/fonts/lxgw-wenkai-display.woff2') format('woff2');font-weight:400 900;font-style:normal;font-display:swap}
@font-face{font-family:'JetBrains Mono';src:url('assets/fonts/jetbrains-mono-regular.woff2') format('woff2');font-weight:400;font-style:normal;font-display:swap}
@font-face{font-family:'JetBrains Mono';src:url('assets/fonts/jetbrains-mono-bold.woff2') format('woff2');font-weight:700;font-style:normal;font-display:swap}
:root{
  --bg:#f6f7fc;
  --ink:#151a2e;
  --ink-dim:rgba(21,26,46,.74);
  --ink-faint:rgba(21,26,46,.62);
  --violet:#6d4dfa;
  --violet-soft:#8b74ff;
  --blue:#3b82f6;
  --sky:#0ea5e9;
  --pink:#c026d3;
  --line:rgba(21,26,46,.09);
  --line-strong:rgba(21,26,46,.16);
  --panel:#ffffff;
  --panel-2:linear-gradient(180deg,#ffffff 0%,#f3f5fd 55%,#eaeefb 100%);
  --bg-top:#fbfcff;
  --bg-mid:#f4f6fd;
  --bg-bot:#eef1fa;
  --dot:rgba(21,26,46,.055);
  --bar-track:rgba(21,26,46,.07);\r\n  /* 半透明顶栏（含小屏下拉面板）与按钮 hover 底色：暗色主题各自换掉 */\r\n  --nav-solid:rgba(252,253,255,.94);\r\n  --panel-hover:#fbfbff;
  --sh-1:rgba(60,70,140,.45);
  --sh-2:rgba(60,70,140,.5);
  --sh-3:rgba(60,70,140,.55);
  --sh-4:rgba(60,70,140,.65);
  --sh-5:rgba(60,70,140,.7);
  --sh-deep:rgba(40,50,120,.55);
  --sh-deep2:rgba(40,50,120,.65);
  --ring:rgba(21,26,46,.06);
  --ring-2:rgba(21,26,46,.02);
  /* 标题渐变色的起点（暗色主题里要换成亮色，否则渐变头一个字在暗底上看不见） */
  --grad-start:#151a2e;
  --ok-ink:#0f766e;
  --bad-ink:#b91c1c;
  --ease:cubic-bezier(.22,.61,.36,1);
  --mono:'JetBrains Mono','Cascadia Mono',Consolas,'SF Mono',Menlo,monospace;
  --sans:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Hiragino Sans GB','Microsoft YaHei',sans-serif;
  --display:'LxgwWenKai','LXGW WenKai','Kaiti SC','KaiTi','STKaiti',serif;
}
*,*::before,*::after{margin:0;padding:0;box-sizing:border-box}
html{scroll-behavior:smooth}
body{
  font-family:var(--sans);background:var(--bg);color:var(--ink);
  line-height:1.75;-webkit-font-smoothing:antialiased;overflow-x:hidden;
}
body.locked{overflow:hidden}
a{color:inherit;text-decoration:none}
::selection{background:rgba(109,77,250,.22)}
:focus-visible{outline:2px solid var(--violet);outline-offset:3px;border-radius:6px}
.mono{font-family:var(--mono);font-variant-numeric:tabular-nums}
.ic{width:18px;height:18px;flex:none}
/* 只有 JS 才填得出来的位置：没有 JS 时不要留下 --:--:-- 这类占位 */
html:not(.js) .js-only{display:none}
/* 键盘用户的跳转链接：只在获得焦点时露出来 */
.skip{
  position:fixed;left:12px;top:-64px;z-index:400;padding:10px 18px;border-radius:10px;
  background:var(--panel);border:1px solid var(--line-strong);font-size:13px;font-weight:600;
  box-shadow:0 16px 34px -20px var(--sh-5);transition:top .25s var(--ease);
}
.skip:focus{top:12px}

/* ---------- 背景层（固定，仅 transform 动） ---------- */
.bg-fx{position:fixed;inset:0;overflow:hidden;pointer-events:none;z-index:0;
  background:linear-gradient(180deg,var(--bg-top) 0%,var(--bg-mid) 42%,var(--bg-bot) 100%)}
.bg-grid{position:absolute;inset:0;opacity:.5;
  background-image:radial-gradient(var(--dot) 1px,transparent 1px);
  background-size:26px 26px;
  -webkit-mask-image:radial-gradient(120% 80% at 50% 0%,#000 25%,transparent 75%);
          mask-image:radial-gradient(120% 80% at 50% 0%,#000 25%,transparent 75%)}
.aurora{position:absolute;border-radius:50%;will-change:transform}
.aurora i{display:block;width:100%;height:100%;border-radius:50%;will-change:transform}
.aurora.a1{width:58vw;height:58vw;left:-16vw;top:-22vw}
.aurora.a1 i{background:radial-gradient(circle at 50% 50%,rgba(109,77,250,.30),rgba(109,77,250,0) 62%);
  animation:drift1 30s ease-in-out infinite alternate}
.aurora.a2{width:50vw;height:50vw;right:-14vw;top:10vh}
.aurora.a2 i{background:radial-gradient(circle at 50% 50%,rgba(14,165,233,.26),rgba(14,165,233,0) 62%);
  animation:drift2 38s ease-in-out infinite alternate}
.aurora.a3{width:40vw;height:40vw;left:24vw;bottom:-16vw}
.aurora.a3 i{background:radial-gradient(circle at 50% 50%,rgba(192,38,211,.16),rgba(192,38,211,0) 62%);
  animation:drift3 46s ease-in-out infinite alternate}
@keyframes drift1{from{transform:translate3d(0,0,0)}to{transform:translate3d(6vw,5vh,0)}}
@keyframes drift2{from{transform:translate3d(0,0,0)}to{transform:translate3d(-6vw,7vh,0)}}
@keyframes drift3{from{transform:translate3d(0,0,0)}to{transform:translate3d(5vw,-6vh,0)}}
.spotlight{
  position:fixed;left:0;top:0;width:560px;height:560px;margin:-280px 0 0 -280px;
  border-radius:50%;pointer-events:none;z-index:1;will-change:transform;
  background:radial-gradient(circle,rgba(109,77,250,.14),rgba(109,77,250,0) 68%);
}

/* ---------- 顶部导航 ---------- */
.progress{
  position:fixed;top:0;left:0;right:0;height:2px;z-index:90;
  background:linear-gradient(90deg,var(--violet),var(--blue) 55%,var(--sky));
  transform:scaleX(0);transform-origin:0 50%;will-change:transform;
}
.nav{
  position:fixed;top:0;left:0;right:0;z-index:80;
  display:flex;align-items:center;gap:18px;
  padding:16px clamp(16px,4vw,40px);
  background:rgba(246,247,252,0);border-bottom:1px solid transparent;
  transform:translate3d(0,-110%,0);
  transition:transform .7s var(--ease),background .4s ease,border-color .4s ease,padding .4s ease;
  will-change:transform;
}
.nav.visible{transform:translate3d(0,0,0)}
.nav.solid{background:var(--nav-solid);border-bottom-color:var(--line);padding-top:10px;padding-bottom:10px}
.brand{display:flex;align-items:center;gap:10px;font-weight:700;letter-spacing:.5px;white-space:nowrap}
.brand svg{width:26px;height:26px}
.brand-sub{font-size:11px;letter-spacing:2px;color:var(--ink-faint);font-weight:500}
.nav-links{display:flex;align-items:center;gap:4px;margin-left:auto;
  overflow-x:auto;scrollbar-width:none;-ms-overflow-style:none}
.nav-links::-webkit-scrollbar{display:none}
.nav-links a{
  position:relative;padding:7px 13px;border-radius:999px;font-size:14px;
  color:var(--ink-dim);white-space:nowrap;transition:color .25s ease,background .25s ease;
}
.nav-links a:hover{color:var(--ink);background:rgba(109,77,250,.08)}
.nav-links a.active{color:var(--violet);background:rgba(109,77,250,.12);font-weight:600}
.nav-tools{display:flex;align-items:center;gap:8px;flex:none}
#viz{width:96px;height:26px;display:block;opacity:0;transition:opacity .4s ease}
#viz.on{opacity:.95}
.icon-btn{
  width:38px;height:38px;border-radius:50%;display:grid;place-items:center;cursor:pointer;
  border:1px solid var(--line-strong);background:var(--panel);color:var(--violet);
  box-shadow:0 6px 16px -10px var(--sh-2);
  transition:color .25s ease,background .25s ease,transform .25s ease;
}
.icon-btn:hover{background:rgba(109,77,250,.10);transform:translateY(-2px)}
.icon-btn.off{color:var(--ink-faint)}
/* 汉堡按钮：桌面隐藏，≤620px 才出现 */
.nav-toggle{display:none}

/* ---------- 首屏封面 ---------- */
.landing{
  position:fixed;inset:0;z-index:120;display:grid;place-items:center;
  text-align:center;padding:32px 20px;overflow:hidden;
  background:var(--panel-2);
  transition:opacity .85s ease,visibility .85s ease;
}
.landing.hidden{opacity:0;visibility:hidden;pointer-events:none}
/* 没有 JS 就没有入口门：直接看内容，也不要锁住滚动 */
html:not(.js) .landing{display:none}
html.entry body{overflow:hidden}
.landing::before{
  content:'';position:absolute;width:120vw;height:80vw;left:-10vw;top:-30vw;
  background:radial-gradient(circle at 50% 50%,rgba(109,77,250,.16),rgba(109,77,250,0) 60%);
}
.landing::after{
  content:'';position:absolute;width:80vw;height:80vw;right:-24vw;bottom:-40vw;
  background:radial-gradient(circle at 50% 50%,rgba(14,165,233,.15),rgba(14,165,233,0) 60%);
}
.landing-inner{position:relative;z-index:2;max-width:620px;width:100%}
.landing-photo{
  position:relative;border-radius:22px;overflow:hidden;
  box-shadow:0 40px 80px -40px var(--sh-deep),0 0 0 1px var(--ring);
  transform-style:preserve-3d;
}
.landing-photo img{display:block;width:100%;height:auto;aspect-ratio:16/9;object-fit:cover}
.landing-latin{margin-top:30px;font-family:var(--mono);font-size:11.5px;letter-spacing:7px;color:var(--ink-faint)}
.landing-proof{margin-top:22px;font-family:var(--mono);font-size:11.5px;letter-spacing:.06em;color:var(--ink-faint)}
.landing-proof.bad{color:var(--bad-ink)}
.landing-title{
  margin-top:12px;font-family:var(--display);font-size:clamp(26px,5vw,44px);font-weight:800;letter-spacing:1.5px;line-height:1.3;
  background:linear-gradient(110deg,var(--grad-start) 0%,var(--violet) 52%,var(--sky) 100%);
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent;
}
.landing-sub{margin-top:10px;color:var(--ink-dim);font-size:15px;font-family:var(--display);letter-spacing:1px}
.enter-btn{
  margin-top:32px;display:inline-flex;align-items:center;gap:10px;
  padding:15px 42px;border-radius:999px;cursor:pointer;border:none;
  font-size:15px;font-weight:650;letter-spacing:3px;color:#fff;
  background:linear-gradient(110deg,var(--violet),var(--blue));
  box-shadow:0 18px 34px -16px rgba(109,77,250,.75);
  transition:box-shadow .35s ease,transform .35s var(--ease);
}
.enter-btn:hover{box-shadow:0 24px 44px -16px rgba(109,77,250,.9)}
.enter-hint{margin-top:20px;font-family:var(--mono);font-size:11.5px;letter-spacing:1.2px;color:var(--ink-faint)}
.landing-fade{opacity:0;transform:translate3d(0,20px,0);animation:landIn .9s var(--ease) forwards}
@keyframes landIn{to{opacity:1;transform:none}}

/* ---------- 签名加载 ---------- */
.loader{
  position:fixed;inset:0;z-index:300;display:none;place-items:center;
  background:var(--bg-top);opacity:0;transition:opacity .45s ease;cursor:pointer;
}
.loader.show{display:grid;opacity:1}
.sig{width:88%;max-width:600px;height:auto;overflow:visible}
.sig text{
  font-family:'Brush Script MT','Segoe Script','Snell Roundhand',cursive;
  font-size:70px;font-weight:700;letter-spacing:2px;
  fill:none;stroke:var(--violet);stroke-width:2.2;
  stroke-linecap:round;stroke-linejoin:round;
  stroke-dasharray:2600;stroke-dashoffset:2600;
  animation:draw .62s cubic-bezier(.45,0,.3,1) forwards;
}
.sig line{stroke:var(--sky);stroke-width:2;stroke-linecap:round;opacity:.7;
  stroke-dasharray:420;stroke-dashoffset:420;animation:draw .3s cubic-bezier(.45,0,.3,1) .18s forwards}
.loader-hint{
  position:absolute;left:0;right:0;bottom:34px;text-align:center;
  font-family:var(--mono);font-size:11.5px;letter-spacing:2.4px;color:var(--ink-faint);
  opacity:0;animation:fadeIn .3s ease .25s forwards;
}
@keyframes fadeIn{to{opacity:1}}
@keyframes draw{to{stroke-dashoffset:0}}

/* ---------- 页面容器 ---------- */
/* 默认可见：入场淡入只在 JS 档里做，脚本挂了内容也照样读得到 */
.page{position:relative;z-index:2;opacity:1;transition:opacity .9s ease;
  padding:0 clamp(16px,4vw,40px)}
html.js .page{opacity:0}
html.js .page.visible{opacity:1}
.section{max-width:1080px;margin:0 auto;padding:76px 0;scroll-margin-top:90px}
.section-head{margin-bottom:36px}
.eyebrow{display:inline-flex;align-items:center;gap:9px;
  font-family:var(--mono);font-size:11.5px;letter-spacing:3px;color:var(--violet);margin-bottom:14px}
.eyebrow::before{content:'';width:26px;height:1px;background:linear-gradient(90deg,var(--violet),transparent)}
.section-title{
  font-family:var(--display);font-size:clamp(23px,3.4vw,32px);font-weight:750;letter-spacing:.5px;
  background:linear-gradient(110deg,var(--grad-start) 0%,var(--violet) 70%,var(--sky) 100%);
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent;
}
.section-desc{margin-top:12px;color:var(--ink-dim);font-size:14.5px;max-width:64ch}

/* ---------- 入场动画 ---------- */
.reveal{opacity:1;transform:none}
html.js .reveal{
  opacity:0;transform:translate3d(0,38px,0) scale(.985);
  transition:opacity .8s var(--ease),transform .9s var(--ease);
  transition-delay:var(--d,0ms);
}
html.js .reveal.in{opacity:1;transform:none}

/* ---------- 通用卡片 ---------- */
.card{
  position:relative;background:var(--panel);border:1px solid var(--line);border-radius:18px;
  padding:26px 24px;box-shadow:0 14px 34px -24px var(--sh-1);
  transition:transform .45s var(--ease),border-color .45s ease;
}
.card::after{
  content:'';position:absolute;inset:-1px;border-radius:18px;pointer-events:none;
  box-shadow:0 0 0 1px rgba(109,77,250,.55),0 30px 55px -26px rgba(109,77,250,.6);
  opacity:0;transition:opacity .45s ease;
}
.card:hover{transform:translate3d(0,-6px,0);border-color:transparent}
.card:hover::after{opacity:1}
.card-index{display:block;font-size:11.5px;letter-spacing:2px;color:var(--violet);margin-bottom:12px}
.card-icon{display:inline-flex;color:var(--violet);margin-bottom:14px}
.card-icon .ic{width:22px;height:22px}
.card-title{font-size:17px;font-weight:650;margin-bottom:9px}
.card-desc{font-size:14px;color:var(--ink-dim);line-height:1.8}
.card-tech{margin-top:16px;padding-top:14px;border-top:1px solid var(--line);
  font-size:11.5px;letter-spacing:.6px;color:var(--ink-faint)}
.link-arrow{display:inline-flex;align-items:center;gap:7px;margin-top:14px;
  font-size:13.5px;font-weight:600;color:var(--violet);
  transition:gap .3s var(--ease),color .3s ease}
.link-arrow:hover{gap:11px;color:var(--blue)}
.link-arrow .ic{width:15px;height:15px}
.grid{display:grid;gap:20px}
.grid-3{grid-template-columns:repeat(auto-fit,minmax(268px,1fr))}
.grid-4{grid-template-columns:repeat(auto-fit,minmax(228px,1fr))}

/* ---------- 主视觉 ---------- */
.hero{padding-top:122px;padding-bottom:8px}
.hero-card{
  position:relative;max-width:640px;margin:0 auto;text-align:center;
  background:var(--panel);border:1px solid var(--line);border-radius:26px;
  padding:44px 34px 34px;
  box-shadow:0 40px 90px -60px var(--sh-deep2),0 0 0 1px var(--ring-2);
}
.hero-card::before{
  content:'';position:absolute;inset:0;border-radius:26px;pointer-events:none;
  background:linear-gradient(180deg,rgba(109,77,250,.07),rgba(109,77,250,0) 44%);
}
.avatar-ring{
  position:relative;width:132px;height:132px;margin:0 auto 20px;border-radius:50%;padding:3px;
  background:linear-gradient(135deg,var(--violet),var(--blue) 55%,var(--sky));
  box-shadow:0 18px 34px -20px rgba(109,77,250,.85);
}
.avatar-ring::after{content:'';position:absolute;inset:-8px;border-radius:50%;
  border:1px solid rgba(109,77,250,.18)}
.avatar-ring img{width:100%;height:100%;border-radius:50%;object-fit:cover;display:block;
  background:var(--bg-bot);cursor:pointer}
.avatar-ring img.pop{animation:avatarPop .7s cubic-bezier(.34,1.56,.64,1)}
@keyframes avatarPop{
  0%{transform:scale(1) rotate(0)}
  25%{transform:scale(1.16) rotate(-6deg)}
  50%{transform:scale(.94) rotate(5deg)}
  75%{transform:scale(1.06) rotate(-2deg)}
  100%{transform:scale(1) rotate(0)}
}
.hero-name{font-size:clamp(26px,4vw,36px);font-weight:800;letter-spacing:2px;font-family:var(--display);
  background:linear-gradient(110deg,var(--grad-start),var(--violet) 58%,var(--sky));
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}
.hero-title{margin-top:6px;color:var(--violet);font-weight:600;letter-spacing:2px;font-size:15px}
.hero-clock{margin-top:20px;font-size:26px;font-weight:700;letter-spacing:7px;color:var(--violet)}
.hero-status{margin-top:6px;font-size:13px;letter-spacing:1.5px;color:var(--ink-faint)}
.hero-bio{margin:24px auto 0;max-width:44ch;font-size:15px;line-height:1.9;color:var(--ink-dim);text-align:left}
.hero-meta{display:flex;flex-wrap:wrap;justify-content:center;gap:10px 26px;
  margin:24px 0 0;padding:18px 0;border-top:1px solid var(--line);border-bottom:1px solid var(--line)}
.hero-meta div{display:flex;align-items:baseline;gap:8px;font-size:13px}
.hero-meta dt{color:var(--ink-faint);letter-spacing:1px}
.hero-meta dd{color:var(--ink);font-weight:600}
.chips{display:flex;flex-wrap:wrap;justify-content:center;gap:9px;margin-top:24px}
.chip{padding:7px 17px;border-radius:999px;font-size:13px;font-weight:500;
  color:var(--violet);background:rgba(109,77,250,.08);border:1px solid rgba(109,77,250,.24);
  transition:transform .3s var(--ease),background .3s ease,border-color .3s ease}
.chip:hover{transform:translateY(-3px);background:rgba(109,77,250,.16);border-color:rgba(109,77,250,.5)}
.hero-actions{display:flex;justify-content:center;flex-wrap:wrap;gap:12px;margin-top:28px}
.btn{display:inline-flex;align-items:center;gap:9px;padding:11px 22px;border-radius:12px;
  font-size:14px;font-weight:600;border:1px solid var(--line-strong);background:var(--panel);
  transition:transform .3s var(--ease),border-color .3s ease,background .3s ease}
.btn:hover{transform:translateY(-3px);background:var(--panel-hover);border-color:rgba(109,77,250,.5)}
.btn-primary{color:#fff;border:none;background:linear-gradient(110deg,var(--violet),var(--blue));
  box-shadow:0 14px 28px -14px rgba(109,77,250,.8)}
.btn-primary:hover{background:linear-gradient(110deg,var(--violet),var(--blue))}
.hero-note{margin-top:26px;font-size:11.5px;letter-spacing:1px;color:var(--ink-faint)}

/* ---------- 数字条 ---------- */
.stats{
  display:flex;justify-content:center;flex-wrap:wrap;gap:18px 54px;
  max-width:780px;margin:36px auto 0;padding:24px 20px;
  border-radius:18px;background:var(--panel);border:1px solid var(--line);
  box-shadow:0 20px 46px -34px var(--sh-3);
}
.stat{display:flex;flex-direction:column;align-items:center;min-width:84px}
.stat-num{font-size:34px;font-weight:800;line-height:1.1;
  background:linear-gradient(140deg,var(--violet),var(--sky));
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}
.stat-k{font-size:11px;letter-spacing:2px;color:var(--ink-faint);margin-top:4px}

/* ---------- 关于我 ---------- */
.about-wrap{display:grid;grid-template-columns:1.35fr 1fr;gap:34px;align-items:start}
.about-text p{color:var(--ink-dim);font-size:15px;margin-bottom:16px}
.about-text p:last-child{margin-bottom:0}
.about-text b{color:var(--ink);font-weight:650}
.facts{background:var(--panel);border:1px solid var(--line);border-radius:18px;padding:8px 20px;
  box-shadow:0 20px 46px -36px var(--sh-2)}
.fact{display:flex;align-items:center;gap:12px;padding:14px 0;border-bottom:1px solid var(--line)}
.fact:last-child{border-bottom:none}
.fact .ic{color:var(--violet)}
.fact-k{font-size:12.5px;color:var(--ink-faint);letter-spacing:1px;width:64px;flex:none}
.fact-v{font-size:14px;font-weight:600}
.quote{margin-top:30px;padding:20px 24px;border-left:3px solid var(--violet);
  background:linear-gradient(90deg,rgba(109,77,250,.07),rgba(109,77,250,0) 72%);
  border-radius:0 12px 12px 0}
.quote p{font-size:16px;color:var(--ink);letter-spacing:.5px;font-family:var(--display);font-weight:600}
.quote span{display:block;margin-top:8px;font-size:12px;color:var(--ink-faint);font-family:var(--mono)}

/* ---------- 经历时间线 ---------- */
.timeline{list-style:none;position:relative;max-width:760px;padding-left:26px}
.timeline::before{content:'';position:absolute;left:3px;top:6px;bottom:6px;width:1px;
  background:linear-gradient(180deg,var(--violet),var(--sky),transparent)}
.tl-item{position:relative;padding:0 0 34px 22px}
.tl-item:last-child{padding-bottom:0}
.tl-dot{position:absolute;left:-26px;top:7px;width:9px;height:9px;border-radius:50%;
  background:var(--violet);box-shadow:0 0 0 4px rgba(109,77,250,.14)}
.tl-period{font-size:12px;letter-spacing:1.6px;color:var(--violet);margin-bottom:6px}
.tl-role{font-size:17.5px;font-weight:650}
.tl-org{font-size:13px;color:var(--ink-faint);margin-top:3px}
.tl-desc{margin-top:10px;font-size:14.5px;color:var(--ink-dim);max-width:56ch}

/* ---------- 技能 ---------- */
.skills-wrap{display:grid;grid-template-columns:1.25fr 1fr;gap:38px;align-items:start}
.skill{margin-bottom:22px}
.skill:last-child{margin-bottom:0}
.skill-head{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:9px}
.skill-name{font-size:14.5px;font-weight:600}
.skill-level{font-size:11px;letter-spacing:2px;color:var(--ink-faint)}
.bar{height:6px;border-radius:99px;background:var(--bar-track);overflow:hidden}
.bar-fill{display:block;height:100%;border-radius:99px;
  background:linear-gradient(90deg,var(--violet),var(--blue) 60%,var(--sky));
  transform:scaleX(var(--w,1));transform-origin:0 50%;
  transition:transform 1.15s var(--ease)}
/* 只有跑得动 JS 的浏览器才从 0 长出来；没 JS 时直接是最终长度 */
html.js .bar-fill{transform:scaleX(0)}
html.js .bar-fill.grow{transform:scaleX(var(--w,1))}
.tools-title{font-size:12px;letter-spacing:2.5px;color:var(--ink-faint);margin-bottom:14px}
.tools{list-style:none;display:flex;flex-wrap:wrap;gap:10px}
.tool{display:inline-flex;align-items:center;gap:8px;padding:8px 14px;border-radius:10px;
  font-size:12.5px;color:var(--ink-dim);background:var(--panel);border:1px solid var(--line);
  transition:color .3s ease,border-color .3s ease,transform .3s var(--ease)}
.tool:hover{color:var(--ink);border-color:rgba(109,77,250,.45);transform:translateY(-2px)}
.tool-dot{width:5px;height:5px;border-radius:50%;background:var(--violet);flex:none}

/* ---------- 番剧清单 ---------- */
.anime-list{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:14px}
.anime{position:relative;display:flex;align-items:flex-start;gap:16px;padding:20px 22px;
  border-radius:16px;background:var(--panel);border:1px solid var(--line);
  box-shadow:0 14px 34px -28px var(--sh-2);
  transition:transform .45s var(--ease),border-color .45s ease}
.anime:hover{transform:translate3d(0,-5px,0);border-color:rgba(109,77,250,.5)}
.anime-num{font-size:20px;font-weight:700;line-height:1.3;flex:none;
  background:linear-gradient(160deg,var(--violet),var(--sky));
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent;opacity:.9}
.anime-body{flex:1;min-width:0}
.anime-title{font-size:15.5px;font-weight:650;letter-spacing:.3px}
.anime-note{margin-top:6px;font-size:13.5px;color:var(--ink-dim)}
.anime-tag{flex:none;font-size:10.5px;letter-spacing:1.4px;color:var(--ink-faint);
  border:1px solid var(--line);border-radius:999px;padding:4px 10px;white-space:nowrap}

/* ---------- 音游（深色演出屏） ---------- */
.rhythm-wrap{
  position:relative;height:420px;max-width:700px;margin:0 auto;border-radius:20px;overflow:hidden;
  background:#0b1020;border:1px solid rgba(109,77,250,.3);
  box-shadow:0 46px 80px -46px rgba(30,40,110,.75);
}
.rhythm-bg{position:absolute;inset:0;background:url('__STAGE_PLAY__') center/cover no-repeat;opacity:.42}
.rhythm-bg::after{content:'';position:absolute;inset:0;
  background:linear-gradient(180deg,rgba(11,16,32,.72),rgba(11,16,32,.93))}
.rhythm-wrap canvas{position:relative;display:block;width:100%;height:100%}
/* 内核状态徽标：告诉看的人这一屏的谱面到底是谁算的 */
.kernel-badge{
  display:inline-flex;align-items:center;gap:8px;margin-top:16px;padding:7px 14px;
  border-radius:10px;background:rgba(109,77,250,.08);border:1px solid rgba(109,77,250,.20);
  font-size:11.5px;letter-spacing:1.2px;color:var(--violet);
}
.kernel-badge.ok{background:rgba(14,165,233,.10);border-color:rgba(14,165,233,.26);color:#0369a1}
.rhythm-overlay{position:absolute;inset:0;pointer-events:none}
.combo{position:absolute;top:22px;left:50%;transform:translate3d(-50%,0,0);text-align:center}
.combo-num{display:block;font-family:var(--mono);font-size:52px;font-weight:800;line-height:1;
  color:#7dd3fc;text-shadow:0 0 26px rgba(125,211,252,.55)}
.combo-num.pop{animation:comboPop .28s ease}
@keyframes comboPop{0%{transform:scale(1)}45%{transform:scale(1.28)}100%{transform:scale(1)}}
.combo-label{font-size:10.5px;letter-spacing:6px;color:rgba(125,211,252,.62);font-family:var(--mono)}
.rhythm-tag{position:absolute;left:16px;bottom:14px;font-size:11px;letter-spacing:2px;
  color:rgba(233,237,247,.42);font-family:var(--mono)}
.rhythm-tag.right{left:auto;right:16px}
.rhythm-judge{position:absolute;left:50%;top:40%;transform:translate3d(-50%,0,0);
  font-size:19px;font-weight:800;letter-spacing:3px;opacity:0;color:#7dd3fc;
  text-shadow:0 0 18px rgba(125,211,252,.5)}
.rhythm-judge.perfect{animation:judgePop .52s ease}
.rhythm-judge.good{color:#a5b4fc;animation:judgePop .52s ease}
.rhythm-judge.miss{color:#fb7185;text-shadow:0 0 18px rgba(251,113,133,.45);animation:judgePop .52s ease}
@keyframes judgePop{
  0%{opacity:0;transform:translate3d(-50%,10px,0) scale(.92)}
  30%{opacity:1;transform:translate3d(-50%,0,0) scale(1.05)}
  100%{opacity:0;transform:translate3d(-50%,-12px,0) scale(1)}
}

/* ---------- 音游：可玩模式的控制条 / 统计 / 结算 ---------- */
[hidden]{display:none!important}
.rhythm-ctl{display:flex;flex-wrap:wrap;align-items:center;justify-content:center;gap:12px;
  max-width:700px;margin:22px auto 0}
.rbtn{
  font:inherit;font-size:14px;font-weight:600;cursor:pointer;padding:11px 20px;border-radius:12px;
  border:1px solid rgba(109,77,250,.26);background:var(--panel);color:var(--ink);
  transition:transform .18s var(--ease),box-shadow .18s var(--ease),background .18s var(--ease);
}
.rbtn:hover{transform:translate3d(0,-1px,0);box-shadow:0 14px 26px -18px var(--sh-4)}
.rbtn:focus-visible{outline:2px solid var(--violet);outline-offset:2px}
.rbtn.primary{background:linear-gradient(135deg,var(--violet),var(--blue));color:#fff;border-color:transparent}
.rbtn.ghost{background:transparent;color:var(--ink-dim)}
.rhythm-hint{font-size:12px;color:var(--ink-faint);letter-spacing:.2px}
.rhythm-stats{display:flex;flex-wrap:wrap;justify-content:center;gap:8px 24px;max-width:700px;
  margin:16px auto 0;font-size:12.5px;color:var(--ink-dim)}
.rhythm-stats b{color:var(--ink);font-weight:700}
.rhythm-result{max-width:700px;margin:16px auto 0;padding:16px 20px;border-radius:12px;
  background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--sky);
  font-family:var(--mono);font-size:13px;color:var(--ink-dim);line-height:1.7;
  box-shadow:0 20px 46px -38px var(--sh-2)}
/* ?audio=debug：左下角的播放状态读数（只在带参数时才创建这个元素） */
.audio-debug{position:fixed;left:12px;bottom:12px;z-index:60;max-width:92vw;padding:8px 12px;
  border-radius:10px;background:rgba(11,16,32,.86);border:1px solid rgba(109,77,250,.35);
  color:#c7d2fe;font-size:11.5px;line-height:1.5;pointer-events:none}
@media (max-width:620px){.audio-debug{font-size:10.5px;left:8px;bottom:8px}}
/* 谱面 seed 输入：改一行数字就换一份谱面 */
.seed-row{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin:6px 0 0}
.seed-label{font-size:11.5px;letter-spacing:1.6px;color:var(--ink-faint)}
.seed-input{padding:9px 12px;border-radius:10px;border:1px solid var(--line);background:var(--panel);color:var(--ink);
  font-family:var(--mono);font-size:13px;min-width:180px;max-width:100%}
.seed-input:focus{outline:2px solid rgba(109,77,250,.35);outline-offset:1px}
.seed-info{margin-top:10px;font-size:12.5px;color:var(--ink-dim);word-break:break-all}
.site-box a{color:var(--violet);text-decoration:none;border-bottom:1px solid rgba(109,77,250,.32)}
.site-box a:hover{border-bottom-color:var(--violet)}
.footer a{color:var(--violet);text-decoration:none}
.footer a:hover{text-decoration:underline}

/* ---------- 关于本站 ---------- */
.site-box{background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--violet);
  border-radius:0 18px 18px 0;padding:30px 34px;
  box-shadow:0 20px 46px -38px var(--sh-2)}
.site-box p{color:var(--ink-dim);font-size:14.5px;margin-bottom:14px}
.site-box p:last-of-type{margin-bottom:0}
.site-box code{font-family:var(--mono);font-size:12.5px;color:var(--violet);
  background:rgba(109,77,250,.09);border:1px solid rgba(109,77,250,.2);
  padding:2px 7px;border-radius:6px}
.site-tech{margin-top:22px;padding-top:18px;border-top:1px solid var(--line)}
/* 英文摘要：给不看中文的访客一段能读懂的话；lang="en" 让朗读器换发音 */
.en-abstract{margin-top:20px;padding:16px 18px;border-radius:12px;
  background:rgba(109,77,250,.05);border-left:3px solid var(--violet)}
.en-abstract .en-tag{display:block;font-family:var(--mono);font-size:11px;letter-spacing:.16em;
  text-transform:uppercase;color:var(--violet);margin-bottom:8px}
.en-abstract p{margin-bottom:10px;font-size:13.5px;line-height:1.85;color:var(--ink-dim);
  font-family:ui-sans-serif,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
.en-abstract p:last-child{margin-bottom:0}
.en-abstract a{color:var(--violet)}

/* ---------- 联系 ---------- */
.contact-wrap{text-align:center}
.contact-row{display:flex;justify-content:center;flex-wrap:wrap;gap:14px}
.contact-card{display:flex;align-items:center;gap:12px;padding:16px 24px;border-radius:14px;
  background:var(--panel);border:1px solid var(--line);box-shadow:0 14px 34px -28px var(--sh-2);
  transition:transform .45s var(--ease),border-color .45s ease}
.contact-card:hover{transform:translate3d(0,-5px,0);border-color:rgba(109,77,250,.5)}
.contact-card .ic{color:var(--violet)}
.contact-k{display:block;font-size:11px;letter-spacing:2px;color:var(--ink-faint);text-align:left}
.contact-v{display:block;font-size:14px;font-weight:600;text-align:left}
.footer{margin-top:56px;padding:26px 0 40px;border-top:1px solid var(--line);
  display:flex;justify-content:space-between;flex-wrap:wrap;gap:12px;
  font-size:12px;color:var(--ink-faint);font-family:var(--mono);letter-spacing:.6px}

/* ---------- 点击涟漪 / 回到顶部 ---------- */
.ripple{position:fixed;width:24px;height:24px;margin:-12px 0 0 -12px;border-radius:50%;
  border:1.5px solid rgba(109,77,250,.75);pointer-events:none;z-index:95;will-change:transform,opacity;
  animation:rip .85s cubic-bezier(.2,.6,.3,1) forwards}
@keyframes rip{from{transform:scale(.4);opacity:.9}to{transform:scale(17);opacity:0}}
#toTop{position:fixed;right:22px;bottom:22px;z-index:85;width:44px;height:44px;border-radius:50%;
  display:grid;place-items:center;cursor:pointer;color:var(--violet);
  border:1px solid var(--line-strong);background:var(--panel);
  box-shadow:0 16px 30px -18px var(--sh-5);
  opacity:0;transform:translate3d(0,14px,0);pointer-events:none;
  transition:opacity .4s ease,transform .4s var(--ease),background .3s ease}
#toTop.show{opacity:1;transform:none;pointer-events:auto}
#toTop:hover{background:rgba(109,77,250,.1)}

/* ---------- 响应式 ---------- */
@media (max-width:900px){
  .about-wrap,.skills-wrap{grid-template-columns:1fr;gap:26px}
  .section{padding:58px 0}
  #viz{display:none}
  .stats{gap:16px 34px}
}
@media (max-width:620px){
  .nav{padding:12px 14px;gap:10px}
  .brand-sub{display:none}
  .nav-toggle{display:grid}
  /* 小屏导航收成下拉面板：不再横向溢出，也不需要盲滑 */
  .nav-links{
    position:absolute;top:100%;left:0;right:0;display:none;
    flex-direction:column;align-items:stretch;gap:2px;
    margin:0;padding:10px 14px 16px;overflow:visible;
    background:var(--nav-solid);border-bottom:1px solid var(--line);
    box-shadow:0 26px 40px -30px var(--sh-2);
  }
  .nav-links.open{display:flex}
  .nav-links a{padding:12px 14px;font-size:15px;border-radius:12px;text-align:left}
  .nav-links a.active{background:rgba(109,77,250,.12)}
  .hero{padding-top:100px}
  .hero-card{padding:34px 20px 26px;border-radius:22px}
  .hero-clock{font-size:21px;letter-spacing:5px}
  .hero-meta{gap:8px 16px}
  .stat-num{font-size:27px}
  .rhythm-wrap{height:320px}
  .combo-num{font-size:38px}
  .site-box{padding:22px 20px}
  .footer{justify-content:center;text-align:center}
}

/* ---------- 减弱动效 ---------- */
@media (prefers-reduced-motion:reduce){
  html{scroll-behavior:auto}
  .aurora i,.landing-photo{animation:none}
  html.js .reveal{opacity:1;transform:none;transition:none}
  .ripple,.spotlight{display:none}
  .sig text,.sig line{animation-duration:.01ms}
  .bar-fill{transition:none}
}
/* ==================================================================
 *  「MyGO 舞台」暗色主题（html.theme-mygo）
 * ------------------------------------------------------------------
 *  不是把亮色反相，而是把整页当成一个关了灯的 Live 现场：
 *    · 底色压到近黑，卡片变成舞台灯下的深色亚克力（面板色都走令牌，所以能整体换肤）；
 *    · 顶部五束成员色顶灯，纯 CSS 渐变自绘，不含任何官方素材；
 *    · 标题渐变起点从墨黑换成亮色，暗底上才读得出来。
 *  五位成员代表色取自粉丝项目 Lee-W/maigo 的 docs/reference/character-colors.md
 *  （https://github.com/Lee-W/maigo/blob/main/docs/reference/character-colors.md ），
 *  属于该项目的色票约定、非官方发布，这里只用于自绘的灯光/圆点/轨道配色。
 * ================================================================== */
html.theme-mygo{
  color-scheme:dark;
  --bg:#07080f;
  --bg-top:#0c1020;
  --bg-mid:#080a13;
  --bg-bot:#05060c;
  --ink:#e9edfb;
  --ink-dim:rgba(233,237,251,.78);
  --ink-faint:rgba(233,237,251,.64);
  --line:rgba(233,237,251,.12);
  --line-strong:rgba(233,237,251,.22);
  --panel:rgba(19,23,40,.88);
  --panel-2:linear-gradient(180deg,rgba(26,31,54,.94) 0%,rgba(17,21,36,.94) 55%,rgba(12,15,27,.96) 100%);
  --dot:rgba(233,237,251,.075);
  --bar-track:rgba(233,237,251,.12);\r\n  --nav-solid:rgba(9,11,20,.92);\r\n  --panel-hover:rgba(30,36,60,.95);
  --grad-start:#e9edfb;
  --violet:#a78bfa;
  --violet-soft:#c4b5fd;
  --blue:#60a5fa;
  --sky:#38bdf8;
  --pink:#f472b6;
  --ok-ink:#7dd3fc;
  --bad-ink:#fca5a5;
  --sh-1:rgba(0,0,0,.45);
  --sh-2:rgba(0,0,0,.5);
  --sh-3:rgba(0,0,0,.55);
  --sh-4:rgba(0,0,0,.62);
  --sh-5:rgba(0,0,0,.68);
  --sh-deep:rgba(0,0,0,.62);
  --sh-deep2:rgba(0,0,0,.72);
  --ring:rgba(255,255,255,.07);
  --ring-2:rgba(255,255,255,.03);
  /* 五位成员代表色（非官方色票，来源见上方注释） */
  --m-tomori:#6EC1E4;
  --m-anon:#FF6F91;
  --m-raana:#6EEB83;
  --m-soyo:#FFC857;
  --m-taki:#7A5CFF;
}
/* 顶灯：从画面上方压下来的五束光 */
html.theme-mygo .bg-fx{
  background:
    radial-gradient(46% 34% at 50% -6%,rgba(110,193,228,.22),rgba(110,193,228,0) 70%),
    radial-gradient(26% 40% at 10% -8%,rgba(255,111,145,.20),rgba(255,111,145,0) 72%),
    radial-gradient(24% 36% at 28% -8%,rgba(110,235,131,.16),rgba(110,235,131,0) 72%),
    radial-gradient(24% 36% at 72% -8%,rgba(255,200,87,.17),rgba(255,200,87,0) 72%),
    radial-gradient(28% 42% at 90% -8%,rgba(122,92,255,.24),rgba(122,92,255,0) 72%),
    linear-gradient(180deg,var(--bg-top) 0%,var(--bg-mid) 46%,var(--bg-bot) 100%);
}
html.theme-mygo .bg-grid{opacity:.38}
html.theme-mygo .aurora{opacity:.75}
html.theme-mygo .spotlight{background:radial-gradient(circle,rgba(110,193,228,.16),rgba(110,193,228,0) 68%)}
html.theme-mygo .progress{background:linear-gradient(90deg,var(--m-taki),var(--m-anon) 32%,var(--m-soyo) 58%,var(--m-raana) 78%,var(--m-tomori))}
html.theme-mygo .landing::before{background:radial-gradient(circle at 50% 50%,rgba(122,92,255,.24),rgba(122,92,255,0) 62%)}
html.theme-mygo .landing::after{background:radial-gradient(circle at 50% 50%,rgba(110,193,228,.2),rgba(110,193,228,0) 62%)}
/* 五色圆点 + 成员行：纯文字与色块，没有图 */
.lineup{display:flex;flex-wrap:wrap;gap:8px 18px;align-items:center;margin-top:14px;font-size:13px;color:var(--ink-dim)}
.lineup li{display:inline-flex;align-items:center;gap:8px;list-style:none}
.lineup i{width:10px;height:10px;border-radius:50%;background:var(--c,#888);box-shadow:0 0 0 3px color-mix(in srgb,var(--c,#888) 22%,transparent)}
.lineup .part{font-size:11px;letter-spacing:1px;color:var(--ink-faint)}
.diff-row{display:flex;flex-wrap:wrap;align-items:center;gap:8px;margin:12px 0 2px}
.diff-label{font-size:12.5px;color:var(--ink-dim)}
.diff-note{font-size:11px;color:var(--ink-faint)}
.diff-row .rbtn[aria-pressed="true"]{border-color:var(--violet);color:var(--violet)}
.onset-strip{margin-top:12px;padding:10px 12px 8px;border-radius:12px;background:var(--panel);border:1px solid var(--line)}
.onset-strip canvas{display:block;width:100%;height:56px}
.strip-note{display:block;margin-top:6px;font-size:11px;color:var(--ink-faint)}
.drop-zone{display:flex;flex-direction:column;gap:7px;margin:18px 0 4px;padding:15px 18px;
  border:1px dashed color-mix(in srgb,var(--violet) 35%,transparent);
  border-radius:14px;background:var(--panel);cursor:pointer;transition:border-color .2s,background .2s,box-shadow .2s}
.drop-zone:hover,.drop-zone:focus-visible{border-color:var(--violet);background:var(--panel-hover)}
.drop-zone.over{border-color:var(--violet);background:var(--panel-hover);box-shadow:0 0 0 4px color-mix(in srgb,var(--violet) 16%,transparent)}
.drop-main{font-size:13px;color:var(--ink)}
.drop-sub{font-size:11.5px;color:var(--ink-faint);line-height:1.75}
.drop-info{font-size:11.5px;color:var(--ink-dim);margin-top:9px}
.drop-info.bad{color:var(--bad-ink)}
.drop-actions{margin-top:10px}
.drop-actions button[hidden]{display:none}
.lineup-src{font-size:11.5px;color:var(--ink-faint);margin-top:8px}
.stage-note{display:inline-flex;align-items:center;gap:8px;margin-top:10px;font-size:12.5px;color:var(--ink-dim)}
.stage-note .star{color:var(--m-soyo);font-size:13px}
.foot-star{color:var(--m-soyo);margin-left:6px;font-size:11px}
/* 主题开关：跟音乐按钮同一排，图标是★/☆ */
.theme-btn{position:relative}
.theme-btn .ic{transition:transform .35s var(--ease),opacity .2s linear}
.theme-btn[aria-pressed="true"] .ic{transform:rotate(72deg)}

)CSS";

/* ==================================================================
 *  交互脚本
 * ================================================================== */

const char* kJS = R"JS(
(function () {
  'use strict';

  var $ = function (s) { return document.querySelector(s); };
  var $$ = function (s) { return Array.prototype.slice.call(document.querySelectorAll(s)); };
  var reduce = window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  var fine = window.matchMedia && window.matchMedia('(hover: hover) and (pointer: fine)').matches;

  /* ============ 时钟 ============ */
  var clockEl = $('#clock');
  function pad(n) { return (n < 10 ? '0' : '') + n; }
  function tick() {
    var d = new Date();
    clockEl.textContent = pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds());
  }
  tick();
  setInterval(tick, 1000);

  /* ============ 状态文案 ============ */
  var statusEl = $('#status');
  var STATUS = [
    [7,  '这个点还没睡？'],
    [9,  '早八人'],
    [12, '在上课'],
    [14, '干饭中'],
    [18, '在写代码'],
    [22, '在摸鱼'],
    [24, '准备睡了']
  ];
  function refreshStatus() {
    var h = new Date().getHours();
    for (var i = 0; i < STATUS.length; i++) {
      if (h < STATUS[i][0]) { statusEl.textContent = STATUS[i][1]; return; }
    }
  }
  refreshStatus();
  setInterval(refreshStatus, 60000);

  /* ============ 点击涟漪 ============ */
  if (!reduce) {
    document.addEventListener('click', function (e) {
      var r = document.createElement('span');
      r.className = 'ripple';
      r.style.left = e.clientX + 'px';
      r.style.top = e.clientY + 'px';
      document.body.appendChild(r);
      setTimeout(function () { r.remove(); }, 880);
    }, { passive: true });
  }

  /* ============ 鼠标聚光 ============ */
  if (fine && !reduce) {
    var spot = document.createElement('div');
    spot.className = 'spotlight';
    document.body.appendChild(spot);
    var px = 0, py = 0, spotRaf = 0;
    document.addEventListener('pointermove', function (e) {
      px = e.clientX; py = e.clientY;
      if (!spotRaf) {
        spotRaf = requestAnimationFrame(function () {
          spotRaf = 0;
          spot.style.transform = 'translate3d(' + px + 'px,' + py + 'px,0)';
        });
      }
    }, { passive: true });
  }

  /* ============ 滚动入场 ============ */
  var revealIO = ('IntersectionObserver' in window)
    ? new IntersectionObserver(function (entries) {
        for (var i = 0; i < entries.length; i++) {
          if (entries[i].isIntersecting) {
            entries[i].target.classList.add('in');
            revealIO.unobserve(entries[i].target);
          }
        }
      }, { threshold: 0.1, rootMargin: '0px 0px -5% 0px' })
    : null;

  function setupReveal() {
    $$('.reveal').forEach(function (el) {
      if (revealIO) revealIO.observe(el); else el.classList.add('in');
    });
  }

  /* ============ 技能条 ============ */
  function setupBars() {
    var bars = $$('.bar-fill');
    if (!('IntersectionObserver' in window)) {
      bars.forEach(function (b) { b.classList.add('grow'); });
      return;
    }
    var io = new IntersectionObserver(function (entries) {
      for (var i = 0; i < entries.length; i++) {
        if (entries[i].isIntersecting) {
          entries[i].target.classList.add('grow');
          io.unobserve(entries[i].target);
        }
      }
    }, { threshold: 0.4 });
    bars.forEach(function (b) { io.observe(b); });
  }

  /* ============ 数字滚动 ============ */
  function setupCounters() {
    var nums = $$('.stat-num');
    /* 标记里写的是最终值（给没有 JS 的浏览器看），有 JS 才从 0 滚起来 */
    nums.forEach(function (el) { el.textContent = '0'; });
    function run(el) {
      var target = parseInt(el.getAttribute('data-count'), 10) || 0;
      if (reduce) { el.textContent = target; return; }
      var start = 0;
      function step(t) {
        if (!start) start = t;
        var k = Math.min((t - start) / 950, 1);
        var e = 1 - Math.pow(1 - k, 3);
        el.textContent = Math.round(target * e);
        if (k < 1) requestAnimationFrame(step); else el.textContent = target;
      }
      requestAnimationFrame(step);
    }
    if (!('IntersectionObserver' in window)) { nums.forEach(run); return; }
    var io = new IntersectionObserver(function (entries) {
      for (var i = 0; i < entries.length; i++) {
        if (entries[i].isIntersecting) { run(entries[i].target); io.unobserve(entries[i].target); }
      }
    }, { threshold: 0.5 });
    nums.forEach(function (n) { io.observe(n); });
  }

  /* ============ 卡片 3D 倾斜 + 磁吸按钮 ============ */
  if (fine && !reduce) {
    $$('.tilt').forEach(function (el) {
      var raf = 0, rx = 0, ry = 0, hover = false;
      function apply() {
        raf = 0;
        el.style.transform = 'perspective(900px) rotateX(' + ry.toFixed(2) + 'deg) rotateY(' +
                             rx.toFixed(2) + 'deg) translate3d(0,' + (hover ? -6 : 0) + 'px,0)';
      }
      el.addEventListener('pointerenter', function () { hover = true; });
      el.addEventListener('pointermove', function (e) {
        if (!el.classList.contains('in')) return;
        var r = el.getBoundingClientRect();
        rx = ((e.clientX - r.left) / r.width - 0.5) * 9;
        ry = -((e.clientY - r.top) / r.height - 0.5) * 7;
        if (!raf) raf = requestAnimationFrame(apply);
      }, { passive: true });
      el.addEventListener('pointerleave', function () {
        hover = false;
        if (raf) { cancelAnimationFrame(raf); raf = 0; }
        el.style.transform = '';
      });
    });

    $$('.btn-primary, .enter-btn').forEach(function (el) {
      el.addEventListener('pointermove', function (e) {
        var r = el.getBoundingClientRect();
        var dx = (e.clientX - (r.left + r.width / 2)) / r.width;
        var dy = (e.clientY - (r.top + r.height / 2)) / r.height;
        el.style.transform = 'translate3d(' + (dx * 8).toFixed(1) + 'px,' +
                             (dy * 6 - (el.classList.contains('enter-btn') ? 0 : 3)).toFixed(1) + 'px,0)';
      }, { passive: true });
      el.addEventListener('pointerleave', function () { el.style.transform = ''; });
    });
  }

  /* ============ 导航：进度条 / 高亮 / 视差 / 回顶 ============ */
  var nav = $('#nav'), bar = $('#progress'), toTop = $('#toTop');
  var links = $$('.nav-links a');
  var sections = $$('section[id]');
  var blobs = $$('.aurora');
  var heroCard = $('.hero-card');
  var queued = false;
  /* 滚动高亮要用的位置：一次量好缓存起来。滚动回调里刚写过 transform，
     再读 getBoundingClientRect 会强制整页重排 —— 这一页 8000px 高，一次就是几十毫秒。 */
  var sectionTops = [], lastActive = '', docH = 0, viewH = 0;
  function measureSections() {
    docH = document.documentElement.scrollHeight;
    viewH = window.innerHeight;
    sectionTops = sections.map(function (s) { return s.getBoundingClientRect().top + window.pageYOffset; });
  }
  /* 滚动高亮要用的位置：一次量好缓存起来。滚动回调里刚写过 transform，
     再读 getBoundingClientRect 会强制整页重排 —— 这一页 8000px 高，一次就是几十毫秒。 */
  var sectionTops = [], lastActive = '', docH = 0, viewH = 0;
  function measureSections() {
    docH = document.documentElement.scrollHeight;
    viewH = window.innerHeight;
    sectionTops = sections.map(function (s) { return s.getBoundingClientRect().top + window.pageYOffset; });
  }

  /* ============ 小屏导航面板 ============ */
  var navToggle = $('#navToggle');
  var navPanel = $('.nav-links');
  function setNavOpen(open) {
    if (!navToggle || !navPanel) return;
    navPanel.classList.toggle('open', open);
    navToggle.setAttribute('aria-expanded', open ? 'true' : 'false');
    navToggle.setAttribute('aria-label', open ? '关闭导航菜单' : '打开导航菜单');
    navToggle.innerHTML = open ? window.__ICON_CLOSE : window.__ICON_MENU;
  }
  if (navToggle) {
    navToggle.addEventListener('click', function (e) {
      e.stopPropagation();
      setNavOpen(!navPanel.classList.contains('open'));
    });
    /* 点了面板里的链接就收起，别挡着内容 */
    links.forEach(function (a) {
      a.addEventListener('click', function () { setNavOpen(false); });
    });
    /* 点面板外 / Esc 收起 */
    document.addEventListener('click', function (e) {
      if (!navPanel.classList.contains('open')) return;
      if (navPanel.contains(e.target) || navToggle.contains(e.target)) return;
      setNavOpen(false);
    });
    document.addEventListener('keydown', function (e) {
      if (e.key === 'Escape') setNavOpen(false);
    });
  }

  function onScroll() {
    queued = false;
    var y = window.pageYOffset;
    var max = docH - viewH;
    bar.style.transform = 'scaleX(' + (max > 0 ? Math.min(y / max, 1) : 0) + ')';
    if (y > 40) nav.classList.add('solid'); else nav.classList.remove('solid');
    if (y > 640) toTop.classList.add('show'); else toTop.classList.remove('show');

    if (!reduce) {
      for (var b = 0; b < blobs.length; b++) {
        blobs[b].style.transform = 'translate3d(0,' + (y * (0.05 + b * 0.035)).toFixed(1) + 'px,0)';
      }
      if (heroCard) heroCard.style.transform = 'translate3d(0,' + (y * 0.05).toFixed(1) + 'px,0)';
    }

    var current = '';
    /* 滚到底时强制点亮最后一节：否则「联系」的 top 永远大于阈值，永远不亮 */
    if (y + viewH >= docH - 2) {
      if (sections.length) current = sections[sections.length - 1].id;
    } else {
      for (var i = 0; i < sectionTops.length; i++) {
        if (sectionTops[i] - 150 <= y) current = sections[i].id;
      }
    }
    /* 值没变就别碰 class：省掉一次样式失效 */
    if (current !== lastActive) {
      lastActive = current;
      links.forEach(function (a) {
        a.classList.toggle('active', a.getAttribute('href') === '#' + current);
      });
    }
  }
  window.addEventListener('scroll', function () {
    if (!queued) { queued = true; requestAnimationFrame(onScroll); }
  }, { passive: true });
  measureSections();
  window.addEventListener('resize', measureSections);
  window.addEventListener('load', measureSections);
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(measureSections);

  /* ============ 锚点平滑滚动 ============ */
  $$('a[href^="#"]').forEach(function (a) {
    a.addEventListener('click', function (e) {
      var href = a.getAttribute('href');
      if (href.length < 2) return;
      var target = document.querySelector(href);
      if (!target) return;
      e.preventDefault();
      var top = target.getBoundingClientRect().top + window.pageYOffset - 84;
      window.scrollTo({ top: top, behavior: reduce ? 'auto' : 'smooth' });
    });
  });

  toTop.addEventListener('click', function () {
    window.scrollTo({ top: 0, behavior: reduce ? 'auto' : 'smooth' });
  });

  /* ============ 头像彩蛋 ============ */
  var avatar = $('#avatarImg');
  avatar.addEventListener('click', function (e) {
    e.stopPropagation();
    this.classList.remove('pop');
    void this.offsetWidth;
    this.classList.add('pop');
  });

  /* ============ 音频 + 频谱 ============ */
  var music = $('#bgMusic');
  var musicBtn = $('#musicBtn');
  var viz = $('#viz');
  var vctx = viz ? viz.getContext('2d') : null;
  var audioCtx = null, analyser = null, freq = null, vGrad = null, vizRaf = 0, vizLast = 0;

  /* 音频通路有两个模式，都能用 URL 切：
       默认     —— 接 Web Audio 图（频谱要拿 AnalyserNode 的数据）；
       ?audio=plain —— 完全不碰 Web Audio，<audio> 走浏览器原生媒体通道。
     接进 Web Audio 之后音频不再由媒体通道直接播放，中间多一次重采样
     （文件 44.1kHz、设备常是 48kHz），个别机器上就是爆音/断续的来源；
     频谱只是导航栏上一个装饰，不值得拿主音频去换。 */
  var AUDIO_PLAIN = /[?&]audio=plain\b/.test(location.search);
  var AUDIO_DEBUG = /[?&]audio=debug\b/.test(location.search);

  function initVizCanvas() {
    if (!viz) return;
    var dpr = Math.min(window.devicePixelRatio || 1, 2);
    var rect = viz.getBoundingClientRect();
    viz.width = Math.max(1, Math.round(rect.width * dpr));
    viz.height = Math.max(1, Math.round(rect.height * dpr));
    vctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    vGrad = vctx.createLinearGradient(0, viz.height / dpr, 0, 0);
    vGrad.addColorStop(0, '#6d4dfa');
    vGrad.addColorStop(1, '#0ea5e9');
  }

  /* 频谱画布不可见时（手机上是 display:none）根本不需要 Web Audio 图 */
  function vizUsable() {
    if (!viz || !vctx) return false;
    var rect = viz.getBoundingClientRect();
    return rect.width >= 8 && rect.height >= 8;
  }

  /* new AudioContext({sampleRate:44100}) 实测要 250ms 上下（LoAF 里就是一次 217ms 的长帧）。
     入口门摆着的时候先把它建出来（没有手势时它天然是 suspended，合法也不出声），
     等真出声只需要接线，卡顿就落在没人操作的那段空档里。 */
  var audioPrimed = false;
  function primeAudioContext() {
    if (audioPrimed || audioCtx || AUDIO_PLAIN || audioFallback || !vizUsable()) return;
    audioPrimed = true;
    var AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    /* 用文件自己的采样率建上下文：44.1kHz 的音频进 48kHz 的图要多过一层重采样 */
    try { audioCtx = new AC({ sampleRate: 44100 }); } catch (e0) { try { audioCtx = new AC(); } catch (e1) { audioCtx = null; } }
  }
  function setupAudioGraph() {
    if (AUDIO_PLAIN || audioFallback || !vizUsable()) return;
    if (!audioCtx) primeAudioContext();
    if (!audioCtx || analyser) return;
    try {
      var src = audioCtx.createMediaElementSource(music);
      analyser = audioCtx.createAnalyser();
      analyser.fftSize = 64;
      analyser.smoothingTimeConstant = 0.82;
      src.connect(analyser);
      analyser.connect(audioCtx.destination);
      freq = new Uint8Array(analyser.frequencyBinCount);
    } catch (err) {
      /* 接线失败就当没这回事：audio 走默认输出照放，别留半吊子的图 */
      audioCtx = null; analyser = null;
    }
  }

  /* 频谱按 ~30fps 画：这条 rAF 只在音乐播放时活着，没必要跟着 60fps 抢主线程 */
  function drawViz(t) {
    if (!analyser || music.paused || document.hidden || !vizUsable()) { vizRaf = 0; return; }
    vizRaf = requestAnimationFrame(drawViz);
    if (t && vizLast && t - vizLast < 33) return;
    vizLast = t || 0;
    analyser.getByteFrequencyData(freq);
    var w = viz.width, h = viz.height;
    vctx.clearRect(0, 0, w, h);
    vctx.fillStyle = vGrad;
    var n = 18, bw = w / n;
    for (var i = 0; i < n; i++) {
      var v = freq[i + 1] / 255;
      var bh = Math.max(2, v * h * 0.95);
      vctx.fillRect(i * bw + 1, h - bh, Math.max(1, bw - 3), bh);
    }
  }

  function startViz() {
    if (!analyser || vizRaf) return;
    initVizCanvas();
    if (viz) viz.classList.add('on');
    vizRaf = requestAnimationFrame(drawViz);
  }

  function stopViz() {
    if (vizRaf) { cancelAnimationFrame(vizRaf); vizRaf = 0; }
    if (viz) viz.classList.remove('on');
  }

  /* ============ 播放健康度与自动降级 ============
     症状是「音乐断断续续」。断流有两个来源：网络没供上数据，或者这条
     Web Audio 通路在这台设备上抖。两者可分：缓冲明明还有好几秒、
     却仍然 waiting / 时间不走，那就不是网络。
     判到这个组合就把音频换成原生元素继续放（同一首、同一进度），
     代价是频谱关掉 —— 频谱是装饰，音乐不是。 */
  var audioFallback = false;
  var audioWatch = { waiting: 0, stalled: 0, frozen: 0, last: 0, tickAt: 0, timer: 0 };
  var playStartedAt = 0;      /* 真正出声之后（或每次起步宽限重算）的时刻 */
  var playRequestedAt = 0;    /* 用户按下播放的时刻 */

  function bufferedAhead(el) {
    try {
      return el.buffered.length ? el.buffered.end(el.buffered.length - 1) - el.currentTime : 0;
    } catch (e) { return 0; }
  }

  /* 刚开播的头几秒本来就要等数据，那不算抖动 —— 只统计稳定播放之后的事件，
     免得在正常网络上误触发降级 */
  function settled() { return playStartedAt > 0 && performance.now() - playStartedAt > 3000; }

  /* 还没出声之前的一切等待（元数据、首包）都算「起步」，不算抖动：计数清零重来。
     线上实测过不这么做的后果 —— 一次正常的起步等待被当成抖动，频谱被白关掉。 */
  function resetWatch(now) {
    audioWatch.waiting = 0; audioWatch.stalled = 0; audioWatch.frozen = 0;
    audioWatch.last = music.currentTime; audioWatch.tickAt = now; playStartedAt = now;
  }

  function bindMusic(el) {
    el.addEventListener('play', function () { setMuted(false); startViz(); watchAudio(); });
    el.addEventListener('pause', function () { setMuted(true); stopViz(); songRezero(); });
    el.addEventListener('ended', function () { setMuted(true); stopViz(); songRezero(); });
    el.addEventListener('waiting', function () { if (settled()) audioWatch.waiting++; });
    el.addEventListener('stalled', function () { if (settled()) audioWatch.stalled++; });
    /* 真的出声了（且还在开头）→ 起步结束，抖动计数从这一刻重新开始 */
    el.addEventListener('playing', function () { if (el.currentTime < 1.0) resetWatch(performance.now()); });
  }

  function watchAudio() {
    if (audioWatch.timer) return;
    playRequestedAt = performance.now();
    resetWatch(playRequestedAt);
    audioWatch.timer = setInterval(function () {
      var now = performance.now();
      if (music.paused || document.hidden) { audioWatch.last = music.currentTime; audioWatch.tickAt = now; return; }
      var starting = music.currentTime < 0.3 || music.readyState < 2;
      var grace = now - playRequestedAt < 8000;      /* 起步最多宽限 8 秒 */
      if (starting) {
        if (grace) { resetWatch(now); return; }
        audioWatch.frozen++;      /* 8 秒还没出声就别再等了：这条通路有问题 */
      } else {
        if (!settled()) { audioWatch.last = music.currentTime; audioWatch.tickAt = now; return; }
        var dt = Math.max(0.2, (now - audioWatch.tickAt) / 1000);
        var adv = music.currentTime - audioWatch.last;
        /* 这一秒里进度落下真实时间一截（丢了 25% 以上）就是用户听到的「断断续续」 */
        if (adv < dt * 0.75) audioWatch.frozen++;
      }
      audioWatch.last = music.currentTime; audioWatch.tickAt = now;
      var bad = audioWatch.waiting + audioWatch.stalled + audioWatch.frozen;
      if (!AUDIO_PLAIN && !audioFallback && bad >= 2 && bufferedAhead(music) > 6) switchToPlain();
    }, 1000);
  }

  function switchToPlain() {
    if (audioFallback || AUDIO_PLAIN) return;
    if (!music.parentNode) return;
    audioFallback = true;
    window.__audioFallback = 'plain';
    var old = music, at = old.currentTime, vol = old.volume, wasPlaying = !old.paused;
    var fresh = document.createElement('audio');
    fresh.id = 'bgMusic';
    old.id = '';
    fresh.loop = true;
    fresh.preload = 'auto';
    fresh.setAttribute('aria-hidden', 'true');
    fresh.volume = vol;
    fresh.muted = old.muted;

    /* 等新元素能放了再交接：先 seek 到同一进度、开始放，再把旧的停掉，
       中间不留静音空档，也不会两个元素同时出声 */
    var started = false;
    function begin() {
      if (started) return;
      started = true;
      window.__audioSeekTarget = at;
      window.__audioSeekable = fresh.seekable && fresh.seekable.length > 0;
      try { fresh.currentTime = at; } catch (e) {}
      fresh.addEventListener('seeked', function () { window.__audioSeekLanded = fresh.currentTime; }, { once: true });
      if (wasPlaying) {
        var p = fresh.play();
        if (p && p.catch) p.catch(function () { setMuted(true); });
        playStartedAt = performance.now();   /* 新元素也给它几秒起步宽限 */
      }
      try { old.pause(); } catch (e) {}
      if (old.parentNode) old.parentNode.removeChild(old);
      if (audioCtx) { try { audioCtx.close(); } catch (e) {} audioCtx = null; }
      analyser = null;                   /* 新元素不在图里，频谱没有数据源了 */
      stopViz();
      if (viz) viz.classList.remove('on');
      if (dbgEl) dbgEl.textContent = 'audio 已降级为原生通路（检测到播放抖动，频谱关闭）';
    }
    fresh.addEventListener('loadedmetadata', begin, { once: true });
    fresh.addEventListener('canplay', begin, { once: true });
    fresh.addEventListener('error', function () { setMuted(true); });
    fresh.src = old.getAttribute('src') || 'music.mp3';
    old.parentNode.insertBefore(fresh, old.nextSibling);
    music = fresh;                       /* 所有闭包读的都是这个变量 */
    bindMusic(fresh);
    if (dbgEl) dbgEl.textContent = 'audio 正在降级为原生通路…';
    /* 万一 loadedmetadata/canplay 都没来（极少见），也别把音乐卡死 */
    setTimeout(function () { if (!started && fresh.readyState >= 1) begin(); }, 1200);
  }

  /* ?audio=debug：把播放状态摊在左下角，出问题时能直接读数字 */
  var dbgEl = null, dbgIv = 0;
  function startAudioDebug() {
    if (!AUDIO_DEBUG) return;
    dbgEl = document.createElement('div');
    dbgEl.className = 'audio-debug mono';
    dbgEl.setAttribute('aria-hidden', 'true');
    document.body.appendChild(dbgEl);
    dbgIv = setInterval(function () {
      dbgEl.textContent =
        'audio ' + (AUDIO_PLAIN ? 'plain(强制)' : audioFallback ? 'plain(降级)' : 'graph') +
        ' | ctx ' + (audioCtx ? audioCtx.sampleRate + '/' + audioCtx.state : 'none') +
        ' | readyState ' + music.readyState +
        ' | buffered +' + bufferedAhead(music).toFixed(1) + 's' +
        ' | waiting/stalled/frozen ' + audioWatch.waiting + '/' + audioWatch.stalled + '/' + audioWatch.frozen +
        ' | t ' + music.currentTime.toFixed(1) + 's';
    }, 400);
  }

  /* 建图要单独走一步，而且排在起播之后：new AudioContext({sampleRate:44100}) 实测要
     250ms 上下，排在 play() 前面就是实打实的「点进去 250ms 没声音」。 */
  function attachGraph() {
    setupAudioGraph();
    if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();
  }

  /* 预热首包：preload="none" 意味着点「进入主页」之后浏览器才开始拉文件，
     实测首包要 0.5–1.9s，那几秒就是「进去之后要等一会儿才有声音」。
     入口门摆着的时候先把头 160KB（约 10 秒的量）拉进缓存，点下去就能直接出声。
     省流量模式 / 2G 跳过。 */
  var warmed = false;
  function warmMusic() {
    if (warmed) return;
    warmed = true;
    try {
      var c = navigator.connection || {};
      if (c.saveData || /(^|-)(slow-)?2g$/.test(c.effectiveType || '')) { window.__musicWarm = 'skipped'; return; }
      if (!window.fetch) return;
      fetch('music.mp3', { headers: { Range: 'bytes=0-163839' }, cache: 'force-cache', priority: 'low' })
        .then(function (r) { window.__musicWarm = r.status; })
        .catch(function () { window.__musicWarm = 'error'; });
    } catch (e) {}
  }
  /* 立刻发，但压到最低优先级：封面是 fetchpriority=high 的 LCP，预热不能抢它的带宽 */
  warmMusic();
  /* 建 AudioContext 放两重保险后面：先等 load（封面 LCP 画完），再等浏览器空闲。
     实测「一进页面就建」会在导航后 0.28s 吃掉 234ms 主线程、正好压在首屏绘制上；
     挪到这里，卡顿落在「入口门摆着、用户还没点」的空档里，交互段实测 0 长动画帧。 */
  (function primeWhenIdle() {
    function go() {
      if (window.requestIdleCallback) requestIdleCallback(primeAudioContext, { timeout: 2500 });
      else setTimeout(primeAudioContext, 900);
    }
    if (document.readyState === 'complete') setTimeout(go, 600);
    else window.addEventListener('load', function () { setTimeout(go, 600); }, { once: true });
  })();
  ['pointermove', 'touchstart', 'keydown'].forEach(function (evName) {
    window.addEventListener(evName, warmMusic, { once: true, passive: true });
  });

  function setMuted(m) {
    musicBtn.classList.toggle('off', !!m);
    musicBtn.innerHTML = m ? window.__ICON_OFF : window.__ICON_ON;
    musicBtn.setAttribute('aria-label', m ? '播放背景音乐' : '暂停背景音乐');
    musicBtn.setAttribute('aria-pressed', m ? 'false' : 'true');
  }

  bindMusic(music);

  musicBtn.addEventListener('click', function (e) {
    e.stopPropagation();
    if (music.paused) {
      var p = music.play();
      if (p && p.catch) p.catch(function () { setMuted(true); });
      setTimeout(attachGraph, 0);      /* 先出声，图随后 */
    } else {
      music.pause();
    }
  });

  /* ============ 进入站点 ============ */
  var entered = false;
  var landing = $('#landing'), loader = $('#loader'), page = $('#page');

  function enterSite() {
    if (entered) return;
    entered = true;

    music.volume = 0.34;
    /* 起播优先：先 play()；Web Audio 建图是 200ms+ 的主线程停顿（实测一次 217ms 的长动画帧），
       放进 loader 这 0.62s 的空档里做，别卡在出声或滚动的那一帧上。 */
    var play = music.play();
    if (play && play.catch) play.catch(function () { setMuted(true); });
    var graphQueued = false;
    function scheduleGraph() {
      if (graphQueued) return;
      graphQueued = true;
      if (window.requestIdleCallback) requestIdleCallback(attachGraph, { timeout: 900 });
      else setTimeout(attachGraph, 240);
    }
    scheduleGraph();
    music.addEventListener('playing', scheduleGraph, { once: true });

    loader.classList.add('show');

    /* loader 只是装饰，不阻塞进入：默认 0.62s，点击或 Esc 可立即跳过 */
    var done = false;
    function finish() {
      if (done) return;
      done = true;
      document.body.classList.remove('locked');
      document.documentElement.classList.remove('entry');
      landing.classList.add('hidden');
      page.classList.add('visible');
      nav.classList.add('visible');
      loader.classList.remove('show');
      setTimeout(function () {
        loader.style.display = 'none';
        setupReveal();
        setupBars();
        setupCounters();
        window.scrollTo(0, 0);
        onScroll();
      }, 260);
    }
    setTimeout(finish, reduce ? 150 : 620);
    loader.addEventListener('click', finish);
    document.addEventListener('keydown', function (e) {
      if (e.key === 'Escape') finish();
    });
  }

  $('#enterBtn').addEventListener('click', enterSite);

  /* ============ 音游（canvas + rAF 时间轴） ============ */
  var cvs = $('#rhythmCanvas');
  var ctx = cvs ? cvs.getContext('2d') : null;
  var LANES = 7, TRAVEL = 1450;
  /* 兜底节拍：起音间隔中位数 302ms（实测这首歌）。有起音表时用不到它。 */
  var BEAT = 302;

  /* ============ 谱面时间轴：跟着这首歌走 ============
     起音点由生成器从 onsets.txt 内联（本机 ffmpeg + 谱通量分析实测，657 个），
     音符落在真正的起音上，而不是拍脑袋写的 BPM。
     判定时钟优先用 audio.currentTime：歌在播时它就是排谱与判定的唯一时钟；
     没播或暂停时换成等速虚拟时钟，接着暂停那一刻的歌曲位置继续走。 */
  var ONSETS = window.__RHYTHM_ONSETS || [];
  var SONG_MS = window.__RHYTHM_SONG_MS || (ONSETS.length ? ONSETS[ONSETS.length - 1] + 4000 : 0);
  /* 当前在用的那份谱面：默认是生成器内联的本站那首歌，拖进来别的歌就换成它 */
  var CUR_ONSETS = ONSETS, CUR_SONG_MS = SONG_MS;
  var songLabel = '本站的歌', BUILTIN_SRC = music ? music.getAttribute('src') : '';
  var songZero = (window.performance && performance.now) ? performance.now() : Date.now();
  var timeline = [], spawnIdx = 0, songLast = 0;

  function perfNow() { return (window.performance && performance.now) ? performance.now() : Date.now(); }
  function audioLive() { return !!(music && !music.paused && music.readyState >= 2); }
  function songNow() { return audioLive() ? music.currentTime * 1000 : perfNow() - songZero; }
  function songRezero() { if (music) songZero = perfNow() - music.currentTime * 1000; }
  function buildTimeline(from, count) {
    /* 从歌曲位置 from 往后取 count 个起音点，只留下与前一个间隔 >= DIFF.gap 的：
       歌里 100ms 级的密集簇会让音符糊成一整排，先按难度拉开，再交给内核判。
       时间戳永远来自实测起音点，不够就绕回开头接着打。 */
    var out = [], first = 0, i, last = -1e9, lap = 0;
    if (!CUR_ONSETS.length) return out;
    while (first < CUR_ONSETS.length && CUR_ONSETS[first] < from) first++;
    for (i = first; out.length < count && lap < 40; i++) {
      var k = i % CUR_ONSETS.length;
      if (k === 0 && i > first) lap++;
      var t = CUR_ONSETS[k] + Math.floor(i / CUR_ONSETS.length) * CUR_SONG_MS;
      if (t - last >= DIFF.gap) { out.push(t); last = t; }
    }
    return out;
  }
  var clockChip = $('#rhythmClock');
  var notes = [], sparks = [];
  var combo = 0, acc = 0, prev = 0, rafId = 0, running = false;
  /* 五位成员的代表色（粉丝整理的色票，非官方）：舞台灯与轨道配色都用它 */
  var MYGO_LANES = ['#6EC1E4', '#FF6F91', '#6EEB83', '#FFC857', '#7A5CFF'];
  var W = 0, H = 0, laneW = 0, hitY = 0, glow = 0, noteGrad = null, laneGrad = null;
  var lastLane = -1, lastHitAt = -1e9, laneGlow = [], seen = false;

  for (var li = 0; li < LANES; li++) laneGlow.push(0);

  function fitCanvas() {
    if (!cvs) return;
    var rect = cvs.getBoundingClientRect();
    if (!rect.width || !rect.height) return;
    var dpr = Math.min(window.devicePixelRatio || 1, 2);
    /* 背板尺寸取整、绘图坐标系由背板反推：避免 CSS 尺寸与背板差一两个像素被拉伸 */
    var bw = Math.max(1, Math.round(rect.width * dpr));
    var bh = Math.max(1, Math.round(rect.height * dpr));
    if (cvs.width === bw && cvs.height === bh) return;
    W = bw / dpr; H = bh / dpr;
    cvs.width = bw;
    cvs.height = bh;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    laneW = W / LANES;
    hitY = H - 66;
    noteGrad = ctx.createLinearGradient(0, 0, 0, 13);
    noteGrad.addColorStop(0, '#f2fbff');
    noteGrad.addColorStop(1, '#7dd3fc');
    /* MyGO 舞台主题下，七条轨道循环用五位成员的代表色（色值来自粉丝色票，非官方） */
    laneGrad = MYGO_LANES.map(function (c) {
      var g = ctx.createLinearGradient(0, 0, 0, 13);
      g.addColorStop(0, '#ffffff');
      g.addColorStop(1, c);
      return g;
    });
    render();
  }

  function roundRect(x, y, w, h, r) {
    ctx.beginPath();
    if (ctx.roundRect) { ctx.roundRect(x, y, w, h, r); return; }
    ctx.moveTo(x + r, y);
    ctx.arcTo(x + w, y, x + w, y + h, r);
    ctx.arcTo(x + w, y + h, x, y + h, r);
    ctx.arcTo(x, y + h, x, y, r);
    ctx.arcTo(x, y, x + w, y, r);
    ctx.closePath();
  }

  /* ============ 谱面来源：C++ / WebAssembly 内核 ============
     两个来源都是同一份 C++ 算法（rhythm_core.hpp）：
       首选 rhythm_core.wasm —— 由 emcc 编译、生成器 base64 内联，浏览器里运行时算轨道；
       兜底 生成器谱面     —— 构建时用同一份算法算好的 256 音符，没有 WebAssembly 也能跑。
     加载后会比对前 64 个音符，结果写进 window.__rhythmParity（ok / mismatch / no-wasm / error）。 */
  var CHART = window.__RHYTHM_CHART || '';
  var chartPos = 0;
  var kernelNext = null;
  /* 内核没有 import（emcc 产物为 0 import）；真出现了就让实例化失败走兜底，别装作没事 */
  var kernelImports = {};

  function base64ToBytes(b64) {
    var bin = atob(b64), n = bin.length, out = new Uint8Array(n);
    for (var i = 0; i < n; i++) out[i] = bin.charCodeAt(i);
    return out;
  }

  function updateKernelBadge() {
    var el = $('#kernelBadge');
    if (!el) return;
    var p = window.__rhythmParity;
    if (p === 'ok') {
      el.textContent = window.__rhythmJudgeParity === 'ok'
        ? 'C++ / WASM 内核 · 谱面与判定自检 ok'
        : 'C++ / WASM 内核 · 谱面自检 ok（判定退回 JS）';
      el.classList.add('ok');
    }
    else if (p === 'mismatch') el.textContent = 'C++ / WASM 内核 · 自检不一致，已退回生成器谱面';
    else if (p === 'no-wasm') el.textContent = '生成器 C++ 谱面（此浏览器不支持 WebAssembly）';
    else el.textContent = '生成器 C++ 谱面（WASM 内核不可用）';
  }

  function loadKernel() {
    var b64 = window.__RHYTHM_WASM;
    if (!b64 || typeof WebAssembly === 'undefined' || !WebAssembly.instantiate) {
      window.__rhythmParity = 'no-wasm'; window.__rhythmLaneSource = 'generator'; window.__rhythmJudgeParity = 'no-wasm';
      updateKernelBadge(); applySeedFromQuery(); return;
    }
    WebAssembly.instantiate(base64ToBytes(b64), { env: kernelImports, wasi_snapshot_preview1: kernelImports })
      .then(function (res) {
        var ex = res.instance ? res.instance.exports : res.exports;
        if (!ex || typeof ex.chart_next !== 'function') throw new Error('chart_next missing');
        kernelExports = ex;
        var seed = window.__RHYTHM_SEED, lanes = window.__RHYTHM_LANES;
        ex.chart_reset(seed, lanes);
        var ok = true;
        for (var i = 0; i < 64 && i < CHART.length; i++) {
          if (ex.chart_next() !== (CHART.charCodeAt(i) - 48)) { ok = false; break; }
        }
        ex.chart_reset(seed, lanes);
        window.__rhythmKernelVersion = ex.kernel_version ? ex.kernel_version() : 0;
      /* 起音点自检：同一个函数在 native 里算过的校验值，这里让 wasm 再算一遍 */
      window.__onsetParity = 'no-wasm';
      if (typeof ex.onset_init === 'function' && typeof ex.onset_selftest === 'function') {
        try {
          ex.onset_init();
          var onsetOutPtr = ex.onset_out_ptr();
          new Int32Array(ex.memory.buffer, onsetOutPtr, 1)[0] = 0;
          var onsetSum = ex.onset_selftest(onsetOutPtr) >>> 0;
          var onsetCnt = new Int32Array(ex.memory.buffer, onsetOutPtr, 1)[0];
          var onsetWant = (window.__ONSET_PROBE_EXPECT || 0) >>> 0;
          window.__onsetProbe = { count: onsetCnt, checksum: onsetSum, expect: onsetWant };
          window.__onsetParity = (onsetSum === onsetWant) ? 'ok' : 'mismatch';
        } catch (e) { window.__onsetParity = 'error'; }
      }

        /* 判定与算分的探针：期望值是生成器用同一份 rhythm_core.hpp 算的，
           探针压在窗口边界（±45 / ±90 与各差 1ms），窗口写错这里就会 mismatch */
        var jp = window.__RHYTHM_JUDGE_PROBE || [], je = window.__RHYTHM_JUDGE_EXPECT || [];
        var ap = window.__RHYTHM_ACC_PROBE || [], ae = window.__RHYTHM_ACC_EXPECT || [];
        var jok = typeof ex.judge_ms === 'function' && jp.length > 0 && jp.length === je.length;
        for (var j = 0; jok && j < jp.length; j++) { if (ex.judge_ms(jp[j]) !== je[j]) jok = false; }
        var aok = typeof ex.accuracy_permille === 'function' && ap.length > 0 && ap.length === ae.length;
        for (var k2 = 0; aok && k2 < ap.length; k2++) {
          if (ex.accuracy_permille(ap[k2][0], ap[k2][1], ap[k2][2]) !== ae[k2]) aok = false;
        }
        window.__rhythmJudgeParity = (jok && aok) ? 'ok' : 'mismatch';
        if (jok && aok) {
          KERNEL_JUDGE = function (d) { return ex.judge_ms(d); };
          KERNEL_ACC = function (p, g, n) { return ex.accuracy_permille(p, g, n); };
        } else { KERNEL_JUDGE = null; KERNEL_ACC = null; }
        kernelReset = function () { ex.chart_reset(window.__RHYTHM_SEED, window.__RHYTHM_LANES); };

        if (ok) { kernelNext = function () { return ex.chart_next(); }; window.__rhythmParity = 'ok'; window.__rhythmLaneSource = 'wasm'; }
        else { window.__rhythmParity = 'mismatch'; window.__rhythmLaneSource = 'generator'; }
        updateKernelBadge();
        applySeedFromQuery();
        refreshStats();
      })
      .catch(function () { window.__rhythmParity = 'error'; window.__rhythmLaneSource = 'generator'; window.__rhythmJudgeParity = 'error'; updateKernelBadge(); applySeedFromQuery(); });
  }

  function chartLane() {
    if (seededChart) return seededChart[chartPos++ % seededChart.length];
    if (kernelNext) return kernelNext();
    if (!CHART.length) return 0;
    return CHART.charCodeAt(chartPos++ % CHART.length) - 48;
  }

  /* ============ 自定义谱面：?seed=xxx 或页面上的输入框 ============
     同一份 C++ 算法有两个独立实现：浏览器里跑的 wasm 内核，和这里的 JS 镜像。
     换 seed 之后由内核算 256 个音符，镜像再算一遍逐位核对，
     结论写进 window.__rhythmSeedParity（ok / mismatch / mirror-only），页面照实标出来源。 */
  var SEED_DEFAULT = Number(window.__RHYTHM_SEED) || 0;
  var LANES_N = Number(window.__RHYTHM_LANES) || 7;
  var SEED_CHART_LEN = 256;
  var seedActive = SEED_DEFAULT, seededChart = null, kernelExports = null;

  function seedToUint(v) {
    if (v === null || v === undefined || String(v).trim() === '') return SEED_DEFAULT;
    var s = String(v).trim();
    if (/^\d+$/.test(s)) return (parseInt(s, 10) >>> 0) || SEED_DEFAULT;
    if (/^0x[0-9a-fA-F]+$/.test(s)) return (parseInt(s.slice(2), 16) >>> 0) || SEED_DEFAULT;
    var h = 2166136261;
    for (var i = 0; i < s.length; i++) { h ^= s.charCodeAt(i); h = Math.imul(h, 16777619) >>> 0; }
    return h >>> 0;
  }

  /* 与 rhythm_core.hpp 的 Rng::next / nextLane 同构：被比对的第二个实现 */
  function mirrorChart(seed, lanes, n) {
    var s = (seed >>> 0) || 0x9e3779b9, last = -1, out = [];
    function nxt() {
      s = (s ^ (s << 13)) >>> 0; s = (s ^ (s >>> 17)) >>> 0; s = (s ^ (s << 5)) >>> 0;
      return s;
    }
    for (var i = 0; i < n; i++) {
      var l = nxt() % lanes;
      if (l === last) l = (l + 1 + (nxt() % (lanes - 1))) % lanes;
      last = l; out.push(l);
    }
    return out;
  }

  function applySeed(value, silent) {
    var seed = seedToUint(value);
    seedActive = seed;
    var parity = 'mirror-only', src = 'js-mirror', first = -1, i;
    if (kernelExports && typeof kernelExports.chart_reset === 'function') {
      kernelExports.chart_reset(seed, LANES_N);
      var k = [];
      for (i = 0; i < SEED_CHART_LEN; i++) k.push(kernelExports.chart_next());
      var m = mirrorChart(seed, LANES_N, SEED_CHART_LEN);
      for (i = 0; i < SEED_CHART_LEN; i++) { if (k[i] !== m[i]) { first = i; break; } }
      parity = first < 0 ? 'ok' : 'mismatch';
      src = first < 0 ? 'wasm' : 'wasm(mismatch)';
      seededChart = k;
      kernelExports.chart_reset(seed, LANES_N);
    } else {
      seededChart = mirrorChart(seed, LANES_N, SEED_CHART_LEN);
    }
    window.__rhythmSeedParity = parity;
    window.__rhythmSeedSource = src;
    window.__rhythmSeedFirstDiff = first;
    window.__rhythmActiveSeed = seed;
    window.__rhythmLaneSource = src === 'wasm' ? 'wasm' : (kernelExports ? 'generator' : 'generator');
    chartPos = 0;
    var hex = '0x' + seed.toString(16);
    var note = seed === SEED_DEFAULT ? '（生成器内联的那份）' : '';
    var el = $('#seedInfo');
    if (el) {
      if (parity === 'ok') {
        el.textContent = '当前 seed ' + hex + note + ' · 内核算了 ' + SEED_CHART_LEN +
          ' 个音符，JS 镜像逐位复核：一致（前 16 个 ' + seededChart.slice(0, 16).join('') + '）';
      } else if (parity === 'mismatch') {
        el.textContent = '当前 seed ' + hex + ' · 内核与 JS 镜像在第 ' + (first + 1) +
          ' 个音符分叉（轨 ' + seededChart[first] + ' vs ' + mirrorChart(seed, LANES_N, first + 1)[first] + '），已按内核为准';
      } else {
        el.textContent = '当前 seed ' + hex + ' · 这台浏览器没有 wasm 内核，用 JS 镜像算（与 C++ 同一份算法）';
      }
    }
    if (!silent) {
      try { history.replaceState(null, '', '?seed=' + encodeURIComponent(String(value === '' || value === undefined ? 'default' : value))); } catch (e) {}
    }
    if (mode === 'play' && roundActive) { startRound(); }
    return parity;
  }

  function applySeedFromQuery() {
    var q = /[?&]seed=([^&#]*)/.exec(location.search);
    if (!q || !q[1] || q[1] === 'default') return false;
    var v = decodeURIComponent(q[1]);
    var input = $('#seedInput');
    if (input) input.value = v;
    applySeed(v, true);
    return true;
  }

  window.__rhythmApplySeed = applySeed;
  window.__rhythmSeedState = function () {
    return {
      seed: seedActive, parity: window.__rhythmSeedParity, source: window.__rhythmSeedSource,
      firstDiff: window.__rhythmSeedFirstDiff, len: seededChart ? seededChart.length : 0,
      head: seededChart ? seededChart.slice(0, 16).join('') : ''
    };
  };

  /* ============ 可玩模式：判定与算分也交给 C++ / WASM 内核 ============
     内核导出（rhythm_core.hpp 里唯一的实现）：
       judge_ms(deltaMs)                     → 0 MISS / 1 GOOD / 2 PERFECT（±45 / ±90ms）
       accuracy_permille(perfect, good, total) → 准确率千分比（PERFECT 记 1.0、GOOD 记 0.6）
     加载内核时，生成器按同一份头文件算好的探针会逐条比对（含 ±45/±90 边界与差 1ms 的点），
     结果写进 window.__rhythmJudgeParity；内核不可用时退回本地 JS 镜像，页面照实标出来源。 */
  var GOODms = 90, PERFECTms = 45, ROUND_NOTES = 64;
  /* 难度分级：只动「音符最密能密到多少」和「喂进内核的窗口」。
     判定永远由内核给出；所有时间戳都来自实测起音点，不做双押、不编造时间。 */
  var DIFF_TIERS = {
    easy: { label: '轻松', gap: 320, win: 1.25 },
    normal: { label: '标准', gap: 170, win: 1 },
    hard: { label: '硬核', gap: 120, win: 0.667 }
  };
  var diffKey = 'normal', DIFF = DIFF_TIERS.normal, WIN_SCALE = 1;
  var KERNEL_JUDGE = null, KERNEL_ACC = null, kernelReset = null;
  var mode = 'auto', roundActive = false, roundIndex = 0, pausedAt = 0, lastJudge = '';
  var round = { perfect: 0, good: 0, miss: 0, accPermille: 0 };
  var maxCombo = 0, best = null;
  var KEYS = ['d', 'f', 'g', 'h', 'j', 'k', 'l'], NUMS = '1234567';
  var BEST_KEY = 'myintro.rhythm.best.v1';

  function jsJudge(d) { var a = d < 0 ? -d : d; return a <= PERFECTms ? 2 : a <= GOODms ? 1 : 0; }
  function jsAcc(p, g, n) { return n <= 0 ? 0 : Math.floor((p * 1000 + g * 600) / n); }
  /* 难度只改喂进内核的窗口：偏差按档缩放后再交给内核，JS 回退同理 —— 判定者始终是内核 */
  function doJudge(d) { var dd = d / WIN_SCALE; return KERNEL_JUDGE ? KERNEL_JUDGE(dd) : jsJudge(dd); }
  function doAcc(p, g, n) { return KERNEL_ACC ? KERNEL_ACC(p, g, n) : jsAcc(p, g, n); }

  function setText(sel, v) { var el = $(sel); if (el) el.textContent = v; }
  function fmtPct(pm) { return (pm / 10).toFixed(1) + '%'; }

  function loadBest() {
    try { best = JSON.parse(localStorage.getItem(BEST_KEY) || 'null'); } catch (e) { best = null; }
  }
  function saveBest(b) { try { localStorage.setItem(BEST_KEY, JSON.stringify(b)); } catch (e) {} }

  function refreshStats() {
    var judged = round.perfect + round.good + round.miss;
    round.accPermille = judged ? doAcc(round.perfect, round.good, judged) : 0;
    setText('#rPerfect', round.perfect);
    setText('#rGood', round.good);
    setText('#rMiss', round.miss);
    setText('#rAcc', judged ? fmtPct(round.accPermille) : '--');
    setText('#rMax', maxCombo);
    setText('#rBest', best ? fmtPct(best.accPermille) + ' · ' + best.maxCombo + ' 连' : '--');
    window.__rhythmStats = {
      mode: mode, roundActive: roundActive, roundIndex: roundIndex,
      perfect: round.perfect, good: round.good, miss: round.miss, judged: judged,
      combo: combo, maxCombo: maxCombo, accPermille: round.accPermille,
      judgeSource: KERNEL_JUDGE ? 'wasm' : 'js', best: best ? best.accPermille : null
    };
  }

  function setModeUI() {
    var pb = $('#rhythmPlay'), sb = $('#rhythmStop'), tag = $('#rhythmTag'), hint = $('#rhythmHint');
    /* 两个按钮都常显：play 时左边那颗变成「重新开始 / 再来一轮」，比藏起来更好找 */
    if (pb) {
      pb.setAttribute('aria-pressed', mode === 'play' ? 'true' : 'false');
      pb.textContent = mode === 'play'
        ? (roundActive ? '重新开始这一轮' : '再来一轮')
        : '开始挑战（64 音符）';
    }
    if (sb) sb.hidden = mode !== 'play';
    var st = $('#rhythmStats'); if (st) st.hidden = mode !== 'play';
    if (tag) tag.textContent = mode === 'play'
      ? 'PLAY · ' + Math.min(roundIndex, ROUND_NOTES) + ' / ' + ROUND_NOTES
      : 'AUTO PLAY';
    if (hint) hint.textContent = mode === 'play'
      ? '键位 D F G H J K L 或数字 1–7 · 也可以直接点按轨道'
      : '判定窗口 ±45 / ±90ms，判定与算分都由 WASM 内核给出';
    if (cvs) cvs.style.touchAction = mode === 'play' ? 'none' : '';
    window.__rhythmMode = mode;
  }

  /* 找同一轨道里离现在最近的、还没判定的音符 */
  function nearestNote(lane, now) {
    var pick = null, bestD = 1e9;
    for (var i = 0; i < notes.length; i++) {
      var n = notes[i];
      if (n.lane !== lane || n.judged || !n.hitAt) continue;
      var d = n.hitAt - now;
      if (Math.abs(d) < Math.abs(bestD)) { bestD = d; pick = n; }
    }
    return pick ? { note: pick, delta: bestD } : null;
  }

  function registerJudge(r, n) {
    if (r === 2) { round.perfect++; combo++; lastJudge = 'PERFECT'; }
    else if (r === 1) { round.good++; combo++; lastJudge = 'GOOD'; }
    else { round.miss++; combo = 0; lastJudge = 'MISS'; }
    if (combo > maxCombo) maxCombo = combo;
    if (n) { n.judged = 1; if (r > 0) n.dead = 1; else n.missed = 1; }
    if (r > 0 && n) { explode(n.lane * laneW + laneW / 2, hitY, n.lane); glow = 1; }
    var ne = $('#comboNum');
    if (ne) { ne.textContent = combo; ne.classList.remove('pop'); void ne.offsetWidth; ne.classList.add('pop'); }
    var je = $('#rhythmJudge');
    if (je) { je.textContent = lastJudge; je.className = 'rhythm-judge ' + lastJudge.toLowerCase(); }
    refreshStats();
  }

  function hitLane(lane, now) {
    if (mode !== 'play' || !roundActive) return;
    var hit = nearestNote(lane, now);
    /* 空击不扣分：只让该轨道亮一下，别把休闲玩家劝退 */
    if (!hit || Math.abs(hit.delta) > GOODms * (WIN_SCALE > 1 ? WIN_SCALE : 1)) { laneGlow[lane] = Math.max(laneGlow[lane], 0.35); return; }
    var r = doJudge(hit.delta);
    if (r <= 0) { laneGlow[lane] = Math.max(laneGlow[lane], 0.35); return; }
    registerJudge(r, hit.note);
  }

  function startRound() {
    mode = 'play'; roundActive = true; roundIndex = 0;
    round.perfect = 0; round.good = 0; round.miss = 0; round.accPermille = 0;
    combo = 0; maxCombo = 0; lastJudge = '';
    notes.length = 0; sparks.length = 0; acc = 0; chartPos = 0; lastLane = -1;
    /* 这一轮从「你现在听到的位置」开始：接下来的 ROUND_NOTES 个起音点 */
    timeline = buildTimeline(songNow() + 900, ROUND_NOTES);
    spawnIdx = 0; songLast = 0;
    if (kernelReset) kernelReset();
    setText('#comboNum', '0');
    var res = $('#rhythmResult'); if (res) { res.hidden = true; res.textContent = ''; }
    var je = $('#rhythmJudge'); if (je) je.textContent = '';
    setModeUI(); refreshStats();
    startRhythm();   /* 用户自己点的，即使系统开了「减少动态效果」也照跑 */
  }

  function backToAuto() {
    mode = 'auto'; roundActive = false; pausedAt = 0;
    notes.length = 0; sparks.length = 0; acc = 0; combo = 0;
    timeline = buildTimeline(songNow(), ROUND_NOTES * 2);
    spawnIdx = 0; songLast = 0;
    round.perfect = 0; round.good = 0; round.miss = 0; round.accPermille = 0; maxCombo = 0;
    setText('#comboNum', '0');
    var res = $('#rhythmResult'); if (res) { res.hidden = true; res.textContent = ''; }
    var je = $('#rhythmJudge'); if (je) je.textContent = '';
    setModeUI(); refreshStats(); startRhythm();
  }

  function endRound() {
    if (!roundActive) return;
    roundActive = false;
    var judged = round.perfect + round.good + round.miss;
    var isRecord = false;
    if (judged > 0 && (!best || round.accPermille > best.accPermille ||
        (round.accPermille === best.accPermille && maxCombo > best.maxCombo))) {
      best = { accPermille: round.accPermille, maxCombo: maxCombo, at: Date.now() };
      saveBest(best); isRecord = true;
    }
    var el = $('#rhythmResult');
    if (el && judged > 0) {
      el.hidden = false;
      el.textContent = '本轮 ' + judged + ' 个音符 · PERFECT ' + round.perfect + ' / GOOD ' + round.good +
        ' / MISS ' + round.miss + ' · 准确率 ' + fmtPct(round.accPermille) + ' · 最大连击 ' + maxCombo +
        ' · 判定来源 ' + (KERNEL_JUDGE && KERNEL_ACC ? 'WASM 内核（C++）' : 'JS 回退') +
        (isRecord ? ' · 新纪录！' : '');
    }
    setModeUI();
    refreshStats();
  }

  /* 预览与自检用的钩子：都是页面自己会用到的函数，不是测试专用开关 */
  /* 换主题时立刻重画一帧：轨道颜色跟着成员色走 */
  /* ---------- 难度：换档只重排这一轮，时间戳仍是实测起音点 ---------- */
  function diffBtn(k) { return document.getElementById('diff' + k.charAt(0).toUpperCase() + k.slice(1)); }
  function diffNote() {
    var el = document.getElementById('diffNote');
    if (!el) return;
    var p = Math.round(PERFECTms * WIN_SCALE), g = Math.round(GOODms * WIN_SCALE);
    el.textContent = '音符之间至少隔 ' + DIFF.gap + 'ms · 判定窗口 ±' + p + ' / ±' + g + 'ms（内核给的）';
  }
  function setDiff(key, save) {
    diffKey = DIFF_TIERS[key] ? key : 'normal';
    DIFF = DIFF_TIERS[diffKey];
    WIN_SCALE = DIFF.win;
    ['easy', 'normal', 'hard'].forEach(function (k) {
      var b = diffBtn(k);
      if (b) b.setAttribute('aria-pressed', k === diffKey ? 'true' : 'false');
    });
    if (save) { try { localStorage.setItem('myintro.diff', diffKey); } catch (e) {} }
    diffNote();
    timeline = buildTimeline(mode === 'play' ? songNow() + 900 : songNow(), ROUND_NOTES);
    spawnIdx = 0;
  }
  ['easy', 'normal', 'hard'].forEach(function (k) {
    var b = diffBtn(k);
    if (b) b.addEventListener('click', function () { setDiff(k, true); });
  });
  (function initDiff() {
    var saved = null;
    try { saved = localStorage.getItem('myintro.diff'); } catch (e) {}
    setDiff(saved, false);
  })();

  /* ---------- 起音点时间条：整首歌 657 个 tick + 播放头 ---------- */
  var stripCvs = document.getElementById('onsetStrip');
  var stripLayer = null, stripDirty = true, stripAccent = '#a78bfa', stripSig = '';
  function stripAccentSync() {
    try { stripAccent = getComputedStyle(document.documentElement).getPropertyValue('--violet').trim() || '#a78bfa'; } catch (e) {}
    stripDirty = true;
  }
  function drawStrip() {
    if (!stripCvs) return;
    var sig = CUR_ONSETS.length + ':' + (CUR_SONG_MS || 0);
    if (sig !== stripSig) { stripSig = sig; stripDirty = true; }
    var w = stripCvs.clientWidth || 640, h = 56, dpr = Math.min(2, window.devicePixelRatio || 1);
    if (stripCvs.width !== Math.round(w * dpr) || stripCvs.height !== Math.round(h * dpr)) {
      stripCvs.width = Math.round(w * dpr); stripCvs.height = Math.round(h * dpr);
      stripDirty = true;
    }
    var g = stripCvs.getContext('2d');
    if (!g) return;
    if (!stripLayer || stripDirty) {
      stripLayer = document.createElement('canvas');
      stripLayer.width = stripCvs.width; stripLayer.height = stripCvs.height;
      var l = stripLayer.getContext('2d');
      l.setTransform(dpr, 0, 0, dpr, 0, 0);
      var dur = CUR_SONG_MS || 1, n = CUR_ONSETS.length, i;
      l.fillStyle = 'rgba(125,211,252,.14)';
      l.fillRect(0, h * 0.46, w, Math.max(1, h * 0.08));
      l.globalAlpha = n > 400 ? 0.55 : 0.8;
      for (i = 0; i < n; i++) {
        var x = Math.round(CUR_ONSETS[i] / dur * (w - 1));
        l.fillStyle = MYGO_LANES[i % MYGO_LANES.length];
        l.fillRect(x, h * 0.18, 1, h * 0.64);
      }
      l.globalAlpha = 1;
      stripDirty = false;
    }
    g.setTransform(1, 0, 0, 1, 0, 0);
    g.clearRect(0, 0, stripCvs.width, stripCvs.height);
    g.drawImage(stripLayer, 0, 0);
    g.setTransform(dpr, 0, 0, dpr, 0, 0);
    var dur2 = CUR_SONG_MS || 1;
    var pos = (((songNow() % dur2) + dur2) % dur2) / dur2 * (w - 1);
    g.fillStyle = stripAccent;
    g.fillRect(Math.round(pos) - 1, 0, 2, h);
    g.beginPath(); g.arc(pos, h * 0.5, 3.2, 0, Math.PI * 2); g.fill();
    var note = document.getElementById('stripNote');
    if (note) {
      var secs = Math.round(dur2 / 1000);
      var txt = '谱面 · ' + songLabel + ' —— 整首 ' + Math.floor(secs / 60) + ':' + ('0' + (secs % 60)).slice(-2) +
        ' · ' + CUR_ONSETS.length + ' 个起音点 · 上面每根 tick 都是歌里真实响过的地方';
      if (note.textContent !== txt) note.textContent = txt;
    }
  }
  stripAccentSync();
  window.addEventListener('resize', function () { stripDirty = true; });
  var stripVisible = false, stripTick = 0;
  setInterval(function () {
    if (document.hidden) return;
    /* 视口判断要读布局，每 ~600ms 做一次就够；画本身还是 150ms 一次 */
    if ((stripTick++ % 4) === 0) {
      var r = stripCvs.getBoundingClientRect();
      stripVisible = !(r.bottom < -80 || r.top > (window.innerHeight || 0) + 80);
    }
    if (stripVisible) drawStrip();
  }, 150);
  setTimeout(drawStrip, 400);

  /* ---------- 入口门那行自证徽章：构建时算的哈希 vs 浏览器当场重算 ---------- */
  (function kernelBadge() {
    var el = document.getElementById('landingProof');
    if (!el) return;
    var want = el.getAttribute('data-sha') || '', bytes = el.getAttribute('data-bytes') || '?';
    function show(tail, bad) {
      el.className = 'landing-proof landing-fade mono' + (bad ? ' bad' : '');
      el.textContent = '内核 SHA-256 ' + want + ' · ' + bytes + ' 字节 · ' + tail;
    }
    var b64 = window.__RHYTHM_WASM || '';
    if (!b64) { show('生成时没有内联 wasm'); return; }
    var sub = window.crypto && window.crypto.subtle;
    if (!sub) { show('这个环境没有 crypto.subtle，没复核'); return; }
    try {
      sub.digest('SHA-256', base64ToBytes(b64)).then(function (buf) {
        var v = new Uint8Array(buf), hex = '', i;
        for (i = 0; i < v.length; i++) hex += (v[i] < 16 ? '0' : '') + v[i].toString(16);
        window.__kernelSha = hex;
        if (hex.slice(0, 12) === want) show('浏览器复核一致', false);
        else show('浏览器复核不一致（' + hex.slice(0, 12) + '，以构建时那份为准）', true);
      }).catch(function () { show('复核失败：crypto.subtle 拒绝了'); });
    } catch (e) { show('复核失败：' + (e && e.message ? e.message : '异常')); }
  })();

  window.addEventListener('themechange', function () { render(); stripAccentSync(); });

  /* ============ 拖一首歌进来：C++/WASM 内核当场量起音点 ============ 
     文件不出浏览器：本地解码 → 重采样到 11025Hz → 写进 wasm 内存 → 内核算。
     页面只把结果（毫秒数组）拿回来当谱面，音频连 <audio> 都没出过这台机器。 */
  var ANALYSIS_RATE = 11025;
  var dropBusy = false;

  function onsetKernelReady() {
    return !!(kernelExports && typeof kernelExports.detect_onsets === 'function' &&
      typeof kernelExports.onset_ptr === 'function' && kernelExports.memory);
  }

  function fmtClock(sec) {
    if (!isFinite(sec)) return '—';
    var m = Math.floor(sec / 60), s = Math.floor(sec % 60);
    return m + ':' + (s < 10 ? '0' + s : s);
  }

  /* 多声道混成单声道 */
  function toMono(chans, frames) {
    var out = new Float32Array(frames), c, i;
    if (!chans.length) return out;
    for (c = 0; c < chans.length; c++) {
      var src = chans[c];
      for (i = 0; i < frames; i++) out[i] += src[i] / chans.length;
    }
    return out;
  }

  /* 线性重采样：内核按 23ms 一帧看谱，11025Hz 足够，快很多 */
  function resampleTo(mono, from, to) {
    if (from === to) return mono;
    var n = Math.max(1, Math.floor(mono.length * to / from));
    var out = new Float32Array(n), step = from / to, i;
    for (i = 0; i < n; i++) {
      var x = i * step, i0 = Math.floor(x), f = x - i0;
      var a = mono[i0] || 0, b = mono[i0 + 1] || 0;
      out[i] = a + (b - a) * f;
    }
    return out;
  }

  /* 内核一次吃 20 秒，长歌自己分块：留 1 秒重叠，拼起来再按 93ms 去重 */
  function onsetsFromPcm(pcm, rate) {
    var cap = kernelExports.onset_capacity();
    var inPtr = kernelExports.onset_ptr(), outPtr = kernelExports.onset_out_ptr();
    var out = [], off = 0;
    while (off < pcm.length) {
      var len = Math.min(cap, pcm.length - off);
      new Float32Array(kernelExports.memory.buffer, inPtr, len).set(pcm.subarray(off, off + len));
      var n = kernelExports.detect_onsets(len, rate);
      var got = new Float32Array(kernelExports.memory.buffer, outPtr, n);
      for (var i = 0; i < n; i++) out.push(got[i] + off * 1000 / rate);
      if (len < cap) break;
      off += cap - rate;
    }
    var merged = [];
    for (var k = 0; k < out.length; k++) {
      if (!merged.length || out[k] - merged[merged.length - 1] >= 93) merged.push(out[k]);
    }
    return merged;
  }

  function dropZoneEl() { return document.getElementById('dropZone'); }
  function dropInfoEl() { return document.getElementById('dropInfo'); }
  function showDrop(text, bad) {
    var el = dropInfoEl();
    if (!el) return;
    el.textContent = text;
    el.className = 'drop-info mono reveal' + (bad ? ' bad' : '');
  }

  /* 换谱面来源：重新排一轮时间轴，时钟跟着新歌走 */
  function useChart(onsets, songMs, label, srcUrl) {
    CUR_ONSETS = onsets;
    CUR_SONG_MS = songMs;
    songLabel = label;
    timeline = [];
    spawnIdx = 0;
    if (music && srcUrl && music.getAttribute('src') !== srcUrl) {
      music.src = srcUrl;
      if (music.load) music.load();
    }
    songRezero();
    timeline = buildTimeline(songNow(), ROUND_NOTES * 2);
    spawnIdx = 0;
    if (clockChip) clockChip.textContent = '跟随 ' + label;
  }

  function backToBuiltinSong() {
    useChart(ONSETS, SONG_MS, '本站的歌', BUILTIN_SRC);
    if (clockChip) clockChip.textContent = '跟随音轨';
    var btn = document.getElementById('dropReset');
    if (btn) btn.hidden = true;
    showDrop('当前谱面：本站的歌 · 内联了 ' + ONSETS.length + ' 个实测起音点', false);
  }

  function handleAudioFile(file) {
    if (!file || dropBusy) return;
    if (!onsetKernelReady()) {
      showDrop('这个浏览器没跑起 wasm 内核，拖歌进来也只能用本站的歌', true);
      return;
    }
    var AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) { showDrop('这个浏览器没有 WebAudio，解不了音频', true); return; }
    dropBusy = true;
    showDrop('正在量《' + file.name + '》的起音点…', false);
    var ctx = new AC();
    var reader = new FileReader();
    reader.onload = function () {
      var done = function (audio) {
        try {
          var chans = [], c;
          for (c = 0; c < audio.numberOfChannels; c++) chans.push(audio.getChannelData(c));
          var mono = toMono(chans, audio.length);
          var pcm = resampleTo(mono, audio.sampleRate, ANALYSIS_RATE);
          var t0 = perfNow();
          var onsets = onsetsFromPcm(pcm, ANALYSIS_RATE);
          var spent = Math.round(perfNow() - t0);
          if (!onsets.length) {
            dropBusy = false;
            showDrop('这首歌没量出起音点（太安静？全是持续音？）—— 还是用本站的歌吧', true);
            return;
          }
          if (ctx.close) ctx.close();
          useChart(onsets, audio.duration * 1000, file.name.slice(0, 16), URL.createObjectURL(file));
          dropBusy = false;
          var btn = document.getElementById('dropReset');
          if (btn) btn.hidden = false;
          showDrop('当前谱面：' + file.name + ' · 量到 ' + onsets.length + ' 个起音点 · 曲长 ' +
            fmtClock(audio.duration) + ' · 内核算了 ' + spent + 'ms（文件没上传）', false);
        } catch (e) {
          dropBusy = false;
          showDrop('量失败了：' + (e && e.message ? e.message : e), true);
        }
      };
      if (ctx.decodeAudioData.length >= 3) {
        ctx.decodeAudioData(reader.result, done, function () {
          dropBusy = false;
          showDrop('这个格式浏览器解不开（试试 mp3 / m4a / wav / ogg）', true);
        });
      } else {
        ctx.decodeAudioData(reader.result).then(done, function () {
          dropBusy = false;
          showDrop('这个格式浏览器解不开（试试 mp3 / m4a / wav / ogg）', true);
        });
      }
    };
    reader.onerror = function () { dropBusy = false; showDrop('读文件失败了', true); };
    reader.readAsArrayBuffer(file);
  }

  function initDropZone() {
    var zone = dropZoneEl(), input = document.getElementById('dropFile');
    if (!zone) return;
    zone.addEventListener('click', function () { if (input) input.click(); });
    zone.addEventListener('keydown', function (e) {
      if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); if (input) input.click(); }
    });
    zone.addEventListener('dragover', function (e) { e.preventDefault(); zone.classList.add('over'); });
    zone.addEventListener('dragleave', function () { zone.classList.remove('over'); });
    zone.addEventListener('drop', function (e) {
      e.preventDefault();
      zone.classList.remove('over');
      var dt = e.dataTransfer;
      if (dt && dt.files && dt.files.length) handleAudioFile(dt.files[0]);
    });
    if (input) input.addEventListener('change', function () {
      if (input.files && input.files.length) handleAudioFile(input.files[0]);
      input.value = '';
    });
    var reset = document.getElementById('dropReset');
    if (reset) reset.addEventListener('click', function (e) { e.stopPropagation(); backToBuiltinSong(); });
    showDrop('当前谱面：本站的歌 · 内联了 ' + ONSETS.length + ' 个实测起音点 · ' +
      (onsetKernelReady() ? 'wasm 内核就绪，拖别的歌进来当场量' : 'wasm 内核还没起来'), false);
  }

  /* 自检钩子：把一段 PCM 直接喂给内核，用于无头测试 */
  window.__onsetsFromPcm = function (arr, rate) {
    if (!onsetKernelReady()) return null;
    return onsetsFromPcm(arr, rate || ANALYSIS_RATE);
  };
  window.__onsetState = function () {
    return {
      parity: window.__onsetParity || 'n/a',
      ready: onsetKernelReady(),
      source: songLabel,
      onsets: CUR_ONSETS.length,
      songMs: Math.round(CUR_SONG_MS),
      probe: window.__onsetProbe || null
    };
  };

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', initDropZone);
  else initDropZone();
  /* 自检钩子：判定时钟现在跟着谁走、下一个音符是不是真的落在实测起音点上 */
  window.__rhythmClockProbe = function () {
    var at = spawnIdx < timeline.length ? timeline[spawnIdx] : null;
    var pos = at === null ? null : at % CUR_SONG_MS;
    return {
      src: audioLive() ? 'audio' : 'virtual',
      song: songNow(),
      audio: music ? music.currentTime * 1000 : 0,
      onsets: CUR_ONSETS.length,
      songMs: CUR_SONG_MS,
      timeline: timeline.length,
      minGap: (function () { var mg = 1e9, i2; for (i2 = 1; i2 < timeline.length; i2++) { var d2 = timeline[i2] - timeline[i2 - 1]; if (d2 < mg) mg = d2; } return timeline.length > 1 ? Math.round(mg) : null; })(),
      spawnIdx: spawnIdx,
      next: at,
      nextIsOnset: pos === null ? null : CUR_ONSETS.indexOf(pos) >= 0
    };
  };
  window.__rhythmNextDelta = function (lane) {
    var h = nearestNote(lane, songNow());
    return h ? Math.round(h.delta) : null;
  };
  window.__rhythmLaneOfNext = function () {
    var pick = null, bestD = 1e9;
    var now = songNow();
    for (var i = 0; i < notes.length; i++) {
      var n = notes[i];
      if (n.judged || !n.hitAt) continue;
      var d = n.hitAt - now;
      if (d < bestD) { bestD = d; pick = n; }
    }
    return pick ? pick.lane : null;
  };
  window.__rhythmStartRound = startRound;
  window.__rhythmEndRound = endRound;
  window.__rhythmBackToAuto = backToAuto;
  window.__rhythmAccuracyPermille = function () { return round.accPermille; };
  window.__rhythmJudgeDelta = doJudge;
  /* 内核不可用时的 JS 镜像也暴露出来：自检会断言它与内核在同一组探针上给同样的答案 */
  window.__rhythmJudgeFallback = jsJudge;
  window.__rhythmAccuracyFallback = jsAcc;

  function spawn(hitAt) {
    /* hitAt 是「歌曲时间」里的落点：由时间轴给，不再由当前时刻推 */
    var lane = chartLane();
    if (!(lane >= 0 && lane < LANES)) lane = 0;
    /* 同一轨道 260ms 内连着来两个会完全叠在一起，往旁边挪一格（时间戳不动） */
    if (lane === lastLane && hitAt - lastHitAt < 260) lane = (lane + 1) % LANES;
    lastLane = lane; lastHitAt = hitAt;
    notes.push({ lane: lane, y: -14, dead: 0, hitAt: hitAt, judged: 0, missed: 0 });
  }

  function explode(x, y, lane) {
    for (var i = 0; i < 7; i++) {
      sparks.push({
        x: x, y: y,
        vx: (Math.random() - 0.5) * 0.34,
        vy: -Math.random() * 0.30 - 0.04,
        life: 1
      });
    }
    laneGlow[lane] = 1;
  }

  function update(dt, now) {
    /* 排谱：到点就生成，y 从 -14 落到判定线正好 TRAVEL 毫秒 */
    if (timeline.length) {
      var guard = 0;
      while (spawnIdx < timeline.length && now >= timeline[spawnIdx] - TRAVEL && guard++ < 8) {
        if (mode === 'play') { roundIndex++; setModeUI(); }
        spawn(timeline[spawnIdx]);
        spawnIdx++;
      }
      if (mode === 'auto' && spawnIdx >= timeline.length) {
        /* 自动演奏：这一排打完再排下一排，一直循环下去 */
        timeline = buildTimeline(now, ROUND_NOTES * 2);
        spawnIdx = 0;
      }
    } else {
      acc += dt;
      var guard2 = 0;
      while (acc >= BEAT && guard2++ < 4) {
        acc -= BEAT;
        if (mode === 'play') { if (roundIndex >= ROUND_NOTES) break; roundIndex++; setModeUI(); }
        spawn(now + TRAVEL);
      }
    }

    var speed = (hitY + 20) / TRAVEL;
    for (var i = notes.length - 1; i >= 0; i--) {
      var n = notes[i];
      if (n.dead) {
        n.dead += dt;
        if (n.dead > 240) notes.splice(i, 1);
        continue;
      }
      n.y += speed * dt;
      if (mode === 'play') {
        /* 打中的音符已经在上面的 dead 分支里淡出；漏掉的让它继续落，落出画面才删 */
        if (n.judged) { if (n.y > H + 24) notes.splice(i, 1); continue; }
        if (now > n.hitAt + GOODms) registerJudge(0, n);
        if (n.y > H + 24) notes.splice(i, 1);
      } else if (n.y >= hitY) {
        n.y = hitY;
        n.dead = 1;
        combo++;
        glow = 1;
        var ne = $('#comboNum');
        ne.textContent = combo;
        ne.classList.remove('pop');
        void ne.offsetWidth;
        ne.classList.add('pop');
        explode(n.lane * laneW + laneW / 2, hitY, n.lane);
      }
    }

    for (var s = sparks.length - 1; s >= 0; s--) {
      var p = sparks[s];
      p.x += p.vx * dt; p.y += p.vy * dt; p.vy += 0.0016 * dt;
      p.life -= dt / 620;
      if (p.life <= 0) sparks.splice(s, 1);
    }

    if (glow > 0) glow = Math.max(0, glow - dt / 420);
    for (var g = 0; g < LANES; g++) {
      if (laneGlow[g] > 0) laneGlow[g] = Math.max(0, laneGlow[g] - dt / 500);
    }

    /* 一轮打完（最后一个音符也判完了）就结算 */
    if (mode === 'play' && roundActive && (timeline.length ? spawnIdx >= timeline.length : roundIndex >= ROUND_NOTES)) {
      var pending = false;
      for (var q = 0; q < notes.length; q++) { if (!notes[q].judged) { pending = true; break; } }
      if (!pending) endRound();
    }
  }

  function render() {
    if (!ctx) return;
    ctx.clearRect(0, 0, W, H);
    var mg = document.documentElement.classList.contains('theme-mygo');

    ctx.fillStyle = 'rgba(125,211,252,' + (0.55 + glow * 0.45) + ')';
    ctx.fillRect(0, hitY + 10, W, 3);
    ctx.fillStyle = 'rgba(232,121,249,' + (0.18 + glow * 0.35) + ')';
    ctx.fillRect(0, hitY + 14, W, 1);

    ctx.fillStyle = 'rgba(125,211,252,.07)';
    for (var i = 1; i < LANES; i++) ctx.fillRect(i * laneW - 0.5, 0, 1, H);

    /* 可玩模式才画判定区：七块踏板，按下去会亮 */
    if (mode === 'play') {
      var padY = hitY + 22, padH = Math.max(12, H - padY - 12);
      ctx.fillStyle = 'rgba(125,211,252,.05)';
      ctx.fillRect(0, hitY + 16, W, H - hitY - 16);
      for (var pd = 0; pd < LANES; pd++) {
        ctx.fillStyle = laneGlow[pd] > 0.05
          ? 'rgba(125,211,252,' + (0.16 + laneGlow[pd] * 0.5).toFixed(3) + ')'
          : 'rgba(255,255,255,.05)';
        roundRect(pd * laneW + 6, padY, laneW - 12, padH, 8);
        ctx.fill();
      }
    }

    for (var g = 0; g < LANES; g++) {
      if (laneGlow[g] > 0.01) {
        var lg = ctx.createLinearGradient(0, hitY - 190, 0, hitY + 12);
        lg.addColorStop(0, 'rgba(125,211,252,0)');
        lg.addColorStop(1, 'rgba(125,211,252,' + (laneGlow[g] * 0.22) + ')');
        ctx.fillStyle = lg;
        ctx.fillRect(g * laneW + 0.5, hitY - 190, laneW - 1, 202);
      }
    }

    for (var n = 0; n < notes.length; n++) {
      var note = notes[n];
      var x = note.lane * laneW + laneW * 0.14;
      var w = laneW * 0.72;
      if (note.dead) {
        var k = note.dead / 240;
        ctx.globalAlpha = 1 - k;
        ctx.fillStyle = '#ffffff';
        roundRect(x - w * 0.3 * k, note.y - 6 - 10 * k, w * (1 + 0.6 * k), 13 * (1 + k), 7);
        ctx.fill();
        ctx.globalAlpha = 1;
      } else {
        ctx.fillStyle = mg && laneGrad ? laneGrad[note.lane % laneGrad.length] : noteGrad;
        roundRect(x, note.y, w, 13, 7);
        ctx.fill();
      }
    }

    for (var s = 0; s < sparks.length; s++) {
      var p = sparks[s];
      ctx.globalAlpha = Math.max(0, p.life);
      ctx.fillStyle = '#dff3ff';
      ctx.fillRect(p.x, p.y, 2.4, 2.4);
    }
    ctx.globalAlpha = 1;
  }

  function loop(t) {
    if (!running) return;
    var now = songNow();
    /* 歌从头开始（循环播放或手动 seek）→ 谱面跟着重排，别让旧音符卡在未来 */
    if (songLast && now < songLast - 250) {
      notes.length = 0; sparks.length = 0;
      timeline = buildTimeline(now, mode === 'play' ? ROUND_NOTES : ROUND_NOTES * 2);
      spawnIdx = 0;
    }
    var dt = songLast ? Math.min(Math.max(now - songLast, 0), 64) : 16;
    songLast = now;
    if (!document.hidden) {
      update(dt, now);
      render();
      if (clockChip) {
        var label = audioLive()
          ? '跟随音轨 ' + music.currentTime.toFixed(0) + 's'
          : (CUR_ONSETS.length ? '起音 ' + CUR_ONSETS.length + ' 个 · 虚拟时钟' : '固定节拍');
        if (clockChip.textContent !== label) clockChip.textContent = label;
      }
    }
    rafId = requestAnimationFrame(loop);
  }

  function startRhythm() {
    /* 「减少动态效果」下自动演奏不跑（那是装饰），但用户自己点开始挑战时照跑 */
    if (!ctx || running || (reduce && mode === 'auto')) { if (ctx) render(); return; }
    running = true;
    prev = 0; acc = 0; songLast = 0;
    /* 第一次跑起来（滚到音游这一段）时排谱：从当前歌曲位置往后取一段起音点 */
    if (!timeline.length) { timeline = buildTimeline(songNow(), ROUND_NOTES * 2); spawnIdx = 0; }
    /* 从暂停里回来：把没有判定的音符整体后移暂停的那段，别让它们瞬间全变成 MISS */
    if (pausedAt) {
      var gap = songNow() - pausedAt;
      pausedAt = 0;
      for (var i = 0; i < notes.length; i++) {
        if (!notes[i].judged && notes[i].hitAt) notes[i].hitAt += gap;
      }
    }
    setModeUI();
    rafId = requestAnimationFrame(loop);
  }

  function stopRhythm() {
    running = false;
    pausedAt = songNow();
    if (rafId) { cancelAnimationFrame(rafId); rafId = 0; }
  }

  if (cvs) {
    fitCanvas();
    seen = true;
    loadKernel();
    loadBest();
    setModeUI();
    refreshStats();

    var playBtn = $('#rhythmPlay'), stopBtn = $('#rhythmStop');
    if (playBtn) playBtn.addEventListener('click', function () { startRound(); });
    if (stopBtn) stopBtn.addEventListener('click', function () { backToAuto(); });

    /* 谱面 seed：输入框 / 回车 / 回默认（也可以直接 ?seed=xxx 进来） */
    var seedBtn = $('#seedApply'), seedReset = $('#seedReset'), seedInput = $('#seedInput');
    if (seedBtn) seedBtn.addEventListener('click', function () { applySeed(seedInput ? seedInput.value : ''); });
    if (seedInput) seedInput.addEventListener('keydown', function (e) {
      if (e.key === 'Enter') { e.preventDefault(); applySeed(seedInput.value); }
    });
    if (seedReset) seedReset.addEventListener('click', function () { if (seedInput) seedInput.value = ''; applySeed(''); });

    /* 键盘：D F G H J K L 对应七条轨道，数字 1–7 也行（只在挑战进行中生效） */
    document.addEventListener('keydown', function (e) {
      if (e.repeat || e.metaKey || e.ctrlKey || e.altKey) return;
      if (mode !== 'play' || !roundActive) return;
      var k = e.key, lane = -1;
      if (typeof k === 'string' && k.length === 1) {
        var lo = k.toLowerCase(), ki = KEYS.indexOf(lo);
        if (ki >= 0) lane = ki;
        else { var ni = NUMS.indexOf(k); if (ni >= 0) lane = ni; }
      }
      if (lane < 0 || lane >= LANES) return;
      e.preventDefault();
      hitLane(lane, songNow());
    });

    /* 触屏/鼠标：按 x 折算轨道，直接用手指打 */
    cvs.addEventListener('pointerdown', function (e) {
      if (mode !== 'play' || !roundActive) return;
      var rect = cvs.getBoundingClientRect();
      if (!rect.width) return;
      var lane = Math.floor((e.clientX - rect.left) / (rect.width / LANES));
      if (lane < 0) lane = 0;
      if (lane > LANES - 1) lane = LANES - 1;
      e.preventDefault();
      hitLane(lane, songNow());
    });

    window.addEventListener('resize', function () {
      clearTimeout(cvs.__t);
      cvs.__t = setTimeout(fitCanvas, 180);
    }, { passive: true });
    /* 字号/布局变化会让 canvas 的 CSS 尺寸变、而 resize 事件不一定来：
       用 ResizeObserver 盯住它，背板尺寸就不会和显示尺寸对不上 */
    if (window.ResizeObserver) {
      new ResizeObserver(function () { fitCanvas(); }).observe(cvs);
    }
    if ('IntersectionObserver' in window) {
      new IntersectionObserver(function (entries) {
        for (var i = 0; i < entries.length; i++) {
          if (entries[i].isIntersecting) startRhythm(); else stopRhythm();
        }
      }, { threshold: 0.2 }).observe(cvs);
    }
    document.addEventListener('visibilitychange', function () {
      if (document.hidden) stopRhythm(); else if (seen) startRhythm();
    });
  }

  /* 进入后启动音游 */
  var watch = setInterval(function () {
    if (page.classList.contains('visible')) {
      clearInterval(watch);
      setTimeout(startRhythm, reduce ? 300 : 1200);
    }
  }, 250);

  window.addEventListener('load', function () { initVizCanvas(); });
  setMuted(true);

  /* 音频通路自检用：强制走一次降级，验证换元素后音乐还在放 */
  window.__audioForceFallback = switchToPlain;
  window.__audioWatch = audioWatch;
  window.__audioIsPlain = function () { return audioFallback || AUDIO_PLAIN; };
  window.__audioMode = function () { return AUDIO_PLAIN ? 'plain-forced' : audioFallback ? 'plain-fallback' : 'graph'; };
  window.__audioCtxInfo = function () { return audioCtx ? audioCtx.sampleRate + '/' + audioCtx.state : 'none'; };
  startAudioDebug();

  /* 预览/截图用：index.html?enter=1 跳过封面直接进入 */
  if (/[?&]enter=1/.test(location.search)) setTimeout(enterSite, 120);
})();
)JS";

/* ==================================================================
 *  页面骨架
 * ================================================================== */

const char* kHead = R"HEAD(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<!-- 第一件事：打上 js / entry 标记。整站默认按「没有 JavaScript」渲染 ——
     内容直接可读、不锁滚动、不做入场动画；只有这两个标记在，才会有入口门。 -->
<script>document.documentElement.classList.add('js','entry');/* 主题必须在首帧之前定下来，否则会闪一下白 */try{var q=new URLSearchParams(location.search).get('theme');if(!q){q=localStorage.getItem('myintro.theme')||(matchMedia('(prefers-color-scheme: dark)').matches?'mygo':'light')}if(q==='mygo')document.documentElement.classList.add('theme-mygo')}catch(e){}</script>
<meta name="description" content="__NAME__的个人主页：C++ 开发者，在学编程，也看动漫。整站由 C++ 生成。">
<meta name="theme-color" content="#f6f7fc">
<link rel="canonical" href="__SITE__">
<meta property="og:type" content="website">
<meta property="og:site_name" content="__NAME__ · 个人主页">
<meta property="og:locale" content="zh_CN">
<meta property="og:url" content="__SITE__">
<meta property="og:title" content="__NAME__ · 个人主页">
<meta property="og:description" content="写代码，也看番。整站由 C++ 生成，部署在 GitHub Pages。">
<!-- 分享卡片用专门生成的 1200×630 图（tools/make-og.mjs 渲染，作为构建输入提交在仓库里） -->
<meta property="og:image" content="__SITE__og-image.jpg">
<meta property="og:image:type" content="image/jpeg">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta property="og:image:alt" content="__NAME__ 的主页分享卡片：C++17 生成器与 WASM 音游内核的构建命令">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="__NAME__ · 个人主页">
<meta name="twitter:description" content="写代码，也看番。整站由 C++ 生成，部署在 GitHub Pages。">
<meta name="twitter:image" content="__SITE__og-image.jpg">
<meta name="twitter:image:alt" content="__NAME__ 的主页分享卡片">
<link rel="icon" type="image/svg+xml" href="favicon.svg">
<link rel="preload" as="image" href="cover.jpg" fetchpriority="high">
<link rel="preload" as="font" type="font/woff2" href="assets/fonts/lxgw-wenkai-display.woff2" crossorigin>
<link rel="preload" as="font" type="font/woff2" href="assets/fonts/jetbrains-mono-regular.woff2" crossorigin>
<link rel="preload" as="font" type="font/woff2" href="assets/fonts/jetbrains-mono-bold.woff2" crossorigin>
<title>__NAME__ · 个人主页</title>
<style>
__CSS__
</style>
</head>
<body>
<a class="skip" href="#about">跳到正文</a>
)HEAD";

/* ==================================================================
 *  favicon（矢量，无 emoji）
 * ================================================================== */

const char* kFavicon = R"ICON(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <defs>
    <linearGradient id="g" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#6d4dfa"/>
      <stop offset="1" stop-color="#0ea5e9"/>
    </linearGradient>
  </defs>
  <rect width="64" height="64" rx="15" fill="#ffffff"/>
  <rect x="2" y="2" width="60" height="60" rx="13.5" fill="none" stroke="url(#g)" stroke-width="2.6"/>
  <path d="M25 22 L15.5 32 L25 42" fill="none" stroke="url(#g)" stroke-width="4.6" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M39 22 L48.5 32 L39 42" fill="none" stroke="url(#g)" stroke-width="4.6" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M35.5 17.5 L28.5 46.5" fill="none" stroke="#c026d3" stroke-width="4" stroke-linecap="round"/>
</svg>
)ICON";

/* ==================================================================
 *  组装
 * ================================================================== */

std::string replaceAll(std::string src, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = src.find(from, pos)) != std::string::npos) {
        src.replace(pos, from.size(), to);
        pos += to.size();
    }
    return src;
}

/* SHA-256：入口门那行自证徽章用 —— 生成器算一遍内联进页面的 wasm 字节，
   浏览器再用 crypto.subtle 对同一份字节算一遍，对不上页面会自己说对不上 */
namespace {
inline uint32_t shaRotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
const uint32_t kShaK[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
struct Sha256 {
    uint32_t h[8];
    unsigned char buf[64];
    size_t n = 0;
    uint64_t total = 0;
    Sha256() {
        h[0] = 0x6a09e667u; h[1] = 0xbb67ae85u; h[2] = 0x3c6ef372u; h[3] = 0xa54ff53au;
        h[4] = 0x510e527fu; h[5] = 0x9b05688cu; h[6] = 0x1f83d9abu; h[7] = 0x5be0cd19u;
    }
    void block(const unsigned char* p) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++)
            w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) | (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = shaRotr(w[i - 15], 7) ^ shaRotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = shaRotr(w[i - 2], 17) ^ shaRotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = shaRotr(e, 6) ^ shaRotr(e, 11) ^ shaRotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + kShaK[i] + w[i];
            uint32_t S0 = shaRotr(a, 2) ^ shaRotr(a, 13) ^ shaRotr(a, 22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    void update(const unsigned char* p, size_t len) {
        total += len;
        while (len) {
            size_t take = 64 - n; if (take > len) take = len;
            for (size_t i = 0; i < take; i++) buf[n + i] = p[i];
            n += take; p += take; len -= take;
            if (n == 64) { block(buf); n = 0; }
        }
    }
    std::string hex() {
        uint64_t bits = total * 8;
        unsigned char pad[72];
        size_t padLen = (n < 56) ? (56 - n) : (120 - n);
        pad[0] = 0x80;
        for (size_t i = 1; i < padLen; i++) pad[i] = 0;
        for (int i = 0; i < 8; i++) pad[padLen + i] = (unsigned char)(bits >> (56 - i * 8));
        update(pad, padLen + 8);
        static const char* hexd = "0123456789abcdef";
        std::string out;
        for (int i = 0; i < 8; i++)
            for (int j = 3; j >= 0; j--) {
                uint32_t byte = (h[i] >> (j * 8)) & 0xffu;
                out.push_back(hexd[byte >> 4]);
                out.push_back(hexd[byte & 0xf]);
            }
        return out;
    }
};
}  // namespace
std::string sha256Hex(const std::string& raw) {
    Sha256 s;
    s.update(reinterpret_cast<const unsigned char*>(raw.data()), raw.size());
    return s.hex();
}

/* base64：把 rhythm_core.wasm 内联进 HTML（避免额外请求、也避免 MIME/路径问题） */
std::string base64(const std::string& raw) {
    static const char* kTable =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((raw.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < raw.size()) {
        unsigned n = (static_cast<unsigned char>(raw[i]) << 16) |
                     (static_cast<unsigned char>(raw[i + 1]) << 8) |
                      static_cast<unsigned char>(raw[i + 2]);
        out.push_back(kTable[(n >> 18) & 63]);
        out.push_back(kTable[(n >> 12) & 63]);
        out.push_back(kTable[(n >> 6) & 63]);
        out.push_back(kTable[n & 63]);
        i += 3;
    }
    size_t rest = raw.size() - i;
    if (rest == 1) {
        unsigned n = static_cast<unsigned char>(raw[i]) << 16;
        out.push_back(kTable[(n >> 18) & 63]);
        out.push_back(kTable[(n >> 12) & 63]);
        out += "==";
    } else if (rest == 2) {
        unsigned n = (static_cast<unsigned char>(raw[i]) << 16) |
                     (static_cast<unsigned char>(raw[i + 1]) << 8);
        out.push_back(kTable[(n >> 18) & 63]);
        out.push_back(kTable[(n >> 12) & 63]);
        out.push_back(kTable[(n >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

/* 读文件，失败返回空串 */
std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return std::string();
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

/* 写文件，返回是否成功 */
bool writeFile(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out << data;
    return true;
}

/* 404 页：GitHub Pages 会用它接管所有不存在的路径，所以自带样式、不依赖别的资源 */
std::string build404(const Profile& me) {
    std::string s = "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n";
    s += "<meta charset=\"UTF-8\">\n";
    s += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    s += "<meta name=\"robots\" content=\"noindex\">\n";
    s += "<title>404 · " + me.name + " 的个人主页</title>\n";
    s += "<link rel=\"icon\" type=\"image/svg+xml\" href=\"favicon.svg\">\n";
    s += R"HTML(<style>
:root{--ink:#151a2e;--dim:rgba(21,26,46,.74);--violet:#6d4dfa;--sky:#0ea5e9}
*{margin:0;padding:0;box-sizing:border-box}
body{min-height:100vh;display:grid;place-items:center;padding:28px;text-align:center;
  font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Microsoft YaHei',sans-serif;
  color:var(--ink);background:linear-gradient(180deg,var(--bg-top),var(--bg-bot))}
.code{font-family:Consolas,'Cascadia Mono',monospace;font-size:clamp(64px,18vw,132px);font-weight:700;
  letter-spacing:6px;background:linear-gradient(110deg,var(--grad-start),var(--violet) 60%,var(--sky));
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}
h1{margin-top:10px;font-size:clamp(18px,3.4vw,24px)}
p{margin-top:14px;color:var(--dim);font-size:14.5px;line-height:1.9}
a{display:inline-block;margin-top:26px;padding:13px 30px;border-radius:999px;color:#fff;text-decoration:none;
  font-size:15px;font-weight:600;background:linear-gradient(110deg,var(--violet),#3b82f6);
  box-shadow:0 18px 34px -18px rgba(109,77,250,.8)}
</style>
</head>
<body>
<main>
  <p class="code">404</p>
  <h1>这个地址没有内容</h1>
  <p>可能链接写错了，或者那一页还没做出来。<br>回主页看看别的吧。</p>
  <a href="/my-intro/">回到主页</a>
</main>
</body>
</html>
)HTML";
    return s;
}

/* robots.txt 与 sitemap.xml：整站只有一个页面，写死即可 */
std::string buildRobots(const Profile& me) {
    return "User-agent: *\nAllow: /\n\nSitemap: " + me.site + "sitemap.xml\n";
}

std::string buildSitemap(const Profile& me) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n"
           "  <url><loc>" + me.site + "</loc><changefreq>monthly</changefreq>"
           "<priority>1.0</priority></url>\n"
           "  <url><loc>" + me.site + "kernel.html</loc><changefreq>monthly</changefreq>"
           "<priority>0.6</priority></url>\n"
           "  <url><loc>" + me.site + "nojs.html</loc><changefreq>monthly</changefreq>"
           "<priority>0.6</priority></url>\n"
           "</urlset>\n";
}

/* ==================================================================
 *  工具页：kernel.html（内核控制台）与 nojs.html（无 JS 对照）
 *  两个页面都由生成器写出，与主页共用同一套 CSS 与内联常量。
 *  表格里的期望值由生成器用 rhythm_core.hpp 算好写进 HTML；页面里的
 *  JS 再用 WebAssembly 内核算一遍逐行比对，不一致就在表格里标红。
 * ================================================================== */

const char* kToolCSS = R"TSS(
/* 工具页：沿用主页的令牌与组件，只补控制台/对照页需要的块 */
.tool-main{max-width:1080px;margin:0 auto;padding:104px 24px 72px}
.tool-h1{font-size:clamp(30px,4.6vw,46px);font-weight:800;letter-spacing:-.5px;margin:10px 0 14px}
.tool-lead{color:var(--ink-dim);font-size:15.5px;line-height:1.95;max-width:76ch}
.tool-lead code,.tbl code{font-family:var(--mono);font-size:.94em;padding:1px 5px;border-radius:5px;background:rgba(109,77,250,.09)}
.tool-links{display:flex;flex-wrap:wrap;gap:8px 18px;margin-top:20px;font-size:14px}
.tool-links a{color:var(--violet);text-decoration:none;border-bottom:1px solid rgba(109,77,250,.3)}
.tool-links a:hover{border-bottom-color:var(--violet)}
.panel{margin-top:26px;padding:24px 24px 20px;border-radius:18px;background:var(--panel);border:1px solid var(--line);
  box-shadow:0 34px 74px -64px rgba(40,50,110,.8)}
.panel h2{font-size:19px;margin-bottom:8px}
.panel p{color:var(--ink-dim);font-size:14.5px;line-height:1.9}
.ctl{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin:18px 0 4px}
.ctl label{font-family:var(--mono);font-size:11.5px;letter-spacing:1.6px;color:var(--ink-faint)}
.ctl input,.ctl select{padding:10px 12px;border-radius:10px;border:1px solid var(--line);background:var(--panel);color:var(--ink);
  font-family:var(--mono);font-size:13.5px;min-width:150px}
.ctl input:focus,.ctl select:focus{outline:2px solid rgba(109,77,250,.35);outline-offset:1px}
.rbtn{font-family:inherit}
.mono-line{font-family:var(--mono);font-size:12.5px;color:var(--ink-dim);word-break:break-all;line-height:1.9;margin-top:10px}
.mono-line b{color:var(--violet)}
.ok-text{color:var(--ok-ink)}
.bad-text{color:var(--bad-ink);font-weight:700}
.grid-wrap{margin-top:16px;padding:12px;border-radius:14px;background:#0d1224;border:1px solid rgba(109,77,250,.3)}
#grid{display:block;width:100%;height:250px}
.tbl{width:100%;border-collapse:collapse;margin-top:14px;font-family:var(--mono);font-size:13px}
.tbl th,.tbl td{padding:8px 10px;text-align:left;border-bottom:1px solid var(--line)}
.tbl th{color:var(--ink-faint);font-weight:600;letter-spacing:1px;font-size:11px;text-transform:uppercase}
.tbl td.ok{color:var(--ok-ink)}
.tbl td.bad{color:var(--bad-ink);font-weight:700}
.tbl tr[data-bad="1"]{background:rgba(185,28,28,.06)}
.frames{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin-top:22px}
@media (max-width:900px){.frames{grid-template-columns:1fr}}
.frame-card{display:flex;flex-direction:column;border-radius:16px;overflow:hidden;border:1px solid var(--line);background:#fff}
.frame-card header{display:flex;align-items:center;gap:8px;padding:10px 14px;border-bottom:1px solid var(--line);
  font-family:var(--mono);font-size:11.5px;letter-spacing:1px;color:var(--ink-dim)}
.frame-card iframe{width:100%;height:520px;border:0;background:#fff}
@media (max-width:620px){.frame-card iframe{height:420px}}
.tool-foot{margin-top:36px;padding-top:18px;border-top:1px solid var(--line);color:var(--ink-faint);font-size:13px}
)TSS";

const char* kBrandMark = R"BM(<svg viewBox="0 0 64 64" aria-hidden="true">
      <defs><linearGradient id="bg1" x1="0" y1="0" x2="1" y2="1">
        <stop offset="0" stop-color="#6d4dfa"/><stop offset="1" stop-color="#0ea5e9"/></linearGradient></defs>
      <rect width="64" height="64" rx="15" fill="#ffffff"/>
      <rect x="2" y="2" width="60" height="60" rx="13.5" fill="none" stroke="url(#bg1)" stroke-width="2.6"/>
      <path d="M25 22 L15.5 32 L25 42" fill="none" stroke="url(#bg1)" stroke-width="4.6" stroke-linecap="round" stroke-linejoin="round"/>
      <path d="M39 22 L48.5 32 L39 42" fill="none" stroke="url(#bg1)" stroke-width="4.6" stroke-linecap="round" stroke-linejoin="round"/>
      <path d="M35.5 17.5 L28.5 46.5" fill="none" stroke="#c026d3" stroke-width="4" stroke-linecap="round"/>
    </svg>)BM";

std::string toolHead(const Profile& me, const std::string& title, const std::string& desc, const std::string& slug) {
    std::string s;
    s += "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n<meta charset=\"UTF-8\">\n";
    s += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    /* 工具页没有入口门：内容本身不需要等脚本，只打一个 js 标记给「实时部分」用 */
    s += "<script>document.documentElement.classList.add('js');try{var q=new URLSearchParams(location.search).get('theme');if(!q){q=localStorage.getItem('myintro.theme')||(matchMedia('(prefers-color-scheme: dark)').matches?'mygo':'light')}if(q==='mygo')document.documentElement.classList.add('theme-mygo')}catch(e){}</script>\n";
    s += "<meta name=\"description\" content=\"" + desc + "\">\n";
    s += "<meta name=\"theme-color\" content=\"#f6f7fc\">\n";
    s += "<link rel=\"canonical\" href=\"" + me.site + slug + "\">\n";
    s += "<meta property=\"og:type\" content=\"website\">\n";
    s += "<meta property=\"og:title\" content=\"" + title + "\">\n";
    s += "<meta property=\"og:description\" content=\"" + desc + "\">\n";
    s += "<meta property=\"og:image\" content=\"" + me.site + "og-image.jpg\">\n";
    s += "<meta name=\"twitter:card\" content=\"summary_large_image\">\n";
    s += "<link rel=\"icon\" type=\"image/svg+xml\" href=\"favicon.svg\">\n";
    s += "<title>" + title + "</title>\n<style>\n";
    s += kCSS;
    s += kToolCSS;
    s += "</style>\n</head>\n<body>\n";
    return s;
}

std::string toolNav(const Profile& me) {
    std::string s;
    s += "<nav class=\"nav visible\" id=\"nav\" aria-label=\"站内导航\">\n";
    s += "  <a class=\"brand\" href=\"index.html\">\n    " + std::string(kBrandMark) + "\n";
    s += "    <span>" + me.name + "<span class=\"brand-sub mono\"> / TOOLS</span></span>\n  </a>\n";
    s += "  <button class=\"icon-btn theme-btn\" id=\"themeBtn\" type=\"button\" aria-label=\"切换主题\" aria-pressed=\"false\"></button>\n";
    s += "</nav>\n";
    return s;
}

/* 工具页共用的一组内联常量：与主页同一套（内核 base64 + seed + 轨道数 + 探针） */
std::string toolConstants(const std::string& chart, const std::string& wasmB64, const std::string& judgeProbe,
                          const std::string& judgeExpect, const std::string& accProbe, const std::string& accExpect) {
    std::string s = "<script>\n";
    s += "window.__RHYTHM_CHART = '" + chart + "';\n";
    s += "window.__RHYTHM_SEED = '" + std::to_string(rhythm::kSeed) + "';\n";
    s += "window.__RHYTHM_LANES = '" + std::to_string(rhythm::kLanes) + "';\n";
    s += "window.__RHYTHM_WASM = '" + wasmB64 + "';\n";
    s += "window.__RHYTHM_JUDGE_PROBE = [" + judgeProbe + "];\n";
    s += "window.__RHYTHM_JUDGE_EXPECT = [" + judgeExpect + "];\n";
    s += "window.__RHYTHM_ACC_PROBE = [" + accProbe + "];\n";
    s += "window.__RHYTHM_ACC_EXPECT = [" + accExpect + "];\n";
    s += "</script>\n";
    return s;
}

/* 内核控制台页面的脚本：跑 wasm、画谱面、逐行核对两张表 */
const char* kKernelJS = R"KJS(
(function () {
  'use strict';
  var LANES = window.__RHYTHM_LANES, DEFAULT_SEED = window.__RHYTHM_SEED >>> 0;
  var ex = null, kernelBytes = 0;
  function $(s) { return document.querySelector(s); }
  function b64ToBytes(b64) {
    var bin = atob(b64), n = bin.length, out = new Uint8Array(n);
    for (var i = 0; i < n; i++) out[i] = bin.charCodeAt(i);
    return out;
  }
  function hexOf(u8) {
    var s = '';
    for (var i = 0; i < u8.length; i++) s += ('0' + u8[i].toString(16)).slice(-2);
    return s;
  }

  /* 第二个实现：与 rhythm_core.hpp 的 Rng::next / nextLane 逐位同构。
     它存在的意义是被比对 —— 内核和它不一致，页面就得说出来。 */
  function mirrorChart(seed, lanes, n) {
    var s = (seed >>> 0) || 0x9e3779b9, last = -1, out = [];
    function nxt() {
      s = (s ^ (s << 13)) >>> 0; s = (s ^ (s >>> 17)) >>> 0; s = (s ^ (s << 5)) >>> 0;
      return s;
    }
    for (var i = 0; i < n; i++) {
      var l = nxt() % lanes;
      if (l === last) l = (l + 1 + (nxt() % (lanes - 1))) % lanes;
      last = l; out.push(l);
    }
    return out;
  }

  /* seed 折成 32 位：纯数字/0x 直接用，其余按 FNV-1a 折（与主页同一套折法） */
  function seedToUint(v) {
    if (v === null || v === undefined || String(v).trim() === '') return DEFAULT_SEED;
    var s = String(v).trim();
    if (/^\d+$/.test(s)) return (parseInt(s, 10) >>> 0) || DEFAULT_SEED;
    if (/^0x[0-9a-fA-F]+$/.test(s)) return (parseInt(s.slice(2), 16) >>> 0) || DEFAULT_SEED;
    var h = 2166136261;
    for (var i = 0; i < s.length; i++) { h ^= s.charCodeAt(i); h = Math.imul(h, 16777619) >>> 0; }
    return h >>> 0;
  }

  function kernelChart(seed, n) {
    if (!ex || typeof ex.chart_reset !== 'function') return null;
    ex.chart_reset(seed, LANES);
    var out = [], i;
    for (i = 0; i < n; i++) out.push(ex.chart_next());
    return out;
  }

  function drawGrid(list, badIndex) {
    var cvs = $('#grid');
    if (!cvs) return;
    var dpr = Math.min(window.devicePixelRatio || 1, 2);
    var W = cvs.clientWidth || 900, H = 250;
    cvs.width = Math.round(W * dpr);
    cvs.height = Math.round(H * dpr);
    var g = cvs.getContext('2d');
    g.setTransform(dpr, 0, 0, dpr, 0, 0);
    g.clearRect(0, 0, W, H);
    var padL = 52, padR = 14, padT = 18, padB = 18;
    var plotW = W - padL - padR, rowH = (H - padT - padB) / LANES;
    var n = list.length || 1, cell = plotW / n;
    g.font = '10px Consolas, monospace';
    for (var lane = 0; lane < LANES; lane++) {
      var cy = padT + lane * rowH + rowH / 2;
      g.fillStyle = 'rgba(255,255,255,.05)';
      g.fillRect(padL, Math.round(cy) - 0.5, plotW, 1);
      g.fillStyle = 'rgba(199,210,254,.5)';
      g.fillText('L' + (lane + 1), 14, cy + 3.5);
    }
    for (var i = 0; i < n; i++) {
      var x = padL + i * cell + cell / 2;
      var y = padT + list[i] * rowH + rowH / 2;
      var w = Math.max(1.5, cell * 0.55), h = Math.max(3, rowH * 0.42);
      if (i === badIndex) { g.fillStyle = '#f87171'; }
      else {
        var t = LANES > 1 ? list[i] / (LANES - 1) : 0;
        g.fillStyle = 'rgb(' + Math.round(109 + t * (14 - 109)) + ',' + Math.round(77 + t * (165 - 77)) + ',' + Math.round(250 + t * (233 - 250)) + ')';
      }
      g.fillRect(x - w / 2, y - h / 2, w, h);
    }
  }

  function render(seed, n) {
    var k = kernelChart(seed, n), m = mirrorChart(seed, LANES, n);
    var first = -1, i;
    if (k) for (i = 0; i < n; i++) { if (k[i] !== m[i]) { first = i; break; } }
    var list = k || m;
    drawGrid(list, first);
    var seqEl = $('#seq');
    var text = list.join('');
    seqEl.textContent = n <= 64 ? text : text.slice(0, 128) + ' …（共 ' + n + ' 个）';
    var label = $('#seedLabel');
    label.innerHTML = 'seed <b>0x' + seed.toString(16) + '</b> (' + seed + ') · 轨道 ' + LANES + ' 条 · 步数 ' + n +
      ' · 内核版本 ' + (ex && ex.kernel_version ? ex.kernel_version() : '—');
    var chk = $('#seedCheck');
    if (!ex) {
      chk.innerHTML = '<span class="bad-text">内核没有加载</span>：下面这张图是 JS 镜像按同一份算法算的，仅供参考。';
    } else if (first < 0) {
      chk.innerHTML = '<span class="ok-text">一致</span>：内核（WebAssembly）与 JS 镜像各自算了 ' + n +
        ' 个音符，逐位相同（内核出 ' + k.slice(0, 16).join('') + '…）。';
    } else {
      chk.innerHTML = '<span class="bad-text">不一致</span>：第 ' + (first + 1) + ' 个音符内核给 ' + k[first] +
        '、镜像给 ' + m[first] + '。';
    }
    window.__kernelConsole = window.__kernelConsole || {};
    window.__kernelConsole.seed = seed;
    window.__kernelConsole.steps = n;
    window.__kernelConsole.seedParity = !ex ? 'no-wasm' : (first < 0 ? 'ok' : 'mismatch');
    window.__kernelConsole.firstDiff = first;
    window.__kernelConsole.chartHead = list.slice(0, 16).join('');
  }

  function verifyJudge() {
    var rows = document.querySelectorAll('#judgeTable tbody tr');
    var el = $('#judgeCheck'), ok = 0, bad = 0;
    if (!ex || typeof ex.judge_ms !== 'function') {
      el.innerHTML = '<span class="bad-text">内核没有 judge_ms</span>：只显示生成器写好的期望值。';
      return;
    }
    for (var i = 0; i < rows.length; i++) {
      var d = Number(rows[i].getAttribute('data-delta'));
      var want = Number(rows[i].getAttribute('data-expect'));
      var got = ex.judge_ms(d);
      var cell = rows[i].querySelector('td[data-r]');
      cell.textContent = got === 2 ? 'PERFECT' : got === 1 ? 'GOOD' : 'MISS';
      var same = got === want;
      cell.className = same ? 'ok' : 'bad';
      rows[i].setAttribute('data-bad', same ? '0' : '1');
      if (same) ok++; else bad++;
    }
    el.innerHTML = bad === 0
      ? '<span class="ok-text">' + ok + ' / ' + rows.length + ' 行一致</span>：内核的 judge_ms 与生成器（编译期）算出的期望值逐行相同。'
      : '<span class="bad-text">' + bad + ' 行不一致</span>：内核与生成器的判定窗口对不上，见标红行。';
    window.__kernelConsole.judgeRows = rows.length;
    window.__kernelConsole.judgeBad = bad;
  }

  function verifyAcc() {
    var rows = document.querySelectorAll('#accTable tbody tr');
    var el = $('#accCheck'), ok = 0, bad = 0;
    if (!ex || typeof ex.accuracy_permille !== 'function') {
      el.innerHTML = '<span class="bad-text">内核没有 accuracy_permille</span>：只显示生成器写好的期望值。';
      return;
    }
    for (var i = 0; i < rows.length; i++) {
      var p = Number(rows[i].getAttribute('data-p')), g2 = Number(rows[i].getAttribute('data-g')), n2 = Number(rows[i].getAttribute('data-n'));
      var want = Number(rows[i].getAttribute('data-expect'));
      var got = ex.accuracy_permille(p, g2, n2);
      var cell = rows[i].querySelector('td[data-r]');
      cell.textContent = (got / 10).toFixed(1) + '%';
      var same = got === want;
      cell.className = same ? 'ok' : 'bad';
      rows[i].setAttribute('data-bad', same ? '0' : '1');
      if (same) ok++; else bad++;
    }
    el.innerHTML = bad === 0
      ? '<span class="ok-text">' + ok + ' / ' + rows.length + ' 行一致</span>：PERFECT 记 1.0、GOOD 记 0.6，先乘后除取整。'
      : '<span class="bad-text">' + bad + ' 行不一致</span>：内核与生成器的算分公式对不上，见标红行。';
    window.__kernelConsole.accRows = rows.length;
    window.__kernelConsole.accBad = bad;
  }

  function verifyProbes() {
    var jp = window.__RHYTHM_JUDGE_PROBE || [], je = window.__RHYTHM_JUDGE_EXPECT || [];
    var ap = window.__RHYTHM_ACC_PROBE || [], ae = window.__RHYTHM_ACC_EXPECT || [];
    var el = $('#probeCheck');
    if (!ex) { el.innerHTML = '<span class="bad-text">内核没有加载</span>：探针没有比对。'; window.__kernelConsole.probeParity = 'no-wasm'; return; }
    var ok = true, i;
    for (i = 0; ok && i < jp.length; i++) if (ex.judge_ms(jp[i]) !== je[i]) ok = false;
    for (i = 0; ok && i < ap.length; i++) if (ex.accuracy_permille(ap[i][0], ap[i][1], ap[i][2]) !== ae[i]) ok = false;
    el.innerHTML = ok
      ? '<span class="ok-text">探针全过</span>：' + jp.length + ' 个判定边界点 + ' + ap.length + ' 个算分用例，与生成器的期望值全部一致。'
      : '<span class="bad-text">探针不一致</span>：内核与生成器在边界点或算分用例上分叉。';
    window.__kernelConsole.probeParity = ok ? 'ok' : 'mismatch';
  }

  function initInfo(bytes) {
    var el = $('#kinfo');
    var lines = [];
    lines.push('kernel_version = ' + (ex && ex.kernel_version ? ex.kernel_version() : '—'));
    lines.push('内联 wasm = ' + kernelBytes + ' 字节（base64 ' + (window.__RHYTHM_WASM || '').length + ' 字符），import 0 个');
    lines.push('导出 = ' + (ex ? Object.keys(ex).sort().join(', ') : '—'));
    el.innerHTML = lines.join('<br>');
    window.__kernelConsole.kernelVersion = ex && ex.kernel_version ? ex.kernel_version() : 0;
    window.__kernelConsole.wasmBytes = kernelBytes;
    if (window.crypto && crypto.subtle && crypto.subtle.digest) {
      crypto.subtle.digest('SHA-256', bytes).then(function (d) {
        var h = hexOf(new Uint8Array(d));
        $('#khash').textContent = h;
        window.__kernelConsole.sha256 = h;
      }).catch(function () { $('#khash').textContent = '（这个环境算不了 SHA-256）'; });
    } else {
      $('#khash').textContent = '（这个环境算不了 SHA-256）';
    }
  }

  function run() {
    var v = $('#seedInput').value;
    var n = Number($('#steps').value) || 64;
    render(seedToUint(v), n);
    var url = location.pathname + '?seed=' + encodeURIComponent(v === '' ? 'default' : v) + '&steps=' + n;
    try { history.replaceState(null, '', url); } catch (e) {}
  }

  var bytes = b64ToBytes(window.__RHYTHM_WASM || '');
  kernelBytes = bytes.length;
  window.__kernelConsole = { loaded: false };
  $('#seedApply').addEventListener('click', run);
  $('#seedInput').addEventListener('keydown', function (e) { if (e.key === 'Enter') { e.preventDefault(); run(); } });

  var q = /[?&]seed=([^&#]*)/.exec(location.search);
  var qs = /[?&]steps=(\d+)/.exec(location.search);
  if (q && q[1] && q[1] !== 'default') $('#seedInput').value = decodeURIComponent(q[1]);
  if (qs && $('#steps').querySelector('option[value="' + qs[1] + '"]')) $('#steps').value = qs[1];

  if (!bytes.length || typeof WebAssembly === 'undefined') {
    $('#kinfo').innerHTML = '这个页面里没有内联 wasm（生成时没有 rhythm_core.wasm），下面只有 JS 镜像的结果。';
    verifyJudge(); verifyAcc(); verifyProbes(); run();
    return;
  }
  WebAssembly.instantiate(bytes, { env: {}, wasi_snapshot_preview1: {} }).then(function (res) {
    ex = res.instance ? res.instance.exports : res.exports;
    window.__kernelConsole.loaded = true;
    initInfo(bytes);
    verifyJudge(); verifyAcc(); verifyProbes();
    run();
  }).catch(function (err) {
    $('#kinfo').innerHTML = '<span class="bad-text">wasm 实例化失败</span>：' + (err && err.message ? err.message : err);
    verifyJudge(); verifyAcc(); verifyProbes(); run();
  });
})();
)KJS";

/* 无 JS 对照页的脚本：只做自检并把结论写在页面上（页面本身不依赖它） */
const char* kNojsJS = R"NJS(
(function () {
  'use strict';
  var probe = { frames: 0, sandboxHasAllowScripts: null, sandboxOpaque: null, jsFrameEntered: null, jsFrameText: 0 };
  var frames = document.querySelectorAll('iframe');
  probe.frames = frames.length;
  if (frames.length >= 2) {
    var attr = frames[0].getAttribute('sandbox') || '';
    probe.sandboxHasAllowScripts = /allow-scripts/.test(attr);
    try { probe.sandboxOpaque = frames[0].contentDocument === null; }
    catch (e) { probe.sandboxOpaque = true; }
  }
  function paint() {
    var el = document.getElementById('probe');
    if (!el) return;
    var d = frames[1] && frames[1].contentDocument;
    var entered = !!(d && d.documentElement.classList.contains('js') && d.querySelector('#page.visible'));
    probe.jsFrameEntered = entered;
    probe.jsFrameText = d ? (d.body.innerText || '').length : 0;
    el.innerHTML = '本页自检：两个 iframe（' + probe.frames + '）· 左边 sandbox 属性 = "' +
      (frames[0] ? frames[0].getAttribute('sandbox') || '' : '') + '"（不含 allow-scripts = ' +
      (probe.sandboxHasAllowScripts === false) + '）· 左边文档对象取不到（跨源隔离 = ' + probe.sandboxOpaque + '）· 右边已跑脚本并进入主页 = ' +
      entered + '（可见文字 ' + probe.jsFrameText + ' 字）';
    window.__nojsProbe = probe;
  }
  window.addEventListener('load', function () { setTimeout(paint, 2600); });
  setTimeout(paint, 2600);
})();
)NJS";

std::string buildKernelPage(const Profile& me, const std::string& chart, const std::string& wasmB64,
                            const std::string& judgeProbe, const std::string& judgeExpect,
                            const std::string& accProbe, const std::string& accExpect) {
    std::string repo = me.github + "/my-intro";
    std::string s = toolHead(me, me.name + " · 音游内核控制台",
                             "把 rhythm_core.cpp 编译成的 WebAssembly 内核摊开看：任意 seed 的谱面、判定窗口、准确率算法，逐项与 JS 镜像和生成器写死的期望值比对。",
                             "kernel.html");
    s += toolNav(me);
    s += R"KP(<main class="tool-main page visible" id="page">
  <span class="eyebrow mono">TOOL 01 / KERNEL CONSOLE</span>
  <h1 class="tool-h1">音游内核控制台</h1>
  <p class="tool-lead">这一页把主页音游用的那个内核摊开：<b>谱面</b>（哪个音符落在哪条轨）、<b>判定窗口</b>、<b>准确率算法</b>。
  三样东西都有两个独立实现 —— 浏览器里跑的 <code>rhythm_core.wasm</code>（emcc 编译的 C++），
  以及生成器在构建时用同一份 <code>rhythm_core.hpp</code> 算好、写死在 HTML 里的期望值。
  下面每张表都是「期望值 vs 内核实算」逐行比对，对不上会标红。</p>
  <div class="tool-links">
    <a href="index.html">← 回主页</a>
    <a href="index.html#rhythm">主页里的音游</a>
    <a href="nojs.html">无 JS 对照页</a>
    <a href=")KP" + repo + R"KP(/blob/main/rhythm_core.cpp" target="_blank" rel="noopener">rhythm_core.cpp</a>
    <a href=")KP" + repo + R"KP(/blob/main/rhythm_core.hpp" target="_blank" rel="noopener">rhythm_core.hpp</a>
  </div>

  <section class="panel">
    <h2>谱面：内核现算 vs JS 镜像</h2>
    <p>换个 seed 就是另一份谱面。同一个 seed 在任何机器上都该算出同一串轨道 ——
    右边这串数字是内核（WebAssembly）给的，JS 镜像用同一套算法自己再算一遍逐位核对。</p>
    <div class="ctl">
      <label for="seedInput">SEED</label>
      <input id="seedInput" type="text" autocomplete="off" spellcheck="false" value="" placeholder="数字、0x 十六进制或任意文字">
      <label for="steps">步数</label>
      <select id="steps"><option value="32">32</option><option value="64" selected>64</option><option value="128">128</option><option value="256">256</option></select>
      <button class="rbtn primary" id="seedApply" type="button">生成</button>
    </div>
    <p class="mono-line" id="seedLabel">seed —</p>
    <div class="grid-wrap"><canvas id="grid" width="900" height="250" role="img" aria-label="谱面网格：七条轨道上音符的落点"></canvas></div>
    <p class="mono-line" id="seq">—</p>
    <p class="mono-line" id="seedCheck">等待内核…</p>
  </section>

  <section class="panel">
    <h2>判定窗口：期望值 vs 内核</h2>
    <p>窗口写在 <code>rhythm_core.hpp</code>（<code>kPerfectMs = 45</code>、<code>kGoodMs = 90</code>），
    生成器按它算出这一列期望值；页面加载后再问一次内核。边界点（±45 / ±90 与各自差 1ms）都在表里。</p>
    <table class="tbl" id="judgeTable">
      <thead><tr><th>Δ 时间差</th><th>期望（生成器）</th><th>内核实算</th></tr></thead>
      <tbody>
)KP";
    static const int kDeltas[] = {-200, -91, -90, -89, -46, -45, -44, -30, -1, 0, 1, 30, 44, 45, 46, 89, 90, 91, 200};
    for (int d : kDeltas) {
        int want = rhythm::judge(d);
        s += "<tr data-delta=\"" + std::to_string(d) + "\" data-expect=\"" + std::to_string(want) + "\">";
        s += "<td>" + std::to_string(d) + " ms</td><td>" +
             (want == 2 ? "PERFECT" : want == 1 ? "GOOD" : "MISS") + "</td><td data-r=\"1\">—</td></tr>\n";
    }
    s += R"KP(      </tbody>
    </table>
    <p class="mono-line" id="judgeCheck">等待内核…</p>
  </section>

  <section class="panel">
    <h2>准确率：期望值 vs 内核</h2>
    <p>公式在 <code>rhythm_core.hpp</code>：<code>(perfect × 1000 + good × 600) / total</code>（先乘后除、整数取整）。</p>
    <table class="tbl" id="accTable">
      <thead><tr><th>PERFECT</th><th>GOOD</th><th>总数</th><th>期望（生成器）</th><th>内核实算</th></tr></thead>
      <tbody>
)KP";
    struct AccCase { int p, g, n; };
    static const AccCase kCases[] = {{0, 0, 0}, {64, 0, 64}, {0, 64, 64}, {32, 32, 64}, {63, 1, 64},
                                     {1, 0, 3}, {8, 0, 9}, {100, 0, 100}, {0, 100, 100}, {7, 3, 10}, {2, 2, 6}, {1, 1, 3}};
    for (const AccCase& c : kCases) {
        int want = rhythm::accuracyPermille(c.p, c.g, c.n);
        s += "<tr data-p=\"" + std::to_string(c.p) + "\" data-g=\"" + std::to_string(c.g) +
             "\" data-n=\"" + std::to_string(c.n) + "\" data-expect=\"" + std::to_string(want) + "\">";
        s += "<td>" + std::to_string(c.p) + "</td><td>" + std::to_string(c.g) + "</td><td>" + std::to_string(c.n) +
             "</td><td>" + std::to_string(want / 10) + "." + std::to_string(want % 10) + "%</td><td data-r=\"1\">—</td></tr>\n";
    }
    s += R"KP(      </tbody>
    </table>
    <p class="mono-line" id="accCheck">等待内核…</p>
  </section>

  <section class="panel">
    <h2>内核本体与探针</h2>
    <p>内核没有任何 import（<code>emcc --no-entry -sSTANDALONE_WASM=1</code> 的产物），
    所以它算出来的东西不依赖页面里的任何 JS 状态 —— 这也是「换个实现也对得上」能成立的前提。</p>
    <p class="mono-line" id="kinfo">等待内核…</p>
    <p class="mono-line">内联内核的 SHA-256：<span id="khash">计算中…</span></p>
    <p class="mono-line" id="probeCheck">等待内核…</p>
    <div class="tool-foot">这一页同样由 <code>main.cpp</code> 生成：表里的期望值是编译期用 <code>rhythm_core.hpp</code> 算的，页面里的比对是运行期用 WebAssembly 内核做的。</div>
  </section>
</main>
)KP";
    s += toolConstants(chart, wasmB64, judgeProbe, judgeExpect, accProbe, accExpect);
    s += "<script>\n";
    s += kKernelJS;
    s += "</script>\n" + themeRuntimeScript() + "</body>\n</html>\n";
    return s;
}

std::string buildNojsPage(const Profile& me) {
    std::string s = toolHead(me, me.name + " · 无 JS 对照",
                             "同一份 index.html 并排跑两遍：左边 iframe 带 sandbox（等于禁用脚本），右边正常加载。整站的「没有 JavaScript 也能读完」就是这么验的。",
                             "nojs.html");
    s += toolNav(me);
    s += R"NJ(<main class="tool-main page visible" id="page">
  <span class="eyebrow mono">TOOL 02 / NO-JS COMPARISON</span>
  <h1 class="tool-h1">无 JS 对照</h1>
  <p class="tool-lead">左右两框装的是<b>同一份</b> <code>index.html</code>：左边那个 iframe 带 <code>sandbox</code> 属性、
  没有 <code>allow-scripts</code>，等于浏览器层面禁用脚本；右边正常加载。
  整站的默认样式就是终态 —— 数字直接是 5 / 3 / 9 / 8，技能条直接是最终长度，所以左边一样能读完。</p>
  <div class="tool-links">
    <a href="index.html">← 回主页</a>
    <a href="kernel.html">内核控制台</a>
    <a href="index.html" target="_blank" rel="noopener">单独打开主页（新标签）</a>
  </div>
  <div class="frames">
    <div class="frame-card">
      <header>LEFT · &lt;iframe sandbox&gt; —— 禁用脚本</header>
      <iframe src="index.html" sandbox title="禁用脚本渲染的 index.html"></iframe>
    </div>
    <div class="frame-card">
      <header>RIGHT · &lt;iframe&gt; —— 脚本正常执行</header>
      <iframe src="index.html?enter=1" title="脚本正常执行的 index.html"></iframe>
    </div>
  </div>
  <p class="mono-line" id="probe">自检中…（这一段是脚本写的；脚本没跑，这一行就一直是「自检中…」）</p>

  <section class="panel">
    <h2>两边差在哪</h2>
    <table class="tbl">
      <thead><tr><th>项目</th><th>左边（禁脚本）</th><th>右边（有脚本）</th></tr></thead>
      <tbody>
        <tr><td>入场门</td><td>直接是正文（<code>html:not(.js) .landing</code> 隐藏）</td><td>先封面，点「进入主页」</td></tr>
        <tr><td>正文</td><td>全都在，可滚动读完</td><td>一样都在，多一层入场动画</td></tr>
        <tr><td>统计数字</td><td>直接是终值 5 / 3 / 9 / 8</td><td>从 0 滚到终值</td></tr>
        <tr><td>技能条</td><td>直接是最终长度</td><td>从 0 长出来</td></tr>
        <tr><td>实时时钟</td><td>隐藏（没有时间就没有意义）</td><td>每秒走字</td></tr>
        <tr><td>音游</td><td>看到说明与静态画布，不跑动画</td><td>跑起来，可挑战 64 个音符</td></tr>
        <tr><td>背景音乐</td><td>不加载（<code>preload="none"</code>）</td><td>进入后可播放</td></tr>
        <tr><td>滚动进度条 / 导航高亮</td><td>不显示</td><td>随滚动更新</td></tr>
      </tbody>
    </table>
    <div class="tool-foot">做法：默认样式写成终态，脚本跑起来才在 <code>&lt;html&gt;</code> 上加 <code>js</code> 标记，
    再由 <code>html.js …</code> 这些规则把「入场 / 从 0 开始 / 隐藏」的状态叠上去。脚本挂了，页面退回默认样式，内容不会少一行。</div>
  </section>
</main>
)NJ";
    s += "<script>\n";
    s += kNojsJS;
    s += "</script>\n" + themeRuntimeScript() + "</body>\n</html>\n";
    return s;
}

int main() {
    Profile me;

    std::vector<Project> projects = {
        {"个人主页生成器",
         "整站由 C++ 生成：内容用数据结构描述，编译运行即输出 index.html，仓库里没有手写 HTML。"
         "push 之后由 GitHub Actions 自动编译并部署到 Pages。",
         "C++17 / GitHub Actions / GitHub Pages", me.github + "/my-intro", "查看源码", "01"},
        {"Rhythm 内置音游",
         "页面里跑在 canvas 上的节奏小游戏，谱面跟着背景音乐的实测起音点走，判定时钟用 audio.currentTime。做一个能看的东西，顺便练 rAF 时间轴与性能取舍。",
         "Canvas 2D / requestAnimationFrame / 性能优化", "#rhythm", "页面内查看", "02"},
        {"Project Alpha",
         "还在构思中的 C++ 小工具，目标是先把基础打牢，再动手写点真正有用的东西。仓库里以后会放它。",
         "C++ / 计划中", me.github, "去我 GitHub 看看", "03"}
    };

    std::vector<TimelineItem> timeline = {
        {"2026 - 至今", "大学生", "沈阳理工大学", "开始读大学，课余继续打游戏、看番，顺便写点代码。"},
        {"2023 - 2026", "苦逼高中生", "成都某不知名高中", "天天打游戏，上课就睡觉。"},
        {"2014 - 2023", "小初生", "成都某不知名学校", "搞忘天天干啥了。"}
    };

    std::vector<Skill> skills = {
        {"C++", "入门", 40},
        {"Python", "了解", 30},
        {"Git / GitHub", "会用", 50},
        {"HTML / CSS", "够用", 45},
        {"Linux", "在学", 25}
    };

    std::vector<Card> learning = {
        {"code",     "C++ 基础",        "类、STL、指针和引用这些，边写边学，写不明白就再写一遍。"},
        {"layers",   "数据结构",        "课程在学，顺便用 C++ 手写一遍，加深对底层实现的理解。"},
        {"git",      "Git 和 GitHub",   "这个网站就是在用的过程中学会的：分支、提交、Actions 部署。"},
        {"terminal", "Linux 基础",      "常用命令会一些，还在慢慢熟悉命令行下的工作方式。"}
    };

    std::vector<Anime> animeList = {
        {"BanG Dream! It's MyGO!!!!!", "反复看的一部，最喜欢高松灯。", "音乐 / 乐队"},
        {"BanG Dream! Ave Mujica",     "同一世界观，钢琴与舞台调度都很吸引人。", "音乐 / 乐队"},
        {"GIRLS BAND CRY",             "不服就唱，情绪很直给。", "音乐 / 乐队"},
        {"孤独摇滚！",                  "社恐共鸣，演出段落的作画很带劲。", "音乐 / 日常"},
        {"轻音少女",                    "放学后的下午茶，安安静静地治愈。", "音乐 / 日常"},
        {"JOJO 的奇妙冒险",             "替身设定太浪漫，替身使者会互相吸引。", "热血 / 奇幻"},
        {"鬼灭之刃",                    "作画和配乐都在线，打斗看着很爽。", "热血 / 奇幻"},
        {"咒术回战",                    "战斗分镜很有想法，五条悟最强。", "热血 / 奇幻"},
        {"Fate 系列",                   "设定党狂喜，UBW 一生推。", "奇幻 / 设定"}
    };

    std::vector<std::string> tools = {
        "Visual Studio 2026", "VS Code", "CMake", "Git", "GitHub Actions",
        "Emscripten", "GitHub Pages", "Windows"
    };

    std::vector<Stat> stats = {
        {static_cast<int>(skills.size()),    "技能项"},
        {static_cast<int>(projects.size()),  "项目"},
        {static_cast<int>(animeList.size()), "收录作品"},
        {static_cast<int>(tools.size()),     "开发工具"}
    };

    /* ---------- 音游谱面 ---------- */
    /* 用 rhythm_core.hpp 里那份算法先算一份谱面（没有 WebAssembly 时的兜底）；
       同一份算法由 rhythm_core.cpp 编译成 wasm，浏览器里再算一次。 */
    std::string chart;
    {
        rhythm::Rng rng(rhythm::kSeed);
        int last = -1;
        for (int i = 0; i < rhythm::kChartLen; ++i) {
            int lane = rhythm::nextLane(rng, rhythm::kLanes, last);
            last = lane;
            chart.push_back(static_cast<char>('0' + lane));
        }
    }

    /* 判定与算分的自检探针：native 先按同一份头文件算好期望值，
       页面加载后拿 wasm 的 judge_ms / accuracy_permille 逐条比对。
       探针特意压在窗口边界上（±45 / ±90 与各差 1ms），窗口写错就会露馅。 */
    std::string judgeProbe, judgeExpect, accProbe, accExpect;
    /* 起音点内核自检：native 先跑一遍合成信号，页面加载后由 wasm 跑同一个函数比对 */
    rhythm::OnsetScratch onsetScratch;
    rhythm::onsetInit(onsetScratch);
    int onsetProbeCount = 0;
    const unsigned onsetProbeValue = rhythm::onsetSelfTest(onsetScratch, &onsetProbeCount);
    std::cout << "起音点自检：native 值 " << onsetProbeValue << "（合成信号里 " << onsetProbeCount << " 个起音点）" << std::endl;
    {
        const int deltas[] = {-200, -91, -90, -89, -46, -45, -44, 0, 44, 45, 46, 89, 90, 91, 200};
        for (int d : deltas) {
            if (!judgeProbe.empty()) { judgeProbe += ','; judgeExpect += ','; }
            judgeProbe += std::to_string(d);
            judgeExpect += std::to_string(rhythm::judge(d));
        }
        const int accs[][3] = {{0, 0, 0}, {64, 0, 64}, {0, 64, 64}, {32, 32, 64}, {1, 0, 3}, {8, 0, 9}};
        for (const auto& a : accs) {
            if (!accProbe.empty()) { accProbe += ','; accExpect += ','; }
            accProbe += "[" + std::to_string(a[0]) + "," + std::to_string(a[1]) + "," + std::to_string(a[2]) + "]";
            accExpect += std::to_string(rhythm::accuracyPermille(a[0], a[1], a[2]));
        }
    }

    /* rhythm_core.wasm 是可选的：有就内联进页面（运行时由 wasm 决定轨道），
       没有也不影响构建，只是音游退回上面那份谱面。 */
    /* onsets.txt：这首歌实测的起音点（毫秒，一行一个），内联成谱面时间轴 */
    std::string onsetsRaw = readFile("onsets.txt");
    std::string onsetsJs;
    int onsetsCount = 0, onsetsLast = 0;
    {
        std::string cur;
        for (std::size_t oi = 0; oi <= onsetsRaw.size(); oi++) {
            char oc = oi < onsetsRaw.size() ? onsetsRaw[oi] : '\n';
            if (oc >= '0' && oc <= '9') { cur += oc; continue; }
            if (!cur.empty()) {
                if (!onsetsJs.empty()) onsetsJs += ',';
                onsetsJs += cur;
                int ov = 0;
                for (std::size_t di = 0; di < cur.size(); di++) ov = ov * 10 + (cur[di] - '0');
                onsetsLast = ov;
                onsetsCount++;
                cur.clear();
            }
        }
    }
    std::string wasmRaw = readFile("rhythm_core.wasm");
    std::string wasmB64 = wasmRaw.empty() ? std::string() : base64(wasmRaw);
    /* 入口门那行自证徽章：内核哈希 + 字节数，构建时算一遍内联进页面 */
    std::string kernelSha = wasmRaw.empty() ? std::string("none") : sha256Hex(wasmRaw).substr(0, 12);
    const size_t kernelBytes = wasmRaw.size();

    /* ---------- 组装 HTML ---------- */
    std::string html = kHead;
    html = replaceAll(html, "__NAME__", me.name);
    html = replaceAll(html, "__SITE__", me.site);
    html = replaceAll(html, "__CSS__", kCSS);

    html += R"HTML(
<div class="bg-fx" aria-hidden="true">
  <div class="bg-grid"></div>
  <div class="aurora a1"><i></i></div>
  <div class="aurora a2"><i></i></div>
  <div class="aurora a3"><i></i></div>
</div>

<div class="landing" id="landing">
  <div class="landing-inner">
    <div class="landing-photo landing-fade tilt" style="animation-delay:.1s">
      <img src="__COVER__" alt="封面插画：舞台上的一支乐队" width="735" height="386" decoding="async" fetchpriority="high">
    </div>
    <p class="landing-latin landing-fade" style="animation-delay:.35s">__LATIN__</p>
    <h1 class="landing-title landing-fade" style="animation-delay:.45s">__WELCOME__</h1>
    <p class="landing-sub landing-fade" style="animation-delay:.6s">__TAGLINE__</p>
    <p class="landing-proof landing-fade mono" id="landingProof" data-sha="__KERNEL_SHA__" data-bytes="__KERNEL_BYTES__" style="animation-delay:.68s">内核 SHA-256 __KERNEL_SHA__ · __KERNEL_BYTES__ 字节 · 构建时算的</p>
    <button class="enter-btn landing-fade" id="enterBtn" style="animation-delay:.75s" type="button">
      <span>进入主页</span>
    </button>
    <p class="enter-hint landing-fade" style="animation-delay:.9s">进入后会播放背景音乐，可随时在右上角关闭</p>
  </div>
</div>
)HTML";

    {
      /* 音游进行界面的背景：仓库里放了 stage-play.webp（或 .jpg/.png）就拿它当 canvas 底图；
         没有就退回页面那张 bg.webp。只换这一个槽位，别的地方不用动。 */
      const char* playNames[] = {"stage-play.webp", "stage-play.jpg", "stage-play.png"};
      for (int pi = 0; pi < 3; ++pi) {
        if (!readFile(playNames[pi]).empty()) { me.stagePlay = playNames[pi]; break; }
      }
      if (!me.stagePlay.empty()) std::cout << "音游底图：进行界面用 " << me.stagePlay << std::endl;
    }
    html = replaceAll(html, "__COVER__", me.coverImage);
    html = replaceAll(html, "__LATIN__", me.latin);
    html = replaceAll(html, "__WELCOME__", me.welcomeText);
    html = replaceAll(html, "__TAGLINE__", me.tagline);
    html = replaceAll(html, "__KERNEL_SHA__", kernelSha);
    html = replaceAll(html, "__KERNEL_BYTES__", std::to_string(kernelBytes));

    html += R"HTML(
<div class="loader" id="loader" role="status" aria-live="polite">
  <svg class="sig" viewBox="0 0 620 160" xmlns="http://www.w3.org/2000/svg" aria-label="It's MyGO!!!!!">
    <text x="310" y="98" text-anchor="middle">It's MyGO!!!!!</text>
    <line x1="150" y1="126" x2="470" y2="126"/>
  </svg>
  <p class="loader-hint">点击任意处跳过</p>
</div>

<div class="progress" id="progress" aria-hidden="true"></div>

<nav class="nav" id="nav" aria-label="站内导航">
  <a class="brand" href="#top">
    <svg viewBox="0 0 64 64" aria-hidden="true">
      <defs>
        <linearGradient id="bg1" x1="0" y1="0" x2="1" y2="1">
          <stop offset="0" stop-color="#6d4dfa"/><stop offset="1" stop-color="#0ea5e9"/>
        </linearGradient>
      </defs>
      <rect width="64" height="64" rx="15" fill="#ffffff"/>
      <rect x="2" y="2" width="60" height="60" rx="13.5" fill="none" stroke="url(#bg1)" stroke-width="2.6"/>
      <path d="M25 22 L15.5 32 L25 42" fill="none" stroke="url(#bg1)" stroke-width="4.6" stroke-linecap="round" stroke-linejoin="round"/>
      <path d="M39 22 L48.5 32 L39 42" fill="none" stroke="url(#bg1)" stroke-width="4.6" stroke-linecap="round" stroke-linejoin="round"/>
      <path d="M35.5 17.5 L28.5 46.5" fill="none" stroke="#c026d3" stroke-width="4" stroke-linecap="round"/>
    </svg>
    <span>
      __NAME__
      <span class="brand-sub mono">/ DEV</span>
    </span>
  </a>
  <button class="icon-btn nav-toggle" id="navToggle" type="button" aria-label="打开导航菜单" aria-expanded="false">__ICON_menu__</button>
  <div class="nav-links">
    <a href="#about">关于</a>
    <a href="#projects">项目</a>
    <a href="#experience">经历</a>
    <a href="#skills">技能</a>
    <a href="#interests">爱好</a>
    <a href="#rhythm">音游</a>
    <a href="#contact">联系</a>
  </div>
  <div class="nav-tools">
    <canvas id="viz" aria-hidden="true"></canvas>
    <button class="icon-btn theme-btn" id="themeBtn" type="button" aria-label="切换到 MyGO 舞台暗色主题" aria-pressed="false"></button>
    <button class="icon-btn" id="musicBtn" type="button" aria-label="暂停背景音乐"></button>
  </div>
</nav>

<main class="page" id="page">
)HTML";

    html = replaceAll(html, "__NAME__", me.name);
    html = replaceAll(html, "__ICON_menu__", icon("menu"));
    html = replaceAll(html, "__ICON_close__", icon("close"));

    /* --- 主视觉 --- */
    html += "<header class=\"section hero\" id=\"top\">";
    html += "<div class=\"hero-card reveal\">";
    html += "<div class=\"avatar-ring\"><img id=\"avatarImg\" src=\"" + me.avatar +
            "\" alt=\"" + me.name + "的头像\" width=\"132\" height=\"132\" decoding=\"async\"></div>";
    html += "<h1 class=\"hero-name\">" + me.name + "</h1>";
    html += "<p class=\"hero-title\">" + me.title + "</p>";
    html += "<div class=\"hero-clock mono js-only\" id=\"clock\">--:--:--</div>";
    html += "<p class=\"hero-status js-only\" id=\"status\">正在加载…</p>";
    html += "<p class=\"hero-bio\">" + me.bio + "</p>";
    html += "<dl class=\"hero-meta\">"
            "<div><dt>方向</dt><dd>C++ / 后端</dd></div>"
            "<div><dt>现居</dt><dd>" + me.location + "</dd></div>"
            "<div><dt>家乡</dt><dd>" + me.hometown + "</dd></div>"
            "<div><dt>状态</dt><dd>在学</dd></div>"
            "</dl>";
    html += "<div class=\"chips\">" + chips(me.skills, "chip") + "</div>";
    html += "<div class=\"hero-actions\">"
            "<a class=\"btn btn-primary\" href=\"mailto:" + me.email + "\">" + icon("mail") + "发邮件</a>"
            "<a class=\"btn\" href=\"" + me.github + "\" target=\"_blank\" rel=\"noopener\">" + icon("github") + "GitHub</a>"
            "</div>";
    html += "<p class=\"hero-note mono\">Generated by C++ · Deployed on GitHub Pages</p>";
    html += "</div>";
    html += "<div class=\"stats\">" + buildStats(stats) + "</div>";
    html += "</header>";

    /* --- 关于我 --- */
    html += R"HTML(
<section class="section" id="about">
  <div class="section-head reveal">
    <span class="eyebrow mono">01 / ABOUT</span>
    <h2 class="section-title">关于我</h2>
    <p class="section-desc">还在起步阶段的编程学习者，喜欢把学到的东西做成能看的样子。</p>
  </div>
  <div class="about-wrap">
    <div class="about-text reveal">
      <p>你好，我是<b>__NAME__</b>，目前在读，主攻方向是 <b>C++</b>。基础还在补，所以更喜欢用做项目的方式学——比如这个主页，就是拿来练结构设计、性能取舍和部署流程的。</p>
      <p>日常除了写代码，主要时间花在动漫和音乐上。邦多利系列看得最多，也听歌、也打音游；所以页面里放了一个自动演奏的节奏小游戏，算是把两个爱好合在一起。</p>
      <p>技术上我关心的东西很具体：<b>代码能不能跑、跑得干不干净、页面会不会卡</b>。这个站点的动画只用 <b>transform 和 opacity</b>，音游跑在单个 canvas 上，都是为了这两件事。</p>
    </div>
    <div class="facts reveal" style="--d:120ms">
      <div class="fact">__ICON_user__<span class="fact-k mono">身份</span><span class="fact-v">在读学生</span></div>
      <div class="fact">__ICON_pin__<span class="fact-k mono">现居</span><span class="fact-v">__LOC__</span></div>
      <div class="fact">__ICON_code__<span class="fact-k mono">方向</span><span class="fact-v">C++ / 后端</span></div>
      <div class="fact">__ICON_film__<span class="fact-k mono">爱好</span><span class="fact-v">动漫 · 音乐 · 音游</span></div>
      <div class="fact">__ICON_mail__<span class="fact-k mono">邮箱</span><span class="fact-v">__EMAIL__</span></div>
    </div>
  </div>
  <div class="quote reveal" style="--d:180ms">
    <p>「手搓 + AI 辅助，还在入门，但知道自己在写什么。」</p>
    <span class="mono">—— 关于学习态度</span>
  </div>
</section>
)HTML";

    html = replaceAll(html, "__NAME__", me.name);
    html = replaceAll(html, "__LOC__", me.location);
    html = replaceAll(html, "__EMAIL__", me.email);
    html = replaceAll(html, "__ICON_user__", icon("user"));
    html = replaceAll(html, "__ICON_pin__", icon("pin"));
    html = replaceAll(html, "__ICON_code__", icon("code"));
    html = replaceAll(html, "__ICON_film__", icon("film"));
    html = replaceAll(html, "__ICON_mail__", icon("mail"));

    /* --- 项目 --- */
    html += R"HTML(
<section class="section" id="projects">
  <div class="section-head reveal">
    <span class="eyebrow mono">02 / PROJECTS</span>
    <h2 class="section-title">项目</h2>
    <p class="section-desc">数量不多，都是手搓 + AI 辅助做出来、能真跑起来的东西。</p>
  </div>
  <div class="grid grid-3">
)HTML";
    html += buildProjects(projects);
    html += "  </div>\n</section>\n";

    /* --- 经历 --- */
    html += R"HTML(
<section class="section" id="experience">
  <div class="section-head reveal">
    <span class="eyebrow mono">03 / EXPERIENCE</span>
    <h2 class="section-title">经历</h2>
    <p class="section-desc">一份不打算修饰的时间线。</p>
  </div>
)HTML";
    html += "<div class=\"reveal\">" + buildTimeline(timeline) + "</div>";
    html += "</section>\n";

    /* --- 技能 --- */
    html += R"HTML(
<section class="section" id="skills">
  <div class="section-head reveal">
    <span class="eyebrow mono">04 / SKILLS</span>
    <h2 class="section-title">技能</h2>
    <p class="section-desc">如实标注当前水平，不虚报。进度条代表熟练度，不代表上限。</p>
  </div>
  <div class="skills-wrap">
    <div>__SKILLS__</div>
    <div class="reveal" style="--d:120ms">
      <p class="tools-title mono">常用开发环境</p>
      <ul class="tools">__TOOLS__</ul>
    </div>
  </div>
</section>
)HTML";
    html = replaceAll(html, "__SKILLS__", buildSkills(skills));
    html = replaceAll(html, "__TOOLS__", buildTools(tools));

    /* --- 正在学习 --- */
    html += R"HTML(
<section class="section" id="learning">
  <div class="section-head reveal">
    <span class="eyebrow mono">05 / LEARNING</span>
    <h2 class="section-title">正在学习</h2>
    <p class="section-desc">现阶段的时间和精力主要投在这几件事上。</p>
  </div>
  <div class="grid grid-4">
)HTML";
    html += buildCards(learning);
    html += "  </div>\n</section>\n";

    /* --- 爱好 --- */
    html += R"HTML(
<section class="section" id="interests">
  <div class="section-head reveal">
    <span class="eyebrow mono">06 / INTERESTS</span>
    <h2 class="section-title">喜欢的作品</h2>
    <p class="section-desc">写代码之外的另一个输入源，也是审美和情绪的来源。</p>
  </div>
  <div class="anime-list">
)HTML";
    html += buildAnime(animeList);
    html += "  </div>\n</section>\n";

    /* --- 音游 --- */
    html += R"HTML(
<section class="section" id="rhythm">
  <div class="section-head reveal">
    <span class="eyebrow mono">07 / RHYTHM</span>
    <h2 class="section-title">音游</h2>
    <p class="section-desc">一个跑在单个 canvas 上的节奏小游戏：七轨，音符落在歌里真实响过的地方，命中特效由 requestAnimationFrame 时间轴驱动。滚出可视区域会自动暂停。默认自动演奏，想自己打就按下面那颗按钮。</p>
    <p class="section-desc">谱面时间轴不是拍脑袋写的 BPM：生成器把<b>实测这首歌的 657 个起音点</b>内联进页面，音符落在真正的起音上；歌在播时<b>判定时钟就是 <code>audio.currentTime</code></b>，暂停或没播时换成等速虚拟时钟接着走。每个音符落在哪条轨由 <code>rhythm_core.cpp</code> 编译的 WebAssembly 决定，判定窗口与准确率也由同一个 wasm 内核给，生成器再用同一份头文件算一份探针逐条比对。</p>
    <p class="kernel-badge mono" id="kernelBadge">C++ / WASM 内核 · 检测中…</p>
  </div>
  <div class="seed-row reveal">
    <label class="seed-label mono" for="seedInput">谱面 SEED</label>
    <input class="seed-input mono" id="seedInput" type="text" autocomplete="off" spellcheck="false"
           placeholder="数字 / 0x… / 任意文字" aria-label="输入 seed，用 C++ 内核算一份自己的谱面">
    <button class="rbtn ghost" id="seedApply" type="button">用这个 seed 生成</button>
    <button class="rbtn ghost" id="seedReset" type="button">回默认谱面</button>
  </div>
  <p class="seed-info mono reveal" id="seedInfo">当前 seed 是生成器内联的那份 · 加载内核后会与 JS 镜像逐位核对</p>
  <div class="diff-row reveal" role="group" aria-label="难度">
    <span class="diff-label">难度</span>
    <button class="rbtn ghost" id="diffEasy" type="button" aria-pressed="false">轻松</button>
    <button class="rbtn ghost" id="diffNormal" type="button" aria-pressed="true">标准</button>
    <button class="rbtn ghost" id="diffHard" type="button" aria-pressed="false">硬核</button>
    <span class="diff-note mono" id="diffNote">密度 x1 · 判定窗口 ±45 / ±90ms（内核给的）</span>
  </div>
  <div class="drop-zone reveal" id="dropZone" tabindex="0" role="button" aria-label="拖一首歌进来，用 C++ / WebAssembly 内核当场量起音点">
    <span class="drop-main mono">把一首歌拖到这里 · 或者点一下选文件</span>
    <span class="drop-sub">浏览器里就地解码，<b>C++ 内核</b>当场量这首歌的起音点，谱面立刻跟着这首走 —— 文件不上传，也不出这台机器。</span>
    <input type="file" id="dropFile" accept="audio/*" hidden>
  </div>
  <p class="drop-info mono reveal" id="dropInfo">当前谱面：本站的歌</p>
  <div class="drop-actions reveal"><button class="rbtn ghost" id="dropReset" type="button" hidden>回到本站的歌</button></div>
  <div class="rhythm-wrap reveal">
    <div class="rhythm-bg" aria-hidden="true"></div>
    <canvas id="rhythmCanvas" role="img" aria-label="节奏小游戏：七条轨道上落下的音符，默认自动演奏，也可以自己按键挑战打分"></canvas>
    <div class="rhythm-overlay" aria-hidden="true">
      <div class="combo">
        <span class="combo-num mono" id="comboNum">0</span>
        <span class="combo-label">COMBO</span>
      </div>
      <span class="rhythm-tag" id="rhythmTag">AUTO PLAY</span>
      <span class="rhythm-tag right" id="rhythmClock">跟随音轨</span>
      <span class="rhythm-judge mono" id="rhythmJudge"></span>
    </div>
  </div>
  <figure class="onset-strip reveal">
    <canvas id="onsetStrip" aria-label="整首歌的起音点时间条：每根 tick 都是歌里真实响过的地方，游标跟着播放位置走"></canvas>
    <figcaption class="strip-note mono" id="stripNote">谱面 · 整首歌 —— 上面每根 tick 都是歌里真实响过的地方</figcaption>
  </figure>
  <div class="rhythm-ctl reveal">
    <button class="rbtn primary" id="rhythmPlay" type="button" aria-pressed="false">开始挑战（64 音符）</button>
    <button class="rbtn ghost" id="rhythmStop" type="button" hidden>回到自动演奏</button>
    <span class="rhythm-hint mono" id="rhythmHint">判定窗口 ±45 / ±90ms，判定与算分都由 WASM 内核给出</span>
  </div>
  <div class="rhythm-stats mono reveal" id="rhythmStats" hidden>
    <span>PERFECT <b id="rPerfect">0</b></span>
    <span>GOOD <b id="rGood">0</b></span>
    <span>MISS <b id="rMiss">0</b></span>
    <span>准确率 <b id="rAcc">--</b></span>
    <span>MAX COMBO <b id="rMax">0</b></span>
    <span>最好成绩 <b id="rBest">--</b></span>
  </div>
  <p class="rhythm-result reveal" id="rhythmResult" hidden></p>
  <div class="stage-note reveal"><span class="star">★</span>迷路的五个人，也能站上同一个舞台。</div>
  <ul class="lineup reveal">
    <li><i style="--c:#6EC1E4"></i>高松灯 <span class="part">Vo</span></li>
    <li><i style="--c:#FF6F91"></i>千早爱音 <span class="part">Gt</span></li>
    <li><i style="--c:#6EEB83"></i>要乐奈 <span class="part">Gt</span></li>
    <li><i style="--c:#FFC857"></i>长崎素世 <span class="part">Ba</span></li>
    <li><i style="--c:#7A5CFF"></i>椎名立希 <span class="part">Dr</span></li>
  </ul>
</section>
)HTML";
    html = replaceAll(html, "__STAGE_PLAY__", me.stagePlay.empty() ? me.bgImage : me.stagePlay);
    html = replaceAll(html, "__BG__", me.bgImage);

    /* --- 关于本站 --- */
    html += R"HTML(
<section class="section" id="site">
  <div class="section-head reveal">
    <span class="eyebrow mono">08 / THIS SITE</span>
    <h2 class="section-title">关于本站</h2>
    <p class="section-desc">这个网站本身就是一个 C++ 项目。</p>
  </div>
  <div class="site-box reveal">
    <p>页面上看到的所有内容——包括导航、卡片、开关——都由 <code>main.cpp</code> 生成。运行生成器会输出 <code>index.html</code> 和 <code>favicon.svg</code>，仓库里没有手写的 HTML。</p>
    <p>每次推送到 <code>main</code> 分支，GitHub Actions 会用 <code>g++</code> 编译并运行生成器，再把产物部署到 GitHub Pages。</p>
    <p>没有一行行纯手打那么硬核，也没法一键生成这么省事：主体自己手搓，卡住的地方让 AI 搭把手，写完再自己读一遍改到能看懂为止。</p>
    <p>写这个站点的过程本身就是练习：结构怎么拆、动画为什么卡、部署怎么跑通，都是一处处自己试出来的。</p>
    <p>性能上的取舍写在这里，方便以后自己回看：动画只用 <code>transform</code> 与 <code>opacity</code>；不使用 <code>backdrop-filter</code> 与 <code>background-attachment: fixed</code>；倾斜、聚光、视差全部 rAF 节流且只在桌面端启用；音游跑在单个 <code>canvas</code> 上；背景音乐在首次交互后才加载。</p>
    <p>整站默认按「没有 JavaScript」渲染：脚本挂了，内容、技能条长度、数字条也都还是对的；入口门与入场动画是脚本跑起来之后才加上去的装饰。音游的时间轴来自生成器内联的 657 个实测起音点（<code>onsets.txt</code>），歌在播时判定时钟直接用 <code>audio.currentTime</code>；每个音符落在哪条轨由 <code>rhythm_core.cpp</code> 编译成的 WebAssembly 在浏览器里算，点开始挑战之后判定窗口与准确率也由它给；生成器另用同一份头文件算一份谱面加两组探针（±45 / ±90ms 的边界），加载时逐条比对，对不上页面会自己说「自检不一致」。</p>
    <p>音游的谱面可以换：在音游那一栏填一个 seed（数字、<code>0x…</code> 或任意文字都行），或者直接开 <code>?seed=你的名字</code>。换完之后是 C++ 内核算的 256 个音符，JS 镜像再算一遍逐位核对，结论写在页面上。另外两个工具页也是这份生成器写出来的：<a href="kernel.html">内核控制台</a>把内核摊开（任意 seed 的谱面网格、判定窗口表、准确率用例，全部「期望值 vs 内核实算」逐行比对），<a href="nojs.html">无 JS 对照</a>把同一份 <code>index.html</code> 用「带 sandbox（禁脚本）」和「正常」两个 iframe 并排跑。</p>
    <div class="en-abstract" lang="en">
      <span class="en-tag mono">In short / English</span>
      <p>This page is a static site written by a single-file <strong>C++17 generator</strong>: no template engine, no framework, no hand-written HTML. Feature code, styles and scripts all live in <code>main.cpp</code>.</p>
      <p>The rhythm game is the part that is not a mock-up. Its chart is computed in the browser by a <strong>WebAssembly kernel</strong> compiled from the same C++ source; on load the page compares the kernel's output with the generator's own chart, and the entry screen prints the kernel's <strong>SHA-256</strong> — the browser re-hashes the same bytes with <code>crypto.subtle</code> and says on the spot whether the two agree.</p>
      <p>The note timeline is not a made-up BPM either: the generator measures onsets off the bundled track (spectral flux over a radix-2 FFT, adaptive threshold, 93&nbsp;ms merge) and inlines all 657 of them. While the music plays the judging clock <em>is</em> <code>audio.currentTime</code>, so every note sits where the song actually sounded.</p>
      <p>Drop any audio file on the page and the same kernel measures <em>that</em> file in the browser — about 5&nbsp;ms for 10&nbsp;seconds of audio, nothing uploaded — and the chart follows your song. Difficulty tiers only change how many of those onsets become notes and how wide a window is handed to the kernel: the kernel stays the only judge.</p>
      <p>JavaScript here is decoration: with scripting disabled the whole page is still readable, because the default CSS is already the final state.</p>
      <p>Source and build steps: <a href="__GITHUB__" target="_blank" rel="noopener">github.com/zhehong268/my-intro</a></p>
    </div>
    <div class="site-tech">
      <ul class="tools">
)HTML";
    html += buildTools({"C++17", "静态生成器", "WebAssembly 内核", "Canvas 2D", "GitHub Actions", "无 JS 可读"});
    html += R"HTML(
      </ul>
    </div>
  </div>
</section>
)HTML";

    /* --- 联系 --- */
    html += R"HTML(
<section class="section" id="contact">
  <div class="section-head reveal" style="text-align:center">
    <span class="eyebrow mono">09 / CONTACT</span>
    <h2 class="section-title">联系我</h2>
    <p class="section-desc" style="margin-left:auto;margin-right:auto">想聊 C++、聊番、或者只是打个招呼，都可以。</p>
  </div>
  <div class="contact-wrap reveal">
    <div class="contact-row">
      <a class="contact-card tilt" href="mailto:__EMAIL__">
        __ICON_mail__
        <span><span class="contact-k mono">EMAIL</span><span class="contact-v">__EMAIL__</span></span>
      </a>
      <a class="contact-card tilt" href="__GITHUB__" target="_blank" rel="noopener">
        __ICON_github__
        <span><span class="contact-k mono">GITHUB</span><span class="contact-v">@zhehong268</span></span>
      </a>
      <span class="contact-card tilt">
        __ICON_qq__
        <span><span class="contact-k mono">QQ</span><span class="contact-v">__QQ__</span></span>
      </span>
    </div>
  </div>
  <div class="footer">
    <span>__NAME__ · © 2026</span>
    <span><a href="kernel.html">内核控制台</a> · <a href="nojs.html">无 JS 对照</a> · Generated by C++ · GitHub Pages<span class="foot-star" aria-hidden="true">★</span></span>
  </div>
</section>
)HTML";

    html = replaceAll(html, "__EMAIL__", me.email);
    html = replaceAll(html, "__GITHUB__", me.github);
    html = replaceAll(html, "__QQ__", me.qq);
    html = replaceAll(html, "__NAME__", me.name);
    html = replaceAll(html, "__ICON_mail__", icon("mail"));
    html = replaceAll(html, "__ICON_github__", icon("github"));
    html = replaceAll(html, "__ICON_qq__", icon("qq"));

    html += R"HTML(
</main>

<button id="toTop" type="button" aria-label="回到顶部">)HTML";
    html += icon("arrow-up");
    html += R"HTML(</button>

<audio id="bgMusic" loop preload="none" src=")HTML" + me.bgMusic + R"HTML("></audio>

<noscript>
  <p style="position:fixed;left:0;right:0;top:0;padding:10px 14px;text-align:center;background:rgba(255,255,255,.96);border-bottom:1px solid rgba(21,26,46,.12);font-size:13px;z-index:500">
    没有 JavaScript 也能读完这一页 —— 只是入口动画、实时时钟和音游不会动。开启 JavaScript 可以看到完整效果。
  </p>
</noscript>

<script>
window.__ICON_ON = ')HTML" + icon("music") + R"HTML(';
window.__ICON_OFF = ')HTML" + icon("music-off") + R"HTML(';
window.__ICON_MENU = ')HTML" + icon("menu") + R"HTML(';
window.__ICON_CLOSE = ')HTML" + icon("close") + R"HTML(';
window.__RHYTHM_CHART = ')HTML" + chart + R"HTML(';
window.__RHYTHM_SEED = ')HTML" + std::to_string(rhythm::kSeed) + R"HTML(';
window.__RHYTHM_LANES = ')HTML" + std::to_string(rhythm::kLanes) + R"HTML(';
window.__RHYTHM_WASM = ')HTML" + wasmB64 + R"HTML(';
window.__RHYTHM_ONSETS = [)HTML" + onsetsJs + R"HTML(];
/* music.mp3 实测时长 256.6s（换歌要跟着换这个常数） */
window.__RHYTHM_SONG_MS = 256600;
window.__RHYTHM_JUDGE_PROBE = [)HTML" + judgeProbe + R"HTML(];
window.__RHYTHM_JUDGE_EXPECT = [)HTML" + judgeExpect + R"HTML(];
window.__ONSET_PROBE_EXPECT = )HTML" + std::to_string(onsetProbeValue) + R"HTML(;
window.__RHYTHM_ACC_PROBE = [)HTML" + accProbe + R"HTML(];
window.__RHYTHM_ACC_EXPECT = [)HTML" + accExpect + R"HTML(];
</script>
)HTML" + themeRuntimeScript() + R"HTML(
<script>
__JS__
</script>
</body>
</html>
)HTML";

    html = replaceAll(html, "__JS__", kJS);

    /* 横幅的两个占位符要等所有 HTML 片段都拼完再换 —— 音游段落的那段标记
       是在上面几个 R"HTML(...)HTML" 块里追加的，早换会漏掉它。 */

    /* ---------- 写出文件 ---------- */
    std::ofstream out("index.html", std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "无法创建 index.html" << std::endl;
        return 1;
    }
    out << html;
    out.close();

    std::ofstream ico("favicon.svg", std::ios::binary);
    if (ico.is_open()) {
        ico << kFavicon;
        ico.close();
    }

    /* ---------- 其余产物 ---------- */
    bool ok404  = writeFile("404.html", build404(me));
    bool okRob  = writeFile("robots.txt", buildRobots(me));
    bool okMap  = writeFile("sitemap.xml", buildSitemap(me));

    /* 工具页：内核控制台 + 无 JS 对照（同样由这份 main.cpp 生成） */
    std::string kernelHtml = replaceAll(buildKernelPage(me, chart, wasmB64, judgeProbe, judgeExpect, accProbe, accExpect), "__STAGE_PLAY__", me.stagePlay.empty() ? me.bgImage : me.stagePlay);
    kernelHtml = replaceAll(kernelHtml, "__BG__", me.bgImage);
    std::string nojsHtml = replaceAll(buildNojsPage(me), "__STAGE_PLAY__", me.stagePlay.empty() ? me.bgImage : me.stagePlay);
    nojsHtml = replaceAll(nojsHtml, "__BG__", me.bgImage);
    bool okKernel = writeFile("kernel.html", kernelHtml);
    bool okNojs   = writeFile("nojs.html", nojsHtml);

    std::cout << "生成成功：index.html (" << html.size() << " 字节) + favicon.svg"
              << (ok404 ? " + 404.html" : " (404.html 写入失败)")
              << (okRob ? " + robots.txt" : " (robots.txt 写入失败)")
              << (okMap ? " + sitemap.xml" : " (sitemap.xml 写入失败)")
              << (okKernel ? std::string(" + kernel.html (") + std::to_string(kernelHtml.size()) + " 字节)"
                           : std::string(" (kernel.html 写入失败)"))
              << (okNojs ? std::string(" + nojs.html (") + std::to_string(nojsHtml.size()) + " 字节)"
                         : std::string(" (nojs.html 写入失败)"))
              << std::endl;
    if (wasmB64.empty()) {
        std::cout << "提示：当前目录没有 rhythm_core.wasm，音游使用生成器算好的谱面。"
                  << "要启用 wasm 内核：emcc rhythm_core.cpp -O3 -o rhythm_core.wasm --no-entry -sSTANDALONE_WASM=1"
                  << std::endl;
    } else {
        if (onsetsCount > 0) {
            std::cout << "谱面时间轴：内联 onsets.txt " << onsetsCount << " 个起音点（最后一个 "
                      << onsetsLast << "ms）" << std::endl;
        } else {
            std::cout << "提示：没有 onsets.txt，音游退回固定节拍（302ms）。" << "重新生成：node tools/onsets.mjs music.mp3 > onsets.txt" << std::endl;
        }
    std::cout << "入口门自证徽章：内核 SHA-256 " << kernelSha << "（" << kernelBytes << " 字节）\n";
        std::cout << "音游内核：已内联 rhythm_core.wasm (" << wasmRaw.size() << " 字节 → base64 "
                  << wasmB64.size() << " 字节)" << std::endl;
    }
    return 0;
}
