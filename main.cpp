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
 *  性能约定（请勿破坏）：
 *    - 所有动画只用 transform / opacity，保证走 GPU 合成
 *    - 不使用 backdrop-filter / background-attachment: fixed
 *    - 倾斜、聚光、视差、磁吸全部 rAF 节流，且只在精确指针设备上启用
 *    - 音游跑在单个 canvas 的 requestAnimationFrame 时间轴上，离开视口自动暂停
 *    - 背景音乐 preload="none"，首次用户交互后才加载
 *    - 触摸设备与 prefers-reduced-motion 下自动降级
 */

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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
    std::string bgMusic     = "music.mp3";
    std::string welcomeText = "欢迎来到遮鸿的个人主页";
    std::string email       = "3255484226@qq.com";
    std::string qq          = "3255484226";
    std::string github      = "https://github.com/zhehong268";
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
    if (name == "user")
        return svgWrap(R"ICON(<circle cx="12" cy="8" r="3.8"/><path d="M4.5 20.5c0-3.9 3.4-5.8 7.5-5.8s7.5 1.9 7.5 5.8"/>)ICON");
    if (name == "pin")
        return svgWrap(R"ICON(<path d="M12 21s6.8-6.3 6.8-10.7A6.8 6.8 0 0 0 5.2 10.3C5.2 14.7 12 21 12 21Z"/><circle cx="12" cy="10.3" r="2.4"/>)ICON");
    if (name == "target")
        return svgWrap(R"ICON(<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="3.4"/><path d="M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3"/>)ICON");
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
               "<span class=\"stat-num mono\" data-count=\"" + std::to_string(s.value) + "\">0</span>"
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
:root{
  --bg:#f6f7fc;
  --ink:#151a2e;
  --ink-dim:rgba(21,26,46,.66);
  --ink-faint:rgba(21,26,46,.44);
  --violet:#6d4dfa;
  --violet-soft:#8b74ff;
  --blue:#3b82f6;
  --sky:#0ea5e9;
  --pink:#c026d3;
  --line:rgba(21,26,46,.09);
  --line-strong:rgba(21,26,46,.16);
  --panel:#ffffff;
  --ease:cubic-bezier(.22,.61,.36,1);
  --mono:'JetBrains Mono','Cascadia Mono',Consolas,'SF Mono',Menlo,monospace;
  --sans:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Hiragino Sans GB','Microsoft YaHei',sans-serif;
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

/* ---------- 背景层（固定，仅 transform 动） ---------- */
.bg-fx{position:fixed;inset:0;overflow:hidden;pointer-events:none;z-index:0;
  background:linear-gradient(180deg,#fbfcff 0%,#f4f6fd 42%,#eef1fa 100%)}
.bg-grid{position:absolute;inset:0;opacity:.5;
  background-image:radial-gradient(rgba(21,26,46,.055) 1px,transparent 1px);
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
.nav.solid{background:rgba(252,253,255,.94);border-bottom-color:var(--line);padding-top:10px;padding-bottom:10px}
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
  border:1px solid var(--line-strong);background:#fff;color:var(--violet);
  box-shadow:0 6px 16px -10px rgba(60,70,140,.5);
  transition:color .25s ease,background .25s ease,transform .25s ease;
}
.icon-btn:hover{background:rgba(109,77,250,.10);transform:translateY(-2px)}
.icon-btn.off{color:var(--ink-faint)}

/* ---------- 首屏封面 ---------- */
.landing{
  position:fixed;inset:0;z-index:120;display:grid;place-items:center;
  text-align:center;padding:32px 20px;overflow:hidden;
  background:linear-gradient(180deg,#ffffff 0%,#f3f5fd 55%,#eaeefb 100%);
  transition:opacity .85s ease,visibility .85s ease;
}
.landing.hidden{opacity:0;visibility:hidden;pointer-events:none}
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
  box-shadow:0 40px 80px -40px rgba(40,50,120,.55),0 0 0 1px rgba(21,26,46,.06);
  transform-style:preserve-3d;
}
.landing-photo img{display:block;width:100%;height:auto;aspect-ratio:16/9;object-fit:cover}
.landing-latin{margin-top:30px;font-family:var(--mono);font-size:11.5px;letter-spacing:7px;color:var(--ink-faint)}
.landing-title{
  margin-top:12px;font-size:clamp(26px,5vw,44px);font-weight:800;letter-spacing:1.5px;line-height:1.3;
  background:linear-gradient(110deg,#151a2e 0%,var(--violet) 52%,var(--sky) 100%);
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent;
}
.landing-sub{margin-top:10px;color:var(--ink-dim);font-size:15px}
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
  background:#fbfcff;opacity:0;transition:opacity .55s ease;
}
.loader.show{display:grid;opacity:1}
.sig{width:88%;max-width:600px;height:auto;overflow:visible}
.sig text{
  font-family:'Brush Script MT','Segoe Script','Snell Roundhand',cursive;
  font-size:70px;font-weight:700;letter-spacing:2px;
  fill:none;stroke:var(--violet);stroke-width:2.2;
  stroke-linecap:round;stroke-linejoin:round;
  stroke-dasharray:2600;stroke-dashoffset:2600;
  animation:draw 3s cubic-bezier(.45,0,.3,1) forwards;
}
.sig line{stroke:var(--sky);stroke-width:2;stroke-linecap:round;opacity:.7;
  stroke-dasharray:420;stroke-dashoffset:420;animation:draw 1.5s cubic-bezier(.45,0,.3,1) .35s forwards}
@keyframes draw{to{stroke-dashoffset:0}}

/* ---------- 页面容器 ---------- */
.page{position:relative;z-index:2;opacity:0;transition:opacity .9s ease;
  padding:0 clamp(16px,4vw,40px)}
.page.visible{opacity:1}
.section{max-width:1080px;margin:0 auto;padding:76px 0;scroll-margin-top:90px}
.section-head{margin-bottom:36px}
.eyebrow{display:inline-flex;align-items:center;gap:9px;
  font-family:var(--mono);font-size:11.5px;letter-spacing:3px;color:var(--violet);margin-bottom:14px}
.eyebrow::before{content:'';width:26px;height:1px;background:linear-gradient(90deg,var(--violet),transparent)}
.section-title{
  font-size:clamp(23px,3.4vw,32px);font-weight:750;letter-spacing:.5px;
  background:linear-gradient(110deg,#151a2e 0%,var(--violet) 70%,var(--sky) 100%);
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent;
}
.section-desc{margin-top:12px;color:var(--ink-dim);font-size:14.5px;max-width:64ch}

/* ---------- 入场动画 ---------- */
.reveal{
  opacity:0;transform:translate3d(0,38px,0) scale(.985);
  transition:opacity .8s var(--ease),transform .9s var(--ease);
  transition-delay:var(--d,0ms);
}
.reveal.in{opacity:1;transform:none}

/* ---------- 通用卡片 ---------- */
.card{
  position:relative;background:var(--panel);border:1px solid var(--line);border-radius:18px;
  padding:26px 24px;box-shadow:0 14px 34px -24px rgba(60,70,140,.45);
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
  box-shadow:0 40px 90px -60px rgba(40,50,120,.65),0 0 0 1px rgba(21,26,46,.02);
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
  background:#eef1fa;cursor:pointer}
.avatar-ring img.pop{animation:avatarPop .7s cubic-bezier(.34,1.56,.64,1)}
@keyframes avatarPop{
  0%{transform:scale(1) rotate(0)}
  25%{transform:scale(1.16) rotate(-6deg)}
  50%{transform:scale(.94) rotate(5deg)}
  75%{transform:scale(1.06) rotate(-2deg)}
  100%{transform:scale(1) rotate(0)}
}
.hero-name{font-size:clamp(26px,4vw,36px);font-weight:800;letter-spacing:2px;
  background:linear-gradient(110deg,#151a2e,var(--violet) 58%,var(--sky));
  -webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}
.hero-title{margin-top:6px;color:var(--violet);font-weight:600;letter-spacing:2px;font-size:15px}
.hero-clock{margin-top:20px;font-size:26px;font-weight:700;letter-spacing:7px;color:var(--violet)}
.hero-status{margin-top:6px;font-size:13px;letter-spacing:1.5px;color:var(--ink-faint)}
.hero-bio{margin:24px auto 0;max-width:46ch;font-size:15px;line-height:1.9;color:var(--ink-dim)}
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
  font-size:14px;font-weight:600;border:1px solid var(--line-strong);background:#fff;
  transition:transform .3s var(--ease),border-color .3s ease,background .3s ease}
.btn:hover{transform:translateY(-3px);background:#fbfbff;border-color:rgba(109,77,250,.5)}
.btn-primary{color:#fff;border:none;background:linear-gradient(110deg,var(--violet),var(--blue));
  box-shadow:0 14px 28px -14px rgba(109,77,250,.8)}
.btn-primary:hover{background:linear-gradient(110deg,var(--violet),var(--blue))}
.hero-note{margin-top:26px;font-size:11.5px;letter-spacing:1px;color:var(--ink-faint)}

/* ---------- 数字条 ---------- */
.stats{
  display:flex;justify-content:center;flex-wrap:wrap;gap:18px 54px;
  max-width:780px;margin:36px auto 0;padding:24px 20px;
  border-radius:18px;background:#fff;border:1px solid var(--line);
  box-shadow:0 20px 46px -34px rgba(60,70,140,.55);
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
.facts{background:#fff;border:1px solid var(--line);border-radius:18px;padding:8px 20px;
  box-shadow:0 20px 46px -36px rgba(60,70,140,.5)}
.fact{display:flex;align-items:center;gap:12px;padding:14px 0;border-bottom:1px solid var(--line)}
.fact:last-child{border-bottom:none}
.fact .ic{color:var(--violet)}
.fact-k{font-size:12.5px;color:var(--ink-faint);letter-spacing:1px;width:64px;flex:none}
.fact-v{font-size:14px;font-weight:600}
.quote{margin-top:30px;padding:20px 24px;border-left:3px solid var(--violet);
  background:linear-gradient(90deg,rgba(109,77,250,.07),rgba(109,77,250,0) 72%);
  border-radius:0 12px 12px 0}
.quote p{font-size:16px;color:var(--ink);letter-spacing:.5px}
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
.bar{height:6px;border-radius:99px;background:rgba(21,26,46,.07);overflow:hidden}
.bar-fill{display:block;height:100%;border-radius:99px;
  background:linear-gradient(90deg,var(--violet),var(--blue) 60%,var(--sky));
  transform:scaleX(0);transform-origin:0 50%;
  transition:transform 1.15s var(--ease)}
.bar-fill.grow{transform:scaleX(var(--w,1))}
.tools-title{font-size:12px;letter-spacing:2.5px;color:var(--ink-faint);margin-bottom:14px}
.tools{list-style:none;display:flex;flex-wrap:wrap;gap:10px}
.tool{display:inline-flex;align-items:center;gap:8px;padding:8px 14px;border-radius:10px;
  font-size:12.5px;color:var(--ink-dim);background:#fff;border:1px solid var(--line);
  transition:color .3s ease,border-color .3s ease,transform .3s var(--ease)}
.tool:hover{color:var(--ink);border-color:rgba(109,77,250,.45);transform:translateY(-2px)}
.tool-dot{width:5px;height:5px;border-radius:50%;background:var(--violet);flex:none}

/* ---------- 番剧清单 ---------- */
.anime-list{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:14px}
.anime{position:relative;display:flex;align-items:flex-start;gap:16px;padding:20px 22px;
  border-radius:16px;background:#fff;border:1px solid var(--line);
  box-shadow:0 14px 34px -28px rgba(60,70,140,.5);
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
.rhythm-bg{position:absolute;inset:0;background:url('__BG__') center/cover no-repeat;opacity:.24}
.rhythm-bg::after{content:'';position:absolute;inset:0;
  background:linear-gradient(180deg,rgba(11,16,32,.72),rgba(11,16,32,.93))}
.rhythm-wrap canvas{position:relative;display:block;width:100%;height:100%}
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

/* ---------- 关于本站 ---------- */
.site-box{background:#fff;border:1px solid var(--line);border-left:3px solid var(--violet);
  border-radius:0 18px 18px 0;padding:30px 34px;
  box-shadow:0 20px 46px -38px rgba(60,70,140,.5)}
.site-box p{color:var(--ink-dim);font-size:14.5px;margin-bottom:14px}
.site-box p:last-of-type{margin-bottom:0}
.site-box code{font-family:var(--mono);font-size:12.5px;color:var(--violet);
  background:rgba(109,77,250,.09);border:1px solid rgba(109,77,250,.2);
  padding:2px 7px;border-radius:6px}
.site-tech{margin-top:22px;padding-top:18px;border-top:1px solid var(--line)}

/* ---------- 联系 ---------- */
.contact-wrap{text-align:center}
.contact-row{display:flex;justify-content:center;flex-wrap:wrap;gap:14px}
.contact-card{display:flex;align-items:center;gap:12px;padding:16px 24px;border-radius:14px;
  background:#fff;border:1px solid var(--line);box-shadow:0 14px 34px -28px rgba(60,70,140,.5);
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
  border:1px solid var(--line-strong);background:#fff;
  box-shadow:0 16px 30px -18px rgba(60,70,140,.7);
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
  .nav-links a{padding:6px 10px;font-size:13px}
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
  .reveal{opacity:1;transform:none;transition:none}
  .ripple,.spotlight{display:none}
  .sig text,.sig line{animation-duration:.01ms}
  .bar-fill{transition:none}
}
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

  function onScroll() {
    queued = false;
    var y = window.pageYOffset;
    var max = document.documentElement.scrollHeight - window.innerHeight;
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
    for (var i = 0; i < sections.length; i++) {
      if (sections[i].getBoundingClientRect().top <= 150) current = sections[i].id;
    }
    links.forEach(function (a) {
      a.classList.toggle('active', a.getAttribute('href') === '#' + current);
    });
  }
  window.addEventListener('scroll', function () {
    if (!queued) { queued = true; requestAnimationFrame(onScroll); }
  }, { passive: true });

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
  var audioCtx = null, analyser = null, freq = null, vGrad = null, vizRaf = 0;

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

  function setupAudioGraph() {
    if (audioCtx) return;
    var AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    try {
      audioCtx = new AC();
      var src = audioCtx.createMediaElementSource(music);
      analyser = audioCtx.createAnalyser();
      analyser.fftSize = 64;
      analyser.smoothingTimeConstant = 0.82;
      src.connect(analyser);
      analyser.connect(audioCtx.destination);
      freq = new Uint8Array(analyser.frequencyBinCount);
    } catch (err) {
      audioCtx = null; analyser = null;
    }
  }

  function drawViz() {
    vizRaf = 0;
    if (!analyser || music.paused || document.hidden) return;
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
    vizRaf = requestAnimationFrame(drawViz);
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

  function setMuted(m) {
    musicBtn.classList.toggle('off', !!m);
    musicBtn.innerHTML = m ? window.__ICON_OFF : window.__ICON_ON;
    musicBtn.setAttribute('aria-label', m ? '播放背景音乐' : '暂停背景音乐');
  }

  music.addEventListener('play', function () { setMuted(false); startViz(); });
  music.addEventListener('pause', function () { setMuted(true); stopViz(); });
  music.addEventListener('ended', function () { setMuted(true); stopViz(); });

  musicBtn.addEventListener('click', function (e) {
    e.stopPropagation();
    setupAudioGraph();
    if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();
    if (music.paused) {
      var p = music.play();
      if (p && p.catch) p.catch(function () { setMuted(true); });
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
    setupAudioGraph();
    if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();
    var play = music.play();
    if (play && play.catch) play.catch(function () { setMuted(true); });

    loader.classList.add('show');

    setTimeout(function () {
      document.body.classList.remove('locked');
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
      }, 600);
    }, reduce ? 400 : 3000);
  }

  $('#enterBtn').addEventListener('click', enterSite);

  /* ============ 音游（canvas + rAF 时间轴） ============ */
  var cvs = $('#rhythmCanvas');
  var ctx = cvs ? cvs.getContext('2d') : null;
  var LANES = 7, BPM = 168, BEAT = 60000 / BPM, TRAVEL = 1450;
  var notes = [], sparks = [];
  var combo = 0, acc = 0, prev = 0, rafId = 0, running = false;
  var W = 0, H = 0, laneW = 0, hitY = 0, glow = 0, noteGrad = null;
  var lastLane = -1, laneGlow = [], seen = false;

  for (var li = 0; li < LANES; li++) laneGlow.push(0);

  function fitCanvas() {
    if (!cvs) return;
    var rect = cvs.getBoundingClientRect();
    if (!rect.width || !rect.height) return;
    var dpr = Math.min(window.devicePixelRatio || 1, 2);
    W = rect.width; H = rect.height;
    cvs.width = Math.round(W * dpr);
    cvs.height = Math.round(H * dpr);
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    laneW = W / LANES;
    hitY = H - 66;
    noteGrad = ctx.createLinearGradient(0, 0, 0, 13);
    noteGrad.addColorStop(0, '#f2fbff');
    noteGrad.addColorStop(1, '#7dd3fc');
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

  function spawn() {
    var lane = Math.floor(Math.random() * LANES);
    if (notes.length > 0 && lane === lastLane && Math.random() < 0.55) {
      lane = (lane + 1 + Math.floor(Math.random() * (LANES - 1))) % LANES;
    }
    lastLane = lane;
    notes.push({ lane: lane, y: -14, dead: 0 });
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

  function update(dt) {
    acc += dt;
    var guard = 0;
    while (acc >= BEAT && guard++ < 4) { acc -= BEAT; spawn(); }

    var speed = (hitY + 20) / TRAVEL;
    for (var i = notes.length - 1; i >= 0; i--) {
      var n = notes[i];
      if (n.dead) {
        n.dead += dt;
        if (n.dead > 240) notes.splice(i, 1);
        continue;
      }
      n.y += speed * dt;
      if (n.y >= hitY) {
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
  }

  function render() {
    if (!ctx) return;
    ctx.clearRect(0, 0, W, H);

    ctx.fillStyle = 'rgba(125,211,252,' + (0.55 + glow * 0.45) + ')';
    ctx.fillRect(0, hitY + 10, W, 3);
    ctx.fillStyle = 'rgba(232,121,249,' + (0.18 + glow * 0.35) + ')';
    ctx.fillRect(0, hitY + 14, W, 1);

    ctx.fillStyle = 'rgba(125,211,252,.07)';
    for (var i = 1; i < LANES; i++) ctx.fillRect(i * laneW - 0.5, 0, 1, H);

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
        ctx.fillStyle = noteGrad;
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
    var dt = prev ? Math.min(t - prev, 64) : 16;
    prev = t;
    if (!document.hidden) { update(dt); render(); }
    rafId = requestAnimationFrame(loop);
  }

  function startRhythm() {
    if (!ctx || running || reduce) { if (ctx) render(); return; }
    running = true;
    prev = 0; acc = 0;
    rafId = requestAnimationFrame(loop);
  }

  function stopRhythm() {
    running = false;
    if (rafId) { cancelAnimationFrame(rafId); rafId = 0; }
  }

  if (cvs) {
    fitCanvas();
    seen = true;
    window.addEventListener('resize', function () {
      clearTimeout(cvs.__t);
      cvs.__t = setTimeout(fitCanvas, 180);
    }, { passive: true });
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
<meta name="description" content="__NAME__的个人主页：C++ 开发者，在学编程，也看动漫。整站由 C++ 生成。">
<meta name="theme-color" content="#f6f7fc">
<meta property="og:type" content="website">
<meta property="og:title" content="__NAME__ · 个人主页">
<meta property="og:description" content="写代码，也看番。整站由 C++ 生成，部署在 GitHub Pages。">
<link rel="icon" type="image/svg+xml" href="favicon.svg">
<title>__NAME__ · 个人主页</title>
<style>
__CSS__
</style>
</head>
<body class="locked">
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

int main() {
    Profile me;

    std::vector<Project> projects = {
        {"个人主页生成器",
         "整站由 C++ 生成：内容用数据结构描述，编译运行即输出 index.html，仓库里没有手写 HTML。"
         "push 之后由 GitHub Actions 自动编译并部署到 Pages。",
         "C++17 / GitHub Actions / GitHub Pages", me.github + "/my-intro", "查看源码", "01"},
        {"Rhythm 内置音游",
         "页面里跑在 canvas 上的节奏小游戏，168 BPM 自动演奏。做一个能看的东西，顺便练 rAF 时间轴与性能取舍。",
         "Canvas 2D / requestAnimationFrame / 性能优化", "#rhythm", "页面内查看", "02"},
        {"Project Alpha",
         "还在构思中的 C++ 小工具，目标是先把基础打牢，再动手写点真正有用的东西。",
         "C++ / 计划中", "#", "敬请期待", "03"}
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

    /* ---------- 组装 HTML ---------- */
    std::string html = kHead;
    html = replaceAll(html, "__NAME__", me.name);
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
      <img src="__COVER__" alt="封面插画" width="1280" height="720">
    </div>
    <p class="landing-latin landing-fade" style="animation-delay:.35s">__LATIN__</p>
    <h1 class="landing-title landing-fade" style="animation-delay:.45s">__WELCOME__</h1>
    <p class="landing-sub landing-fade" style="animation-delay:.6s">__TAGLINE__</p>
    <button class="enter-btn landing-fade" id="enterBtn" style="animation-delay:.75s" type="button">
      <span>进入主页</span>
    </button>
    <p class="enter-hint landing-fade" style="animation-delay:.9s">进入后会播放背景音乐，可随时在右上角关闭</p>
  </div>
</div>
)HTML";

    html = replaceAll(html, "__COVER__", me.coverImage);
    html = replaceAll(html, "__LATIN__", me.latin);
    html = replaceAll(html, "__WELCOME__", me.welcomeText);
    html = replaceAll(html, "__TAGLINE__", me.tagline);

    html += R"HTML(
<div class="loader" id="loader" role="status" aria-live="polite">
  <svg class="sig" viewBox="0 0 620 160" xmlns="http://www.w3.org/2000/svg" aria-label="It's MyGO!!!!!">
    <text x="310" y="98" text-anchor="middle">It's MyGO!!!!!</text>
    <line x1="150" y1="126" x2="470" y2="126"/>
  </svg>
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
    <button class="icon-btn" id="musicBtn" type="button" aria-label="暂停背景音乐"></button>
  </div>
</nav>

<main class="page" id="page">
)HTML";

    html = replaceAll(html, "__NAME__", me.name);

    /* --- 主视觉 --- */
    html += "<header class=\"section hero\" id=\"top\">";
    html += "<div class=\"hero-card reveal\">";
    html += "<div class=\"avatar-ring\"><img id=\"avatarImg\" src=\"" + me.avatar +
            "\" alt=\"" + me.name + "的头像\" width=\"132\" height=\"132\" decoding=\"async\"></div>";
    html += "<h1 class=\"hero-name\">" + me.name + "</h1>";
    html += "<p class=\"hero-title\">" + me.title + "</p>";
    html += "<div class=\"hero-clock mono\" id=\"clock\">--:--:--</div>";
    html += "<p class=\"hero-status\" id=\"status\">正在加载…</p>";
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
    <p class="section-desc">一个跑在单个 canvas 上的自动演奏小游戏：168 BPM，七轨，命中特效由 requestAnimationFrame 时间轴驱动。滚出可视区域会自动暂停。</p>
  </div>
  <div class="rhythm-wrap reveal">
    <div class="rhythm-bg" aria-hidden="true"></div>
    <canvas id="rhythmCanvas" role="img" aria-label="自动演奏的节奏小游戏"></canvas>
    <div class="rhythm-overlay">
      <div class="combo">
        <span class="combo-num mono" id="comboNum">0</span>
        <span class="combo-label">COMBO</span>
      </div>
      <span class="rhythm-tag">AUTO PLAY</span>
      <span class="rhythm-tag right">168 BPM</span>
    </div>
  </div>
</section>
)HTML";
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
    <div class="site-tech">
      <ul class="tools">
)HTML";
    html += buildTools({"C++17", "静态生成器", "GitHub Actions", "GitHub Pages", "Canvas 2D", "原生 JS"});
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
    <span>Generated by C++ · GitHub Pages</span>
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
  <p style="position:fixed;left:0;right:0;bottom:0;padding:12px;text-align:center;background:#fff;font-size:13px">
    本站的交互需要启用 JavaScript。
  </p>
</noscript>

<script>
window.__ICON_ON = ')HTML" + icon("music") + R"HTML(';
window.__ICON_OFF = ')HTML" + icon("music-off") + R"HTML(';
</script>
<script>
__JS__
</script>
</body>
</html>
)HTML";

    html = replaceAll(html, "__JS__", kJS);

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

    std::cout << "生成成功：index.html (" << html.size() << " 字节) + favicon.svg" << std::endl;
    return 0;
}
