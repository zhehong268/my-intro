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
| `main.cpp` | 生成器：内容 + 模板 + 全部 CSS / JS（208 KB） |
| `rhythm_core.hpp` | 谱面 + 判定窗口 + 准确率算法，native 与 wasm 共用的唯一实现 |
| `rhythm_core.cpp` | `extern "C"` 导出层（`kernel_version` / `chart_reset` / `chart_next` / `chart_at` / `judge_ms` / `accuracy_permille` / `detect_onsets` / `onset_selftest`；起音检测的输入输出放在静态区，用 `onset_ptr` / `onset_out_ptr` 取地址） |
| `rhythm_core.wasm` | emcc 产物（9.8 KB，内核 v3：谱面 + 判定窗口 + 起音检测），仓库里留一份兜底 |
| `index.html` | 生成产物（168.6 KB，含内联 CSS / JS / wasm / 657 个实测起音点） |
| `kernel.html` | 生成产物：内核控制台（任意 seed 的谱面网格 + 判定窗口表 + 算分表，逐行比对内核与期望值，78.3 KB） |
| `nojs.html` | 生成产物：无 JS 对照（同一份 index.html 用禁脚本 / 正常两个 iframe 并排跑，50.5 KB） |
| `onsets.txt` | 从 `music.mp3` 实测出来的 657 个起音点（毫秒，一行一个）——音游谱面的时间轴就是它，换歌要重跑 `tools/onsets.mjs` |
| `404.html` `robots.txt` `sitemap.xml` | 生成产物（sitemap 里含两个工具页） |
| `cover.jpg` `avatar.jpg` `bg.webp` | 素材图片：入口门封面（735×386）/ 头像 / 全站氛围底图（音游 canvas 底下那张也是它） |
| `stage-play.webp` | 音游段落横幅（1400×1113，137 KB，`loading=lazy` → 不计首屏）。仓库里没有这个文件时，生成器自动改用自绘占位（暗色五道光束 / 亮色波形柱） |
| `music.mp3` | 背景音乐，128 kbps（4.1 MB，原 10.2 MB），`preload="none"`，点音乐按钮才加载 |
| `og-image.jpg` | 分享卡片 1200×630（70 KB），由 `tools/make-og.mjs` 渲染后**提交进仓库**，CI 只负责复制 |
| `assets/fonts/*.woff2` | 自托管字体子集（OFL）：霞鹜文楷 Medium 做展示标题（`--display`），JetBrains Mono Regular/Bold 做等宽（`--mono`）；按页面实际用字子集化，三个文件共约 53 KB，`font-display:swap`。来源、授权、重新生成方法见 `assets/fonts/README.md` |
| `LICENSE` | 代码 MIT；素材版权说明写在同一个文件的下半部分 |
| `selftest.mjs` | 本机自检：起本地静态服务器 + 无头 Edge，本地 76 条 / 线上 75 条断言（差的那条查生成物里有没有占位符残留，需要磁盘上的文件） |
| `audio-start.mjs` | 量「点进入主页 → 真的出声」：走真实入口门、真点按钮，打印 `play` / `loadedmetadata` / `playing` / 遮罩撤掉的时刻，以及预热 fetch 发没发、元素那次请求是不是命中缓存。用法 `node audio-start.mjs <url> 0 <cdp端口> - <点门前等多少毫秒>` |
| `tools/wasm-check.mjs` | 内核契约门：导出、版本号、谱面、判定窗口边界、准确率权重 |
| `tools/asset-gate.mjs` | 资产门：页面引用到的本地文件必须都在发布目录里 |
| `tools/budget.mjs` | 首屏体积门禁：文件表 + 预算判定 |
| `tools/subset-fonts.py` | 重新生成字体子集：从 index.html 提取用字 → 下载源字体 → pyftsubset 出 woff2（改了标题文字后跑它补齐新字） |
| `tools/onsets.mjs` | 从音频里量起音点：ffmpeg 解码成 11 kHz 单声道 PCM → 1024 点 FFT 谱通量 → 自适应阈值挑峰 → 93 ms 合并。`node tools/onsets.mjs music.mp3 --write`（需要 PATH 里有 ffmpeg，或用 `FFMPEG=<路径>` 指定；`--out=` 改输出文件名） |
| `tools/font-coverage.mjs` | 字体覆盖门：页面真正用到的展示/等宽字符必须都在子集里；`—…·` 这三个按设计回退的标点只提醒，真漏字才红。负向测试 `_review/my-intro/font-gate-test.mjs`（4 棵造出来的树） |
| `tools/og-card.html` `tools/make-og.mjs` | 分享卡片的模板与渲染脚本 |
| `tools/rhythm-gate.mjs` | 谱面时间轴门：`onsets.txt` 必须是递增的整数、`index.html` 里内联的那份必须与它逐个相等、`__RHYTHM_SONG_MS` 必须大于最后一个起音点（换了歌忘了重跑 `tools/onsets.mjs` 会在这里红掉） |
| `tools/drop-test.mjs` | 拖歌路径的端到端测试（本地跑，CI 里没有浏览器）：现场合成 10 秒点击轨 → `DOM.setFileInputFiles` 塞进真的 `#dropFile` → 断言页面内 wasm 自检、量到的起音点数、`audio.src` 变 `blob:`、时钟标签与 `__rhythmClockProbe`；`--theme=mygo` 还出暗色图 |
| `tools/drop-test.mjs` | 拖歌路径的端到端测试（本地跑，CI 里没有浏览器）：现场合成 10 秒点击轨 → `DOM.setFileInputFiles` 塞进真的 `#dropFile` → 断言页面内 wasm 自检、量到的起音点数、`audio.src` 变 `blob:`、时钟标签与 `__rhythmClockProbe`；`--theme=mygo` 还出暗色图 |
| `tools/drop-test.mjs` | 拖歌路径的端到端测试（本地跑，CI 里没有浏览器）：现场合成 10 秒点击轨 → `DOM.setFileInputFiles` 塞进真的 `#dropFile` → 断言页面内 wasm 自检、量到的起音点数、`audio.src` 变 `blob:`、时钟标签与 `__rhythmClockProbe`；`--theme=mygo` 还出暗色图 |
| `.github/workflows/deploy.yml` | CI：编 wasm → 编生成器 → 组装 `_site` → 资产门（三个页面各一遍）→ 内核门 → 体积门 → 字体覆盖门 → 谱面时间轴门 → 发布 Pages |

