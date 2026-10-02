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
    me.bio = "在学 C++，目前还在打基础，写点小项目练手。平时喜欢看动漫：邦多利、jojo、鬼灭、轻音、咒术、孤独摇滚、闺泣、fate等。";
    me.avatar = "avatar.jpg";
    me.coverImage = "cover.jpg";
    me.bgImage = "bg.webp";
    me.bgMusic = "music.mp3";
    me.welcomeText = "欢迎来到遮鸿的个人主页";
    me.skills = {"C++", "Python", "Git", "Linux"};
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

        /* ========== 点击光波 ========== */
        .click-ripple {
            position: fixed;
            width: 30px; height: 30px;
            border: 2px solid rgba(125, 211, 252, 0.9);
            border-radius: 50%;
            pointer-events: none;
            z-index: 9998;
            transform: translate(-50%, -50%);
            animation: rippleExpand 0.9s cubic-bezier(0.2, 0.6, 0.3, 1) forwards;
        }
        @keyframes rippleExpand {
            to { width: 460px; height: 460px; opacity: 0; border-width: 1px; }
        }

        /* ========== 粒子背景 ========== */
        .particle-canvas {
            position: fixed;
            inset: 0;
            z-index: 1;
            pointer-events: none;
            opacity: 0;
            transition: opacity 1.2s ease;
        }
        .particle-canvas.visible { opacity: 0.6; }

        /* ========== 首页封面 ========== */
        .landing {
            position: fixed; inset: 0; z-index: 100;
            display: flex; justify-content: center; align-items: center;
            background: #000;
            transition: opacity 0.8s ease, visibility 0.8s ease;
        }
        .landing.hidden { opacity: 0; visibility: hidden; pointer-events: none; }
        .landing-bg {
            position: absolute; inset: 0;
            background-size: cover; background-position: center;
            filter: brightness(0.5);
            animation: slowZoom 20s ease-in-out infinite alternate;
        }
        @keyframes slowZoom {
            from { transform: scale(1.05); }
            to { transform: scale(1.15); }
        }
        .landing-overlay {
            position: absolute; inset: 0;
            background: linear-gradient(180deg, rgba(15,12,41,0.4) 0%, rgba(15,12,41,0.85) 100%);
        }
        .landing-content {
            position: relative; z-index: 2;
            text-align: center;
            animation: fadeIn 1.2s ease-out;
        }
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }
        .landing-content h1 {
            font-size: 42px; font-weight: 700; letter-spacing: 3px;
            margin-bottom: 40px;
            background: linear-gradient(135deg, #fff, #a78bfa);
            -webkit-background-clip: text; -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        .enter-btn {
            padding: 16px 48px;
            font-size: 16px; font-weight: 600; letter-spacing: 2px;
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

        /* ========== 签名加载 ========== */
        .loader {
            position: fixed; inset: 0; z-index: 300;
            background: #0a0e1a;
            display: none;
            justify-content: center; align-items: center;
            opacity: 0;
            transition: opacity 0.6s ease;
        }
        .loader.show { display: flex; opacity: 1; }
        .signature-svg { width: 90%; max-width: 560px; height: auto; overflow: visible; }
        #sigText {
            animation: drawSignature 3.5s cubic-bezier(0.45, 0, 0.3, 1) forwards;
            filter: drop-shadow(0 0 12px rgba(125, 211, 252, 0.8));
        }
        @keyframes drawSignature { to { stroke-dashoffset: 0; } }

        /* ========== 主页面 ========== */
        .main-page {
            position: relative;
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
            background-position: center, 50% 50%;
            background-attachment: fixed, fixed;
            opacity: 0;
            transition: opacity 0.8s ease, background-position 0.6s ease-out;
        }
        .main-page.visible { opacity: 1; }
        .main-page .card, .main-page .section { position: relative; z-index: 2; }

        /* ========== 滚动入场（只作用于 section） ========== */
        .section {
            opacity: 0;
            transform: translateY(40px);
            transition: opacity 0.8s ease, transform 0.8s cubic-bezier(0.45, 0, 0.3, 1);
        }
        .section.revealed {
            opacity: 1;
            transform: translateY(0);
        }

        /* ========== 卡片（初始隐藏，通过 JS 加 revealed 触发滑入） ========== */
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
            opacity: 0;
            transform: translateY(40px);
            transition: opacity 0.8s ease, transform 0.8s cubic-bezier(0.45, 0, 0.3, 1);
        }
        .card.revealed {
            opacity: 1;
            transform: translateY(0);
        }

        /* ========== 名字逐字浮现 ========== */
        .name-char {
            display: inline-block;
            opacity: 0;
            transform: translateY(15px) scale(0.7);
            filter: blur(8px);
            animation: charReveal 0.7s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
        }
        @keyframes charReveal {
            to { opacity: 1; transform: translateY(0) scale(1); filter: blur(0); }
        }

        /* 头像 */
        .avatar { margin-bottom: 20px; animation: float 3s ease-in-out infinite; }
        .avatar img {
            width: 120px; height: 120px; border-radius: 50%;
            object-fit: cover;
            border: 3px solid rgba(167, 139, 250, 0.5);
            box-shadow: 0 8px 25px rgba(167, 139, 250, 0.3);
            cursor: pointer;
        }
        .avatar img.pop { animation: avatarPop 0.7s cubic-bezier(0.34, 1.56, 0.64, 1); }
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
            font-size: 32px; font-weight: 700;
            background: linear-gradient(135deg, #a78bfa, #60a5fa);
            -webkit-background-clip: text; -webkit-text-fill-color: transparent;
            background-clip: text;
            margin-bottom: 8px;
            min-height: 40px;
        }
        .card .title {
            font-size: 16px; color: #a78bfa; font-weight: 500;
            letter-spacing: 1px; margin-bottom: 16px;
        }
        .clock {
            font-family: 'Courier New', 'Consolas', monospace;
            font-size: 22px; font-weight: 700;
            color: #7dd3fc; letter-spacing: 8px;
            margin-bottom: 12px;
            text-shadow: 0 0 18px rgba(125, 211, 252, 0.8);
        }
        .status {
            font-size: 13px;
            color: rgba(167, 139, 250, 0.9);
            margin-bottom: 24px; letter-spacing: 1px;
        }
        .card .bio {
            font-size: 15px;
            color: rgba(255, 255, 255, 0.7);
            line-height: 1.8;
            margin-bottom: 30px;
        }
        .skills {
            display: flex; flex-wrap: wrap; justify-content: center;
            gap: 12px; margin-bottom: 20px;
        }
        .skill-tag {
            background: rgba(167, 139, 250, 0.15);
            color: #c4b5fd;
            padding: 10px 22px;
            border-radius: 22px;
            font-size: 14px; font-weight: 500;
            border: 1px solid rgba(167, 139, 250, 0.3);
            transition: all 0.3s ease;
        }
        .skill-tag:hover {
            background: rgba(167, 139, 250, 0.3);
            transform: translateY(-3px);
            box-shadow: 0 8px 20px rgba(167, 139, 250, 0.3);
        }
        .contact {
            display: flex; justify-content: center; gap: 16px;
            flex-wrap: wrap; font-size: 14px;
            margin-bottom: 20px;
        }
        .contact a {
            color: #60a5fa; text-decoration: none;
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

        /* ========== 音乐开关 ========== */
        .music-toggle {
            position: fixed; top: 20px; right: 20px;
            z-index: 200;
            width: 44px; height: 44px;
            border-radius: 50%;
            background: rgba(167, 139, 250, 0.2);
            border: 1px solid rgba(167, 139, 250, 0.5);
            color: #c4b5fd;
            font-size: 20px;
            cursor: pointer;
            backdrop-filter: blur(10px);
            display: none;
            align-items: center; justify-content: center;
            transition: all 0.3s ease;
        }
        .music-toggle.visible { display: flex; }
        .music-toggle:hover {
            background: rgba(167, 139, 250, 0.4);
            transform: scale(1.1);
        }
        .music-toggle.paused { color: #666; border-color: #666; }

        /* ========== 区块 ========== */
        .section {
            max-width: 1000px;
            margin: 0 auto;
            padding: 40px 20px;
        }
        .section-title {
            font-size: 28px; font-weight: 700;
            text-align: center;
            margin-bottom: 40px;
            background: linear-gradient(135deg, #a78bfa, #60a5fa);
            -webkit-background-clip: text; -webkit-text-fill-color: transparent;
            background-clip: text;
        }

        /* ========== 项目卡片 ========== */
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
            color: #60a5fa; text-decoration: none;
            font-size: 14px; font-weight: 500;
        }

        /* ========== 时间线 ========== */
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
            opacity: 0;
            transform: translateX(-40px);
            transition: opacity 0.7s ease, transform 0.7s cubic-bezier(0.45, 0, 0.3, 1);
        }
        .timeline-item.revealed { opacity: 1; transform: translateX(0); }
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
            font-size: 13px; color: #a78bfa; font-weight: 600;
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

        /* ========== 技能条 ========== */
        .skill-bars {
            max-width: 640px; margin: 0 auto;
            display: flex; flex-direction: column; gap: 22px;
        }
        .skill-bar .skill-name {
            font-size: 15px; color: #e0f2fe;
            margin-bottom: 10px;
            display: flex; justify-content: space-between; align-items: center;
        }
        .skill-bar .skill-name span {
            font-size: 12px;
            color: rgba(167, 139, 250, 0.8);
            letter-spacing: 2px;
        }
        .bar {
            height: 8px;
            background: rgba(255, 255, 255, 0.08);
            border-radius: 4px;
            overflow: hidden;
        }
        .bar-fill {
            height: 100%; width: 0;
            background: linear-gradient(90deg, #a78bfa, #60a5fa);
            border-radius: 4px;
            box-shadow: 0 0 12px rgba(167, 139, 250, 0.6);
            transition: width 1.2s cubic-bezier(0.45, 0, 0.3, 1);
        }

        /* ========== 正在学习 ========== */
        .learning-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
            gap: 20px;
        }
        .learning-card {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 16px;
            padding: 24px;
            transition: all 0.3s ease;
        }
        .learning-card:hover {
            transform: translateY(-4px);
            border-color: rgba(167, 139, 250, 0.4);
            box-shadow: 0 10px 30px rgba(167, 139, 250, 0.2);
        }
        .learning-card h3 { font-size: 16px; margin-bottom: 12px; color: #c4b5fd; }
        .learning-card p {
            font-size: 14px;
            color: rgba(255, 255, 255, 0.65);
            line-height: 1.7;
        }

        /* ========== 音游 ========== */
        .rhythm-container {
            position: relative;
            max-width: 560px; margin: 0 auto;
            height: 520px;
            background: linear-gradient(180deg, rgba(10,14,26,0.9), rgba(10,14,26,0.5));
            border: 1px solid rgba(125, 211, 252, 0.3);
            border-radius: 16px;
            overflow: hidden;
            box-shadow: 0 0 50px rgba(125, 211, 252, 0.15) inset,
                        0 10px 40px rgba(0, 0, 0, 0.5);
        }
        .lanes { display: flex; height: 100%; position: relative; }
        .lane {
            flex: 1;
            border-right: 1px solid rgba(125, 211, 252, 0.08);
            position: relative; overflow: hidden;
        }
        .lane:last-child { border-right: none; }
        .judge-line {
            position: absolute;
            bottom: 100px; left: 0; right: 0;
            height: 3px; background: #7dd3fc;
            box-shadow: 0 0 25px rgba(125, 211, 252, 1),
                        0 0 50px rgba(125, 211, 252, 0.6);
            z-index: 5;
        }
        .judge-line.pulse { animation: judgePulse 0.3s ease; }
        @keyframes judgePulse {
            0% { box-shadow: 0 0 25px rgba(125,211,252,1), 0 0 50px rgba(125,211,252,0.6); }
            50% { box-shadow: 0 0 50px rgba(255,255,255,1), 0 0 100px rgba(125,211,252,1); }
            100% { box-shadow: 0 0 25px rgba(125,211,252,1), 0 0 50px rgba(125,211,252,0.6); }
        }
        .judge-text {
            position: absolute;
            left: 50%; top: 45%;
            transform: translate(-50%, -50%);
            font-size: 34px; font-weight: 900;
            letter-spacing: 4px;
            pointer-events: none;
            z-index: 20;
            opacity: 0;
            font-family: 'Courier New', monospace;
        }
        .judge-text.perfect {
            color: #fbbf24;
            text-shadow: 0 0 20px rgba(251, 191, 36, 0.9), 0 0 40px rgba(251, 191, 36, 0.5);
        }
        .judge-text.great {
            color: #7dd3fc;
            text-shadow: 0 0 20px rgba(125, 211, 252, 0.9), 0 0 40px rgba(125, 211, 252, 0.5);
        }
        .judge-text.show { animation: judgeShow 0.6s cubic-bezier(0.34, 1.56, 0.64, 1) forwards; }
        @keyframes judgeShow {
            0%   { opacity: 0; transform: translate(-50%, -50%) scale(0.5); }
            30%  { opacity: 1; transform: translate(-50%, -50%) scale(1.2); }
            70%  { opacity: 1; transform: translate(-50%, -50%) scale(1); }
            100% { opacity: 0; transform: translate(-50%, -60%) scale(1); }
        }
        .combo-display {
            position: absolute; top: 30px; left: 50%;
            transform: translateX(-50%);
            text-align: center; z-index: 10;
            pointer-events: none;
        }
        .combo-count {
            font-size: 64px; font-weight: 800;
            color: #7dd3fc; line-height: 1;
            text-shadow: 0 0 40px rgba(125, 211, 252, 1), 0 0 80px rgba(125, 211, 252, 0.6);
            font-family: 'Courier New', monospace;
        }
        .combo-count.pop { animation: comboPop 0.3s ease; }
        @keyframes comboPop {
            0% { transform: scale(1); }
            50% { transform: scale(1.35); color: #fff; }
            100% { transform: scale(1); }
        }
        .combo-label {
            font-size: 13px;
            color: rgba(125, 211, 252, 0.7);
            letter-spacing: 8px;
            margin-top: 8px; font-weight: 700;
        }
        .note {
            position: absolute;
            width: 70%; left: 15%;
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

        /* ========== 关于本站 ========== */
        .about-site {
            max-width: 720px; margin: 0 auto;
            background: rgba(255, 255, 255, 0.03);
            border-left: 3px solid #a78bfa;
            padding: 30px 36px;
            border-radius: 0 12px 12px 0;
        }
        .about-site p {
            font-size: 15px;
            color: rgba(255, 255, 255, 0.7);
            line-height: 1.9;
            margin-bottom: 14px;
        }
        .about-site p:last-child { margin-bottom: 0; }
        .about-site code {
            background: rgba(167, 139, 250, 0.15);
            color: #c4b5fd;
            padding: 2px 8px;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
        }
        .about-tech {
            margin-top: 20px !important;
            padding-top: 16px;
            border-top: 1px solid rgba(255, 255, 255, 0.08);
            font-size: 13px !important;
            color: #a78bfa !important;
            letter-spacing: 1px;
        }

        /* ========== 响应式 ========== */
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

    <canvas class="particle-canvas" id="particleCanvas"></canvas>

    <div class="landing" id="landing">
        <div class="landing-bg" style="background-image: url(')HTML" << me.coverImage << R"HTML(');"></div>
        <div class="landing-overlay"></div>
        <div class="landing-content">
            <h1>)HTML" << me.welcomeText << R"HTML(</h1>
            <button class="enter-btn" id="enterBtn">点击进入</button>
        </div>
    </div>

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

    <button class="music-toggle" id="musicBtn" title="暂停/播放音乐">&#9834;</button>

    <div class="main-page" id="mainPage">

        <div class="card" id="mainCard">
            <div class="avatar"><img src=")HTML" << me.avatar << R"HTML(" alt="头像" id="avatarImg"></div>
            <h1 id="nameTyping" data-name=")HTML" << me.name << R"HTML("></h1>
            <div class="title">)HTML" << me.title << R"HTML(</div>
            <div class="clock" id="clock">00:00:00</div>
            <div class="status" id="status">加载中...</div>
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
                    <h3>个人主页生成器</h3>
                    <p>用 C++ 写的静态站点生成器，运行时拼接 HTML 并输出 index.html，推送到 GitHub 后由 Actions 自动编译部署。</p>
                    <div class="project-tech">C++ / GitHub Actions / HTML / CSS / JavaScript</div>
                    <a href="https://github.com/zhehong268/my-intro" target="_blank">查看源码</a>
                </div>
                <div class="project-card">
                    <h3>Project Alpha</h3>
                    <p>还在构思中，打算写一个 C++ 小工具练手。</p>
                    <div class="project-tech">C++</div>
                    <a href="#" target="_blank">敬请期待</a>
                </div>
            </div>
        </section>

        <section class="section">
            <h2 class="section-title">Experience</h2>
            <div class="timeline" id="timeline">
                <div class="timeline-item">
                    <div class="timeline-period">2025 - 至今</div>
                    <div class="timeline-content">
                        <h3>BanGDream 高级工程师</h3>
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
            <h2 class="section-title">技能</h2>
            <div class="skill-bars">
                <div class="skill-bar">
                    <div class="skill-name">C++ <span>入门</span></div>
                    <div class="bar"><div class="bar-fill" data-width="40%"></div></div>
                </div>
                <div class="skill-bar">
                    <div class="skill-name">Python <span>了解</span></div>
                    <div class="bar"><div class="bar-fill" data-width="30%"></div></div>
                </div>
                <div class="skill-bar">
                    <div class="skill-name">Git / GitHub <span>会用</span></div>
                    <div class="bar"><div class="bar-fill" data-width="50%"></div></div>
                </div>
                <div class="skill-bar">
                    <div class="skill-name">HTML / CSS <span>够用</span></div>
                    <div class="bar"><div class="bar-fill" data-width="45%"></div></div>
                </div>
                <div class="skill-bar">
                    <div class="skill-name">Linux <span>在学</span></div>
                    <div class="bar"><div class="bar-fill" data-width="25%"></div></div>
                </div>
            </div>
        </section>

        <section class="section">
            <h2 class="section-title">正在学习</h2>
            <div class="learning-grid">
                <div class="learning-card">
                    <h3>C++ 基础</h3>
                    <p>类、STL、指针和引用这些，边写边学。</p>
                </div>
                <div class="learning-card">
                    <h3>数据结构</h3>
                    <p>课程在学，顺便用 C++ 手写一遍加深理解。</p>
                </div>
                <div class="learning-card">
                    <h3>Git 和 GitHub</h3>
                    <p>这个网站就是在用的过程中学会的。</p>
                </div>
                <div class="learning-card">
                    <h3>Linux 基础</h3>
                    <p>会一些常用命令，还在慢慢熟悉。</p>
                </div>
            </div>
        </section>

        <section class="section">
            <h2 class="section-title">Rhythm</h2>
            <div class="rhythm-container" id="rhythmContainer">
                <div class="judge-text" id="judgeText"></div>
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

        <section class="section">
            <h2 class="section-title">关于本站</h2>
            <div class="about-site">
                <p>这个网站是一个 C++ 项目。所有内容由 <code>main.cpp</code> 生成，源文件里没有手写的 HTML。</p>
                <p>每次推送到 GitHub，Actions 会自动编译运行，生成新的 index.html 并部署到 Pages。</p>
                <p class="about-tech">C++ · GitHub Actions · HTML · CSS · JavaScript</p>
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

        /* ============ 状态 ============ */
        function updateStatus() {
            var h = new Date().getHours();
            var el = document.getElementById('status');
            if (h >= 0 && h < 7) el.textContent = '这个点还没睡？';
            else if (h < 9) el.textContent = '早八人';
            else if (h < 12) el.textContent = '在上课';
            else if (h < 14) el.textContent = '干饭中';
            else if (h < 18) el.textContent = '在写代码';
            else if (h < 22) el.textContent = '在摸鱼';
            else el.textContent = '准备睡了';
        }
        setInterval(updateStatus, 60000);
        updateStatus();

        /* ============ 点击光波 ============ */
        document.addEventListener('click', function(e) {
            var ripple = document.createElement('div');
            ripple.className = 'click-ripple';
            ripple.style.left = e.clientX + 'px';
            ripple.style.top = e.clientY + 'px';
            document.body.appendChild(ripple);
            setTimeout(function() { ripple.remove(); }, 900);
        });

        /* ============ 鼠标视差 ============ */
        (function() {
            var mp = document.getElementById('mainPage');
            var targetX = 50, targetY = 50, curX = 50, curY = 50;
            document.addEventListener('mousemove', function(e) {
                targetX = 50 + (e.clientX / window.innerWidth - 0.5) * 4;
                targetY = 50 + (e.clientY / window.innerHeight - 0.5) * 4;
            });
            function tick() {
                curX += (targetX - curX) * 0.05;
                curY += (targetY - curY) * 0.05;
                mp.style.backgroundPosition = 'center, ' + curX + '% ' + curY + '%';
                requestAnimationFrame(tick);
            }
            tick();
        })();

        /* ============ 粒子背景 ============ */
        (function() {
            var canvas = document.getElementById('particleCanvas');
            var ctx = canvas.getContext('2d');
            var particles = [];
            var w = 0, h = 0;

            function resize() {
                w = canvas.width = window.innerWidth;
                h = canvas.height = window.innerHeight;
            }
            resize();
            window.addEventListener('resize', resize);

            for (var i = 0; i < 45; i++) {
                particles.push({
                    x: Math.random() * 2000,
                    y: Math.random() * 1500,
                    r: Math.random() * 1.8 + 0.6,
                    vx: (Math.random() - 0.5) * 0.3,
                    vy: (Math.random() - 0.5) * 0.3,
                    a: Math.random() * 0.45 + 0.25
                });
            }

            function draw() {
                ctx.clearRect(0, 0, w, h);
                for (var i = 0; i < particles.length; i++) {
                    var p = particles[i];
                    p.x += p.vx;
                    p.y += p.vy;
                    if (p.x < 0) p.x = w;
                    if (p.x > w) p.x = 0;
                    if (p.y < 0) p.y = h;
                    if (p.y > h) p.y = 0;
                    ctx.beginPath();
                    ctx.fillStyle = 'rgba(125, 211, 252, ' + p.a + ')';
                    ctx.arc(p.x, p.y, p.r, 0, Math.PI * 2);
                    ctx.fill();
                }
                requestAnimationFrame(draw);
            }
            draw();
        })();

        /* ============ 滚动入场（只作用于 section） ============ */
        function setupReveal() {
            var targets = document.querySelectorAll('.section');
            // 兜底：浏览器不支持 IntersectionObserver 时直接显示
            if (!('IntersectionObserver' in window)) {
                targets.forEach(function(el) { el.classList.add('revealed'); });
                return;
            }
            var observer = new IntersectionObserver(function(entries) {
                entries.forEach(function(entry) {
                    if (entry.isIntersecting) {
                        entry.target.classList.add('revealed');
                        observer.unobserve(entry.target);
                    }
                });
            }, { threshold: 0.05, rootMargin: '0px 0px -60px 0px' });
            targets.forEach(function(el) { observer.observe(el); });
        }

        /* ============ 技能条 ============ */
        function setupBars() {
            var bars = document.querySelectorAll('.bar-fill');
            if (!('IntersectionObserver' in window)) {
                bars.forEach(function(bar) { bar.style.width = bar.getAttribute('data-width'); });
                return;
            }
            var observer = new IntersectionObserver(function(entries) {
                entries.forEach(function(entry) {
                    if (entry.isIntersecting) {
                        var bar = entry.target;
                        setTimeout(function() {
                            bar.style.width = bar.getAttribute('data-width');
                        }, 150);
                        observer.unobserve(bar);
                    }
                });
            }, { threshold: 0.3 });
            bars.forEach(function(b) { observer.observe(b); });
        }

        /* ============ 时间线 ============ */
        function setupTimeline() {
            var timeline = document.getElementById('timeline');
            if (!timeline) return;
            var items = timeline.querySelectorAll('.timeline-item');
            if (!('IntersectionObserver' in window)) {
                items.forEach(function(item) { item.classList.add('revealed'); });
                return;
            }
            var observer = new IntersectionObserver(function(entries) {
                if (entries[0].isIntersecting) {
                    items.forEach(function(item, i) {
                        setTimeout(function() { item.classList.add('revealed'); }, i * 200);
                    });
                    observer.disconnect();
                }
            }, { threshold: 0.1 });
            observer.observe(timeline);
        }

        /* ============ 名字逐字浮现 ============ */
        function revealName() {
            var el = document.getElementById('nameTyping');
            var text = el.getAttribute('data-name');
            el.textContent = '';
            for (var i = 0; i < text.length; i++) {
                var span = document.createElement('span');
                span.className = 'name-char';
                span.textContent = text.charAt(i);
                span.style.animationDelay = (i * 0.12) + 's';
                el.appendChild(span);
            }
        }

        /* ============ 进入站点 ============ */
        function enterSite() {
            var loader = document.getElementById('loader');
            loader.classList.add('show');

            // 1. 签名动画播 3.5 秒
            setTimeout(function() {
                // 2. 隐藏 landing，显示 mainPage
                document.getElementById('landing').classList.add('hidden');
                document.getElementById('mainPage').classList.add('visible');
                document.body.style.overflowY = 'auto';
                document.getElementById('particleCanvas').classList.add('visible');

                // 3. 播音乐
                var music = document.getElementById('bgMusic');
                var btn = document.getElementById('musicBtn');
                btn.classList.add('visible');
                music.play().then(function() {
                    btn.classList.remove('paused');
                }).catch(function(err) {
                    console.log('音乐播放失败:', err);
                    btn.classList.add('paused');
                });

                // 4. loader 开始淡出
                loader.classList.remove('show');

                // 5. loader 完全消失后，正式开始动画
                setTimeout(function() {
                    loader.style.display = 'none';

                    // 卡片先滑入
                    document.getElementById('mainCard').classList.add('revealed');

                    // 卡片滑入 400ms 后，名字逐个浮现
                    setTimeout(revealName, 400);

                    // 其他 section 注册滚动入场
                    setupReveal();
                    setupBars();
                    setupTimeline();
                }, 500);
            }, 3500);
        }

        document.getElementById('enterBtn').addEventListener('click', enterSite);

        /* ============ 音乐开关 ============ */
        document.getElementById('musicBtn').addEventListener('click', function(e) {
            e.stopPropagation();
            var music = document.getElementById('bgMusic');
            var btn = document.getElementById('musicBtn');
            if (music.paused) {
                music.play();
                btn.classList.remove('paused');
            } else {
                music.pause();
                btn.classList.add('paused');
            }
        });

        /* ============ 头像弹跳 ============ */
        document.getElementById('avatarImg').addEventListener('click', function(e) {
            e.stopPropagation();
            this.classList.remove('pop');
            void this.offsetWidth;
            this.classList.add('pop');
        });

        /* ============ 音游 ============ */
        var BPM = 168;
        var beatInterval = 60000 / BPM;
        var noteTravelTime = 1800;
        var rhythmStarted = false;
        var combo = 0;

        function startRhythm() {
            if (rhythmStarted) return;
            rhythmStarted = true;

            var lanes = document.querySelectorAll('#lanes .lane');
            var comboEl = document.getElementById('comboCount');
            var judgeLine = document.getElementById('judgeLine');
            var judgeText = document.getElementById('judgeText');
            var lastJudge = 0;

            function spawnNote() {
                var laneIndex = Math.floor(Math.random() * lanes.length);
                var lane = lanes[laneIndex];

                var note = document.createElement('div');
                note.className = 'note';
                lane.appendChild(note);

                var laneHeight = lane.clientHeight;
                var targetTop = laneHeight - 100 - 8;

                note.animate(
                    [
                        { top: '-20px', opacity: 1 },
                        { top: targetTop + 'px', opacity: 1 }
                    ],
                    { duration: noteTravelTime, easing: 'linear', fill: 'forwards' }
                );

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

                    judgeLine.classList.remove('pulse');
                    void judgeLine.offsetWidth;
                    judgeLine.classList.add('pulse');

                    var isPerfect = Math.random() < 0.65;
                    var word = isPerfect ? 'PERFECT' : 'GREAT';
                    var cls = isPerfect ? 'perfect' : 'great';

                    if (isPerfect || Date.now() - lastJudge > 200) {
                        judgeText.textContent = word;
                        judgeText.className = 'judge-text ' + cls;
                        void judgeText.offsetWidth;
                        judgeText.classList.add('show');
                        lastJudge = Date.now();
                    }

                    combo++;
                    comboEl.textContent = combo;
                    comboEl.classList.remove('pop');
                    void comboEl.offsetWidth;
                    comboEl.classList.add('pop');
                }, noteTravelTime);
            }

            setInterval(spawnNote, beatInterval);
        }

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