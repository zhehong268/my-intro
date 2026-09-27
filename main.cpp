#include <iostream>
#include <fstream>
#include <string>
#include <vector>

struct Profile {
    std::string name;
    std::string title;
    std::string bio;
    std::string avatar;
    std::string coverImage;
    std::string bgImage;
    std::string bgMusic;
    std::string welcomeText;
    std::vector<std::string> skills;
    std::string email;
    std::string github;
};

std::string generateSkillTags(const std::vector<std::string>& skills) {
    std::string tags;
    for (const auto& skill : skills) {
        tags += "<span class=\"skill-tag\">" + skill + "</span>";
    }
    return tags;
}

int main() {
    Profile me;
    me.name = "遮鸿";
    me.title = "C++ 开发者";
    me.bio = "热爱系统底层开发，熟悉 C++17/20、Linux 系统编程。";
    me.avatar = "avatar.jpg";
    me.coverImage = "cover.jpg";
    me.bgImage = "bg.webp";
    me.bgMusic = "music.mp3";
    me.welcomeText = "欢迎来到遮鸿的个人主页";
    me.skills = {"C++", "Python"};
    me.email = "3255484226@qq.com";
    me.github = "https://github.com/zhehong268";

    std::ofstream html("index.html");
    if (!html.is_open()) {
        std::cerr << "无法创建 index.html" << std::endl;
        return 1;
    }

    html << R"HTML(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)HTML" << me.name << R"HTML( - 自我介绍</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        html, body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
            background: #0f0c29;
            color: #fff;
        }

        /* ========== 首页封面 ========== */
        .landing {
            position: fixed;
            inset: 0;
            z-index: 100;
            display: flex;
            justify-content: center;
            align-items: center;
            background: #000;
            transition: opacity 0.8s ease, visibility 0.8s ease;
        }
        .landing.hidden { opacity: 0; visibility: hidden; pointer-events: none; }
        .landing-bg {
            position: absolute;
            inset: 0;
            background-size: cover;
            background-position: center;
            filter: brightness(0.5);
            animation: slowZoom 20s ease-in-out infinite alternate;
        }
        @keyframes slowZoom {
            from { transform: scale(1.05); }
            to { transform: scale(1.15); }
        }
        .landing-overlay {
            position: absolute;
            inset: 0;
            background: linear-gradient(180deg, rgba(15,12,41,0.4) 0%, rgba(15,12,41,0.85) 100%);
        }
        .landing-content {
            position: relative;
            z-index: 2;
            text-align: center;
            animation: fadeIn 1.2s ease-out;
        }
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }
        .landing-content h1 {
            font-size: 42px;
            font-weight: 700;
            letter-spacing: 3px;
            margin-bottom: 40px;
            background: linear-gradient(135deg, #fff, #a78bfa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        .enter-btn {
            padding: 16px 48px;
            font-size: 16px;
            font-weight: 600;
            letter-spacing: 2px;
            color: #fff;
            background: rgba(167, 139, 250, 0.2);
            border: 2px solid rgba(167, 139, 250, 0.6);
            border-radius: 50px;
            cursor: pointer;
            backdrop-filter: blur(10px);
            transition: all 0.3s ease;
        }
        .enter-btn:hover {
            background: rgba(167, 139, 250, 0.4);
            transform: translateY(-3px);
            box-shadow: 0 10px 30px rgba(167, 139, 250, 0.4);
        }

        /* ========== 主页面 ========== */
        .main-page {
            min-height: 100vh;
            padding: 60px 20px;
            background-image:
                linear-gradient(180deg,
                    rgba(125,211,252,0.65) 0%,
                    rgba(56,189,248,0.3) 25%,
                    rgba(15,12,41,0.85) 65%,
                    rgba(15,12,41,0.98) 100%),
                url(')HTML" << me.bgImage << R"HTML(');
            background-size: cover, cover;
            background-position: center, center;
            background-attachment: fixed, fixed;
            opacity: 0;
            transition: opacity 0.8s ease;
        }
        .main-page.visible { opacity: 1; }

        /* 卡片 */
        .card {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 24px;
            padding: 50px 40px;
            max-width: 560px;
            width: 100%;
            margin: 0 auto 60px auto;
            box-shadow: 0 25px 50px rgba(0, 0, 0, 0.5);
            text-align: center;
            animation: fadeInUp 0.8s ease-out;
        }
        @keyframes fadeInUp {
            from { opacity: 0; transform: translateY(30px); }
            to { opacity: 1; transform: translateY(0); }
        }

        /* 头像 */
        .avatar { margin-bottom: 20px; animation: float 3s ease-in-out infinite; }
        .avatar img {
            width: 120px;
            height: 120px;
            border-radius: 50%;
            object-fit: cover;
            border: 3px solid rgba(167, 139, 250, 0.5);
            box-shadow: 0 8px 25px rgba(167, 139, 250, 0.3);
            cursor: pointer;
        }
        .avatar img.pop {
            animation: avatarPop 0.7s cubic-bezier(0.34, 1.56, 0.64, 1);
        }
        @keyframes avatarPop {
            0%   { transform: scale(1) rotate(0); }
            20%  { transform: scale(1.3) rotate(-8deg); }
            40%  { transform: scale(0.9) rotate(6deg); }
            60%  { transform: scale(1.15) rotate(-4deg); }
            80%  { transform: scale(0.98) rotate(2deg); }
            100% { transform: scale(1) rotate(0); }
        }
        @keyframes float {
            0%, 100% { transform: translateY(0); }
            50% { transform: translateY(-10px); }
        }

        /* 标题 */
        .card h1 {
            font-size: 32px;
            font-weight: 700;
            background: linear-gradient(135deg, #a78bfa, #60a5fa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            margin-bottom: 8px;
        }
        .card .title {
            font-size: 16px;
            color: #a78bfa;
            font-weight: 500;
            letter-spacing: 1px;
            margin-bottom: 16px;
        }

        /* 时钟 */
        .clock {
            font-family: 'Courier New', 'Consolas', monospace;
            font-size: 22px;
            font-weight: 700;
            color: #7dd3fc;
            letter-spacing: 8px;
            margin-bottom: 24px;
            text-shadow: 0 0 18px rgba(125, 211, 252, 0.8);
        }

        .card .bio {
            font-size: 15px;
            color: rgba(255, 255, 255, 0.7);
            line-height: 1.8;
            margin-bottom: 30px;
        }

        /* 技能标签 */
        .skills {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 12px;
            margin-bottom: 20px;
        }
        .skill-tag {
            background: rgba(167, 139, 250, 0.15);
            color: #c4b5fd;
            padding: 10px 22px;
            border-radius: 22px;
            font-size: 14px;
            font-weight: 500;
            border: 1px solid rgba(167, 139, 250, 0.3);
            transition: all 0.3s ease;
        }
        .skill-tag:hover {
            background: rgba(167, 139, 250, 0.3);
            transform: translateY(-3px);
            box-shadow: 0 8px 20px rgba(167, 139, 250, 0.3);
        }

        /* 联系方式 */
        .contact {
            display: flex;
            justify-content: center;
            gap: 16px;
            flex-wrap: wrap;
            font-size: 14px;
            margin-bottom: 20px;
        }
        .contact a {
            color: #60a5fa;
            text-decoration: none;
            padding: 10px 22px;
            border-radius: 12px;
            background: rgba(96, 165, 250, 0.1);
            border: 1px solid rgba(96, 165, 250, 0.2);
            transition: all 0.3s ease;
        }
        .contact a:hover {
            background: rgba(96, 165, 250, 0.2);
            transform: translateY(-2px);
        }
        .footer {
            margin-top: 30px;
            font-size: 12px;
            color: rgba(255, 255, 255, 0.3);
        }

        /* 音乐开关 */
        .music-toggle {
            position: fixed;
            top: 20px;
            right: 20px;
            z-index: 200;
            width: 44px;
            height: 44px;
            border-radius: 50%;
            background: rgba(167, 139, 250, 0.2);
            border: 1px solid rgba(167, 139, 250, 0.5);
            color: #c4b5fd;
            font-size: 20px;
            cursor: pointer;
            backdrop-filter: blur(10px);
            display: none;
            align-items: center;
            justify-content: center;
            transition: all 0.3s ease;
        }
        .music-toggle.visible { display: flex; }
        .music-toggle:hover {
            background: rgba(167, 139, 250, 0.4);
            transform: scale(1.1);
        }
        .music-toggle.paused { color: #666; border-color: #666; }

        /* 区块 */
        .section {
            max-width: 1000px;
            margin: 0 auto;
            padding: 40px 20px;
        }
        .section-title {
            font-size: 28px;
            font-weight: 700;
            text-align: center;
            margin-bottom: 40px;
            background: linear-gradient(135deg, #a78bfa, #60a5fa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }

        /* 项目卡片 */
        .projects-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
            gap: 24px;
        }
        .project-card {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 16px;
            padding: 28px;
            transition: all 0.3s ease;
        }
        .project-card:hover {
            transform: translateY(-6px);
            border-color: rgba(167, 139, 250, 0.4);
            box-shadow: 0 15px 40px rgba(167, 139, 250, 0.2);
        }
        .project-card h3 { font-size: 18px; margin-bottom: 10px; }
        .project-card p {
            font-size: 14px;
            color: rgba(255, 255, 255, 0.6);
            line-height: 1.7;
            margin-bottom: 14px;
        }
        .project-tech {
            font-size: 12px;
            color: #a78bfa;
            margin-bottom: 14px;
            padding-bottom: 14px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }
        .project-card a {
            color: #60a5fa;
            text-decoration: none;
            font-size: 14px;
            font-weight: 500;
        }

        /* 时间线 */
        .timeline { position: relative; max-width: 720px; margin: 0 auto; }
        .timeline::before {
            content: '';
            position: absolute;
            left: 0; top: 0; bottom: 0;
            width: 2px;
            background: linear-gradient(180deg, #a78bfa, #60a5fa);
        }
        .timeline-item {
            position: relative;
            padding-left: 36px;
            margin-bottom: 36px;
        }
        .timeline-item::before {
            content: '';
            position: absolute;
            left: -6px; top: 8px;
            width: 14px; height: 14px;
            border-radius: 50%;
            background: #a78bfa;
            box-shadow: 0 0 0 4px rgba(167, 139, 250, 0.2);
        }
        .timeline-period {
            font-size: 13px;
            color: #a78bfa;
            font-weight: 600;
            margin-bottom: 6px;
        }
        .timeline-content h3 { font-size: 18px; margin-bottom: 4px; }
        .timeline-company {
            font-size: 14px;
            color: rgba(255, 255, 255, 0.5);
            margin-bottom: 10px;
        }
        .timeline-content p {
            font-size: 15px;
            color: rgba(255, 255, 255, 0.7);
            line-height: 1.7;
        }

        /* ========== 音游区域 ========== */
        .rhythm-container {
            position: relative;
            max-width: 560px;
            margin: 0 auto;
            height: 520px;
            background: linear-gradient(180deg, rgba(10,14,26,0.9), rgba(10,14,26,0.5));
            border: 1px solid rgba(125, 211, 252, 0.3);
            border-radius: 16px;
            overflow: hidden;
            box-shadow: 0 0 50px rgba(125, 211, 252, 0.15) inset,
                        0 10px 40px rgba(0, 0, 0, 0.5);
        }
        .lanes {
            display: flex;
            height: 100%;
            position: relative;
        }
        .lane {
            flex: 1;
            border-right: 1px solid rgba(125, 211, 252, 0.08);
            position: relative;
            overflow: hidden;
        }
        .lane:last-child { border-right: none; }

        /* 判定线 */
        .judge-line {
            position: absolute;
            bottom: 100px;
            left: 0;
            right: 0;
            height: 3px;
            background: #7dd3fc;
            box-shadow: 0 0 25px rgba(125, 211, 252, 1),
                        0 0 50px rgba(125, 211, 252, 0.6);
            z-index: 5;
        }
        .judge-line.pulse {
            animation: judgePulse 0.3s ease;
        }
        @keyframes judgePulse {
            0% { box-shadow: 0 0 25px rgba(125,211,252,1), 0 0 50px rgba(125,211,252,0.6); }
            50% { box-shadow: 0 0 50px rgba(255,255,255,1), 0 0 100px rgba(125,211,252,1); }
            100% { box-shadow: 0 0 25px rgba(125,211,252,1), 0 0 50px rgba(125,211,252,0.6); }
        }

        /* COMBO 显示 */
        .combo-display {
            position: absolute;
            top: 30px;
            left: 50%;
            transform: translateX(-50%);
            text-align: center;
            z-index: 10;
            pointer-events: none;
        }
        .combo-count {
            font-size: 64px;
            font-weight: 800;
            color: #7dd3fc;
            line-height: 1;
            text-shadow: 0 0 40px rgba(125, 211, 252, 1),
                         0 0 80px rgba(125, 211, 252, 0.6);
            font-family: 'Courier New', monospace;
        }
        .combo-count.pop {
            animation: comboPop 0.3s ease;
        }
        @keyframes comboPop {
            0% { transform: scale(1); }
            50% { transform: scale(1.35); color: #fff; }
            100% { transform: scale(1); }
        }
        .combo-label {
            font-size: 13px;
            color: rgba(125, 211, 252, 0.7);
            letter-spacing: 8px;
            margin-top: 8px;
            font-weight: 700;
        }

        /* 音符 */
        .note {
            position: absolute;
            width: 70%;
            left: 15%;
            height: 16px;
            background: linear-gradient(180deg, #bae6fd, #7dd3fc);
            border-radius: 8px;
            box-shadow: 0 0 18px rgba(125, 211, 252, 0.9),
                        0 0 35px rgba(125, 211, 252, 0.5);
            z-index: 3;
        }
        .note.hit {
            background: #fff;
            box-shadow: 0 0 30px rgba(255, 255, 255, 1),
                        0 0 60px rgba(125, 211, 252, 1);
        }

        /* ========== 签名加载 ========== */
        .loader {
            position: fixed;
            inset: 0;
            z-index: 300;
            background: #0a0e1a;
            display: none;
            justify-content: center;
            align-items: center;
            opacity: 0;
            transition: opacity 0.6s ease;
        }
        .loader.show { display: flex; opacity: 1; }

        .signature-svg {
            width: 90%;
            max-width: 560px;
            height: auto;
            overflow: visible;
        }
        #sigText {
            animation: drawSignature 3.5s cubic-bezier(0.45, 0, 0.3, 1) forwards;
            filter: drop-shadow(0 0 12px rgba(125, 211, 252, 0.8));
        }
        @keyframes drawSignature {
            to { stroke-dashoffset: 0; }
        }

        /* 响应式 */
        @media (max-width: 600px) {
            .landing-content h1 { font-size: 32px; }
            .card h1 { font-size: 26px; }
            .section-title { font-size: 22px; }
            .clock { font-size: 18px; letter-spacing: 4px; }
            .combo-count { font-size: 48px; }
            .rhythm-container { height: 420px; }
        }
    </style>
</head>
<body>

    <!-- 首页封面 -->
    <div class="landing" id="landing">
        <div class="landing-bg" style="background-image: url(')HTML" << me.coverImage << R"HTML(');"></div>
        <div class="landing-overlay"></div>
        <div class="landing-content">
            <h1>)HTML" << me.welcomeText << R"HTML(</h1>
            <button class="enter-btn" onclick="enterSite();">点击进入</button>
        </div>
    </div>

    <!-- 签名加载 -->
    <div class="loader" id="loader">
        <svg class="signature-svg" viewBox="0 0 500 140" xmlns="http://www.w3.org/2000/svg">
            <text x="250" y="100" text-anchor="middle"
                  font-family="'Brush Script MT', 'Segoe Script', 'Apple Chancery', cursive"
                  font-size="72" font-weight="bold"
                  fill="none" stroke="#7dd3fc" stroke-width="2"
                  stroke-linecap="round" stroke-linejoin="round"
                  stroke-dasharray="3000" stroke-dashoffset="3000"
                  id="sigText">It's MyGO!!!!!</text>
        </svg>
    </div>

    <!-- 音乐开关 -->
    <button class="music-toggle" id="musicBtn" onclick="toggleMusic();" title="暂停/播放音乐">&#9834;</button>

    <!-- 主页面 -->
    <div class="main-page" id="mainPage">
        <div class="card">
            <div class="avatar"><img src=")HTML" << me.avatar << R"HTML(" alt="头像" onclick="popAvatar(this);"></div>
            <h1>)HTML" << me.name << R"HTML(</h1>
            <div class="title">)HTML" << me.title << R"HTML(</div>
            <div class="clock" id="clock">00:00:00</div>
            <p class="bio">)HTML" << me.bio << R"HTML(</p>
            <div class="skills">)HTML" << generateSkillTags(me.skills) << R"HTML(</div>
            <div class="contact">
                <a href="mailto:)HTML" << me.email << R"HTML(">📧 邮箱</a>
                <a href=")HTML" << me.github << R"HTML(" target="_blank">🐙 GitHub</a>
            </div>
            <div class="footer">Generated by C++ · Powered by GitHub Pages</div>
        </div>

        <section class="section">
            <h2 class="section-title">Projects</h2>
            <div class="projects-grid">
                <div class="project-card">
                    <h3>WASM Intro Site</h3>
                    <p>用 C++ 生成并部署到 GitHub Pages 的个人主页。</p>
                    <div class="project-tech">C++ / GitHub Actions</div>
                    <a href="https://github.com/zhehong268/my-intro" target="_blank">查看详情</a>
                </div>
                <div class="project-card">
                    <h3>Project Alpha</h3>
                    <p>使用现代 C++ 编写的高性能数据处理引擎。（并非）</p>
                    <div class="project-tech">C++17</div>
                    <a href="#" target="_blank">查看详情</a>
                </div>
            </div>
        </section>

        <section class="section">
            <h2 class="section-title">Experience</h2>
            <div class="timeline">
                <div class="timeline-item">
                    <div class="timeline-period">2025-至今</div>
                    <div class="timeline-content">
                        <h3>BanGDream高级工程师</h3>
                        <div class="timeline-company">沈阳理工大学</div>
                        <p>游玩并严肃观看邦邦所有内容，最喜欢高松灯（不是凑企鹅）</p>
                    </div>
                </div>
                <div class="timeline-item">
                    <div class="timeline-period">2023 - 2026</div>
                    <div class="timeline-content">
                        <h3>苦逼高中生</h3>
                        <div class="timeline-company">成都某不知名高中</div>
                        <p>天天打游戏，上课就睡觉</p>
                    </div>
                </div>
                <div class="timeline-item">
                    <div class="timeline-period">2014 - 2023</div>
                    <div class="timeline-content">
                        <h3>小初生</h3>
                        <div class="timeline-company">成都某不知名学校</div>
                        <p>搞忘天天干啥了</p>
                    </div>
                </div>
            </div>
        </section>

        <section class="section">
            <h2 class="section-title">Rhythm</h2>
            <div class="rhythm-container" id="rhythmContainer">
                <div class="combo-display">
                    <div class="combo-count" id="comboCount">0</div>
                    <div class="combo-label">COMBO</div>
                </div>
                <div class="judge-line" id="judgeLine"></div>
                <div class="lanes" id="lanes">
                    <div class="lane"></div>
                    <div class="lane"></div>
                    <div class="lane"></div>
                    <div class="lane"></div>
                    <div class="lane"></div>
                    <div class="lane"></div>
                    <div class="lane"></div>
                </div>
            </div>
        </section>

        <section class="section section-contact">
            <h2 class="section-title">联系我</h2>
            <div class="contact">
                <a href="mailto:)HTML" << me.email << R"HTML(">📧 邮箱</a>
                <a href=")HTML" << me.github << R"HTML(" target="_blank">🐙 GitHub</a>
            </div>
            <div class="footer">Generated by C++ · Powered by GitHub Pages</div>
        </section>
    </div>

    <audio id="bgMusic" loop>
        <source src=")HTML" << me.bgMusic << R"HTML(" type="audio/mpeg">
    </audio>

    <script>
        /* ============ 时钟 ============ */
        function updateClock() {
            var now = new Date();
            var h = String(now.getHours()).padStart(2, '0');
            var m = String(now.getMinutes()).padStart(2, '0');
            var s = String(now.getSeconds()).padStart(2, '0');
            document.getElementById('clock').textContent = h + ':' + m + ':' + s;
        }
        setInterval(updateClock, 1000);
        updateClock();

        /* ============ 进入站点 ============ */
        function enterSite() {
            var loader = document.getElementById('loader');
            loader.classList.add('show');

            setTimeout(function() {
                document.getElementById('landing').classList.add('hidden');
                document.getElementById('mainPage').classList.add('visible');
                document.body.style.overflowY = 'auto';

                var music = document.getElementById('bgMusic');
                var btn = document.getElementById('musicBtn');
                btn.classList.add('visible');

                music.play().then(function() {
                    btn.classList.remove('paused');
                }).catch(function(err) {
                    console.log('音乐播放失败:', err);
                    btn.classList.add('paused');
                });

                setTimeout(function() {
                    loader.classList.remove('show');
                    setTimeout(function() {
                        loader.style.display = 'none';
                    }, 600);
                }, 300);
            }, 3500);
        }

        /* ============ 音乐开关 ============ */
        function toggleMusic() {
            var music = document.getElementById('bgMusic');
            var btn = document.getElementById('musicBtn');
            if (music.paused) {
                music.play();
                btn.classList.remove('paused');
            } else {
                music.pause();
                btn.classList.add('paused');
            }
        }

        /* ============ 头像弹跳 ============ */
        function popAvatar(el) {
            el.classList.remove('pop');
            void el.offsetWidth;
            el.classList.add('pop');
        }

        /* ============ 音游自动打 ============ */
        // 改 BPM 可以调节奏快慢
        // 春日影大约 168 BPM
        var BPM = 168;
        var beatInterval = 60000 / BPM;    // 一拍多少毫秒，168 BPM 约 357ms
        var noteTravelTime = 1800;          // 音符从顶部落到判定线需要多久
        var rhythmStarted = false;
        var combo = 0;

        function startRhythm() {
            if (rhythmStarted) return;
            rhythmStarted = true;

            var lanes = document.querySelectorAll('#lanes .lane');
            var comboEl = document.getElementById('comboCount');
            var judgeLine = document.getElementById('judgeLine');

            function spawnNote() {
                // 每一拍随机挑一条轨道
                var laneIndex = Math.floor(Math.random() * lanes.length);
                var lane = lanes[laneIndex];

                var note = document.createElement('div');
                note.className = 'note';
                lane.appendChild(note);

                var laneHeight = lane.clientHeight;
                var targetTop = laneHeight - 100 - 8;   // 判定线位置 - 半个音符高

                // 用 Web Animations API 做匀速下落
                note.animate(
                    [
                        { top: '-20px', opacity: 1 },
                        { top: targetTop + 'px', opacity: 1 }
                    ],
                    { duration: noteTravelTime, easing: 'linear', fill: 'forwards' }
                );

                // 到判定线时击打
                setTimeout(function() {
                    note.classList.add('hit');
                    note.animate(
                        [
                            { opacity: 1, transform: 'scale(1)' },
                            { opacity: 0, transform: 'scale(2.5)' }
                        ],
                        { duration: 280, fill: 'forwards' }
                    );
                    setTimeout(function() { note.remove(); }, 280);

                    // 判定线闪一下
                    judgeLine.classList.remove('pulse');
                    void judgeLine.offsetWidth;
                    judgeLine.classList.add('pulse');

                    // COMBO 累加
                    combo++;
                    comboEl.textContent = combo;
                    comboEl.classList.remove('pop');
                    void comboEl.offsetWidth;
                    comboEl.classList.add('pop');
                }, noteTravelTime);
            }

            setInterval(spawnNote, beatInterval);
        }

        // 进入主界面 1 秒后自动开始
        var rhythmWatcher = setInterval(function() {
            if (document.getElementById('mainPage').classList.contains('visible')) {
                clearInterval(rhythmWatcher);
                setTimeout(startRhythm, 1000);
            }
        }, 200);
    </script>
</body>
</html>)HTML";

    html.close();
    std::cout << "index.html 生成成功！" << std::endl;
    return 0;
}