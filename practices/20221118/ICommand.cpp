#include "ICommand.hpp"
#include "RegistryTemplate.hpp"

class CommandFactory :public
    abc::RegistryTemplate<
        std::pair<std::string, std::function<std::unique_ptr<ICommand>()>>
    >
{
public:
    static CommandFactory& Get() {
        static CommandFactory object{};
        return object;
    }
};

std::unique_ptr<ICommand> ICommand::Create(std::string const& code)
{
    for (auto& e : CommandFactory::Get()) {
        if (e.first == code) {
            return e.second();
        }
    }
    return {};
}

void ICommand::Destory(std::size_t k)
{
    CommandFactory::Get().remove(k);
}

std::size_t ICommand::RegisterImpl(std::string const& code, std::function<std::unique_ptr<ICommand>()>&& builder)
{
    return CommandFactory::Get().add(code, std::move(builder));
}
