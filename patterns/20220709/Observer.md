[TOC]

# 观察者`Observer`模式的实现

观察者模式是常见的一种设计模式,通常应用在需要保证一致性的场景下,例如同步模型和界面的状态.

它的实现比较简单,这里基于现代`C++`,来不同诉求下更合适的实现方式.

## 常规的实现方式

观察者以接口方式实现,例如`IObserver`,接收一个`double`值来进行更新:

```C++
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void update(double event) = 0;
};
```

被观察的事物被称为`Subject`,需要对要通知的观察者`IObserver`进行管理:

```C++
class  Subject {
public:
    void notify(double event) {
        for (auto& observer : m_observers) {
            if (observer) {
                observer->update(event);
            }
        }
    }

    void subscribe(IObserver* observer) {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it == m_observers.end()) {
            m_observers.emplace_back(observer);
        }
    }

    void unsubscribe(IObserver* observer) {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it != m_observers.end()) {
            *it = nullptr;
        }
    }
private:
    std::vector<IObserver*> m_observers;
};
```

**这是非常常规且直接的实现方式,**对于不同的观察者再实现一遍即可:

```C++
struct Event {
    std::string payload;
};

class IEventObserver {
public:
    virtual ~IEventObserver() = default;
    virtual void update(Event const& event) = 0;
};

class  EventSubject {
public:
    void notify(const Event& event) {
        for (auto& observer : m_observers) {
            if (observer) {
                observer->update(event);
            }
        }
    }

    void subscribe(IEventObserver* observer) {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it == m_observers.end()) {
            m_observers.emplace_back(observer);
        }
    }

    void unsubscribe(IEventObserver* observer) {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it != m_observers.end()) {
            *it = nullptr;
        }
    }
private:
    std::vector<IEventObserver*> m_observers;
};
```

使用者如果要观察不同的`Subject`,则需要派生自对应的观察者接口类:

```C++
class SubjectObserver :public IObserver {
public:
    void update(double event) override {
        std::cout << __FUNCSIG__ << ":" << event << '\n';
    }
};

class EventObserver :public IEventObserver {
public:
    void update(const Event& event) override {
        std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
    }
};

void test()
{
    Subject obj1{};
    SubjectObserver ob1{};
    EventSubject obj2{};
    EventObserver ob2{};

    obj1.subscribe(&ob1);
    obj2.subscribe(&ob2);

    obj1.notify(3.1415926);
    obj2.notify(Event{ "message coming" });
}
```

只要实现过超过一次,就会发现无论是`Observer`还是`Subject`,其代码就是不断地重复,差别在于通知时附带的负载不同. 那么能否避免这些重复代码呢?

## 利用模板减少重复代码

将观察者模式通知时提供的附加信息包装成一个结构体,那么`Observer`和`Subject`在模板层面就完全统一了,首先看一看`Observer`接口:

```C++
template <typename T>
class IObserver
{
public:
    virtual ~IObserver() = default;
    virtual void update(const T &event) = 0;
};
```

然后是`Subject`:

```C++
template <typename T>
class Subject
{
public:
    void notify(const T &event)
    {
        for (auto &observer : m_observers)
        {
            if (observer)
            {
                observer->update(event);
            }
        }
    }

    void subscribe(IObserver<T> *observer)
    {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it == m_observers.end())
        {
            m_observers.emplace_back(observer);
        }
    }

    void unsubscribe(IObserver<T> *observer)
    {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it != m_observers.end())
        {
            *it = nullptr;
        }
    }

private:
    std::vector<IObserver<T> *> m_observers;
};
```

在实现`Observer`接口时:

```C++
class SubjectObserver :public IObserver<double> {
public:
    void update(const double& event) override {
        std::cout << __FUNCSIG__ << ":" << event << '\n';
    }
};

class EventObserver :public IObserver<Event> {
public:
    void update(const Event& event) override {
        std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
    }
};
```

使用`Subject`时:

```C++
void test()
{
    Subject<double> obj1{};
    SubjectObserver ob1{};
    Subject<Event> obj2{};
    EventObserver ob2{};

    obj1.subscribe(&ob1);
    obj2.subscribe(&ob2);

    obj1.notify(3.1415926);
    obj2.notify(Event{ "message coming" });
}
```

