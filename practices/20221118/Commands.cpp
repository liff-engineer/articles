#include "ICommand.hpp"
#include <iostream>

class MyCommand :public ICommand {
    std::string m_code;
public:
    explicit MyCommand(std::string code)
        :m_code(std::move(code)) {};

    void run() const override {
        std::cout<<"["<<m_code<<"]" << __FUNCSIG__ << "\n";
    }
};

namespace
{
    ICommandStub stubs[]{
        ICommand::Register<MyCommand>("MyCommand-X","CommandX"),
        ICommand::Register<MyCommand>("MyCommand-Y","CommandY"),
        ICommand::Register<MyCommand>("MyCommand-Z","CommandZ")
    };
}
