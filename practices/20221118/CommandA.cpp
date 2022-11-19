#include "ICommand.hpp"
#include <iostream>

class CommandA :public ICommand {
public:
    void run() const override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

namespace
{
    auto stub{ ICommand::Register<CommandA>("CommandA") };
}
