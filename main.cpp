#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::ofstream html("index.html");
    
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<title>自我介绍</title>\n";
    html << "<style>\n";
    html << "body { font-family: sans-serif; background: #667eea; color: white; text-align: center; padding: 50px; }\n";
    html << "</style>\n</head>\n<body>\n";
    
    html << "<h1>张兆恒</h1>\n";
    html << "<p>C++ 开发者</p>\n";
    html << "<p>热爱系统底层开发</p>\n";
    
    html << "</body>\n</html>\n";
    
    html.close();
    std::cout << "index.html 生成成功！" << std::endl;
    return 0;
}