## 站点上的几条约定

- **没有 JavaScript 也要能读完这一页**。默认样式就是最终状态：`html` 上没有 `.js` 类时，入口门不显示、`.reveal` 全部可见、技能条已有长度、数字条直接显示终值；入场动画、实时时钟、音游才是脚本加上去的装饰（`html.js …` 规则负责接管）。脚本挂了，页面只是不动。
- **动画只用 `transform` 与 `opacity`**：不使用 `backdrop-filter`、`background-attachment: fixed`；倾斜 / 聚光 / 视差 rAF 节流且只在桌面端启用；音游跑在单个 `canvas` 上；`ResizeObserver` 重算背板，避免高分屏下拉伸。
- **对比度按 AA 收口**：小字用的 `--ink-faint` 从 `rgba(21,26,46,.44)`（最差 2.7:1）提到 `.62`，实测 4.74–4.86:1。
- **可访问性**：跳转正文的 skip-link、音乐按钮的 `aria-pressed`、canvas 的 `role="img"` + `aria-label`、装饰层 `aria-hidden`、`prefers-reduced-motion` 下关掉全部动效。
- **字体是自托管的子集，不是系统的**：标题用霞鹜文楷（Medium），等宽用 JetBrains Mono，都是 OFL 授权、按页面实际用字子集化（合计约 53 KB，首屏预算内），`font-display:swap` 不阻塞首屏。子集没覆盖到的字按 `Kaiti SC → KaiTi → serif` 回退；改了标题文字后跑 `python tools/subset-fonts.py` 补齐。
- **分享卡片是构建输入**：`og-image.jpg` 由 `tools/make-og.mjs` 用无头 Edge 渲染 `tools/og-card.html` 得到，提交进仓库；CI 只把它复制进 `_site`，不重新渲染（生成器是纯 C++，没有图像库）。
- **音游是可玩的，判定也来自内核**：默认自动演奏；点「开始挑战（64 音符）」进一轮，键盘 `D F G H J K L`（或数字 `1–7`）以及直接点轨道都能打，判定窗口 `±45ms` PERFECT / `±90ms` GOOD 由 `judge_ms` 给、准确率千分比由 `accuracy_permille` 给。空击不扣分，漏掉的算 MISS；一轮结束出结算面板，最好成绩写进 `localStorage`（键 `myintro.rhythm.best.v1`）。滚出可视区域会暂停，回来时把暂停时长补给还没判定的音符，不会一回来就一串 MISS。
- **两道门都是硬失败**：资产门保证页面引用的文件都发布了，内核门保证 wasm 的导出与边界行为没变，体积门保证首屏没变重；任一道不过，部署直接红掉。
- **谱面可以换 seed**：音游那一栏填一个 seed（纯数字、`0x…` 或任意文字，后者按 FNV-1a 折成 32 位），或者直接开 `?seed=你的名字`。换完是内核现算的 256 个音符，JS 镜像再算一遍逐位核对，结论写在页面上（`window.__rhythmSeedParity`）。三份独立实现——C++/wasm 内核、页面里的 JS 镜像、`selftest.mjs` 里 Node 算的那份——必须给出同一串数字。
- **两个工具页**：`kernel.html` 把内核摊开（换 seed 看谱面网格、19 个判定边界点、12 个算分用例，每张表都是「生成器编译期算的期望值 vs 浏览器里内核实算」逐行比对，不一致标红；附内核导出列表与 SHA-256）；`nojs.html` 把同一份 `index.html` 用「带 `sandbox`（禁脚本）」和「正常」两个 iframe 并排跑，加一张两边差在哪的表。
- **音频抖动会自动降级**：Web Audio 那条通路（`createMediaElementSource` → `AnalyserNode`，用来画导航栏那根频谱）只负责可视化，一旦发现 `waiting` / `stalled` 累计到阈值（且缓冲还够），就新建一个干净的 `<audio>` 元素接上进度、关掉 `AudioContext`、停掉频谱——音乐转成原生通路继续放，绝不为了那根频谱牺牲播放。开播头 3 秒内的缓冲不算抖动（那是正常的起播等待）。`?audio=plain` 直接跳过建图，`?audio=debug` 在左下角打印实时状态（通路 / 采样率 / readyState / 缓冲 / waiting / stalled / 进度）。
- **频谱画布不在就 30fps**：手机上 `#viz` 是 `display:none`，此时根本不建 `AudioContext`；桌面端频谱降到约 30fps（`transform`/`opacity` 之外唯一每帧画的东西），音游滚出可视区域仍然暂停。
- **起播优先，频谱让路**：`enterSite()` 里先 `music.play()`，图等 `playing` 之后再建——`new AudioContext({sampleRate:44100})` 实测要 250ms 上下，排在 `play()` 前面就是实打实的「点进去先静一下」（线上实测 `play` 从点击后 288ms 变成 22ms）。入口门摆着的时候还会用 `fetch` + `Range: bytes=0-163839` 预热音频首包（160 KB ≈ 10 秒的量，`priority:"low"`，省流量模式 / 2G 跳过），这样点下去时容器头已经在缓存里：线上实测 `loadedmetadata` 从 1635ms 变成 29ms、点击到出声从约 1.9s 变成约 0.8s。预热这 160 KB **算进首屏预算**（`tools/budget.mjs` 里那笔 `WARMUP`），不装作不存在。

