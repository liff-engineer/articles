#include "AnySubject.hpp"
#include <Windows.h>
#include <iostream>

typedef void(* Initialize)();
int main() {
        
    for (auto lib : {"AUser.dll","BUser.dll"}) {
        HINSTANCE dll = LoadLibrary(lib);
        if (!dll) {
            std::cout << "cannot load '" << lib << "'\n";
        }
        else
        {
            auto op = (Initialize)GetProcAddress(dll, "Initialize");
            if (!op) {
                std::cout << "cannot locate 'Initialize' function (" << lib << ")'\n";
            }
            else
            {
                op();
            }
        }
    }

    {
        auto subject = abc::v0::AnySubject::Get();
        subject->Notify(1024);
        subject->Notify(3.1415);
        subject->Notify(std::string{ "liff.engineer@gmail.com" });
        subject->Notify(true);
    }
    {
        auto subject = abc::v1::AnySubject::Get();
        subject->Notify(1024);
        subject->Notify(3.1415);
        subject->Notify(std::string{ "liff.engineer@gmail.com" });
        subject->Notify(3.1415);
        subject->Notify(true);
        subject->Notify('A');
    }

    return 0;
}
