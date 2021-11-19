# 通用消息系统的设计

在面向对象设计中,为了让对象之间能够传递消息,开发者会采用各种技术,例如监听者/观察者、`Qt`的信号与槽、事件机制.开发者在使用相应技术时,经常会面临各种问题:

- 耦合,如果使用则需要派生自基类;
- 手动生命周期管理,需要注册订阅者到消息源,还要寻找时机取消注册;
- 较差的扩展性,新增消息时要重新设计一套基类,或做出较大调整;
- 较差的复用性,重复地实现类似的代码.

这里展示一种通用的消息系统,提供以下特性来满足开发者单向消息传递的需求:

- 低耦合,仅有消息类型定义依赖;
- 自动生命周期管理,提供订阅存根,可在析构时自动取消订阅,不会出现野指针等情况;
- 较高的扩展性,只需新增消息类型,即可使用,无需其它修改;
- 较高的复用性,单一消息源支持任意消息类型,无需重写;
- 可组合,无论是消息源还是订阅者均可以任意组合;
- 统一、简洁的`API`设计;
- 支持将现有监听者/观察者等实现整合进该系统.

## 使用示例

基本用法如下:

```c++
//定义消息源
publisher source{};
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
```

上述代码输出内容如下:

```bash
int:19
int:20
```

基于作用域的自动取消订阅示例如下:

```C++
//定义消息源
publisher source{};
{//作用域退出引发存根析构,自动取消订阅
    auto stub = subscribe(source.channel<int>(), [](auto arg) {
        std::cout << "int:" << arg << "\n";
    });
    source.publish(22);
}
source.publish(23);
```

上述代码输出内容如下:

```bash
int:22
```

针对能够响应多种消息的处理类,提供链式调用,以及默认的`T::on`订阅方式:

```C++
struct Actor {
    subscribe_stub stub;

    void launch(publisher* source) {
        assert(source != nullptr);
        //存根可追加,
        stub += subscribe(source->channel<bool>(), [](auto arg) {
            std::cout << "bool:" << std::boolalpha << arg << "\n";
            });
        //支持链式调用,可利用on成员函数
        stub += subscriber(this)
            .subscribe<double>(source)
            .subscribe(source->channel<std::string>());
    }

    void on(std::string const& msg) {
        std::cout << "msg:" << msg << "\n";
    }

    void on(double dV) {
        std::cout << "dV:" << dV << "\n";
    }
};
```

使用方式如下:

```c++
//定义消息源
publisher source{};

//消息处理类的用法
Actor actor{};
actor.launch(&source);

source.publish(false);
source.publish(1.414);
source.publish(std::string{ "liff.engineer@gmail.com" });
```

输出内容如下:

```bash
bool:false
dV:1.414
msg:liff.engineer@gmail.com
```

## 如何支持现有观察者等的整合

该消息系统提供了`subscribe_helper`扩展点,以支持其它实现整合到该系统.

假设观察者定义如下:

```C++
#include <list>

/// @brief 消息
struct Payload {
    int iV;
    double dV;
};

/// @brief 观察者
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void Update(Payload const& msg) = 0;
};

/// @brief 对象
class Subject {
public:
    Subject() = default;
    void Attach(IObserver* ob) {
        m_observers.emplace_back(ob);
    }
    void Detach(IObserver* ob) {
        m_observers.remove(ob);
    }
    void Notify(Payload const& msg) {
        for (auto ob : m_observers) {
            ob->Update(msg);
        }
    }
private:
    std::list<IObserver*> m_observers;
};
```

针对消息源`Subject`提供`subscribe_helper`特化实现:

```C++
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
    static publisher::subscribe_stub subscribe(Subject* source, Fn&& fn) {
        assert(source != nullptr);
        //构造观察者
        auto handler = std::make_shared<Observer<Fn>>(std::move(fn));
        //添加观察者
        source->Attach(handler.get());
        //返回函数,注意函数执行时会移除观察者
        return[src = source, h = std::move(handler)]() {
            src->Detach(h.get());
        };
    }
};
```

为上文的`Actor`添加新成员函数:

```C++
struct Actor {
    subscribe_stub stub;
	//...省略的内容
    void launch(Subject* source) {
        stub += subscriber(this).subscribe(source,
            [](auto obj, Payload const& arg) {
                std::cout << "Actor>>Payload:(" << arg.iV << "," << arg.dV << ")\n";
            });
    }
};
```

这样就可以以类似方式使用了:

```c++
//消息处理类的用法
Actor actor{};
//自定义消息源的使用
Subject subject{};
actor.launch(&subject);
{
    auto stub = subscribe(&subject, [](Payload const& arg) {
        std::cout << "Payload:(" << arg.iV << "," << arg.dV << ")\n";
    });

    subject.Notify(Payload{ 1,1.1 });
}
subject.Notify(Payload{ 2,2.2 });

//注意actor中的订阅存根会自动取消订阅,
//如果不手动取消,则要确保消息源生命周期超过actor
//否则取消订阅时Subject已经不存在,从而引发崩溃.
actor.stub.unsubscribe();
```

上述示例输出如下:

```C++
Actor>>Payload:(1,1.1)
Payload:(1,1.1)
Actor>>Payload:(2,2.2)
```

## 设计说明

对使用者来讲,消息系统共3种类型与1个接口:

- 消息源/发行者`publisher`;
- 订阅存根`subscribe_stub`;
- 订阅辅助类`subscriber`;
- 订阅接口`subscribe`.

其中`publisher`只提供两个接口:

1. `publish`:发送消息
2. `channel<T>`:获取特定类型消息通道/源

订阅存根类`subscribe_stub`不支持复制,只包含一个取消订阅的接口`unsubscribe`,只在特殊情况下需要调用.

订阅辅助类`subscriber`接收消息处理类的指针,支持链式调用,仅包含一种接口`subscribe`,如果消息处理类提供了`on`形式的处理函数,`subscribe`则只需要提供事件源即可,否则需要提供处理函数,第一个参数为消息处理类的指针.

订阅接口`subscribe`接收消息源和处理函数,并返回订阅存根.

## 总结

无论是面向对象设计，还是采用事件驱动架构，通过对需求场景的分析与抽象，借助于`Modern C++`的强大能力，能够做出在易用性、可维护性、可扩展性、可复用性，以及性能上表现都比较出色的设计。



