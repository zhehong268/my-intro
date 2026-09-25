#include <emscripten.h>

extern "C" {

    EMSCRIPTEN_KEEPALIVE
    const char* getName() {
        return "张兆恒";
    }

    EMSCRIPTEN_KEEPALIVE
    const char* getTitle() {
        return "C++ 开发者";
    }

    EMSCRIPTEN_KEEPALIVE
    const char* getBio() {
        return "对底层系统、C++17/20 和 Linux 编程充满热情。";
    }

    EMSCRIPTEN_KEEPALIVE
    int getYearsOfExperience() {
        return 3;
    }

    EMSCRIPTEN_KEEPALIVE
    const char* getSkills() {
        return "[\"C++\"]";
    }
}