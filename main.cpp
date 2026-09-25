#include <emscripten.h>

extern "C" {

    EMSCRIPTEN_KEEPALIVE
    const char* getName() {
        return "Zhang San";
    }

    EMSCRIPTEN_KEEPALIVE
    const char* getTitle() {
        return "C++ Developer";
    }

    EMSCRIPTEN_KEEPALIVE
    const char* getBio() {
        return "Passionate about low-level systems, C++17/20 and Linux programming.";
    }

    EMSCRIPTEN_KEEPALIVE
    int getYearsOfExperience() {
        return 3;
    }

    EMSCRIPTEN_KEEPALIVE
    const char* getSkills() {
        return "[\"C++\", \"Python\", \"Linux\", \"CMake\", \"WebAssembly\"]";
    }
}