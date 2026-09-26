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
    me.name = "张兆恒";
    me.title = "C++ 开发者";
    me.bio = "热爱系统底层开发，熟悉 C++17/20、Linux 系统编程。";
    me.avatar = "avatar.jpg";
    me.coverImage = "cover.jpg";
    me.bgMusic = "music.mp3";
    me.welcomeText = "欢迎来到我的主页";
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

        /* 首页封面 */
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

        /* 主页面 */
        .main-page {
            min-height: 100vh;
            padding: 60px 20px;
            background: linear-gradient(135deg, #0f0c29, #302b63, #24243e);
            opacity: 0;
            transition: opacity 0.8s ease;
        }
        .main-page.visible { opacity: 1; }

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
        .avatar { margin-bottom: 20px; animation: float 3s ease-in-out infinite; }
        .avatar img {
            width: 120px;
            height: 120px;
            border-radius: 50%;
            object-fit: cover;
            border: 3px solid rgba(167, 139, 250, 0.5);
            box-shadow: 0 8px 25px rgba(167, 139, 250, 0.3);
        }
        @keyframes float {
            0%, 100% { transform: translateY(0); }
            50% { transform: translateY(-10px); }
        }
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
            margin-bottom: 24px;
        }
        .card .bio {
            font-size: 15px;
            color: rgba(255, 255, 255, 0.7);
            line-height: 1.8;
            margin-bottom: 30px;
        }
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

        /* 内容区块 */
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

        .section-contact {
            text-align: center;
            padding-bottom: 60px;
        }

        @media (max-width: 600px) {
            .landing-content h1 { font-size: 32px; }
            .card h1 { font-size: 26px; }
            .section-title { font-size: 22px; }
        }
    </style>
</head>
<body>

    <div class="landing" id="landing">
        <div class="landing-bg" style="background-image: url(')HTML" << me.coverImage << R"HTML(');"></div>
        <div class="landing-overlay"></div>
        <div class="landing-content">
            <h1>)HTML" << me.welcomeText << R"HTML(</h1>
            <button class="enter-btn" onclick="enterSite();">点击进入</button>
        </div>
    </div>

    <button class="music-toggle" id="musicBtn" onclick="toggleMusic();" title="暂停/播放音乐">&#9834;</button>

    <div class="main-page" id="mainPage">
        <div class="card">
            <div class="avatar"><img src=")HTML" << me.avatar << R"HTML(" alt="头像"></div>
            <h1>)HTML" << me.name << R"HTML(</h1>
            <div class="title">)HTML" << me.title << R"HTML(</div>
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
                    <div class="project-tech">C++17 </div>
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
                        <div class="timeline-company">某科技公司</div>
                        <p>设计高性能计算平台与核心系统模块。</p>
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
        function enterSite() {
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
        }

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
    </script>
</body>
</html>)HTML";

    html.close();
    std::cout << "index.html 生成成功！" << std::endl;
    return 0;
}