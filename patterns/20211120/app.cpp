#include "publisher.hpp"
#include <iostream>
#include <string>
#include "observer.hpp"

/// @brief 针对Subject提供特化实现
template<>
struct subscribe_helper<Subject> {
    
    /// @brief 提供IObserver实现
    /// @tparam Fn 
    template<typename Fn>
    class Observer final :public IObserver {
        Fn m_action;
    public:
        explicit Observer(Fn&& fn) :m_action(std::move(fn)) {}

        void Update(Payload const& msg) override {
            m_action(msg);
        }
    };

    /// @brief subscribe_helper必须提供该静态函数实现
    template<typename Fn>
    static subscribe_stub_unit subscribe(Subject& source, Fn&& fn) {
        //构造观察者
        auto handler = std::make_shared<Observer<Fn>>(std::move(fn));
        //添加观察者
        source.Attach(handler.get());
        //返回函数,注意函数执行时会移除观察者
        return[src = std::addressof(source), h = std::move(handler)]() {
            src->Detach(h.get());
        };
    }
};

struct Actor {
    subscribe_stub stub;

    void launch(publisher& source) {
        stub += subscribe(source.channel<bool>(), [](auto arg) {
            std::cout << "bool:" << std::boolalpha << arg << "\n";
            });
        stub += subscriber(*this).subscribe<double,std::string>(source);
            //.subscribe(source.channel<std::string>());
    }

    void launch(Subject& source) {
        stub += subscriber(*this).subscribe(source,
            [](auto obj, Payload const& arg) {
                std::cout << "Actor>>Payload:(" << arg.iV << "," << arg.dV << ")\n";
            });
    }

    void on(std::string const& msg) const {
        std::cout << "msg:" << msg << "\n";
    }

    void on(double dV) {
        std::cout << "dV:" << dV << "\n";
    }
};

int main(int argc, char** argv) {
    //定义消息源
    publisher source{};

    {//基本用法
        //订阅
        auto stub = subscribe(source.channel<int>(), [](auto arg) {
            std::cout << "int:" << arg << "\n";
            });

        //发布消息
        source.publish(19);
        source.publish(20);

        //取消订阅
        stub.unsubscribe();

        source.publish(21);
    }

    {//作用域退出引发存根析构,自动取消订阅
        auto stub = subscribe(source.channel<int>(), [](auto arg) {
            std::cout << "int:" << arg << "\n";
            });
        source.publish(22);
    }
    source.publish(23);

    //消息处理类的用法
    Actor actor{};
    actor.launch(source);

    source.publish(false);
    source.publish(1.414);
    source.publish(std::string{ "liff.engineer@gmail.com" });

    //自定义消息源的使用
    Subject subject{};
    actor.launch(subject);
    {
        auto stub = subscribe(subject, [](Payload const& arg) {
            std::cout << "Payload:(" << arg.iV << "," << arg.dV << ")\n";
            });

        subject.Notify(Payload{ 1,1.1 });
    }
    subject.Notify(Payload{ 2,2.2 });

    //注意actor中的订阅存根会自动取消订阅,
    //如果不手动取消,则要确保消息源生命周期超过actor
    //否则取消订阅时Subject已经不存在,从而引发崩溃.
    actor.stub.unsubscribe();
    return 0;
}
