# assets/fonts —— 自托管字体子集

全站只用这两款开源字体（均为 **SIL OFL 1.1** 授权，可自由商用、可嵌入网站；
许可证全文在本目录 `OFL-*.txt`，与两款字体一一对应）：

| 文件 | 字体 | 用途 | 子集大小 |
|---|---|---|---|
| `lxgw-wenkai-display.woff2` | 霞鹜文楷 Medium v1.522 | 展示字体：大门标题、Hero 姓名、分区标题、引言（`--display`） | ~13 KB |
| `jetbrains-mono-regular.woff2` | JetBrains Mono Regular | 等宽：标签、时钟、代码、数字（`--mono`，400） | ~15 KB |
| `jetbrains-mono-bold.woff2` | JetBrains Mono Bold | 等宽粗体（`--mono`，700/800） | ~25 KB |

三个文件合计约 **53 KB**，在首屏体积预算（450 KB）之内，门禁无需放水。

## 为什么是子集

中文字体全量 25 MB 起，而页面用到的字很集中：展示字体只覆盖
`display-chars.txt` 里的 60 个字（当前全部标题/引言用字），等宽只覆盖
`mono-chars.txt` 里的 96 个字符（页面上出现过的非 CJK 字符——CJK 在等宽
上下文里会按字体链回退系统字体，子集不需要收）。

**改了标题/引言文字后**，新出现的字不在子集里，会按回退链渲染成系统楷体
（`Kaiti SC` / `KaiTi` / serif），不影响阅读，但那个字不是文楷。想补齐就
重跑子集（见下），或者干脆不管——回退链就是为这一刻准备的。

## 重新生成

```bash
pip install fonttools brotli
python tools/subset-fonts.py     # 仓库根目录跑；需要已生成的 index.html
```

脚本会：从 `index.html` 重新提取两类用字 → 下载源字体 → 子集化 → 覆盖本目录
的 woff2 和两个字表。跑完记得重新编译 `main.cpp` 并提交。

## 来源

- 霞鹜文楷：<https://github.com/lxgw/LxgwWenKai>（v1.522，Medium）
- JetBrains Mono：<https://github.com/JetBrains/JetBrainsMono>（master webfonts）
