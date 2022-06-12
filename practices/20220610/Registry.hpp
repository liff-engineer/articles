/// 全局实例(注册、查询、创建)支持库
/// >> 注册处Registry
/// 1. 类型单一实例存储
/// 2. 支持绑定基类到派生类实现(适用于单例型扩展点)
/// 3. 默认提供全局单例
/// 
/// >> 实例(全局)获取:Get<T>(owner)
/// >> 实例(全局)创建:
/// 1. Make<I>
/// 2. MakeBy<I>
/// 
/// 适用于应用全局单例的场景,可为后续解耦+重构提供支持
/// 提供的Get/MakeBy可用来提供一致接口
#pragma once
#include <memory>
#include <vector>

namespace abc
{
    class Registry final {
    public:
        static Registry* Get() {
            static Registry obj{};
            return &obj;
        }

        template<typename T, typename... Args>
        T* emplace(Args&&... args);

        template<typename T>
        void erase() noexcept;

        template<typename T>
        T* get() const;

        void clear() {
            m_entries.clear();
        }

        template<typename I, typename T>
        bool bind();
    private:
        class Impl;
        class IValue {
        public:
            virtual ~IValue() = default;
            virtual std::size_t TypeIndex() const noexcept = 0;
        };
        std::vector<std::unique_ptr<IValue>> m_entries;
    };

    template<typename I, typename = void>
    struct RegistryFactoryInject;

    template<typename Owner, typename T, typename = void>
    struct Getter;

    template<typename Owner, typename I, typename = void>
    struct Maker;

    template<typename T>
    decltype(auto) Get() {
        return Getter<Registry, T>{}.Get(Registry::Get());
    }

    template<typename T, typename Owner>
    decltype(auto) Get(Owner owner) {
        using U = std::remove_pointer_t<std::remove_const_t<Owner>>;
        return Getter<U, T>{}.Get(owner);
    }

    template<typename I, typename... Args>
    decltype(auto) Make(Args&&... args) {
        return Maker<Registry, I>{}.Make(Registry::Get(), std::forward<Args>(args)...);
    }

    template<typename I, typename Owner, typename... Args>
    decltype(auto) MakeBy(Owner owner, Args&&... args) {
        using U = std::remove_pointer_t<std::remove_const_t<Owner>>;
        return Maker<U, I>{}.Make(owner, std::forward<Args>(args)...);
    }

    ////////////////////////////////////////////////////////////////
    ///以下为实现相关,使用者无需关注
    ////////////////////////////////////////////////////////////////

    template<typename T, typename = void>
    struct RegistryGetter {
        T* Get(Registry* owner) {
            if constexpr (std::is_constructible_v<T>) {
                if (auto vp = owner->get<T>()) return vp;
                return owner->emplace<T>();
            }
            else {
                return owner->get<T>();
            }
        }
    };

    template<typename I, typename = void>
    struct RegistryMaker {
        template<typename... Args>
        auto Make(Registry* owner, Args&&... args) {
            //通过特化RegistryFactoryInject指定I对应的工厂
            using U = typename RegistryFactoryInject<I>::type;
            auto factory = Get<U>(owner);
            if (!factory) {
                throw std::runtime_error("invalid factory instance");
            }
            return factory->Make(std::forward<Args>(args)...);
        }
    };

    template<typename T>
    struct Getter<Registry, T> {
        T* Get(Registry* owner) {
            return RegistryGetter<T>{}.Get(owner);
        }
    };

    template<typename I>
    struct Maker<Registry, I> {
        template<typename... Args>
        auto Make(Registry* owner, Args&&... args) {
            return RegistryMaker<I>{}.Make(owner, std::forward<Args>(args)...);
        }
    };

    class Registry::Impl final {
    public:
        template<typename I>
        class Proxy :public IValue {
        public:
            Proxy() = default;

            template<typename T>
            explicit Proxy(T& v) :vp(&v), code(Index<T>()) {};

            I* get() noexcept { return vp; }
            std::size_t TypeIndex() const noexcept override { return code; }
        protected:
            I* vp{};
            std::size_t code{};
        };

        template<typename T>
        class Value final :public Proxy<T> {
            T v;
        public:
            template<typename... Args>
            explicit Value(Args&&... args)
                :v(std::forward<Args>(args)...)
            {
                vp = &v;
                code = Index<T>();
            }
        };

        static std::size_t GetIndex(const char* code);

        template<typename T>
        static std::size_t Index() {
            static auto r = GetIndex(typeid(T).name());
            return r;
        }
    };
    template<typename T, typename ...Args>
    inline T* Registry::emplace(Args && ...args)
    {
        static auto index = Impl::Index<T>();
        if (index >= m_entries.size()) {
            m_entries.resize(index + 1);
        }
        auto&& obj = m_entries[index];
        obj = std::make_unique<Impl::Value<T>>(std::forward<Args>(args)...);
        return static_cast<Impl::Value<T>*>(obj.get())->get();
    }

    template<typename T>
    inline void Registry::erase() noexcept
    {
        static auto index = Impl::Index<T>();
        if (index >= m_entries.size() || m_entries[index] == nullptr)
            return;

        auto realTypeIndex = m_entries[index]->TypeIndex();
        if (realTypeIndex == index) {
            //当T为真实实例时,移除真实实例涉及的所有代理
            for (auto&& o : m_entries) {
                if (!o) continue;
                if (o->TypeIndex() == realTypeIndex) {
                    o.reset();
                }
            }
        }
        else {
            //当T为代理类时,只移除代理
            m_entries[index].reset();
        }
    }

    template<typename T>
    inline T* Registry::get() const
    {
        static auto index = Impl::Index<T>();
        if (index >= m_entries.size())
            return nullptr;
        if (auto vp = dynamic_cast<Impl::Proxy<T>*>(m_entries[index].get())) {
            return vp->get();
        }
        return nullptr;
    }

    template<typename I, typename T>
    inline bool Registry::bind()
    {
        static_assert((!std::is_same<I, T>::value) && (std::is_base_of<I, T>::value),
            "I!=T and std::is_base_of<I,T>");
        static auto index = Impl::Index<I>();
        if (auto vp = get<T>()) {
            if (index >= m_entries.size()) {
                m_entries.resize(index + 1);
            }
            m_entries[index] = std::make_unique<Impl::Proxy<I>>(*vp);
            return true;
        }
        return false;
    }
}