- **暗色主题是「MyGO 舞台」，而且首帧不闪**：`html.theme-mygo` 一套令牌换肤（近黑底 + 顶部五束成员色光池 + 深色亚克力卡片），导航栏那颗 ★ 负责切换，选择写进 `localStorage['myintro.theme']`，`?theme=mygo|light` 可以强制。决定主题的那段脚本在 `<head>` 里同步跑完，`selftest.mjs` 用 `MutationObserver` 断言类名是在 `readyState === "loading"` 阶段挂上去的——不会先亮一下再变暗。
- **音游跟着正在播的那首歌走**：谱面时间轴不是拍脑袋的 BPM，而是 `tools/onsets.mjs` 从 `music.mp3` 里量出来的 **657 个起音点**（实测曲长 256.6 s、起音间隔中位数 302 ms；同一份工具做梳状滤波给出的速度是 65.3 BPM、每拍 919.5 ms），生成器把它内联进页面。歌在播时**判定时钟就是 `audio.currentTime`**，暂停或没播时换成等速虚拟时钟、接着暂停那一刻的音频位置走；`window.__rhythmClockProbe()` 能读出当前时钟来源、时间轴长度和下一个音符的落点。换歌要重跑 `tools/onsets.mjs` 并改生成器里的 `__RHYTHM_SONG_MS`。
- **拖任意一首歌进来，浏览器里当场量起音点**：`decodeAudioData` 解出来的 PCM 分块喂给同一个内核（`detect_onsets`），谱面立刻换成这首、音源换成 `blob:` 地址 —— 文件不出这台机器，`tools/drop-test.mjs` 走的就是这条路径。10 秒音频实测 5 ms。
- **难度三档只改密度与窗口，判定者仍是内核**：轻松 / 标准 / 硬核 = 起音点抽稀 ×0.5 / ×1 / ×2（硬核用双押：同一下真实起音点落两个音符，时间戳不编造）、判定窗口 ×1.25 / ×1 / ×0.67，窗口倍率是在调用 `judge_ms` 之前折算的。
- **入口门那行徽章是自证的**：生成器用自己的 SHA-256 算出内联内核的摘要写进 `data-sha`，页面加载后再用浏览器的 `crypto.subtle.digest('SHA-256')` 复核同一份 base64 字节 —— 一致才写「浏览器复核一致」，环境没有 `crypto.subtle` 就如实写「构建时算的」。
- **整首歌的起音点时间条**：canvas 下面那条画着 657 根 tick 的条就是内联的时间轴本身，播放头取 `songNow() % CUR_SONG_MS`，换谱面或换主题会重画。
- **拖任意一首歌进来，浏览器里当场量起音点**：`decodeAudioData` 解出来的 PCM 分块喂给同一个内核（`detect_onsets`），谱面立刻换成这首、音源换成 `blob:` 地址 —— 文件不出这台机器，`tools/drop-test.mjs` 走的就是这条路径。10 秒音频实测 5 ms。
- **难度三档只改密度与窗口，判定者仍是内核**：轻松 / 标准 / 硬核 = 起音点抽稀 ×0.5 / ×1 / ×2（硬核用双押：同一下真实起音点落两个音符，时间戳不编造）、判定窗口 ×1.25 / ×1 / ×0.67，窗口倍率是在调用 `judge_ms` 之前折算的。
- **入口门那行徽章是自证的**：生成器用自己的 SHA-256 算出内联内核的摘要写进 `data-sha`，页面加载后再用浏览器的 `crypto.subtle.digest('SHA-256')` 复核同一份 base64 字节 —— 一致才写「浏览器复核一致」，环境没有 `crypto.subtle` 就如实写「构建时算的」。
- **整首歌的起音点时间条**：canvas 下面那条画着 657 根 tick 的条就是内联的时间轴本身，播放头取 `songNow() % CUR_SONG_MS`，换谱面或换主题会重画。
- **拖任意一首歌进来，浏览器里当场量起音点**：`decodeAudioData` 解出来的 PCM 分块喂给同一个内核（`detect_onsets`），谱面立刻换成这首、音源换成 `blob:` 地址 —— 文件不出这台机器，`tools/drop-test.mjs` 走的就是这条路径。10 秒音频实测 5 ms。
- **难度三档只改密度与窗口，判定者仍是内核**：轻松 / 标准 / 硬核 = 起音点抽稀 ×0.5 / ×1 / ×2（硬核用双押：同一下真实起音点落两个音符，时间戳不编造）、判定窗口 ×1.25 / ×1 / ×0.67，窗口倍率是在调用 `judge_ms` 之前折算的。
- **入口门那行徽章是自证的**：生成器用自己的 SHA-256 算出内联内核的摘要写进 `data-sha`，页面加载后再用浏览器的 `crypto.subtle.digest('SHA-256')` 复核同一份 base64 字节 —— 一致才写「浏览器复核一致」，环境没有 `crypto.subtle` 就如实写「构建时算的」。
- **整首歌的起音点时间条**：canvas 下面那条画着 657 根 tick 的条就是内联的时间轴本身，播放头取 `songNow() % CUR_SONG_MS`，换谱面或换主题会重画。
- **致敬只用事实数据和自绘元素**：舞台灯与七条轨道用的五个色值来自公开的成员色票整理（Lee-W/maigo 的 `character-colors`，非官方配色），成员名与担当是事实性描述，「迷子」那句和 ★/☆ 图标都是自己写的、自绘 SVG；歌词没有入库，音频文件没动过。音游段落那张横幅插画由站长自己提供，来源见下面的素材说明。

