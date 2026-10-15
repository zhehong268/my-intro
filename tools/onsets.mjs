/* onsets.mjs —— 从 music.mp3 里量出「起音点」和真实速度（离线工具，需要本机 ffmpeg）
   1) ffmpeg 解码成 11 kHz 单声道 PCM
   2) 1024 点 FFT，谱通量（只累正增量）当起音强度包络
   3) 自适应阈值 + 局部极大值挑峰，93 ms 内合并（保留更强者）
   4) 用「挑出来的脉冲串」做梳状滤波（comb filter）搜速度，带 log-normal 先验压住倍频歧义
   5) 输出：总起音点数、速度排名、前 N 个起音时间（ms），并可选写成 onsets.txt
   用法：node tools/onsets.mjs [mp3路径] [--write] [--out=onsets.txt] [--max=360000]
   需要 PATH 里有 ffmpeg，或用 FFMPEG=<路径> 指定 */
import fs from "node:fs";
import { spawnSync } from "node:child_process";

const args = process.argv.slice(2);
const MP3 = args.find((a) => !a.startsWith("--")) || "music.mp3";
const WRITE = args.includes("--write");
const MAXMS = Number((args.find((a) => a.startsWith("--max=")) || "").slice(6)) || 360000;
const FF = process.env.FFMPEG || "ffmpeg";
const SR = 11025, N = 1024, HOP = 256, HOPMS = (HOP / SR) * 1000;

const r = spawnSync(FF, ["-v", "error", "-i", MP3, "-ac", "1", "-ar", String(SR), "-f", "s16le", "-"], { maxBuffer: 1 << 30 });
if (r.status !== 0) { console.error("ffmpeg 失败:", String(r.stderr).slice(-400)); process.exit(1); }
const buf = r.stdout, n = buf.length >> 1;
console.log(`解码 ${(n / SR).toFixed(1)}s · ${SR} Hz · hop ${HOPMS.toFixed(2)} ms`);

function fft(re, im) {
  const n = re.length;
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) { const tr = re[i]; re[i] = re[j]; re[j] = tr; const ti = im[i]; im[i] = im[j]; im[j] = ti; }
  }
  for (let len = 2; len <= n; len <<= 1) {
    const ang = -2 * Math.PI / len;
    for (let i = 0; i < n; i += len) {
      for (let k = 0; k < len / 2; k++) {
        const wr = Math.cos(ang * k), wi = Math.sin(ang * k);
        const ur = re[i + k], ui = im[i + k];
        const xr = re[i + k + len / 2], xi = im[i + k + len / 2];
        const vr = xr * wr - xi * wi, vi = xr * wi + xi * wr;
        re[i + k] = ur + vr; im[i + k] = ui + vi;
        re[i + k + len / 2] = ur - vr; im[i + k + len / 2] = ui - vi;
      }
    }
  }
}

const frames = Math.floor((n - N) / HOP);
const flux = new Float64Array(frames);
const prev = new Float64Array(N / 2), re = new Float64Array(N), im = new Float64Array(N), win = new Float64Array(N);
for (let i = 0; i < N; i++) win[i] = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / (N - 1));
for (let f = 0; f < frames; f++) {
  im.fill(0);
  for (let i = 0; i < N; i++) re[i] = (buf.readInt16LE(((f * HOP + i) << 1)) / 32768) * win[i];
  fft(re, im);
  let s = 0;
  for (let k = 1; k < N / 2; k++) {
    const mag = Math.hypot(re[k], im[k]);
    const d = mag - prev[k];
    if (d > 0) s += d;
    prev[k] = mag;
  }
  flux[f] = s;
}
console.log(`帧 ${frames} · ${(frames * HOPMS / 1000).toFixed(1)}s`);

/* 自适应阈值：跟 1.5 s 的滑动均值比，超 1.6 倍才算候选峰 */
const W = Math.round(1500 / HOPMS);
const onsets = [];
const smooth = new Float64Array(frames);
{
  let acc = 0;
  for (let i = 0; i < frames; i++) {
    acc += flux[i];
    if (i >= W) acc -= flux[i - W];
    smooth[i] = acc / Math.min(i + 1, W);
  }
  for (let i = 2; i < frames - 2; i++) {
    const v = flux[i];
    if (v < smooth[i] * 1.6) continue;
    if (v <= flux[i - 1] || v < flux[i + 1] || v <= flux[i - 2] || v < flux[i + 2]) continue;
    onsets.push({ f: i, ms: i * HOPMS, s: v });
  }
}
/* 合并 93 ms 内的相邻峰：留强的 */
const merged = [];
for (const o of onsets) {
  const last = merged[merged.length - 1];
  if (last && o.ms - last.ms < 93) { if (o.s > last.s) merged[merged.length - 1] = o; continue; }
  merged.push(o);
}
const inRange = merged.filter((o) => o.ms <= MAXMS);
console.log(`候选峰 ${onsets.length} · 合并后 ${merged.length} · 前 ${(MAXMS / 1000).toFixed(0)}s 内 ${inRange.length}`);
const gaps = inRange.slice(1).map((o, i) => o.ms - inRange[i].ms).sort((a, b) => a - b);
if (gaps.length) console.log(`起音间隔中位数 ${gaps[gaps.length >> 1].toFixed(0)} ms · 最小 ${gaps[0].toFixed(0)} ms`);

/* 梳状滤波速度估计：把起音当脉冲串，找让脉冲最对齐的周期 */
const train = new Float64Array(frames);
for (const o of merged) train[o.f] = o.s;
const tp = train.reduce((a, b) => a + b, 0) / frames;
for (let i = 0; i < frames; i++) train[i] -= tp;
const cands = [];
for (let bpm = 50; bpm <= 210; bpm += 0.25) {
  const lag = (60 / bpm) / (HOPMS / 1000);
  const li = Math.round(lag);
  if (li < 3) continue;
  let s = 0;
  for (let i = 0; i + li < frames; i++) s += train[i] * train[i + li];
  s /= (frames - li);
  /* log-normal 先验：以 120 BPM 为中心，压住 2 倍/半倍歧义 */
  const prior = Math.exp(-0.5 * Math.pow(Math.log2(bpm / 120) / 0.9, 2));
  cands.push({ bpm, s, score: s * prior });
}
cands.sort((a, b) => b.score - a.score);
console.log("速度候选（带先验）:");
for (const c of cands.slice(0, 6)) console.log(`  ${c.bpm.toFixed(2).padStart(7)} BPM  相关 ${c.s.toExponential(3)}  加权 ${c.score.toExponential(3)}`);
const top = cands[0];
console.log(`结论：${top.bpm.toFixed(1)} BPM（每拍 ${(60000 / top.bpm).toFixed(1)} ms）`);

if (WRITE) {
  const list = inRange.map((o) => Math.round(o.ms));
  const OUT = (args.find((a) => a.startsWith("--out=")) || "").slice(6) || "onsets.txt";
  fs.writeFileSync(OUT, list.join("\n") + "\n");
  console.log(`已写 ${OUT}：${list.length} 行`);
}
