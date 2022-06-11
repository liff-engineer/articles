/// 工厂实现支持库
/// 1. 基本的派生类注册,通过key构造接口实例支持;
/// 2. 支持为某key设置构造参数类型,可设置默认参数或构造时传入;
/// 3. 提供FactoryBase模板类,以支持自定义;
/// 4. 提供常规的工厂Factory;
/// 5. 提供Factory2L工厂以支持别名场景(注册用K1+K2,构造用K1,K1单向映射到K2);
/// 6. 支持非派生类注册(通过包装类形成派生关系)

#pragma once
#include <memory>
#include <unordered_map>
#include <cstring>

namespace abc {

    template<typename T,typename I,typename = void>
    struct IFactoryInject;

    class IFactory {
    public:
        virtual ~IFactory() = default;
    protected:
        struct IArgument {
            virtual ~IArgument() = default;
        };

        template<typename I>
        struct ObjGen {
            const char* argCode;
            std::unique_ptr<IArgument> arg;
            std::unique_ptr<I>(*op)(const IArgument&);
        };

        struct Impl;
    };

    template<typename I, typename K>
    class FactoryBase :public IFactory {
    protected:
        template<typename T, typename Arg>
        bool RegisterImpl(K key);

        template<typename Arg>
        std::unique_ptr<I> MakeImpl(const K& key, const Arg& arg) const;

        std::unique_ptr<I> MakeImpl(const K& key) const;

        template<typename Arg>
        bool SetArgumentImpl(const K& key, Arg&& arg);

        bool ResetArgumentImpl(const K& key);
    protected:
        std::unordered_map<K, ObjGen<I>> m_builders;
    };

    template<typename I, typename K>
    class Factory :public FactoryBase<I, K> {
    public:
        using Super = Factory;

        template<typename T, typename Arg = void>
        bool Register(K key) {
            return this->RegisterImpl<T, Arg>(key);
        }

        template<typename Arg>
        std::unique_ptr<I> Make(const K& key, const Arg& v) const {
            return this->MakeImpl(key, v);
        }

        std::unique_ptr<I> Make(const K& key) const {
            return this->MakeImpl(key);
        }

        template<typename Arg>
        bool SetDefaultArgument(const K& key, Arg&& arg) {
            return this->SetArgumentImpl(key, std::forward<Arg>(arg));
        }

        bool ResetDefaultArgument(const K& key) {
            return this->ResetArgumentImpl(key);
        }
    };

    /// 双层Key的工厂:K1用来找到K2,然后用K2进行操作
    template<typename I, typename K1, typename K2 = K1>
    class Factory2L :public FactoryBase<I, K2> {
    public:
        using Super = Factory2L;

        template<typename T, typename Arg = void>
        bool Register(K1 k1, K2 k2) {
            if (this->RegisterImpl<T, Arg>(k2)) {
                m_implements[k1] = k2;
                return true;
            }
            return false;
        }

        template<typename Arg>
        std::unique_ptr<I> Make(const K1& key, const Arg& v) const {
            if (auto it = this->m_implements.find(key); it != this->m_implements.end()) {
                return this->MakeImpl(it->second, v);
            }
            return {}；
        }

        std::unique_ptr<I> Make(const K1& key) const {
            if (auto it = this->m_implements.find(key); it != this->m_implements.end()) {
                return this->MakeImpl(it->second);
            }
            return {}；
        }

        template<typename Arg>
        bool SetDefaultArgument(const K1& key, Arg&& arg) {
            if (auto it = this->m_implements.find(key); it != this->m_implements.end()) {
                return this->SetArgumentImpl(it->second, std::forward<Arg>(arg));
            }
            return false;
        }

        bool ResetDefaultArgument(const K1& key) {
            if (auto it = this->m_implements.find(key); it != this->m_implements.end()) {
                return this->ResetArgumentImpl(it->second);
            }
            return false;
        }
    protected:
        std::unordered_map<K1, K2> m_implements;
    };

    ////////////////////////////////////////////////////////////////
    ///以下为实现相关,使用者无需关注
    ////////////////////////////////////////////////////////////////

    class IFactory::Impl {
    public:
        template<typename T>
        struct Proxy : IArgument {
            Proxy() = default;
            explicit Proxy(const T& v) :vp(&v) {};

            const T* vp{};
        };

        template<>
        struct Proxy<void> :IArgument {
            Proxy() = default;
        };

        template<typename T>
        struct Argument : Proxy<T> {

            template<typename... Args>
            explicit Argument(Args&&... args)
                :v(std::forward<Args>(args)...) {
                vp = &v;
            }

            T v;
        };
    public:
        template<typename I, typename T, typename Arg>
        static ObjGen<I> ObjGenOf() {
            std::unique_ptr<IArgument> arg{};
            if constexpr (std::is_same_v<Arg, void>) {
                arg = std::make_unique<Proxy<Arg>>();
            }

            return ObjGen<I>{typeid(Arg).name(), std::move(arg),
                [](const IArgument& arg)->std::unique_ptr<I> {
                    using U = std::conditional_t<std::is_base_of_v<I, T>, T, IFactoryInject<T, I>::type>;
                    if constexpr (std::is_same_v<Arg, void>) {
                        return std::make_unique<U>();
                    }
                    else
                    {
                        return std::make_unique<U>(*(static_cast<const Proxy<Arg>*>(&arg)->vp));
                    }
                }
            };
        }

        template<typename I, typename Arg>
        static std::unique_ptr<I> Make(const ObjGen<I>& gen, const Proxy<Arg>& arg) {
            if constexpr (std::is_same_v<Arg, void>) {
                if (gen.arg) return gen.op(*gen.arg.get());
                return {};
            }
            else {
                static const auto code = typeid(Arg).name();
                if (std::strcmp(code, gen.argCode) != 0) return {};
                return gen.op(arg);
            }
        }
    };


    template<typename I, typename K>
    template<typename T, typename Arg>
    inline bool FactoryBase<I, K>::RegisterImpl(K key)
    {
        return m_builders.try_emplace(key, Impl::ObjGenOf<I, T, Arg>()).second;
    }

    template<typename I, typename K>
    template<typename Arg>
    inline std::unique_ptr<I> FactoryBase<I, K>::MakeImpl(const K& key, const Arg& arg) const
    {
        if (auto it = m_builders.find(key); it != m_builders.end()) {
            return Impl::Make(it->second, Impl::Proxy<Arg>(arg));
        }
        return {};
    }


    template<typename I, typename K>
    inline std::unique_ptr<I> FactoryBase<I, K>::MakeImpl(const K& key) const
    {
        if (auto it = m_builders.find(key); it != m_builders.end()) {
            return Impl::Make(it->second, Impl::Proxy<void>{});
        }
        return {};
    }

    template<typename I, typename K>
    template<typename Arg>
    inline bool FactoryBase<I, K>::SetArgumentImpl(const K& key, Arg&& arg)
    {
        if (auto it = m_builders.find(key); it != m_builders.end()) {
            if (std::strcmp(it->second.argCode, typeid(Arg).name()) == 0) {
                it->second.arg = std::make_unique<Impl::Argument<Arg>>(std::move(arg));
                return true;
            }
        }
        return false;
    }


    template<typename I, typename K>
    inline bool FactoryBase<I, K>::ResetArgumentImpl(const K& key)
    {
        if (auto it = m_builders.find(key); it != m_builders.end()) {
            it->second.arg.reset();
            return true;
        }
        return false;
    }
}
