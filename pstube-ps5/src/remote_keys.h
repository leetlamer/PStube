#pragma once
#include <stdint.h>
#include <string>
namespace probe {
inline uint32_t remoteKey(const std::string& key) {
    if(key=="up") return PS5_PAD_BUTTON_UP;if(key=="down") return PS5_PAD_BUTTON_DOWN;
    if(key=="left") return PS5_PAD_BUTTON_LEFT;if(key=="right") return PS5_PAD_BUTTON_RIGHT;
    if(key=="cross") return PS5_PAD_BUTTON_CROSS;if(key=="circle") return PS5_PAD_BUTTON_CIRCLE;
    if(key=="square") return PS5_PAD_BUTTON_SQUARE;if(key=="triangle") return PS5_PAD_BUTTON_TRIANGLE;
    if(key=="l1") return PS5_PAD_BUTTON_L1;if(key=="r1") return PS5_PAD_BUTTON_R1;
    if(key=="l2") return PS5_PAD_BUTTON_L2;if(key=="options") return PS5_PAD_BUTTON_OPTIONS;return 0;
}
}
