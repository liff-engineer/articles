/// 模型一致性保证的层次化处理
/// 1. 类级别:通过成员函数实现
/// 2. 文档级别:使用观察者模式
/// 3. 应用级别:使用发布订阅模式
/// 
/// 对于文档内部的一致性:
/// > 内容一旦变化,通过notify发出消息
/// > 观察者观察到变化消息后执行update
/// > 应用程序启动时观察者就需要注册到notifyer上
/// >> 由于有严格的一致性约定,观察者属于文档内部,
///    由文档的Notifyer来管理
/// 
/// 对于应用内部的一致性,比较典型的是文档和界面:
/// > 需要界面联动的,通过publish发出消息
/// > 订阅者接受到消息后响应
/// > 考虑到文档主动变化,界面被动变化,订阅者能够识别自身状态
///   因而,可以随时订阅与取消订阅
/// >> 订阅者需要处理自身的生命周期,notifyer只观察

#pragma once
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace abc
{
    class SubscribeStub {
        std::vector<std::function<void()>> m_actions;
    public:
        SubscribeStub() = default;
        ~SubscribeStub() noexcept {
            try { release(); }
            catch (...) {};
        }

        SubscribeStub(SubscribeStub const&) = delete;
        SubscribeStub& operator=(SubscribeStub const&) = delete;

        SubscribeStub(SubscribeStub&& other) noexcept
            :m_actions(std::move(other.m_actions)) {};
        SubscribeStub& operator=(SubscribeStub&& other) noexcept {
            if (std::addressof(other) != this) {
                release();
                m_actions = std::move(other.m_actions);
            }
            return *this;
        }

        template<typename Fn>
        SubscribeStub& operator+=(Fn&& fn) noexcept {
            m_actions.emplace_back(std::move(fn));
            return *this;
        }

        void release() {
            for (auto& op : m_actions) {
                if (op) { op(); }
            }
            m_actions.clear();
        }
    };

    class Publisher {
        struct ISubscriber {
            struct IPayload {
                virtual ~IPayload() = default;
            };

            template<typename T>
            struct Payload final : IPayload {
                const T* vp;
                explicit Payload(const T& obj) :vp(std::addressof(obj)) {};
            };
        public:
            template<typename E>
            void on(const E& e) { onImpl(Payload<E>{e}); }
        protected:
            virtual void onImpl(const IPayload& payload) const = 0;
        };

        template<typename T, typename E>
        struct Subscriber final :public ISubscriber {
            T* vp;
            explicit Subscriber(T& obj) :vp(std::addressof(obj)) {};
        protected:
            void onImpl(const IPayload& payload) const override {
                if (auto op = dynamic_cast<const Payload<E>*>(std::addressof(payload))) {
                    vp->on(*op->vp);
                }
            }
        };

        struct Stub {
            std::string code;
            std::weak_ptr<ISubscriber> subscriber;
        };
        std::vector<Stub> m_stubs;
    public:
        template<typename E>
        void publish(const E& e) {
            static const auto code = typeid(E).name();
            for (auto&& o : m_stubs) {
                if (o.code == code) {
                    if (auto h = o.subscriber.lock()) {
                        h->on(e);
                    }
                }
            }
        }

        template<typename... Es, typename T>
        SubscribeStub subscribe(T& obj) {
            SubscribeStub stub;
            subscribeImpl<Es...>(obj, stub);
            return stub;
        }
    private:
        template<typename E, typename... Es, typename T>
        void subscribeImpl(T& obj, SubscribeStub& stub) {
            auto handler = std::make_shared<Subscriber<T, E>>(obj);
            m_stubs.emplace_back(Stub{ typeid(E).name(),handler });
            stub += [h = std::move(handler)](){};
            if constexpr (sizeof...(Es) > 0) {
                subscribeImpl<Es...>(obj, stub);
            }
        }
    };

    template<typename M, typename T, typename E, typename = void>
    struct Observer;

    template<typename M, typename E>
    class IObserver {
    public:
        virtual ~IObserver() = default;
        virtual const std::string& code() const noexcept = 0;
        virtual void update(M& m, const E&) = 0;
    };

    //采用Observer模板的实现
    template<typename M, typename T, typename E>
    class TObserver final : public IObserver<M, E> {
        Observer<M, T, E> m_observer;
    public:
        template<typename... Args>
        explicit TObserver(Args&&... args)
            :m_observer(std::forward<Args>(args)...) {};

        const std::string& code() const noexcept {
            static const std::string key = typeid(T).name();
            return key;
        }

        void update(M& m, const E& e) {
            return m_observer.update(m, e);
        }
    };

    template<typename M>
    class Notifyer: public Publisher {
        struct IChannel {
            virtual ~IChannel() = default;
        };

        template<typename E>
        struct SubjectChannel final : public IChannel {
            std::unordered_map<std::string, std::unique_ptr<IObserver<M, E>>>
                observers;

            void notify(M& m, const E& e) const {
                for (auto&& [code, observer] : observers) {
                    observer->update(m, e);
                }
            }

            void insert(std::unique_ptr<IObserver<M, E>> observer) {
                observers[observer->code()] = std::move(observer);
            }
        };

        std::unordered_map<std::string, std::unique_ptr<IChannel>> m_channels;
    public:
        template<typename E>
        void notify(M& owner, const E& e) const {
            if (auto it = m_channels.find(typeid(E).name()); it != m_channels.end()) {
                if (auto subject = dynamic_cast<SubjectChannel<E>*>(it->second.get())) {
                    subject->notify(owner, e);
                }
            }
        }

        template<typename E,typename O>
        void registerObserver(O&& observer)
        {
            static const auto code = typeid(E).name();
            if (auto it = m_channels.find(code); it != m_channels.end()) {
                if (auto subject = dynamic_cast<SubjectChannel<E>*>(it->second.get())) {
                    subject->insert(std::move(observer));
                }
            }
            else {
                auto subject = std::make_unique<SubjectChannel<E>>();
                subject->insert(std::move(observer));
                m_channels[code] = std::move(subject);
            }
        }

        template<typename T, typename E, typename... Args>
        void registerObserver(Args&&... args) {
            registerObserver<E>(std::make_unique<TObserver<M, T, E>>(
                std::forward<Args>(args)...));
        }
    };
}