**当然,不使用模板也可以实现**:

> 将通知负载的信息提供统一基类,然后通过`dynamic_cast`转换成实际类型使用.从而使得`Observer`和`Subject`只需要一个实现就可以应用在各种场景.
>
> `dynamic_cast`在实际代码中可以视为一种坏味道,这代表着设计存在问题,谨慎使用.

采用模板避免了重复代码,不过`Subject`都是限制好的,一旦新增通知信息类型,就要修改原有代码.

能否提供一种通用的`Subject`集合,在不修改原有代码的基础上应对后续变化呢?

## 提供发布者`Publisher`以支持`Subject`自动扩展

这里提供发布者`Publisher`概念,它是一种动态扩展的`Subject`集合,可以发送不同的消息类型给对应的观察者,以替换掉之前写死的`Subject<T>`.

实现思路是:

1. 将所有观察者派生自统一基类`IObserver`;
2. 具体消息类型的观察者接口模板`Observer<T>`派生自`IObserver`,并提供`update`接口定义;
3. `Publisher`通过统一的基类`IObserver`来管理`Observer<T>`.

首先是观察者相关实现:

```C++
//用来识别不同的类型
using TypeCode = std::string;

class IObserver
{
public:
    virtual ~IObserver() = default;
    virtual TypeCode code() const noexcept = 0;
};

template <typename T>
class Observer : public IObserver
{
public:
    virtual void update(const T &event) = 0;
    TypeCode code() const noexcept override final {
        return typeid(T).name();
    }
};
```

然后是发布者`Publisher`:

```C++
class Publisher
{
public:
    template <typename T>
    void notify(const T &event)
    {
        auto code = typeid(T).name();
        for (auto &observer : m_observers)
        {
            //检查类型是否匹配
            if (observer && observer->code() == code)
            {
                //这里实际上可以使用static_cast,不过一般来讲不至于有性能瓶颈
                if (auto vp = dynamic_cast<Observer<T> *>(observer))
                {
                    vp->update(event);
                }
            }
        }
    }

    //订阅接口时用具体的接口类
    template <typename T>
    void subscribe(Observer<T> *observer)
    {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it == m_observers.end())
        {
            m_observers.emplace_back(observer);
        }
    }

    //取消订阅时用基类即可
    void unsubscribe(IObserver *observer)
    {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it != m_observers.end())
        {
            *it = nullptr;
        }
    }

private:
    std::vector<IObserver *> m_observers;
};
```

这样就不需要定义不同的`Subject`了:

```C++
class SubjectObserver :public Observer<double> {
public:
    void update(const double& event) override {
        std::cout << __FUNCSIG__ << ":" << event << '\n';
    }
};

class EventObserver :public Observer<Event> {
public:
    void update(const Event& event) override {
        std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
    }
};

void test()
{
    Publisher publisher{};
    SubjectObserver ob1{};
    EventObserver ob2{};

    publisher.subscribe(&ob1);
    publisher.subscribe(&ob2);

    publisher.notify(3.1415926);
    publisher.notify(Event{ "message coming" });
}
```

不过,这种实现有一个副作用,就是当观察者需要同时观察不同类型时,面临以下问题要处理:

- 菱形继承;
- `subscribe`接口存在异议;
- `unsubscribe`接口存在异议.

那么,是否可以无需派生来实现观察者呢?这就需要用到类型擦除和`duck type`了.

## 无需派生的观察者实现

这里假设要作为观察者的类型`T`具有对应的`update`函数,例如:

```C++
class MyObserver {
public:
    //能够处理double消息
    void update(double event)  {
        std::cout << __FUNCSIG__ << ":" << event << '\n';
    }
    //能够处理Event消息
    void update(const Event& event)  {
        std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
    }
};
```

那么只需要实现通用的观察者,接收类型`T`的指针,实现对应的`update`函数时调用`T`对应方法即可:

```C++
class ObserverImpl:public Observer<Event>{
	MyObserver* m_obj;
    
    void update(const Event& event) override{
        return m_obj->update(event);
    }
};
```

