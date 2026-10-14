# my-intro

遮鸿的个人主页 —— 一个用 **C++17 单文件生成器** 拼出来的静态站点，外加一份同样用 C++ 写成、编译成 **WebAssembly** 的音游谱面内核。

线上地址：<https://zhehong268.github.io/my-intro/>

## C++ 在这个项目里干了两件事

1. **`main.cpp` —— 静态生成器**：把整站 HTML / CSS / JS 拼成 `index.html`，另写 `favicon.svg`、`404.html`、`robots.txt`、`sitemap.xml`。内容是 C++ 结构体（`Profile` / `Project` / `Skill` / `Anime` …），模板是原始字符串，没有第三方依赖、没有模板引擎。
2. **`rhythm_core.cpp` —— 音游谱面内核**：xorshift32 随机数 + 「不连续落在同一条轨道」的规则，**同一份源码编译两次**：
   - `emcc` → `rhythm_core.wasm`（616 B，0 个 import）→ 生成器把它 base64 内联进页面，浏览器里 `WebAssembly.instantiate` 起一个实例算谱面；
   - `cl` / `g++` → 生成器自己用同一份算法算一份 256 音符的谱面内联进页面。
   页面加载时浏览器比对内核与自己算的前 64 个音符：一致就显示「C++ / WASM 内核 · 谱面自检 ok」，不一致或没有 wasm 就退回 JS 谱面并如实标注。这样 wasm 不是摆设 —— 它对不上，页面会自己说出来。

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
生成成功：index.html (83610 字节) + favicon.svg + 404.html + robots.txt + sitemap.xml
音游内核：已内联 rhythm_core.wasm (616 字节 → base64 824 字节)
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

`rhythm_core.wasm` 缺失时生成器只打印一行 emcc 命令提示，其余文件照常生成；页面的音游退回 JS 谱面，徽章显示「内核缺失」。CI 里的 emsdk 两步都是 `continue-on-error`，靠的就是这条降级路径 —— 仓库里也提交了一份 616 B 的 wasm 做兜底。

## 文件

| 文件 | 说明 |
| --- | --- |
| `main.cpp` | 生成器：内容 + 模板 + 全部 CSS / JS（96 KB） |
| `rhythm_core.hpp` | 谱面算法，native 与 wasm 共用的唯一实现 |
| `rhythm_core.cpp` | `extern "C"` 导出层（`kernel_version` / `chart_reset` / `chart_next` / `chart_at`） |
| `rhythm_core.wasm` | emcc 产物（616 B），仓库里留一份兜底 |
| `index.html` | 生成产物（83.6 KB，含内联 CSS / JS / wasm） |
| `404.html` `robots.txt` `sitemap.xml` | 生成产物 |
| `cover.jpg` `avatar.jpg` `bg.webp` | 素材图片 |
| `music.mp3` | 背景音乐，128 kbps（4.1 MB，原 10.2 MB），`preload="none"`，点音乐按钮才加载 |
| `og-image.jpg` | 分享卡片 1200×630（65 KB），由 `tools/make-og.mjs` 渲染后**提交进仓库**，CI 只负责复制 |
| `LICENSE` | 代码 MIT；素材版权说明写在同一个文件的下半部分 |
| `selftest.mjs` | 本机自检：起本地静态服务器 + 无头 Edge，跑 32 条断言 |
| `tools/asset-gate.mjs` | 资产门：页面引用到的本地文件必须都在发布目录里 |
| `tools/budget.mjs` | 首屏体积门禁：文件表 + 预算判定 |
| `tools/og-card.html` `tools/make-og.mjs` | 分享卡片的模板与渲染脚本 |
| `.github/workflows/deploy.yml` | CI：编 wasm → 编生成器 → 组装 `_site` → 资产门 → 体积门 → 发布 Pages |

## 站点上的几条约定

- **没有 JavaScript 也要能读完这一页**。默认样式就是最终状态：`html` 上没有 `.js` 类时，入口门不显示、`.reveal` 全部可见、技能条已有长度、数字条直接显示终值；入场动画、实时时钟、音游才是脚本加上去的装饰（`html.js …` 规则负责接管）。脚本挂了，页面只是不动。
- **动画只用 `transform` 与 `opacity`**：不使用 `backdrop-filter`、`background-attachment: fixed`；倾斜 / 聚光 / 视差 rAF 节流且只在桌面端启用；音游跑在单个 `canvas` 上；`ResizeObserver` 重算背板，避免高分屏下拉伸。
- **对比度按 AA 收口**：小字用的 `--ink-faint` 从 `rgba(21,26,46,.44)`（最差 2.7:1）提到 `.62`，实测 4.74–4.86:1。
- **可访问性**：跳转正文的 skip-link、音乐按钮的 `aria-pressed`、canvas 的 `role="img"` + `aria-label`、装饰层 `aria-hidden`、`prefers-reduced-motion` 下关掉全部动效。
- **分享卡片是构建输入**：`og-image.jpg` 由 `tools/make-og.mjs` 用无头 Edge 渲染 `tools/og-card.html` 得到，提交进仓库；CI 只把它复制进 `_site`，不重新渲染（生成器是纯 C++，没有图像库）。
- **两道门都是硬失败**：资产门保证页面引用的文件都发布了，体积门保证首屏没变重；任一道不过，部署直接红掉。

## 自检

```bash
node selftest.mjs --dir=.                       # 本地：起服务器 + 无头 Edge
node selftest.mjs --url=https://zhehong268.github.io/my-intro/   # 线上同一套断言
node selftest.mjs --dir=. --edge="C:\...\msedge.exe"
node tools/asset-gate.mjs --dir=_site           # 资产门（CI 里跑的是同一条命令）
node tools/budget.mjs                           # 首屏体积门禁
```

32 条断言覆盖：无 JS 下的可见性 / 滚动 / 数字 / 技能条 / `.reveal`，入口门，wasm 谱面一致性（`parity`、`laneSource`、`kernel_version`），内联谱面 256 个合法轨道，canvas 背板 = CSS 尺寸 × dpr，滚到底部导航高亮，COMBO 递增，rAF 帧率，skip-link / aria-pressed / canonical / 1200×630 og 卡片 / twitter card / 英文摘要 / 预加载，小字对比度 ≥ 4.5:1，控制台无异常。

首屏体积（`node tools/budget.mjs` 实测，预算 450 KB）：

```
avatar.jpg    17.8 KB   bg.webp  43.9 KB   cover.jpg  70.4 KB
favicon.svg    0.8 KB   index.html 81.7 KB
首屏合计     214.5 KB   其中 HTML gzip 后 24.2 KB
music.mp3   4010.7 KB   按需（预算 4.5 MB）
```

## 已知限制

- `music.mp3` 已从 320 kbps（10.2 MB）压到 128 kbps（4.1 MB），页面仍是 `preload="none"`；不参与首屏，但点开音乐仍要下 4 MB。仓库历史里还留着旧的那个 10 MB blob（想让 clone 变小要重写历史，暂没做）。
- 页面正文只有中文，英文只有「关于本站」里的一段摘要，没有完整英文版、没有 RSS。
- 音游是**自动演奏展示**，不接受判定输入；它的意义是「演示 + 证明算得出的谱面」。
- Pages 部署走 artifact，`_site` 里只有该发布的文件；源码不再随站点一起公开。
- 素材（封面、头像、音乐）不是我的作品，版权说明见 `LICENSE` 下半部分。
