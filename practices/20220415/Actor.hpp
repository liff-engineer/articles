#pragma once
#include <type_traits>
#include <typeinfo>
#include <cstring>
#include <memory>
#include <vector>
#include <cassert>
#include <string>
#include <unordered_map>

namespace abc
{
    class Broker final {

        struct IPayload {
            virtual ~IPayload() = default;
        };

        template<typename E, typename R>
        struct Payload final : IPayload {
            const E* arg;
            R* result;

            explicit Payload(const E& arg_, R& result_)
                :arg(std::addressof(arg_)), result(std::addressof(result_)) {};

            template<typename T>
            void Handle(T* obj) { return obj->Reply(*arg, *result); }
        };

        template<typename E>
        struct Payload<E, void> : IPayload {
            const E* arg;
            explicit Payload(const E& arg_) :arg(std::addressof(arg_)) {};

            template<typename T>
            void Handle(T* obj) { return obj->On(*arg); }
        };

        template<typename R>
        struct Payload<void, R> : IPayload {
            R* result;
            explicit Payload(R& result_) :result(std::addressof(result_)) {};

            template<typename T>
            void Handle(T* obj) { return obj->Reply(*result); }
        };

        struct IMessageHandler {
            virtual void Handle(IPayload& payload, const char* code) const = 0;
        };

        template<typename T, typename E, typename R>
        struct Handler final :IMessageHandler {
            T* obj;
            explicit Handler(T& o) :obj(std::addressof(o)) {};

            void Handle(IPayload& payload, const char* code) const override {
                static const char* codeReq = typeid(Payload<E, R>).name();
                if (std::strcmp(codeReq, code) != 0) {
                    assert(false);
                    //传递的参数有问题,不满足要求
                    return;
                }
                if (auto op = dynamic_cast<Payload<E, R>*>(&payload)) {
                    op->Handle(obj);
                }
            }
        };

        template<typename T>
        struct HubHandler final :IMessageHandler {
            T* obj;
            explicit HubHandler(T& o) :obj(std::addressof(o)) {};

            void Handle(IPayload& payload, const char* code) const override {
                obj->Handle(payload, code);
            }
        };

        struct HandlerStub {
            const char* code;
            bool hub;
            std::weak_ptr<IMessageHandler> handler;

            inline bool Accept(const char* tCode) {
                if (hub) return true;
                return std::strcmp(code, tCode) == 0;
            }
        };

        std::vector<HandlerStub> m_stubs;
        unsigned m_concurrentCount;
    public:
        Broker();

        template<typename T>
        void Publish(const T& msg) { Handle(Payload<T, void>{msg}); }

        template<typename T, typename R>
        void Request(const T& msg, R& result) { Handle(Payload<T, R>{msg, result}); }

        template<typename R>
        void Request(R& result) { Handle(Payload<void, R>{result}); }


        template<typename E, typename T>
        auto Subscribe(T& obj) { return AddHandler<E, void>(obj); }

        template<typename E, typename R, typename T>
        auto Bind(T& obj) { return AddHandler<E, R>(obj); }

        template<typename R, typename T>
        auto Bind(T& obj) { return AddHandler<void, R>(obj); }

        template<typename T>
        auto Connect(T& obj) { return AddHubHandler(obj); }
    private:
        template<typename T, typename R>
        void Handle(Payload<T, R>& payload) {
            Handle(payload, typeid(Payload<T, R>).name());
        }
        void Handle(IPayload& payload, const char* code);
    private:
        template<typename E, typename R, typename T>
        std::shared_ptr<IMessageHandler> AddHandler(T& obj) {
            auto handler = std::make_shared<Handler<T, E, R>>(obj);
            m_stubs.emplace_back(HandlerStub{ typeid(Payload<E,R>).name(),false,handler });
            return handler;
        }

        template<typename T>
        std::shared_ptr<IMessageHandler> AddHubHandler(T& obj) {
            auto handler = std::make_shared<HubHandler<T>>(obj);
            m_stubs.emplace_back(HandlerStub{ typeid(HubHandler<T>).name(),true,handler });
            return handler;
        }
    };

    class Registry {
        struct IEntry {
            virtual ~IEntry() = default;
            virtual void* Address() noexcept = 0;
        };

        template<typename T>
        struct Instance final :public IEntry {
            T obj;
            explicit Instance(T&& o) :obj(std::move(o)) {};

            template<typename... Args>
            void Replace(Args&&... args) {
                obj = std::move(T{ std::forward<Args>(args)... });
            }

            void* Address() noexcept final override { return std::addressof(obj); }
        };

        struct Entry {
            const char* code;
            std::shared_ptr<IEntry> instance;

            template<typename T>
            inline bool Is() const noexcept { return std::strcmp(code, typeid(T).name()) == 0; }
        };

        std::vector<Entry> m_entries;
    public:
        template<typename T, typename... Args>
        T* Emplace(Args&&... args) {
            for (auto&& o : m_entries) {
                if (!o.Is<T>()) continue;
                if (auto vp = dynamic_cast<Instance<T>*>(o.instance.get())) {
                    vp->Replace(std::forward<Args>(args)...);
                    return std::addressof(vp->obj);
                }
            }

            auto h = std::make_shared<Instance<T>>(T{ std::forward<Args>(args)... });
            auto result = std::addressof(h->obj);
            m_entries.emplace_back(Entry{ typeid(Instance<T>).name(),h });
            return result;
        }

        template<typename T>
        bool Erase(T* pointer) noexcept {
            for (auto&& o : m_entries) {
                if (o.instance && o.instance->Address() == pointer) {
                    o.instance.reset();
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        bool Erase() noexcept {
            for (auto&& o : m_entries) {
                if (o.Is<T>()) {
                    o.instance.reset();
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        T* Find() const noexcept {
            for (auto&& o : m_entries) {
                if (o.Is<T>() && o.instance) {
                    if (auto vp = dynamic_cast<Instance<T>*>(o.instance.get())) {
                        return std::addressof(vp->obj);
                    }
                }
            }
            return nullptr;
        }

        void Clear();
    };

    template<typename I, typename K, typename... Ts>
    class Factory {
        struct IBuilder {
            virtual ~IBuilder() = default;
            virtual std::unique_ptr<I> Make(Ts&&... args) const = 0;
        };

        template<typename Fn>
        struct Builder final :IBuilder {
            Fn fn;
            explicit Builder(Fn&& f) :fn(std::move(f)) {};
            std::unique_ptr<I> Make(Ts&&... args) const override {
                return fn(std::forward<Ts>(args)...);
            }
        };

        std::unordered_map<K, std::unique_ptr<IBuilder>> m_factories;
    public:
        template<typename... Args>
        std::unique_ptr<I> Make(const K& code, Args&&... args) {
            if (auto it = m_factories.find(code); it != m_factories.end()) {
                return it->second->Make(std::forward<Args>(args)...);
            }
            return nullptr;
        }

        template<typename Fn>
        void RegisterFactory(K code, Fn&& fn) {
            m_factories[std::move(code)] = std::make_unique<Builder<Fn>>(std::move(fn));
        }
    };

    class IActor {
    public:
        virtual ~IActor() = default;
        virtual void Launch(Registry& registry, Broker& broker) = 0;
    };

    class ActorFactory :public Factory<IActor, std::string> {};
}
