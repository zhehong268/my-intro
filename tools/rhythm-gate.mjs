/* rhythm-gate.mjs —— 音游「谱面时间轴」一致性门
 *
 * 谱面时间轴是生成器从 onsets.txt 读进去、内联到 index.html 里的。
 * 这条门盯的就是这条链路没断：换歌、改了子集、手改 HTML 都能被抓到。
 *
 * 1) onsets.txt 必须是递增的整数毫秒值，条数够多，最后一个点不能超过曲长
 * 2) index.html 里 window.__RHYTHM_ONSETS 的数字必须和 onsets.txt **逐个相等**
 * 3) window.__RHYTHM_SONG_MS 必须大于最后一个起音点（否则谱面会落到歌外面）
 *
 * 用法：node tools/rhythm-gate.mjs [--dir=_site] [--onsets=onsets.txt]
 *       --dir 指生成物所在目录（CI 里是 _site），--onsets 指实测起音点文件（仓库根目录）
 */
import fs from "node:fs";
import path from "node:path";

const args = process.argv.slice(2);
const DIR = (args.find((a) => a.startsWith("--dir=")) || "").slice(6) || ".";
const ONSETS = (args.find((a) => a.startsWith("--onsets=")) || "").slice(9) || "onsets.txt";

let bad = 0;
const fail = (msg) => { console.log("FAIL  " + msg); bad++; };
const ok = (msg) => console.log("PASS  " + msg);

/* ---- 1) onsets.txt ---- */
const onsetsFile = path.join(ONSETS);
if (!fs.existsSync(onsetsFile)) {
  fail(`找不到 ${onsetsFile} —— 生成器会退回固定节拍，谱面就不再跟着歌走`);
} else {
  const raw = fs.readFileSync(onsetsFile, "utf8").split(/\r?\n/).map((s) => s.trim()).filter(Boolean);
  const nums = raw.map(Number);
  if (!nums.length || nums.some((n) => !Number.isInteger(n) || n < 0)) {
    fail(`${onsetsFile} 里有非整数或负数（前 3 行：${raw.slice(0, 3).join(",")}）`);
  } else if (nums.length < 100) {
    fail(`${onsetsFile} 只有 ${nums.length} 个起音点，太少了（这份音乐实测是 657 个）`);
  } else if (nums.some((n, i) => i > 0 && n <= nums[i - 1])) {
    fail(`${onsetsFile} 不是严格递增的（第一处：${nums.findIndex((n, i) => i > 0 && n <= nums[i - 1])} 行）`);
  } else {
    ok(`${onsetsFile}: ${nums.length} 个递增起音点，${nums[0]}–${nums[nums.length - 1]} ms`);
  }

  /* ---- 2) 内联进 index.html 的那份 ---- */
  const htmlFile = path.join(DIR, "index.html");
  if (!fs.existsSync(htmlFile)) {
    fail(`找不到 ${htmlFile}`);
  } else {
    const html = fs.readFileSync(htmlFile, "utf8");
    const m = html.match(/window\.__RHYTHM_ONSETS\s*=\s*\[([^\]]*)\]/);
    const s = html.match(/window\.__RHYTHM_SONG_MS\s*=\s*(\d+)/);
    if (!m) {
      fail("index.html 里没有 window.__RHYTHM_ONSETS（生成器没内联，或片段名被改了）");
    } else {
      const inlined = m[1].split(",").map((x) => Number(x.trim())).filter((x) => Number.isFinite(x));
      if (inlined.length !== nums.length) {
        fail(`内联 ${inlined.length} 个起音点，${onsetsFile} 有 ${nums.length} 个 —— 八成是改了文件没重新编译生成器`);
      } else {
        const firstDiff = inlined.findIndex((v, i) => v !== nums[i]);
        if (firstDiff >= 0) fail(`第 ${firstDiff + 1} 个起音点对不上：内联 ${inlined[firstDiff]}，文件 ${nums[firstDiff]}`);
        else ok(`index.html 内联的 ${inlined.length} 个起音点与 ${onsetsFile} 逐个相等（首个 ${inlined[0]} ms，末个 ${inlined[inlined.length - 1]} ms）`);
      }
      if (!s) fail("index.html 里没有 window.__RHYTHM_SONG_MS（虚拟时钟没有终点，谱面会越过歌尾）");
      else {
        const songMs = Number(s[1]);
        const last = nums[nums.length - 1];
        if (songMs <= last) fail(`__RHYTHM_SONG_MS=${songMs} 不大于最后一个起音点 ${last} ms`);
        else ok(`__RHYTHM_SONG_MS=${songMs} ms（最后一个起音点 ${last} ms，还余 ${songMs - last} ms）`);
      }
    }
  }
}

console.log(bad ? `rhythm gate FAILED: ${bad} 处不一致` : "rhythm gate OK");
process.exit(bad ? 1 : 0);
