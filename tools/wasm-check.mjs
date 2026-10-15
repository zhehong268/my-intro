/* wasm-check.mjs —— 校验 rhythm_core.wasm 的对外契约
   在 CI 里跑，保证「编出来的 wasm 真的导出了这些函数，且算法结果与生成器一致」。
   用法：node tools/wasm-check.mjs [--wasm=rhythm_core.wasm] */
import fs from "node:fs";

const args = process.argv.slice(2);
const arg = (k, d) => { const a = args.find((x) => x.startsWith("--" + k + "=")); return a ? a.slice(k.length + 3) : d; };
const WASM = arg("wasm", "rhythm_core.wasm");

let bad = 0;
const check = (name, ok, extra = "") => {
  console.log((ok ? "PASS  " : "FAIL  ") + name + (extra ? "  — " + extra : ""));
  if (!ok) bad = 1;
};

const bytes = fs.readFileSync(WASM);
const mod = await WebAssembly.compile(bytes);
const imports = WebAssembly.Module.imports(mod);
check("wasm 不需要任何 import", imports.length === 0, JSON.stringify(imports.map((i) => i.module + "." + i.name)));

const inst = await WebAssembly.instantiate(mod, {});
const e = inst.exports;
if (typeof e._initialize === "function") e._initialize(); // standalone wasm 的初始化（若存在）
const want = ["kernel_version", "chart_reset", "chart_next", "chart_at", "judge_ms", "accuracy_permille",
  "onset_init", "onset_capacity", "onset_max", "onset_ptr", "onset_out_ptr", "detect_onsets", "onset_selftest"];
check("导出函数齐全", want.every((n) => typeof e[n] === "function"), want.filter((n) => typeof e[n] !== "function").join(",") || "全部存在");

const SEED = 0x5eed2026, LANES = 7;
check("kernel_version()=3", e.kernel_version() === 3, String(e.kernel_version()));

/* 谱面前 12 个轨道（与生成器内联的那份必须一致，页面也会比这 12 个） */
e.chart_reset(SEED, LANES);
const seq = [];
for (let i = 0; i < 12; i++) seq.push(e.chart_next());
const expect = "105126514252".split("").map(Number);
check("谱面前 12 个轨道", JSON.stringify(seq) === JSON.stringify(expect), seq.join(""));
check("chart_at 与 chart_next 一致", expect.every((v, i) => e.chart_at(i, SEED, LANES) === v));

/* 判定：探针必须落在窗口边界上（含 ±1ms） */
const probes = [-200, -91, -90, -89, -46, -45, -44, 0, 44, 45, 46, 89, 90, 91, 200];
const wantJudge = probes.map((d) => { const a = Math.abs(d); return a <= 45 ? 2 : a <= 90 ? 1 : 0; });
const gotJudge = probes.map((d) => e.judge_ms(d));
check("judge_ms 边界与窗口一致", JSON.stringify(gotJudge) === JSON.stringify(wantJudge), gotJudge.join(""));

/* 准确率：PERFECT 记 1.0、GOOD 记 0.6，导出给的是千分比 */
const accCases = [[0, 0, 0, 0], [64, 0, 64, 1000], [0, 64, 64, 600], [32, 32, 64, 800], [1, 0, 3, 333], [8, 0, 9, 888]];
const accGot = accCases.map(([p, g, n]) => e.accuracy_permille(p, g, n));
check("accuracy_permille 与整数权重一致", JSON.stringify(accGot) === JSON.stringify(accCases.map((c) => c[3])), accGot.join(","));

/* v3：浏览器内的起音点检测。合成信号的自检值必须与 native 生成器内联进页面的那个一致，
   否则「页面里量的」和「仓库里量好的」就不是同一套算法。 */
const mem = e.memory instanceof WebAssembly.Memory ? e.memory : null;
check("导出了 memory（页面要往里写 PCM）", !!mem);
if (mem) {
  e.onset_init();
  check("onset_capacity() = 20 秒分析窗", e.onset_capacity() === 11025 * 20, String(e.onset_capacity()));
  check("onset_max() = 4096 帧上限", e.onset_max() === 4096, String(e.onset_max()));
  const outPtr = e.onset_out_ptr();
  const sum = e.onset_selftest(outPtr) >>> 0;
  const count = new Int32Array(mem.buffer, outPtr, 1)[0];
  check("onset_selftest() 与 native 校验值一致", sum === 1435864337, "checksum=" + sum);
  check("onset_selftest() 数出 23 个起音点", count === 23, "count=" + count);

  /* 再走一遍真实调用路径：8 秒、每 500ms 一击、每击 50ms 衰减音 */
  const rate = 11025, total = rate * 8;
  const pcm = new Float32Array(mem.buffer, e.onset_ptr(), total);
  for (let i = 0; i < total; i++) {
    const t = i / rate, phase = t % 0.5;
    pcm[i] = phase < 0.05 ? Math.sin(2 * Math.PI * 880 * t) * 0.8 * (1 - phase / 0.05) : 0;
  }
  const got = e.detect_onsets(total, rate);
  const ms = new Float32Array(mem.buffer, outPtr, got);
  check("detect_onsets() 在 500ms 一击的合成信号上量出 14~17 个起音点", got >= 14 && got <= 17,
    "n=" + got + (got ? " 首个 " + ms[0].toFixed(0) + "ms" : ""));
}

console.log(bad ? "wasm check FAILED" : "wasm check OK - " + bytes.length + " 字节");
process.exit(bad);