这里首先调整基类`IObserver`,以满足订阅和取消订阅时去重的需要:

```C++
using TypeCode = std::string;
class IObserver
{
public:
    virtual ~IObserver() = default;
    //要观察的消息类型
    virtual TypeCode code() const noexcept = 0;
    //观察者对象地址
    virtual const void *address() const noexcept = 0;
};
```

然后是观察者的接口类和实现类:

```C++
template <typename E>
class Observer : public IObserver
{
public:
    virtual void update(const E &event) = 0;
    TypeCode code() const noexcept override final
    {
        return typeid(E).name();
    }
};

template <typename T, typename E>
class ObserverImpl final : public Observer<E>
{
    T *m_obj;

public:
    explicit ObserverImpl(T *obj) : m_obj(obj){};

    void update(const E &event) override
    {
        return m_obj->update(event);
    }

    const void *address() const noexcept override
    {
        return m_obj;
    }
};
```

然后调整发布者`Publisher`的订阅、取消订阅定义和实现:

```C++
class Publisher
{
public:
    template <typename E>
    void notify(const E &event)
    {
        auto code = typeid(E).name();
        for (auto &observer : m_observers)
        {
            if (observer && observer->code() == code)
            {
                if (auto vp = dynamic_cast<Observer<E> *>(observer.get()))
                {
                    vp->update(event);
                }
            }
        }
    }

    //订阅时要指定T订阅的消息类型是什么
    template <typename E, typename T>
    void subscribe(T *obj)
    {
        auto code = typeid(E).name();
        auto it = std::find_if(m_observers.begin(), m_observers.end(),
                                [&](auto &&observer) -> bool
                                {
                                    return (observer &&
                                            (observer->code() == code) &&
                                            (observer->address() == obj));
                                });
        if (it == m_observers.end())
        {
            m_observers.emplace_back(std::make_unique<ObserverImpl<T, E>>(obj));
        }
    }

    //取消T类型实例的所有订阅
    template <typename T>
    void unsubscribe(T *obj)
    {
        //TODO FIXME:要查找到所有匹配的并清空
        auto it = std::find_if(m_observers.begin(), m_observers.end(),
                                [=](auto &&observer) -> bool
                                {
                                    return (observer && observer->address() == obj);
                                });
        if (it != m_observers.end())
        {
            *it = nullptr;
        }
    }

    //取消T类型实例的E类型订阅
    template <typename E, typename T>
    void unsubscribe(T *obj)
    {
        auto code = typeid(E).name();
        auto it = std::find_if(m_observers.begin(), m_observers.end(),
                                [&](auto &&observer) -> bool
                                {
                                    return (observer &&
                                            (observer->code() == code) &&
                                            (observer->address() == obj));
                                });
        if (it != m_observers.end())
        {
            *it = nullptr;
        }
    }

private:
    std::vector<std::unique_ptr<IObserver>> m_observers;
};
```

这样可以将任意满足需求的类实例作为观察者注册,不会面临菱形继承等问题:

```C++
class MyObserver {
public:
    void update(double event)  {
        std::cout << __FUNCSIG__ << ":" << event << '\n';
    }
    void update(const Event& event)  {
        std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
    }
};

void test()
{
    Publisher publisher{};
    MyObserver ob{};

    publisher.subscribe<double>(&ob);
    publisher.subscribe<Event>(&ob);

    publisher.notify(3.1415926);
    publisher.notify(Event{ "message coming" });
}
```

**在此基础上,`IObserver`等接口暴露已无必要,可以实现在`Publisher`内部.**

## 使用`RAII`管理订阅关系

目前为止,发布者并不清楚观察者何时被销毁,取消订阅是观察者的职责.这就和内存申请一样,手工处理总会面临遗漏等问题.

通过以`RAII`逻辑提供订阅存根`Stub`,将其作为观察者成员变量,可以在观察者析构时自动取消订阅,从而避免手工处理.

考虑到我们在内部用`vector`管理观察者,可以用索引来取消订阅,因而存根通过如下方式实现:

