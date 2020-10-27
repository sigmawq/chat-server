#pragma once
#include <iostream>
#include <signal.h>

int       last_code;

// Does error messaging, includes last UNIX error code.
static void ErrorOut(int line, std::string file){
    std::cout << "An error occured, line: " << line << ". File: " << file << std::endl;
    std::cout << "Error code: " << errno << std::endl;
    raise(SIGTRAP);
}

static void ErrorOut_Custom(int line, std::string file, int code){
    std::cout << "An error occured, line: " << line << ". File: " << file << std::endl;
    std::cout << "Error code: " << code << std::endl;
    raise(SIGTRAP);
}

#define ASSERT(action) last_code = (action);\
    if (last_code == -1) ErrorOut(__LINE__, __FILE__);