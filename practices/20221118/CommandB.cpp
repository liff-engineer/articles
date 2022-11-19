#include "ICommand.hpp"
#include <iostream>

class CommandB :public ICommand {
public:
    void run() const override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

namespace
{
    auto stub{ ICommand::Register<CommandB>("CommandB") };
}
