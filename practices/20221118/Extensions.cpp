#include "IExtension.hpp"
#include <iostream>
#include <string>

class MyExtension :public IExtension {
    std::string  m_code;
public:
    explicit MyExtension(std::string code)
        :m_code(std::move(code))
    {}

    void doX() override {
        std::cout << "(" << m_code << ")" << __FUNCSIG__ << "\n";
    }
    void doY() override {
        std::cout << "(" << m_code << ")" << __FUNCSIG__ << "\n";
    }
};

namespace
{
    IExtensionStub stubs[]{
        IExtension::Register<MyExtension>("E1"),
        IExtension::Register<MyExtension>("E2")
    };
}
