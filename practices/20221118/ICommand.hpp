#pragma once
#include "RegistryStub.hpp"
#include <memory>
#include <string>
#include <functional>

class ICommand {
public:
    virtual ~ICommand() = default;
    ICommand& operator=(ICommand&&) = delete;
public:
    virtual void run() const = 0;
public:
    static std::unique_ptr<ICommand> Create(std::string const& code);
    static void Destory(std::size_t);

    template<typename T, typename K, typename... Args>
    static abc::RegistryStub<ICommand> Register(K&& k, Args&&... args) {
        return abc::RegistryStub<ICommand>{
            RegisterImpl(std::forward<K>(k), [&args...]()->std::unique_ptr<ICommand> {
                return std::make_unique<T>(std::forward<Args>(args)...);
                })
        };
    }
private:
    static std::size_t RegisterImpl(std::string const& code,
        std::function<std::unique_ptr<ICommand>()>&& builder
    );
};

using ICommandStub = abc::RegistryStub<ICommand>;