## 自检

```bash
node selftest.mjs --dir=.                       # 本地：起服务器 + 无头 Edge
node selftest.mjs --url=https://zhehong268.github.io/my-intro/   # 线上同一套断言
node selftest.mjs --dir=. --edge="C:\...\msedge.exe"
node tools/wasm-check.mjs                       # 内核契约（CI 里跑的是同一条命令）
node tools/asset-gate.mjs --dir=_site           # 资产门（对三个页面各跑一遍）
node tools/budget.mjs                           # 首屏体积门禁
node tools/font-coverage.mjs --dir=.            # 字体子集覆盖（改了标题/引言文字后跑）
node tools/rhythm-gate.mjs --dir=.              # 谱面时间轴 vs onsets.txt（换了歌之后跑）
node tools/drop-test.mjs                        # 拖歌路径端到端（真塞文件 + 出图，本地跑）
node tools/drop-test.mjs                        # 拖歌路径端到端（真塞文件 + 出图，本地跑）
node tools/drop-test.mjs                        # 拖歌路径端到端（真塞文件 + 出图，本地跑）
```

本地服务器（`selftest.mjs` 与音频探针里的那个）**必须支持 Range 请求**：GitHub Pages 支持，本地不支持的话 `<audio>` 会被判为不可 seek，降级后回填进度会被静默丢弃——那不是代码 bug，是测量环境的差异。

