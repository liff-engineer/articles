#pragma once
#include <memory>
#include <unordered_map>
#include <string>

namespace abc
{
    //扩展点:当T没有派生自I时提供映射类型
    template<typename T, typename I, typename = void>
    struct FactoryInject;

    class FactoryBase {
    public:
        virtual ~FactoryBase() = default;
    protected:
        struct IArgument {
            virtual ~IArgument() = default;
        };

        template<typename T>
        struct Argument;
        
        template<>
        struct Argument<void>;

        template<typename T>
        struct ArgumentHolder;

        template<typename I>
        struct iGen;

        template<typename I>
        struct ObjGen {
            const char* argCode;
            std::unique_ptr<IArgument> arg;
            std::unique_ptr<I>(*op)(const IArgument&);

            template<typename Arg>
            std::unique_ptr<I> Make(const Argument<Arg>& v) const;

            std::unique_ptr<I> Make(const Argument<void>&) const;

            template<typename T, typename Arg>
            static ObjGen Create();
        };
    };

    template<typename I, typename K = std::string>
    class Factory :public FactoryBase {
    public:
        using Super = Factory;

        template<typename T, typename Arg = void>
        bool Register(K code) {
            auto result = m_builders.try_emplace(code, ObjGen<I>::Create<T, Arg>());
            return result.second;
        }

        template<typename T>
        std::unique_ptr<I> Make(const K& key, const T& v) const {
            return MakeImpl(key, Argument<T>{v});
        }

        std::unique_ptr<I> Make(const K& key) const {
            return MakeImpl(key, Argument<void>{});
        }

        template<typename Arg>
        bool SetDefaultArgument(const K& code, Arg&& arg) {
            auto it = m_builders.find(code);
            if (it != m_builders.end()) {
                if (std::strcmp(it->second.argCode, typeid(Arg).name()) == 0) {
                    it->second.arg = std::make_unique<ArgumentHolder<Arg>>(std::move(arg));;
                    return true;
                }
            }
            return false;
        }

        bool ResetDefaultArgument(const K& code) {
            auto it = m_builders.find(code);
            if (it != m_builders.end()) {
                it->second.arg.reset();
                return true;
            }
            return false;
        }
    protected:
        template<typename Arg>
        std::unique_ptr<I> MakeImpl(const K& key, const Argument<Arg>& arg) const {
            auto it = m_builders.find(key);
            if (it != m_builders.end())
                return it->second.Make(arg);
            return {};
        }
    protected:
        std::unordered_map<K, ObjGen<I>> m_builders;
    };

    class Registry final {
    public:
        static Registry* Get() {
            static Registry obj{};
            return &obj;
        }

        template<typename T, typename... Args>
        T* emplace(Args&&... args) {
            static auto index = Index<T>();
            if (index >= m_entries.size()) {
                m_entries.resize(index + 1);
            }
            auto&& obj = m_entries[index];
            obj = std::make_unique<Value<T>>(std::forward<Args>(args)...);
            return static_cast<Value<T>*>(obj.get())->get();
        }

        template<typename T>
        void erase() noexcept {
            static auto index = Index<T>();
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
        T* get() const {
            static auto index = Index<T>();
            if (index >= m_entries.size())
                return nullptr;
            if (auto vp = dynamic_cast<Proxy<T>*>(m_entries[index].get())) {
                return vp->get();
            }
            return nullptr;
        }

        void clear() {
            m_entries.clear();
        }

        template<typename I, typename T>
        bool bind() {
            static_assert((!std::is_same<I, T>::value) && (std::is_base_of<I, T>::value),
                "I!=T and std::is_base_of<I,T>");
            static auto index = Index<I>();
            if (auto vp = get<T>()) {
                if (index >= m_entries.size()) {
                    m_entries.resize(index + 1);
                }
                m_entries[index] = std::make_unique<Proxy<I>>(*vp);
                return true;
            }
            return false;
        }
    private:
        template<typename T>
        static std::size_t Index() {
            static auto r = GetIndex(typeid(T).name());
            return r;
        }
        static std::size_t GetIndex(const char* code) {
            static std::vector<std::string> codes;
            for (std::size_t i = 0; i < codes.size(); i++) {
                if (codes[i] == code) {
                    return i;
                }
            }
            codes.emplace_back(code);
            return codes.size() - 1;
        }

        class IValue {
        public:
            virtual ~IValue() = default;
            virtual std::size_t TypeIndex() const noexcept = 0;
        };

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

        std::vector<std::unique_ptr<IValue>> m_entries;
    };

    template<typename Owner,typename T,typename = void>
    struct Getter;

    template<typename Owner,typename I,typename = void>
    struct Maker;

    template<typename T>
    struct Getter<Registry, T, std::enable_if_t<std::is_constructible<T>::value>> {
        T* Get(Registry* owner) {
            if (auto vp = owner->get<T>()) return vp;
            return owner->emplace<T>();
        }
    };

    template<typename T>
    struct Getter<Registry, T, std::enable_if_t<!std::is_constructible<T>::value>> {
        T* Get(Registry* owner) {
            return owner->get<T>();
        }
    };

    template<typename T>
    decltype(auto) Get() {
        return Getter<Registry, T>{}.Get(Registry::Get());
    }

    template<typename T, typename Owner>
    decltype(auto) Get(Owner owner) {
        using U = std::remove_pointer_t<std::remove_const_t<Owner>>;
        return Getter<U, T>{}.Get(owner);
    }

    template<typename I,typename... Args>
    decltype(auto) Make(Args&&... args) {
        return Maker<Registry, I>{}.Make(Registry::Get(),std::forward<Args>(args)...);
    }

    template<typename I, typename Owner, typename... Args>
    decltype(auto) MakeBy(Owner owner, Args&&... args) {
        using U = std::remove_pointer_t<std::remove_const_t<Owner>>;
        return Maker<U, I>{}.Make(owner,std::forward<Args>(args)...);
    }
}

#include "Registry.inl"
