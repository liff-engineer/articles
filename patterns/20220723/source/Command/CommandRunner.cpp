#include "CommandRunner.hpp"

CommandRunner::CommandRunner(UiContext* context)
    :m_context(context)
{
}

void CommandRunner::registerCommand(std::string commandCode, std::unique_ptr<ICommand> command)
{
    m_commands[commandCode] = std::move(command);
}

void CommandRunner::runCommand(std::string commandCode)
{
    auto it = m_commands.find(commandCode);
    if (it == m_commands.end()) {
        return;
    }

    if (it->second.get() != m_current) {
        if (m_current) {
            m_current->stop();
            m_current = nullptr;
        }
    }

    m_current = it->second.get();
    m_current->start(m_context, [&](ICommand* command) {
        onCommandFinish(command);
        });
}

void CommandRunner::setDefaultCommand(std::string commandCode)
{
    m_defaultCommand = commandCode;
}

void CommandRunner::onCommandFinish(ICommand* command)
{
    if (command) {
        command->stop();//停止命令
        if (command == m_current) {
            m_current = nullptr;
        }
    }
    //运行默认命令
    runCommand(m_defaultCommand);
}