```C++
class Publisher
{
public:
    //存根类比较麻烦的是它只能move,否则会因自动取消订阅造成代码逻辑错误
    class Stub
    {
    public:
        Stub() = default;
        Stub(const Stub &other) = delete;
        Stub &operator=(const Stub &other) = delete;
        Stub(Stub &&other) noexcept
            : m_owner(other.m_owner), m_index(other.m_index)
        {
            other.m_owner = nullptr;
            other.m_index = 0;
        }

        Stub &operator=(Stub &&other) noexcept
        {
            if (this != std::addressof(other))
            {
                unsubscribe();
                m_owner = other.m_owner;
                m_index = other.m_index;
                other.m_owner = nullptr;
            }
            return *this;
        }

        ~Stub() noexcept
        {
            unsubscribe();
        }

    private:
        friend class Publisher;
        explicit Stub(Publisher *owner, std::size_t index)
            : m_owner(owner), m_index(index)
        {
        }

        void unsubscribe() noexcept
        {
            if (m_owner && m_index < m_owner->m_observers.size())
            {
                m_owner->m_observers[m_index].reset();
            }
        }

        Publisher *m_owner{};
        std::size_t m_index{};
    };
};
```

这样订阅时返回存根即可,也不需要提供取消订阅的方法:

```C++
class Publisher
{
public:
    template <typename E>
    void notify(const E &event)
    {
        auto code = typeid(E).name();
        for (auto &observer : m_observers)
        {
            if (observer && observer->match(code))
            {
                if (auto vp = dynamic_cast<Observer<E> *>(observer.get()))
                {
                    vp->update(event);
                }
            }
        }
    }

    template <typename E, typename T>
    Stub subscribe(T *obj)
    {
        auto code = typeid(E).name();
        for (auto &&observer : m_observers)
        {
            if (observer && observer->match(code, obj))
            {
                return Stub{};
            }
        }
        m_observers.emplace_back(std::make_unique<ObserverImpl<T, E>>(obj));
        return Stub(this, m_observers.size() - 1);
    }

private:
    using TypeCode = std::string;
    class IObserver
    {
    public:
        virtual ~IObserver() = default;
        virtual bool match(const TypeCode &code) const noexcept = 0;
        virtual bool match(void *obj) const noexcept = 0;
        inline bool match(const TypeCode &code, void *obj) const noexcept
        {
            return match(code) && match(obj);
        }
    };

    template <typename E>
    class Observer : public IObserver
    {
    public:
        virtual void update(const E &event) = 0;

        bool match(const TypeCode &code) const noexcept override final
        {
            return typeid(E).name() == code;
        }
    };

    template <typename T, typename E>
    class ObserverImpl final : public Observer<E>
    {
        T *m_obj;

    public:
        explicit ObserverImpl(T *obj) : m_obj(obj){};

        void update(const E &event) override
        {
            return m_obj->update(event);
        }

        bool match(void *obj) const noexcept override
        {
            return obj == m_obj;
        }
    };

private:
    std::vector<std::unique_ptr<IObserver>> m_observers;
};
```

## 支持用函数进行订阅

目前建立订阅关系时传递的是类指针,也要求用户提供类似`update`的公开成员方法,如果能够提供函数来订阅,则:

- 简洁易用;
- 避免对观察者类进行成员方法约束.

可以通过提供函数用的订阅接口,并提供对应的`Observer<T>`实现来讲解决.

首先定义`CallableObserver`以满足观察者接口要求:

```C++
template <typename Fn, typename E>
class CallableObserver final : public Observer<E>
{
    Fn m_op;

public:
    explicit CallableObserver(Fn &&fn) : m_op(std::move(fn)){};

    void update(const E &event) override
    {
        return m_op(event);
    }

    bool match(void *obj) const noexcept override
    {
        return false;
    }
};
```

然后添加订阅接口:

```C++
template <typename E, typename Fn>
std::size_t subscribe(Fn &&fn)
{
    using T = std::remove_cv_t<std::remove_reference_t<Fn>>;
    m_observers.emplace_back(std::make_unique<CallableObserver<T, E>>(std::forward<Fn>(fn)));
    return static_cast<int>(m_observers.size() - 1);
}
```

其它实现参考前面内容,不再赘述.
