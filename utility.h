#pragma once
#include <iostream>

// I had to implement this because for some reason std::cout didn't
// catch all NULL terminators on all platforms
// Prints contet of buff till 000 or "max" char has been reached
void Out(const char* buff, int max = INT32_MAX){
    for (int i = 0; i < max; i++){
        if (buff[i] == 0) return;
        std::cout << buff[i];
    }
}