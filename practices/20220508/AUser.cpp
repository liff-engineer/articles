#include "AnySubject.hpp"
#include <iostream>

struct APrinter {

    template<typename T>
    void Update(const T& obj) {
        std::cout << "APrinter:" << obj << "\n";
    }
};

extern "C" {
    void __declspec(dllexport) Initialize();
}

void  Initialize()
{
    //注册观察者
    {
        auto subject = abc::v0::AnySubject::Get();
        subject->RegisterObserver<int>(APrinter{});
        subject->RegisterObserver<double>(APrinter{});
        subject->RegisterObserver<std::string>(APrinter{});
    }

    {
        auto subject = abc::v1::AnySubject::Get();
        subject->RegisterObserver<int>(APrinter{});
        subject->RegisterObserver<double>(APrinter{});
        subject->RegisterObserver<std::string>(APrinter{});
    }
}
