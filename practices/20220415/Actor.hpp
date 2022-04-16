#pragma once
#include <type_traits>
#include <typeinfo>
#include <cstring>
#include <memory>
#include <vector>
#include <cassert>
#include <string>
#include <unordered_map>
#include <functional>

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
            void handle(T& obj) { return obj.Reply(*arg, *result); }
        };

        template<typename R>
        struct Payload<void, R> final : IPayload {
            R* result;
            explicit Payload(R& result_) :result(std::addressof(result_)) {};

            template<typename T>
            void handle(T& obj) { return obj.Reply(*result); }
        };

        template<typename E>
        struct Payload<E, void> final : IPayload {
            const E* arg;
            explicit Payload(const E& arg_) :arg(std::addressof(arg_)) {};

            template<typename T>
            void handle(T& obj) { return obj.On(*arg); }
        };

        struct IMessageHandler {
            virtual void handle(IPayload& payload, const char* code) const = 0;
        };

        template<typename T, typename E, typename R>
        struct Handler final :IMessageHandler {
            T* obj;
            explicit Handler(T& o) :obj(std::addressof(o)) {};

            void handle(IPayload& payload, const char* code) const override {
                static const char* codeReq = typeid(Payload<E, R>).name();
                if (std::strcmp(code, codeReq) == 0) {
                    if (auto op = dynamic_cast<Payload<E, R>*>(&payload)) {
                        op->handle(*obj);
                    }
                }
                else {
                    assert(false);
                }
            }
        };

        template<typename T>
        struct HubHandler final :IMessageHandler {
            T* obj;
            explicit HubHandler(T& o) :obj(std::addressof(o)) {};

            void handle(IPayload& payload, const char* code) const override { obj->handle(payload, code);}
        };

        struct HandlerStub {
            const char* code;
            bool hub;
            std::weak_ptr<IMessageHandler> handler;

            inline bool accept(const char* tCode) { return hub ? true : (std::strcmp(code, tCode) == 0); }
        };

        std::vector<HandlerStub> m_stubs;
        unsigned m_concurrentCount{};
    public:
        Broker();

        static std::function<void(void*, const char*)>& logger();

        template<typename T>
        void publish(const T& msg) { handle(Payload<T, void>{msg}); }

        template<typename T, typename R>
        void request(const T& msg, R& result) { handle(Payload<T, R>{msg, result}); }

        template<typename R>
        void request(R& result) { handle(Payload<void, R>{result}); }


        template<typename E, typename T>
        auto subscribe(T& obj) { return addHandler<E, void>(obj); }

        template<typename E, typename R, typename T>
        auto bind(T& obj) { return addHandler<E, R>(obj); }

        template<typename R, typename T>
        auto bind(T& obj) { return addHandler<void, R>(obj); }

        template<typename T>
        auto connect(T& obj) { return addHubHandler(obj); }
    private:
        template<typename T, typename R>
        void handle(Payload<T, R>& payload) {
            handle(payload, typeid(Payload<T, R>).name());
        }
        void handle(IPayload& payload, const char* code);
    private:
        template<typename E, typename R, typename T>
        std::shared_ptr<IMessageHandler> addHandler(T& obj) {
            auto handler = std::make_shared<Handler<T, E, R>>(obj);
            m_stubs.emplace_back(HandlerStub{ typeid(Payload<E,R>).name(),false,handler });
            return handler;
        }

        template<typename T>
        std::shared_ptr<IMessageHandler> addHubHandler(T& obj) {
            auto handler = std::make_shared<HubHandler<T>>(obj);
            m_stubs.emplace_back(HandlerStub{ typeid(HubHandler<T>).name(),true,handler });
            return handler;
        }
    };

    class Registry {
        struct IEntry {
            virtual ~IEntry() = default;
            virtual void* address() noexcept = 0;
        };

        template<typename T>
        struct Instance final :public IEntry {
            T obj;
            explicit Instance(T&& o) :obj(std::move(o)) {};

            void* address() noexcept final override { return std::addressof(obj); }
        };

        struct Entry {
            const char* code;
            std::shared_ptr<IEntry> instance;

            template<typename T>
            inline bool is() const noexcept { return std::strcmp(code, typeid(T).name()) == 0; }
        };

        std::vector<Entry> m_entries;
    public:
        template<typename T, typename... Args>
        T* emplace(Args&&... args) {
            if (auto vp = find<T>()) {
                *vp = std::move(T{ std::forward<Args>(args)... });
                return vp;
            }
            auto h = std::make_shared<Instance<T>>(T{ std::forward<Args>(args)... });
            auto result = std::addressof(h->obj);
            m_entries.emplace_back(Entry{ typeid(T).name(),h });
            return result;
        }

        template<typename T>
        bool erase(T* pointer) noexcept {
            for (auto&& o : m_entries) {
                if (o.instance && o.instance->address() == pointer) {
                    o.instance.reset();
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        bool erase() noexcept {
            for (auto&& o : m_entries) {
                if (o.is<T>()) {
                    o.instance.reset();
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        std::enable_if_t<std::is_pointer<T>::value, T> at() const noexcept {
            return find<std::remove_pointer_t<T>>();
        }

        template<typename T>
        std::enable_if_t<!std::is_pointer<T>::value, T> at() const {
            if (auto vp = find<T>()) {
                return *vp;
            }
            throw std::invalid_argument("cann't find require type value in registry!");
        }

        void clear();
    protected:
        template<typename T>
        T* find() const noexcept {
            for (auto&& o : m_entries) {
                if (o.is<T>() && o.instance) {
                    if (auto vp = dynamic_cast<Instance<T>*>(o.instance.get())) {
                        return std::addressof(vp->obj);
                    }
                }
            }
            return nullptr;
        }
    };

    template<typename I, typename K, typename... Ts>
    class Factory {
        std::unordered_map<K, std::function<std::unique_ptr<I>(Ts&&...)>> m_factories;
    public:
        template<typename... Args>
        std::unique_ptr<I> make(const K& code, Args&&... args) {
            if (auto it = m_factories.find(code); it != m_factories.end()) {
                return it->second->make(std::forward<Args>(args)...);
            }
            return nullptr;
        }

        template<typename Fn>
        void registerMaker(K code, Fn&& fn) { m_factories[std::move(code)] = std::move(fn); }
    };

    class IActor {
    public:
        virtual ~IActor() = default;
        virtual void launch(Registry& registry, Broker& broker) = 0;
    };

    class ActorFactory :public Factory<IActor, std::string> {};
}
