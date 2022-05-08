#include "AnySubject.hpp"
#include <iostream>

struct BPrinter {

    template<typename T>
    void Update(const T& obj) {
        std::cout << "BPrinter:" << obj << "\n";
    }
};

extern "C" {
    void __declspec(dllexport) Initialize();
}


void Initialize()
{
    //注册观察者
    {
        auto subject = abc::v0::AnySubject::Get();
        subject->RegisterObserver<int>(BPrinter{});
        subject->RegisterObserver<bool>(BPrinter{});
        subject->RegisterObserver<std::string>(BPrinter{});
    }

    {
        auto subject = abc::v1::AnySubject::Get();
        subject->RegisterObserver<int>(BPrinter{});
        subject->RegisterObserver<bool>(BPrinter{});
        subject->RegisterObserver<std::string>(BPrinter{});
    }
}
