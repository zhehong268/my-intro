/* tools/subset-fonts.py —— 重新生成 assets/fonts/ 下的 woff2 子集
 *
 * 背景：全站只用两款自托管字体（OFL 授权）：
 *   - 霞鹜文楷 Medium（展示字体：大门标题 / Hero 姓名 / 分区标题 / 引言）
 *   - JetBrains Mono Regular + Bold（等宽：标签 / 时钟 / 代码 / 数字）
 * 中文字体全量 25 MB 起步，所以按「页面实际用到的字」子集化，
 * 三个 woff2 合计约 53 KB，首屏预算（450 KB）内放得下。
 *
 * 什么时候需要重跑：
 *   - 改了任意展示字体覆盖的元素文字（标题、姓名、引言），出现了子集里没有的字
 *   - 升级字体版本
 * 不跑也不会坏：缺的字会按 --display 的回退链（Kaiti SC / KaiTi / serif）渲染，
 * 只是那个字不是文楷。
 *
 * 依赖：python3 + fonttools（pip install fonttools brotli）
 * 用法：python tools/subset-fonts.py            # 在仓库根目录跑
 * 前提：index.html 已生成（先编译运行 main.cpp），源 TTF 见下方 SRC 常量。
 */
import io, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HTML = os.path.join(ROOT, "index.html")
OUT = os.path.join(ROOT, "assets", "fonts")
WORK = os.path.join(ROOT, ".fontsubset-tmp")
os.makedirs(OUT, exist_ok=True)
os.makedirs(WORK, exist_ok=True)

# 字体来源（OFL-1.1）。版本号写死在这里，升级时改 URL + 更新 assets/fonts/README.md
SRC = {
    "lxgw-wenkai-display.woff2": {
        "url": "https://raw.githubusercontent.com/lxgw/LxgwWenKai/v1.522/fonts/TTF/LXGWWenKai-Medium.ttf",
        "type": "ttf",
    },
    "jetbrains-mono-regular.woff2": {
        "url": "https://raw.githubusercontent.com/JetBrains/JetBrainsMono/master/fonts/webfonts/JetBrainsMono-Regular.woff2",
        "type": "woff2",
    },
    "jetbrains-mono-bold.woff2": {
        "url": "https://raw.githubusercontent.com/JetBrains/JetBrainsMono/master/fonts/webfonts/JetBrainsMono-Bold.woff2",
        "type": "woff2",
    },
}

# 展示字体覆盖的元素（与 main.cpp 里 font-family:var(--display) 的选择器一一对应）
DISPLAY_CLASSES = ["landing-title", "landing-sub", "hero-name", "section-title", "quote"]

def inner_text(fragment):
    return re.sub(r"<[^>]+>", "", fragment)

def is_cjk(ch):
    o = ord(ch)
    return (0x4E00 <= o <= 0x9FFF) or (0x3400 <= o <= 0x4DBF) or (0xF900 <= o <= 0xFAFF) \
        or (0x3000 <= o <= 0x303F) or (0xFF00 <= o <= 0xFFEF) or (0x2018 <= o <= 0x201D) \
        or ch in "—…·"

def main():
    html = io.open(HTML, encoding="utf-8").read()

    display_text = ""
    for cls in DISPLAY_CLASSES:
        for m in re.finditer(r'class="[^"]*\b' + cls + r'\b[^"]*"[^>]*>(.*?)</', html, re.S):
            display_text += inner_text(m.group(1))
    display_chars = sorted(set(c for c in display_text if c.strip()))

    body = re.sub(r"<script.*?</script>", "", html, flags=re.S)
    body = re.sub(r"<style.*?</style>", "", body, flags=re.S)
    visible = inner_text(body)
    # 等宽语境只可能渲染非 CJK 字符（CJK 会按字体链回退），所以等宽子集只收这些
    mono_chars = sorted(set(c for c in visible if not is_cjk(c) and c.strip()))
    for extra in "+-*/=<>#$%&@_^~|\\()[]{};:,.?!'\"`":
        if extra not in mono_chars:
            mono_chars.append(extra)
    mono_chars = sorted(set(mono_chars))

    io.open(os.path.join(OUT, "display-chars.txt"), "w", encoding="utf-8").write("".join(display_chars))
    io.open(os.path.join(OUT, "mono-chars.txt"), "w", encoding="utf-8").write("".join(mono_chars))
    print("display %d chars, mono %d chars" % (len(display_chars), len(mono_chars)))

    for name, info in SRC.items():
        src = os.path.join(WORK, os.path.basename(info["url"]))
        if not os.path.exists(src):
            print("downloading " + info["url"])
            subprocess.check_call(["curl", "-sL", "--retry", "4", "-o", src, info["url"]])
        charset = "display-chars.txt" if "wenkai" in name else "mono-chars.txt"
        cmd = [sys.executable, "-m", "fontTools.subset", src,
               "--text-file=" + os.path.join(OUT, charset),
               "--flavor=woff2", "--no-hinting",
               "--output-file=" + os.path.join(OUT, name)]
        print("subset " + name)
        subprocess.check_call(cmd)
        print("  -> %s (%d B)" % (name, os.path.getsize(os.path.join(OUT, name))))

    print("done. 记得 git add assets/fonts/ 后重新编译 main.cpp 并提交。")

if __name__ == "__main__":
    main()
