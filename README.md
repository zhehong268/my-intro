# my-intro

遮鸿的个人主页 —— 一个用 **C++17 单文件生成器** 拼出来的静态站点，外加一份同样用 C++ 写成、编译成 **WebAssembly** 的音游谱面内核。

线上地址：<https://zhehong268.github.io/my-intro/>

## C++ 在这个项目里干了两件事

1. **`main.cpp` —— 静态生成器**：把整站 HTML / CSS / JS 拼成 `index.html`，另写 `favicon.svg`、`404.html`、`robots.txt`、`sitemap.xml`，以及两个工具页 `kernel.html`（内核控制台）和 `nojs.html`（无 JS 对照）。内容是 C++ 结构体（`Profile` / `Project` / `Skill` / `Anime` …），模板是原始字符串，没有第三方依赖、没有模板引擎。
2. **`rhythm_core.cpp` —— 音游内核（谱面 + 判定 + 算分）**：xorshift32 随机数 + 「不连续落在同一条轨道」的规则，加上判定窗口与准确率；**同一份源码编译两次**：
   - `emcc` → `rhythm_core.wasm`（732 B，0 个 import）→ 生成器把它 base64 内联进页面，浏览器里 `WebAssembly.instantiate` 起一个实例算谱面、判 `judge_ms`、算 `accuracy_permille`；
   - `cl` / `g++` → 生成器自己用同一份头文件算一份 256 音符的谱面，外加两组探针（判定窗口 `±45 / ±90ms` 的边界点、准确率的整数权重）内联进页面。
   页面加载时浏览器比对内核与自己算的前 64 个音符**和这两组探针**：全对就显示「C++ / WASM 内核 · 谱面与判定自检 ok」，不一致或没有 wasm 就退回 JS 镜像并如实标注。这样 wasm 不是摆设 —— 它对不上，页面会自己说出来。

## 构建

生成器把文件写在**当前工作目录**，所以先 `cd` 到仓库根目录再跑。

### Linux / CI（两步）

```bash
emcc rhythm_core.cpp -O3 -o rhythm_core.wasm --no-entry \
  -sSTANDALONE_WASM=1 -sERROR_ON_UNDEFINED_SYMBOLS=1
g++ -std=c++17 -O2 -o generator main.cpp
./generator
```

```
生成成功：index.html (115578 字节) + favicon.svg + 404.html + robots.txt + sitemap.xml + kernel.html (55688 字节) + nojs.html (39650 字节)
音游内核：已内联 rhythm_core.wasm (732 字节 → base64 976 字节)
```

`-std=c++17` 是必须的（`rhythm_core.hpp` 里用了 `inline` 变量）。

