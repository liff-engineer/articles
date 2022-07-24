#pragma once
#include "ICommand.hpp"
#include <unordered_map>
#include <string>
#include <memory>

class UiContext;
class CommandRunner {
public:
    explicit CommandRunner(UiContext* context);
    void registerCommand(std::string commandCode, std::unique_ptr<ICommand> command);
    void runCommand(std::string commandCode);
    void setDefaultCommand(std::string commandCode);
private:
    void onCommandFinish(ICommand* command);
private:
    UiContext* m_context{};
    std::unordered_map<std::string, std::unique_ptr<ICommand>> m_commands;
    ICommand* m_current{};
    std::string m_defaultCommand;
};
