/*
 *  rhythm_core.hpp — 音游谱面内核（native 与 WebAssembly 共用同一份算法）
 *  ------------------------------------------------------------------
 *  同一份代码有两个编译目标：
 *    1) 生成器（native，MSVC / g++）：构建时算出一份 256 音符的谱面，
 *       直接内联进 index.html，作为「没有 WebAssembly 时」的兜底；
 *    2) rhythm_core.cpp（Emscripten）：编译成 rhythm_core.wasm，
 *       由生成器读出来、base64 内联进 HTML，在浏览器里运行时生成轨道。
 *
 *  两条路径必须逐字节一致：页面加载后会自己比对前 64 个音符，结果写进
 *  window.__rhythmParity（'ok' / 'mismatch' / 'no-wasm'），不一致会报警。
 *
 *  这里只用 32 位无符号整数运算，所以 native(x86-64) 与 wasm32 结果必然相同。
 */
#pragma once

#include <cmath>

namespace rhythm {

/* 固定参数：与页面上的 LANES / 种子保持一致 */
inline const int      kLanes    = 7;
inline const unsigned kSeed     = 0x5eed2026u;
inline const int      kChartLen = 256;

/* xorshift32：状态只有 4 字节，纯整数运算，跨平台确定 */
struct Rng {
    unsigned s;
    explicit Rng(unsigned seed) : s(seed ? seed : 0x9e3779b9u) {}
    unsigned next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
};

/* 下一个音符落在哪条轨道：与上一条同轨时换轨，避免连续同轨看着像卡住 */
inline int nextLane(Rng& r, int lanes, int last) {
    int l = static_cast<int>(r.next() % static_cast<unsigned>(lanes));
    if (l == last) {
        l = (l + 1 + static_cast<int>(r.next() % static_cast<unsigned>(lanes - 1))) % lanes;
    }
    return l;
}

/* 第 index 个音符的轨道（从头重放，给自检用；index 很小） */
inline int noteAt(int index, int lanes, unsigned seed) {
    Rng r(seed);
    int last = -1, l = 0;
    for (int i = 0; i <= index; ++i) {
        l = nextLane(r, lanes, last);
        last = l;
    }
    return l;
}

/* ---------- 判定与算分 ----------
 * 谱面之外，打击判定与准确率也走这里：页面把按键时刻与音符时刻的差值
 * （毫秒）交给 judge()，把 PERFECT / GOOD / 总数交给 accuracyPermille()。
 * 全部是整数运算，native 与 wasm32 结果必然相同，页面加载时会拿一组
 * 探针逐条比对（见 main.cpp 里的 __RHYTHM_JUDGE_PROBE / _EXPECT）。
 */
inline const int kPerfectMs = 45;  /* |Δt| ≤ 45ms → PERFECT */
inline const int kGoodMs    = 90;  /* |Δt| ≤ 90ms → GOOD，再外面就是 MISS */

/* 判定结果编码：0 = MISS，1 = GOOD，2 = PERFECT */
inline int judge(int deltaMs) {
    const int d = deltaMs < 0 ? -deltaMs : deltaMs;
    if (d <= kPerfectMs) return 2;
    if (d <= kGoodMs)    return 1;
    return 0;
}

/* 准确率千分比：PERFECT 记 1.0、GOOD 记 0.6；先乘后除，避免整数截断偏小 */
inline int accuracyPermille(int perfect, int good, int total) {
    if (total <= 0) return 0;
    const int p = perfect < 0 ? 0 : perfect;
    const int g = good    < 0 ? 0 : good;
    return (p * 1000 + g * 600) / total;
}

/* ---------- 起音点检测 ----------
 * 把一段单声道 PCM 变成「这首歌里真的响过」的时刻表（毫秒，升序）。
 *
 * 两条路径共用这一份代码：
 *   1) native 生成器（构建时）——量 music.mp3，产出仓库里的 onsets.txt，
 *      再把结果内联进页面当默认谱面；
 *   2) rhythm_core.cpp（浏览器里跑的 wasm）——用户在音游段落拖进任意一首歌，
 *      页面当场解码、重采样，交给这同一个函数量出起音点，立刻生成跟这首歌
 *      同步的谱面。
 *
 * 为什么敢说两边一致：主循环里只有 + - * / 与 sqrt（IEEE 对四则与开方是
 * 精确定义的），窗函数与旋转因子这种要调 sin/cos 的东西只在 onsetInit()
 * 里算一次。整数毫秒层面两边必须一致：页面加载时会跑 onsetSelfTest()，
 * 与 native 生成器算出的校验值比对，结果写进 window.__onsetParity。
 */
inline const int kOnsetFft       = 1024;         /* 分析窗长度 */
inline const int kOnsetHopMs     = 23;           /* 帧间隔（毫秒）*/
inline const int kOnsetMergeMs   = 93;           /* 这个窗口内的相邻峰合并成一个 */
inline const int kOnsetBandLo    = 4;            /* 丢掉低频隆隆声（11025Hz 下 ≈ 43Hz）*/
inline const int kOnsetGuard     = 8;            /* 自适应阈值的左右半窗（帧）*/
inline const int kOnsetMaxFrames = 4096;
inline const int kOnsetCapacity  = 11025 * 20;   /* 一次最多吃 20 秒（更长的歌由调用方分块）*/

/* 工作区：调用方自己持有一份（native 放栈上 / wasm 放静态区），函数内部不藏状态 */
struct OnsetScratch {
    float win[kOnsetFft];        /* 汉宁窗 */
    float twCos[kOnsetFft / 2];  /* 旋转因子 */
    float twSin[kOnsetFft / 2];
    float re[kOnsetFft];
    float im[kOnsetFft];
    float mag[kOnsetFft / 2];
    float prev[kOnsetFft / 2];
    float flux[kOnsetMaxFrames];
};

inline void onsetInit(OnsetScratch& s) {
    const double twoPi = 6.283185307179586;
    for (int i = 0; i < kOnsetFft; ++i) {
        s.win[i] = static_cast<float>(0.5 - 0.5 * std::cos(twoPi * i / kOnsetFft));
    }
    for (int i = 0; i < kOnsetFft / 2; ++i) {
        s.twCos[i] = static_cast<float>(std::cos(twoPi * i / kOnsetFft));
        s.twSin[i] = static_cast<float>(std::sin(twoPi * i / kOnsetFft));
    }
}

/* 原地 radix-2 FFT（n = kOnsetFft） */
inline void onsetFft(float* re, float* im, const OnsetScratch& s) {
    const int n = kOnsetFft;
    for (int i = 1, j = 0; i < n; ++i) {          /* 位反转置换 */
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {       /* 蝶形 */
        const int half = len >> 1;
        const int step = n / len;
        for (int i = 0; i < n; i += len) {
            for (int k = 0; k < half; ++k) {
                const int tw = k * step;
                const float wr = s.twCos[tw], wi = -s.twSin[tw];
                const float xr = re[i + k + half] * wr - im[i + k + half] * wi;
                const float xi = re[i + k + half] * wi + im[i + k + half] * wr;
                re[i + k + half] = re[i + k] - xr;
                im[i + k + half] = im[i + k] - xi;
                re[i + k] += xr;
                im[i + k] += xi;
            }
        }
    }
}

/* 返回写进 outMs 的个数（升序毫秒，相对 pcm[0]） */
inline int detectOnsets(const float* pcm, int n, int sampleRate, float* outMs, int maxOut, OnsetScratch& s) {
    if (!pcm || n < kOnsetFft || sampleRate <= 0 || maxOut <= 0) return 0;

    const int hop = (sampleRate * kOnsetHopMs + 500) / 1000;
    if (hop <= 0) return 0;
    int frames = (n - kOnsetFft) / hop + 1;
    if (frames > kOnsetMaxFrames) frames = kOnsetMaxFrames;
    if (frames < 3) return 0;

    const int binLo = kOnsetBandLo;
    const int binHi = kOnsetFft / 2;
    const float invBins = 1.0f / static_cast<float>(binHi - binLo);

    for (int b = 0; b < binHi; ++b) s.prev[b] = 0.0f;

    for (int i = 0; i < frames; ++i) {
        const float* src = pcm + static_cast<long long>(i) * hop;
        for (int k = 0; k < kOnsetFft; ++k) {
            s.re[k] = src[k] * s.win[k];
            s.im[k] = 0.0f;
        }
        onsetFft(s.re, s.im, s);
        float flux = 0.0f;
        for (int b = binLo; b < binHi; ++b) {
            const float m = std::sqrt(s.re[b] * s.re[b] + s.im[b] * s.im[b]);
            s.mag[b] = m;
            const float d = m - s.prev[b];
            if (d > 0.0f) flux += d;
            s.prev[b] = m;
        }
        s.flux[i] = flux * invBins;
    }

    int cnt = 0;
    int lastIdx = -1;
    float lastVal = 0.0f;
    for (int i = 1; i + 1 < frames; ++i) {
        const float cur = s.flux[i];
        if (cur <= 0.0f) continue;
        if (!(cur >= s.flux[i - 1] && cur > s.flux[i + 1])) continue;   /* 局部峰 */
        int lo = i - kOnsetGuard; if (lo < 0) lo = 0;
        int hi = i + kOnsetGuard; if (hi > frames - 1) hi = frames - 1;
        float sum = 0.0f;
        for (int j = lo; j <= hi; ++j) if (j != i) sum += s.flux[j];
        const int span = (hi - lo) > 0 ? (hi - lo) : 1;
        const float mean = sum / static_cast<float>(span);
        if (cur < mean * 1.55f + 1e-7f) continue;                       /* 自适应阈值 */

        const float ms = static_cast<float>(static_cast<double>(i) * hop * 1000.0 / sampleRate);
        if (lastIdx >= 0 && (ms - lastVal) < static_cast<float>(kOnsetMergeMs)) {
            if (cur > s.flux[lastIdx]) {            /* 合并窗口内留更响的那个 */
                lastIdx = i;
                lastVal = ms;
                outMs[cnt - 1] = ms;
            }
            continue;
        }
        if (cnt >= maxOut) break;
        outMs[cnt++] = ms;
        lastIdx = i;
        lastVal = ms;
    }
    return cnt;
}

/* 合成信号自检：500ms 一击的 12 秒点击串，返回位置与个数的校验值。
 * native 生成器把它内联进页面，浏览器里的 wasm 跑同一个函数比对。 */
inline unsigned onsetSelfTest(OnsetScratch& s, int* outCount) {
    const int sr = 11025;
    const int n = sr * 12;
    static float pcm[11025 * 12];
    for (int i = 0; i < n; ++i) pcm[i] = 0.0f;
    for (int t = 0; t < n; t += sr / 2) {           /* 每 500ms 一个脉冲 */
        pcm[t] = 0.9f;
        if (t + 1 < n) pcm[t + 1] = -0.45f;
    }
    unsigned state = 0x1234567u;
    for (int i = 0; i < n; ++i) {                    /* 一点确定性底噪，避免通量恒为 0 */
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        pcm[i] += static_cast<float>(state % 1000u) * 1e-6f;
    }
    float outMs[kOnsetMaxFrames];
    const int cnt = detectOnsets(pcm, n, sr, outMs, kOnsetMaxFrames, s);
    if (outCount) *outCount = cnt;
    unsigned long long h = 2166136261ull;
    for (int i = 0; i < cnt; ++i) {
        h = (h ^ static_cast<unsigned>(outMs[i] + 0.5f)) * 16777619ull;
        h &= 0xffffffffull;
    }
    return static_cast<unsigned>(h ^ static_cast<unsigned>(cnt) * 2654435761u);
}

}  // namespace rhythm
