#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// 定义个人信息结构体（体现 C++ 的封装）
struct Profile {
    std::string name;
    std::string title;
    std::string bio;
    std::string avatar;
    std::vector<std::string> skills;
    std::string email;
    std::string github;
};

// 生成技能标签的 HTML 片段
std::string generateSkillTags(const std::vector<std::string>& skills) {
    std::string tags;
    for (const auto& skill : skills) {
        tags += "<span class=\"skill-tag\">" + skill + "</span>\n";
    }
    return tags;
}

int main() {
    // 初始化数据
    Profile me;
    me.name = "张兆恒";
    me.title = "C++ 开发者 ";
    me.bio = "热爱系统底层开发 C++17/20、Linux 系统编程";
    me.avatar = "d:\\图片\\OIP-C.jpg" ;// 用 emoji 代替头像，避免图片路径问题
    me.skills = {"C++", "Python", "Linux", "CMake", "WebAssembly", "Docker"};
    me.email = "3255484226@qq.com";
    me.github = "https://github.com/zhehong268";

    // 打开文件准备写入
    std::ofstream html("index.html");
    if (!html.is_open()) {
        std::cerr << "无法创建 index.html" << std::endl;
        return 1;
    }

    // 用 C++ 拼接完整的 HTML（使用原始字符串 R"(...) 处理多行内容）
    html << R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" << me.name << R"( - 自我介绍</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
            background: linear-gradient(135deg, #0f0c29, #302b63, #24243e);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
            color: #fff;
        }
        .card {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 24px;
            padding: 50px 40px;
            max-width: 560px;
            width: 100%;
            box-shadow: 0 25px 50px rgba(0, 0, 0, 0.5);
            text-align: center;
            animation: fadeInUp 0.8s ease-out;
        }
        @keyframes fadeInUp {
            from { opacity: 0; transform: translateY(30px); }
            to { opacity: 1; transform: translateY(0); }
        }
        .avatar {
            font-size: 64px;
            margin-bottom: 16px;
            animation: float 3s ease-in-out infinite;
        }
        @keyframes float {
            0%, 100% { transform: translateY(0); }
            50% { transform: translateY(-10px); }
        }
        h1 {
            font-size: 32px;
            font-weight: 700;
            background: linear-gradient(135deg, #a78bfa, #60a5fa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 8px;
        }
        .title {
            font-size: 16px;
            color: #a78bfa;
            font-weight: 500;
            letter-spacing: 1px;
            margin-bottom: 24px;
        }
        .bio {
            font-size: 15px;
            color: rgba(255, 255, 255, 0.7);
            line-height: 1.8;
            margin-bottom: 30px;
            text-align: left;
            padding: 0 10px;
        }
        .skills {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 10px;
            margin-bottom: 30px;
        }
        .skill-tag {
            background: rgba(167, 139, 250, 0.15);
            color: #c4b5fd;
            padding: 8px 18px;
            border-radius: 20px;
            font-size: 13px;
            font-weight: 500;
            border: 1px solid rgba(167, 139, 250, 0.3);
            transition: all 0.3s ease;
        }
        .skill-tag:hover {
            background: rgba(167, 139, 250, 0.3);
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(167, 139, 250, 0.3);
        }
        .contact {
            display: flex;
            justify-content: center;
            gap: 16px;
            font-size: 14px;
        }
        .contact a {
            color: #60a5fa;
            text-decoration: none;
            padding: 8px 16px;
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
    </style>
</head>
<body>
    <div class="card">
        <div class="avatar">)" << me.avatar << R"(</div>
        <h1>)" << me.name << R"(</h1>
        <div class="title">)" << me.title << R"(</div>
        <p class="bio">)" << me.bio << R"(</p>
        <div class="skills">
            )" << generateSkillTags(me.skills) << R"(
        </div>
        <div class="contact">
            <a href="mailto:)" << me.email << R"(">📧 邮箱</a>
            <a href=")" << me.github << R"(" target="_blank">🐙 GitHub</a>
        </div>
        <div class="footer">Generated by C++ · Powered by GitHub Pages</div>
    </div>
</body>
</html>)";

    html.close();
    std::cout << "index.html 生成成功！" << std::endl;
    return 0;
}