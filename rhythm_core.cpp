/*
 *  rhythm_core.cpp — 音游谱面内核的 WebAssembly 版本
 *  ------------------------------------------------------------------
 *  构建（本地或 CI）：
 *    emcc rhythm_core.cpp -O3 -o rhythm_core.wasm --no-entry \
 *         -s STANDALONE_WASM=1 -s ERROR_ON_UNDEFINED_SYMBOLS=1
 *
 *  产物只有几百字节，由生成器（main.cpp）读入并 base64 内联进 index.html，
 *  浏览器里用 WebAssembly.instantiate 实例化，导出的 chart_next() 直接
 *  决定音游每个音符的轨道。导出的只有下面这几个函数，没有胶水 JS。
 */
#include "rhythm_core.hpp"

namespace {
rhythm::Rng g_rng(rhythm::kSeed);
int         g_last  = -1;
int         g_lanes = rhythm::kLanes;
}  // namespace

extern "C" {

/* 内核版本：页面拿它做兼容性检查（2 = 加入判定与算分导出） */
__attribute__((export_name("kernel_version"))) int kernel_version(void) {
    return 2;
}

/* 重置谱面状态：种子与轨道数由 JS 传入（与生成器使用的值相同） */
__attribute__((export_name("chart_reset"))) void chart_reset(unsigned seed, int lanes) {
    g_rng   = rhythm::Rng(seed);
    g_last  = -1;
    g_lanes = (lanes > 0 && lanes <= 64) ? lanes : rhythm::kLanes;
}

/* 取下一个音符的轨道 */
__attribute__((export_name("chart_next"))) int chart_next(void) {
    int l = rhythm::nextLane(g_rng, g_lanes, g_last);
    g_last = l;
    return l;
}

/* 取第 index 个音符的轨道（重放式，供 JS 侧自检比对） */
__attribute__((export_name("chart_at"))) int chart_at(int index, unsigned seed, int lanes) {
    return rhythm::noteAt(index, lanes, seed);
}

/* 打击判定：入参是与音符时刻的毫秒差，出参 0=MISS / 1=GOOD / 2=PERFECT */
__attribute__((export_name("judge_ms"))) int judge_ms(int deltaMs) {
    return rhythm::judge(deltaMs);
}

/* 准确率千分比：PERFECT 记 1.0、GOOD 记 0.6 */
__attribute__((export_name("accuracy_permille"))) int accuracy_permille(int perfect, int good, int total) {
    return rhythm::accuracyPermille(perfect, good, total);
}

}  // extern "C"
