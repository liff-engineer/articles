///  用来支持复杂界面交互的支持库
///  >>> 复杂界面交互可以拆分为:界面、流程、业务三大部分
///  >>> 1. 界面负责输入与显示;
///  >>> 2. 业务负责完成具体业务逻辑;
///  >>> 3. 流程负责统筹界面、业务,以表达用户行为/场景逻辑.
///  
///  这里提供如下支持:
///  > 以Actor为核心抽象,由输入消息触发执行;
///  > 通过Broker建立Actor之间消息流动关系;
///  > Broker支持发布订阅、请求回复、桥接三种模式,并可跟踪运行全过程;
///  > 提供Actor抽象工厂支持,用来存储不同类型的Actor(界面、业务其构造方式不同) 
/// 
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
#include <chrono>

namespace abc
{
    class Broker final {
    public:
        std::string description;
        Broker();

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
    public:
        struct Action {
            const Broker* broker;//触发时的消息中间人
            int    type;//0:enter;1:leave
            std::uintptr_t handler;//handler地址,用来区分不同handler实例
            const char* handlerCode;//记录handler类型信息
            const char* message;//记录消息类型信息
            std::chrono::high_resolution_clock::time_point timepoint;//时间戳
        };

        using Reporter = std::function<void(const Action&)>;

        static Reporter RegisterReporter(Reporter&& handler);
    private:
        class Tracer {
            Action log;
        public:
            Tracer(const Broker* source, std::uintptr_t address,const char* handlerCode, const char* code);
            template<typename T>
            Tracer(const Broker* source, const T* obj, const char* code)
                :Tracer(source, (std::uintptr_t)obj, typeid(T).name() , code) {};

            ~Tracer() noexcept;
        };

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
            void handle(T& obj) { return obj.reply(*arg, *result); }
        };

        template<typename R>
        struct Payload<void, R> final : IPayload {
            R* result;
            explicit Payload(R& result_) :result(std::addressof(result_)) {};

            template<typename T>
            void handle(T& obj) { return obj.reply(*result); }
        };

        template<typename E>
        struct Payload<E, void> final : IPayload {
            const E* arg;
            explicit Payload(const E& arg_) :arg(std::addressof(arg_)) {};

            template<typename T>
            void handle(T& obj) { return obj.on(*arg); }
        };

        struct IMessageHandler {
            virtual void handle(const Broker* source, IPayload& payload, const char* code) const = 0;
        };

        template<typename T, typename E, typename R>
        struct Handler final :IMessageHandler {
            T* obj;
            explicit Handler(T& o) :obj(std::addressof(o)) {};

            void handle(const Broker* source, IPayload& payload, const char* code) const override {
                static const char* codeReq = typeid(Payload<E, R>).name();
                Tracer log{ source,obj,code };
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

            void handle(const Broker* source, IPayload& payload, const char* code) const override {
                Tracer log{ source,obj,code };
                obj->handle(source, payload, code);
            }
        };

        struct HandlerStub {
            const char* code;
            bool hub;
            std::weak_ptr<IMessageHandler> handler;

            inline bool accept(const char* tCode) { return hub ? true : (std::strcmp(code, tCode) == 0); }
        };

        std::vector<HandlerStub> m_stubs;
    private:
        template<typename T, typename R>
        void handle(Payload<T, R>& payload) {
            handle(this, payload, typeid(Payload<T, R>).name());
        }
        void handle(const Broker* source, IPayload& payload, const char* code);

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

    template<typename R, typename K, typename... Ts>
    class Factory {
        std::unordered_map<K, std::function<R(Ts...)>> m_factories;
    public:
        template<typename T>
        bool contains(const T& code) const noexcept {
            return m_factories.find(code) != m_factories.end();
        }

        template<typename... Args>
        R make(const K& code, Args&&... args) const {
            return m_factories.at(code)(std::forward<Args>(args)...);
        }

        template<typename Fn>
        void emplace(K code, Fn&& fn) { m_factories[std::move(code)] = std::move(fn); }
    };

    class IActor {
    public:
        virtual ~IActor() = default;
        virtual void launch(Broker& broker) = 0;
    };

    class ActorFactory :public Factory<std::unique_ptr<IActor>, std::string> {};
}
