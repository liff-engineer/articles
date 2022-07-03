#include "Actor.h"
#include <Windows.h>
#include <iostream>

typedef void(__stdcall* Initialze)();
//typedef void(__stdcall* Finialize)();
int main()
{
    //加载插件
    auto h = LoadLibrary("PyPlugin.dll");
    if (h == nullptr) {
        std::cout << "Cannot find PyPlugin.dll" << std::endl;
        return EXIT_FAILURE;
    }

    auto op = (Initialze)GetProcAddress(h, "Initialize");
    if (!op) {
        std::cout << "Cannot find function Initialize" << std::endl;
        return EXIT_FAILURE;
    }

    op();
    
    auto factory = abc::gActorFactory();
    std::cout << "ActorFactory:\n";
    for (auto&& code : factory->Codes()) {
        std::cout << "\tCode:" << code << "\n";
    }
    std::cout << "<|>\n";
    std::cout << "Create And Run:\n";
    for (auto&& code : factory->Codes()) {
        std::cout << "\tCode(" << code << "):\n";
        auto result = factory->Make(code);
        if (result) {
            result->Launch();
        }
        std::cout << "<|>\n";
    }
    return 0;
}