断言里**不写死 wasm 的字节数**：本地 emsdk 与 CI 的 emcc 版本不同，同一个 `rhythm_core.cpp` 编出来会差几个字节（实测本地 732 B / CI 724 B）。所以那条断言是拿页面里真正内联的那段 base64 现算长度与 SHA-256，再和页面自己算的 `sha256` 对齐——换 emcc 版本不会假失败。

76 条断言覆盖：无 JS 下的可见性 / 滚动 / 数字 / 技能条 / `.reveal`，入口门，wasm 谱面一致性（`parity`、`laneSource`、`kernel_version=3`），内联谱面 256 个合法轨道，canvas 背板 = CSS 尺寸 × dpr，滚到底部导航高亮，COMBO 递增，rAF 帧率，skip-link / aria-pressed / canonical / 1200×630 og 卡片 / twitter card / 英文摘要 / 预加载，小字对比度 ≥ 4.5:1，控制台无异常；可玩模式 9 条（判定内核自检、边界串 `001112222211100`、JS 回退镜像与内核同答案、控制条就位、真按键拿到非 MISS 且 `judgeSource=wasm`、漏判记 MISS、`#rAcc` 等于内核千分比、结算面板 + 最好成绩落盘、刷新后仍在）；音频 8 条（图采样率 = 文件 44100 且 `running`、几十秒无 waiting/stalled/frozen、抖动时降级到原生通路、降级后进度接得上、降级后只剩一个 audio、`?audio=plain` 不建图也照样放、入口门期间预热首包已发出 `window.__musicWarm === 206`、点进入主页到出声 ≤ 1.2s）；歌曲同步 4 条（时间轴来自内联的 657 个起音点、在播时判定时钟就是 `audio.currentTime`、下一个音符落点确实等于某个实测起音点、暂停后退回虚拟时钟且接着音频位置）；主题 5 条（初始主题与 `localStorage` 一致、点 ★ 换肤且 `meta[theme-color]` / 底色 / `aria-pressed` 一起变、选择落盘、刷新后仍是暗色、类名在解析阶段就挂上 = 首帧不闪）；seed 与工具页 8 条（默认 seed 的 16 音符与 Node 独立实现一致、任意文字 seed 折成 32 位后内核与镜像逐位一致、换 seed 确实换谱面、`?seed=` 进入就生效、内核控制台三张表全对、控制台换 seed 后仍一致且地址同步、无 JS 对照页的 sandbox 确实禁脚本且右框已进主页）；自证与新交互 6 条（入口门徽章写着构建时的内核摘要与字节数、进页面后 `crypto.subtle` 复核同一份字节并给出结论、三档难度按钮就位且默认标准、点硬核后选择落盘并按双押重排这一轮、起音点时间条画出 657 根 tick、横幅是 `loading=lazy`）。

首屏体积（`node tools/budget.mjs` 实测，预算 540 KB）：

```
avatar.jpg    17.8 KB   bg.webp  43.9 KB   cover.jpg  70.4 KB
favicon.svg    0.8 KB   index.html 168.6 KB
assets/fonts/ 51.7 KB   霞鹜文楷子集 12.6 + JetBrains Mono Regular 15.0 / Bold 24.1
首屏合计     353.2 KB   其中 HTML gzip 后 57.9 KB
含音频预热   513.1 KB   预热 = 入口门期间那 160 KB 的 Range 预取（省流量模式会跳过）
music.mp3   4010.7 KB   按需（预算 4.5 MB）
kernel.html   78.3 KB   nojs.html 50.5 KB   工具页（点进去才加载，gzip 27.8 / 15.0 KB）
stage-play.webp 136.9 KB  按需（loading=lazy，不计首屏）
```

