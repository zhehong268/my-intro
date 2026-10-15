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

/* 内核版本：页面拿它做兼容性检查（3 = 加入起音点检测导出） */
__attribute__((export_name("kernel_version"))) int kernel_version(void) {
    return 3;
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

/* ---------- 浏览器里的起音点检测（拖进任意一首歌） ----------
 * 页面把解码+重采样后的单声道 PCM 写进 onset_ptr()，调 detect_onsets()，
 * 再按返回的个数从 onset_out_ptr() 读回毫秒。缓冲区按 20 秒一段复用
 * （onset_capacity），更长的歌由页面自己分块、拼接时去重。
 */
namespace {
rhythm::OnsetScratch g_onsetScratch;
float g_onsetIn[rhythm::kOnsetCapacity];
float g_onsetOut[rhythm::kOnsetMaxFrames];
}  // namespace

/* 窗与旋转因子只算一次，页面加载时调一次即可 */
__attribute__((export_name("onset_init"))) void onset_init(void) {
    rhythm::onsetInit(g_onsetScratch);
}

__attribute__((export_name("onset_capacity"))) int onset_capacity(void) {
    return rhythm::kOnsetCapacity;
}

__attribute__((export_name("onset_max"))) int onset_max(void) {
    return rhythm::kOnsetMaxFrames;
}

__attribute__((export_name("onset_ptr"))) float* onset_ptr(void) {
    return g_onsetIn;
}

__attribute__((export_name("onset_out_ptr"))) float* onset_out_ptr(void) {
    return g_onsetOut;
}

/* 量一段 PCM（n 个采样），采样率由页面告诉我们，返回起音点个数 */
__attribute__((export_name("detect_onsets"))) int detect_onsets(int n, int sampleRate) {
    if (n > rhythm::kOnsetCapacity) n = rhythm::kOnsetCapacity;
    if (n < 0) n = 0;
    return rhythm::detectOnsets(g_onsetIn, n, sampleRate, g_onsetOut, rhythm::kOnsetMaxFrames, g_onsetScratch);
}

/* 合成信号自检：页面拿它与 native 生成器内联的校验值比对 */
__attribute__((export_name("onset_selftest"))) unsigned onset_selftest(int* outCount) {
    return rhythm::onsetSelfTest(g_onsetScratch, outCount);
}

}  // extern "C"