### Windows / MSVC

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /Fo.\ /Fe.\generator.exe main.cpp
generator.exe
```

`/utf-8` 必须带：源码里全是中文，不带会按本地代码页解释。

### 没有 wasm 也能构建

`rhythm_core.wasm` 缺失时生成器只打印一行 emcc 命令提示，其余文件照常生成；页面的音游退回 JS 谱面与 JS 判定（镜像算法与内核同答案，`selftest.mjs` 会比对），徽章显示「内核缺失」。CI 里的 emsdk 两步都是 `continue-on-error`，靠的就是这条降级路径 —— 仓库里也提交了一份 732 B 的 wasm 做兜底。

## 文件

| 文件 | 说明 |
| --- | --- |
| `main.cpp` | 生成器：内容 + 模板 + 全部 CSS / JS（113 KB） |
| `rhythm_core.hpp` | 谱面 + 判定窗口 + 准确率算法，native 与 wasm 共用的唯一实现 |
| `rhythm_core.cpp` | `extern "C"` 导出层（`kernel_version` / `chart_reset` / `chart_next` / `chart_at` / `judge_ms` / `accuracy_permille`） |
| `rhythm_core.wasm` | emcc 产物（732 B），仓库里留一份兜底 |
| `index.html` | 生成产物（112.9 KB，含内联 CSS / JS / wasm） |
| `kernel.html` | 生成产物：内核控制台（任意 seed 的谱面网格 + 判定窗口表 + 算分表，逐行比对内核与期望值，54.4 KB） |
| `nojs.html` | 生成产物：无 JS 对照（同一份 index.html 用禁脚本 / 正常两个 iframe 并排跑，38.7 KB） |
| `404.html` `robots.txt` `sitemap.xml` | 生成产物（sitemap 里含两个工具页） |
| `cover.jpg` `avatar.jpg` `bg.webp` | 素材图片 |
| `music.mp3` | 背景音乐，128 kbps（4.1 MB，原 10.2 MB），`preload="none"`，点音乐按钮才加载 |
| `og-image.jpg` | 分享卡片 1200×630（70 KB），由 `tools/make-og.mjs` 渲染后**提交进仓库**，CI 只负责复制 |
| `LICENSE` | 代码 MIT；素材版权说明写在同一个文件的下半部分 |
| `selftest.mjs` | 本机自检：起本地静态服务器 + 无头 Edge，本地 59 条 / 线上 58 条断言（差的那条查生成物里有没有占位符残留，需要磁盘上的文件） |
| `tools/wasm-check.mjs` | 内核契约门：导出、版本号、谱面、判定窗口边界、准确率权重 |
| `tools/asset-gate.mjs` | 资产门：页面引用到的本地文件必须都在发布目录里 |
| `tools/budget.mjs` | 首屏体积门禁：文件表 + 预算判定 |
| `tools/og-card.html` `tools/make-og.mjs` | 分享卡片的模板与渲染脚本 |
| `.github/workflows/deploy.yml` | CI：编 wasm → 编生成器 → 组装 `_site` → 资产门（三个页面各一遍）→ 内核门 → 体积门 → 发布 Pages |

## 站点上的几条约定

- **没有 JavaScript 也要能读完这一页**。默认样式就是最终状态：`html` 上没有 `.js` 类时，入口门不显示、`.reveal` 全部可见、技能条已有长度、数字条直接显示终值；入场动画、实时时钟、音游才是脚本加上去的装饰（`html.js …` 规则负责接管）。脚本挂了，页面只是不动。
- **动画只用 `transform` 与 `opacity`**：不使用 `backdrop-filter`、`background-attachment: fixed`；倾斜 / 聚光 / 视差 rAF 节流且只在桌面端启用；音游跑在单个 `canvas` 上；`ResizeObserver` 重算背板，避免高分屏下拉伸。
- **对比度按 AA 收口**：小字用的 `--ink-faint` 从 `rgba(21,26,46,.44)`（最差 2.7:1）提到 `.62`，实测 4.74–4.86:1。
- **可访问性**：跳转正文的 skip-link、音乐按钮的 `aria-pressed`、canvas 的 `role="img"` + `aria-label`、装饰层 `aria-hidden`、`prefers-reduced-motion` 下关掉全部动效。
- **分享卡片是构建输入**：`og-image.jpg` 由 `tools/make-og.mjs` 用无头 Edge 渲染 `tools/og-card.html` 得到，提交进仓库；CI 只把它复制进 `_site`，不重新渲染（生成器是纯 C++，没有图像库）。
- **音游是可玩的，判定也来自内核**：默认自动演奏；点「开始挑战（64 音符）」进一轮，键盘 `D F G H J K L`（或数字 `1–7`）以及直接点轨道都能打，判定窗口 `±45ms` PERFECT / `±90ms` GOOD 由 `judge_ms` 给、准确率千分比由 `accuracy_permille` 给。空击不扣分，漏掉的算 MISS；一轮结束出结算面板，最好成绩写进 `localStorage`（键 `myintro.rhythm.best.v1`）。滚出可视区域会暂停，回来时把暂停时长补给还没判定的音符，不会一回来就一串 MISS。
- **两道门都是硬失败**：资产门保证页面引用的文件都发布了，内核门保证 wasm 的导出与边界行为没变，体积门保证首屏没变重；任一道不过，部署直接红掉。
- **谱面可以换 seed**：音游那一栏填一个 seed（纯数字、`0x…` 或任意文字，后者按 FNV-1a 折成 32 位），或者直接开 `?seed=你的名字`。换完是内核现算的 256 个音符，JS 镜像再算一遍逐位核对，结论写在页面上（`window.__rhythmSeedParity`）。三份独立实现——C++/wasm 内核、页面里的 JS 镜像、`selftest.mjs` 里 Node 算的那份——必须给出同一串数字。
- **两个工具页**：`kernel.html` 把内核摊开（换 seed 看谱面网格、19 个判定边界点、12 个算分用例，每张表都是「生成器编译期算的期望值 vs 浏览器里内核实算」逐行比对，不一致标红；附内核导出列表与 SHA-256）；`nojs.html` 把同一份 `index.html` 用「带 `sandbox`（禁脚本）」和「正常」两个 iframe 并排跑，加一张两边差在哪的表。
- **音频抖动会自动降级**：Web Audio 那条通路（`createMediaElementSource` → `AnalyserNode`，用来画导航栏那根频谱）只负责可视化，一旦发现 `waiting` / `stalled` 累计到阈值（且缓冲还够），就新建一个干净的 `<audio>` 元素接上进度、关掉 `AudioContext`、停掉频谱——音乐转成原生通路继续放，绝不为了那根频谱牺牲播放。开播头 3 秒内的缓冲不算抖动（那是正常的起播等待）。`?audio=plain` 直接跳过建图，`?audio=debug` 在左下角打印实时状态（通路 / 采样率 / readyState / 缓冲 / waiting / stalled / 进度）。
- **频谱画布不在就 30fps**：手机上 `#viz` 是 `display:none`，此时根本不建 `AudioContext`；桌面端频谱降到约 30fps（`transform`/`opacity` 之外唯一每帧画的东西），音游滚出可视区域仍然暂停。

