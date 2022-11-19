#pragma once
#include "RegistryStub.hpp"
#include <memory>

class IExtension {
public:
    virtual ~IExtension() = default;
    IExtension& operator=(IExtension&&) = delete;
public:
    virtual void doX() = 0;
    virtual void doY() = 0;
public:
    static IExtension& Get();
    static void Destory(std::size_t);

    template<typename T,typename... Args>
    static abc::RegistryStub<IExtension> Register(Args&&... args) {
        return abc::RegistryStub<IExtension>{
            RegisterImpl(std::make_unique<T>(std::forward<Args>(args)...))
        };
    }
private:
    static std::size_t RegisterImpl(std::unique_ptr<IExtension> e);
};

using IExtensionStub = abc::RegistryStub<IExtension>;
