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

}  // namespace rhythm