## 自检

```bash
node selftest.mjs --dir=.                       # 本地：起服务器 + 无头 Edge
node selftest.mjs --url=https://zhehong268.github.io/my-intro/   # 线上同一套断言
node selftest.mjs --dir=. --edge="C:\...\msedge.exe"
node tools/wasm-check.mjs                       # 内核契约（CI 里跑的是同一条命令）
node tools/asset-gate.mjs --dir=_site           # 资产门（对三个页面各跑一遍）
node tools/budget.mjs                           # 首屏体积门禁
```

本地服务器（`selftest.mjs` 与音频探针里的那个）**必须支持 Range 请求**：GitHub Pages 支持，本地不支持的话 `<audio>` 会被判为不可 seek，降级后回填进度会被静默丢弃——那不是代码 bug，是测量环境的差异。

断言里**不写死 wasm 的字节数**：本地 emsdk 与 CI 的 emcc 版本不同，同一个 `rhythm_core.cpp` 编出来会差几个字节（实测本地 732 B / CI 724 B）。所以那条断言是拿页面里真正内联的那段 base64 现算长度与 SHA-256，再和页面自己算的 `sha256` 对齐——换 emcc 版本不会假失败。

59 条断言覆盖：无 JS 下的可见性 / 滚动 / 数字 / 技能条 / `.reveal`，入口门，wasm 谱面一致性（`parity`、`laneSource`、`kernel_version=2`），内联谱面 256 个合法轨道，canvas 背板 = CSS 尺寸 × dpr，滚到底部导航高亮，COMBO 递增，rAF 帧率，skip-link / aria-pressed / canonical / 1200×630 og 卡片 / twitter card / 英文摘要 / 预加载，小字对比度 ≥ 4.5:1，控制台无异常；可玩模式 9 条（判定内核自检、边界串 `001112222211100`、JS 回退镜像与内核同答案、控制条就位、真按键拿到非 MISS 且 `judgeSource=wasm`、漏判记 MISS、`#rAcc` 等于内核千分比、结算面板 + 最好成绩落盘、刷新后仍在）；音频 6 条（图采样率 = 文件 44100 且 `running`、几十秒无 waiting/stalled/frozen、抖动时降级到原生通路、降级后进度接得上、降级后只剩一个 audio、`?audio=plain` 不建图也照样放）；seed 与工具页 8 条（默认 seed 的 16 音符与 Node 独立实现一致、任意文字 seed 折成 32 位后内核与镜像逐位一致、换 seed 确实换谱面、`?seed=` 进入就生效、内核控制台三张表全对、控制台换 seed 后仍一致且地址同步、无 JS 对照页的 sandbox 确实禁脚本且右框已进主页）。

首屏体积（`node tools/budget.mjs` 实测，预算 450 KB）：

```
avatar.jpg    17.8 KB   bg.webp  43.9 KB   cover.jpg  70.4 KB
favicon.svg    0.8 KB   index.html 112.9 KB
首屏合计     245.7 KB   其中 HTML gzip 后 34.5 KB
music.mp3   4010.7 KB   按需（预算 4.5 MB）
kernel.html   54.4 KB   nojs.html 38.7 KB   工具页（点进去才加载，gzip 16.5 / 11.3 KB）
```

## 已知限制

- `music.mp3` 已从 320 kbps（10.2 MB）压到 128 kbps（4.1 MB），页面仍是 `preload="none"`；不参与首屏，但点开音乐仍要下 4 MB。仓库历史里还留着旧的那个 10 MB blob（想让 clone 变小要重写历史，暂没做）。
- 页面正文只有中文，英文只有「关于本站」里的一段摘要，没有完整英文版、没有 RSS。
- 音游现在是**可玩的一轮挑战**（64 音符、两档判定），但没有长按 / 滑条 / 多押，也没有难度分级；默认仍然是自动演奏展示。
- Pages 部署走 artifact，`_site` 里只有该发布的文件；源码不再随站点一起公开。
- 音频降级只在**真的观测到** `waiting` / `stalled` 时才发生，无头浏览器用的是空音频设备，声卡那端的实际听感没法自动断言——这一条只能靠 `?audio=debug` 读数和人耳确认。
- 素材（封面、头像、音乐）不是我的作品，版权说明见 `LICENSE` 下半部分。