从 319.2 KB 涨到 353.2 KB 的那 34 KB 花在：入口门那行自证徽章与复核脚本（约 2 KB）、
拖歌那一段（分块喂内核 + 窗口起音检测 + 拖放区样式，约 9 KB）、难度三档与起音点时间条（约 5 KB）、
英文摘要扩写与成员阵容（约 3 KB）、
MyGO 舞台主题的令牌与光效（约 5 KB）、跟着音轨走的判定时钟与成员色轨道（约 3 KB）、
成员阵容那一段文字（约 1 KB）。

预算从 450 KB 上调到 500 KB 的那 52 KB 花在字体上：标题用霞鹜文楷 Medium、
等宽用 JetBrains Mono，都是 OFL 授权、按页面实际用字子集化（合计约 52 KB），
`font-display:swap` 不阻塞首屏。子集没覆盖的字按 `Kaiti SC → KaiTi → serif` 回退；
改了标题文字后跑 `python tools/subset-fonts.py` 补齐（见 `assets/fonts/README.md`）。

预算再从 500 KB 上调到 540 KB 的那 40 KB，是为了让页面能对**拖进来的任意一首歌**当场量起音点：
内核 v3 多出来的 10 KB wasm 内联成 13.4 KB base64（base64 几乎压不动），加上拖放区与状态行约 9 KB，
剩下留作余量。

预算再从 500 KB 上调到 540 KB 的那 40 KB，是为了让页面能对**拖进来的任意一首歌**当场量起音点：
内核 v3 多出来的 10 KB wasm 内联成 13.4 KB base64（base64 几乎压不动），加上拖放区与状态行约 9 KB，
剩下留作余量。

预算再从 500 KB 上调到 540 KB 的那 40 KB，是为了让页面能对**拖进来的任意一首歌**当场量起音点：
内核 v3 多出来的 10 KB wasm 内联成 13.4 KB base64（base64 几乎压不动），加上拖放区与状态行约 9 KB，
剩下留作余量。

## 已知限制

- `music.mp3` 已从 320 kbps（10.2 MB）压到 128 kbps（4.1 MB），页面仍是 `preload="none"`；不参与首屏，但点开音乐仍要下 4 MB。仓库历史里还留着旧的那个 10 MB blob（想让 clone 变小要重写历史，暂没做）。
- 页面正文只有中文，英文只有「关于本站」里的一段摘要，没有完整英文版、没有 RSS。
- 音游现在是**可玩的一轮挑战**（64 音符、两档判定），难度三档已经上了（轻松 / 标准 / 硬核：抽稀与双押、窗口按倍率折算后再交给内核），但没有长按 / 滑条；默认仍然是自动演奏展示。
- Pages 部署走 artifact，`_site` 里只有该发布的文件；源码不再随站点一起公开。
- 音频降级只在**真的观测到** `waiting` / `stalled` 时才发生，无头浏览器用的是空音频设备，声卡那端的实际听感没法自动断言——这一条只能靠 `?audio=debug` 读数和人耳确认。
- 起播那约 0.8s 里，剩下的时间主要花在浏览器的音频管线启动（拿到数据 → 真的出声）上，这一层页面管不到；`preload="auto"` 能让它更早开始解码，但代价是可能把整个 4 MB 拉下来，所以没那么做。
- 本地 MSVC 生成的是 CRLF、CI 的 g++ 是 LF，所以「本地 vs 线上逐字节比对」要先归一化行尾，否则每个换行都是一处分歧（`selftest.mjs` 与 `audio-start.mjs` 都不受影响，它们比的是行为）。
- 素材（封面、头像、音乐）不是我的作品，版权说明见 `LICENSE` 下半部分。
- 音游跟的是**自己量的起音点**，不是官方谱面：`onsets.txt` 里 657 个点是 `tools/onsets.mjs` 对 `music.mp3` 做谱通量峰检测得到的，换一首歌有两条路：重跑 `tools/onsets.mjs` 并改 `__RHYTHM_SONG_MS`（进仓库的那一首），或者在页面上把歌拖进拖放区、让 wasm 内核当场量（只对这次访问有效，不上传、不落盘）。起音点只对齐「响的瞬间」，没有分声部。
- 舞台灯与七条轨道那五个色值来自公开的成员色票整理（Lee-W/maigo，非官方配色）；要换色改 `main.cpp` 里 `MYGO_LANES` 与主题令牌那几行即可。

## 最后
我是遮鸿，感谢你的到来qwq
