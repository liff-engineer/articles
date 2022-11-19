#include "IExtension.hpp"
#include "RegistryTemplate.hpp"

class Extension :public 
    abc::RegistryTemplate<
        std::unique_ptr<IExtension>
    >,
    public IExtension
{
public:
    static Extension& Get() {
        static Extension object{};
        return object;
    }

    void doX() override {
        for (auto& e : *this) {
            e->doX();
        }
    }
    
    void doY() override {
        for (auto& e : *this) {
            e->doY();
        }
    }
};

IExtension& IExtension::Get()
{
    return Extension::Get();
}

void IExtension::Destory(std::size_t k)
{
    return Extension::Get().remove(k);
}

std::size_t IExtension::RegisterImpl(std::unique_ptr<IExtension> e)
{
    return Extension::Get().add(std::move(e));
}
