#include "IExtension.hpp"
#include <iostream>

class ExtensionA :public IExtension {
public:
    void doX() override {
        std::cout << __FUNCSIG__ << "\n";
    }
    void doY() override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

class ExtensionB :public IExtension {
public:
    void doX() override {
        std::cout << __FUNCSIG__ << "\n";
    }
    void doY() override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

namespace
{
    IExtensionStub stubs[]{
        IExtension::Register<ExtensionA>(),
        IExtension::Register<ExtensionB>()
    };
}
