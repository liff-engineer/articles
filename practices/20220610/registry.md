[TOC]

# 以注册处应对单例模式使用场景

单例模式究竟是不是好的实践,或者说有多么糟糕,这个开发者有不同的看法和认识.没有共识基础的纷争并无意义,毕竟工作总要继续.而且再专业的软件工程师,也难免遇到"遗留代码",因为各种各样的因素,要在泥坑里打滚.想要做出改变,渐进式地调整更为合适.

虽然有"打不过就加入"的嫌疑,但是专业的软件工程师,哪怕用单例也要比别人用得更专业,并为后续的重构提前铺平道路.

这里展示一种注册处`Registry`实现,以期:

1. 提升单例模式使用体验;
2. 统一管理单例;
3. 为良好编码实践提供支持.

## 基于类型的注册处`Registry`

单例的应用场景在于某种类型的实例只有一个,基于这个场景,注册处采用的是特定类型只能注册一个实例的方式,可以理解为动态可扩展的异构容器,它提供如下支持:

1. 获取全局注册处实例;
2. 添加、更新类型`T`的实例;
3. 查询类型`T`的实例;
4. 移除类型`T`的实例.

参考接口定义如下:

```C++
class Registry final {
public:
    //获取全局注册处实例
    static Registry* Get();
    
    //添加或更新类型T的实例
    template<typename T, typename... Args>
    T* emplace(Args&&... args);

    //移除类型T的实例
    template<typename T>
    void erase() noexcept;

    //查询实例
    template<typename T>
    T* get() const;
	
    //清空
    void clear();
};
```

另外还有一种可能的场景-单例类型的扩展点:某个接口类的派生类只允许有一个实例,这里也提供相应支持:

```C++
class Registry final {
public:
    //将基类I绑定到类型T实例上,以支持get<I>来获取I实例
	template<typename I, typename T>
    bool bind();
};
```

当然,提供了上述支持体验并不好,毕竟需要:

1. 通过`Registry::Get()->emplace`注册类实例到`Registry`;
2. 通过`Registry::Get()->get`获取类实例.

这里通过提供全局的`Get<T>()`方法来完成上述操作:当注册处没有实例`T`时创建并注册,当注册处有实例`T`时返回实例,接口定义如下:

```C++
template<typename T>
decltype(auto) Get() {
    return Getter<Registry, T>{}.Get(Registry::Get());
}
```

当然,这种行为应当能够被用户定制,所以提供了两级扩展点:

```C++
template<typename T, typename = void>
struct RegistryGetter {
    T* Get(Registry* owner) {
        //默认可构造?不存在就构造一份并返回
        if constexpr (std::is_constructible_v<T>) {
            if (auto vp = owner->get<T>()) return vp;
            return owner->emplace<T>();
        }
        else {//不可构造则直接返回
            return owner->get<T>();
        }
    }
};

template<typename T>
struct Getter<Registry, T> {
    T* Get(Registry* owner) {
        return RegistryGetter<T>{}.Get(owner);
    }
};
```

在此基础上提供泛化版本的`Get`:

```C++
template<typename Owner, typename T, typename = void>
struct Getter;

template<typename T, typename Owner>
decltype(auto) Get(Owner owner) {
    using U = std::remove_pointer_t<std::remove_const_t<Owner>>;
    return Getter<U, T>{}.Get(owner);
}
```

即,可以通过定制`Getter`,只要`Get<T>(owner)`逻辑上能够找到唯一路径,都可以用这种方式获取.

## 使用示例

这里以之前的工厂模式示例为基础,假设`ReporterFactory`是全局唯一的工厂,其使用方式如下:

```c++
//获取工厂实例,不存在则创建并返回
auto factory = abc::Get<ReporterFactory>();

factory->Register<IntReporter, int>("Int");
factory->Register<DoubleReporter, double>("Double");
```

而且`Registry`默认可构造,可以作为驱动模块的总输入:

```C++
abc::Registry registry{};
registry.emplace<IntReporter>(10);
registry.emplace<DoubleReporter>(3.14);
registry.emplace<Reporter<int>>(1234);
registry.emplace<Reporter<double>>(1234.456);

auto vp1 = registry.get<IntReporter>();
auto vp2 = registry.get<DoubleReporter>();
vp1->Run();
vp2->Run();

//指定IReporter查找的是Reporter<double>实例
registry.bind<IReporter, Reporter<double>>();
auto vp3 = registry.get<IReporter>();
vp3->Run();
```

## 与其它类型全局操作的整合

目前注册处`Registry`主要是存储了类型的实例,还有一些场景是有全局的工厂,来创建类实例,这种场景可以结合注册处提供全局支持,譬如全局的`Make`或`MakeBy`:

```C++
//从注册处获取对应的全局工厂来创建接口I
template<typename I, typename... Args>
decltype(auto) Make(Args&&... args) {
    return Maker<Registry, I>{}.Make(Registry::Get(), std::forward<Args>(args)...);
}

//从Owner获取对应工厂来创建
template<typename I, typename Owner, typename... Args>
decltype(auto) MakeBy(Owner owner, Args&&... args) {
    using U = std::remove_pointer_t<std::remove_const_t<Owner>>;
    return Maker<U, I>{}.Make(owner, std::forward<Args>(args)...);
}
```

针对`Make`方法,提供`RegistryFactoryInject`扩展点来指定工厂类型:

```C++
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

template<typename I>
struct Maker<Registry, I> {
    template<typename... Args>
    auto Make(Registry* owner, Args&&... args) {
        return RegistryMaker<I>{}.Make(owner, std::forward<Args>(args)...);
    }
};
```

使用方式如下:

```C++
//指定IReporter的工厂为ReporterFactory
template<>
struct abc::RegistryFactoryInject<IReporter> {
    using type = ReporterFactory;
};

void TestRegistryFactory() {
    {//注册类实例
        auto factory = abc::Get<ReporterFactory>();
        factory->Register<IntReporter, int>("Int");
        factory->Register<DoubleReporter, double>("Double");
    }
	
    //全局的Make会找ReporterFactory实例然后调用其Make方法
    auto vp1 = abc::Make<IReporter>("Int", 1024);
    auto vp2 = abc::Make<IReporter>("Double", 3.333);
    if (vp1) {
        vp1->Run();
    }
    if (vp2) {
        vp2->Run();
    }
}
```

## 设计与实现

`Registry`的关键点在于类型的区分及其实例的存储,这里提供如下基类:

```C++
class Registry final {
    private:
    class Impl;
    //类实例基类
    class IValue {
    public:
        virtual ~IValue() = default;
        //类实例对应的类型ID
        virtual std::size_t TypeIndex() const noexcept = 0;
    };
    std::vector<std::unique_ptr<IValue>> m_entries;
};
```

其中类型`ID`的生成之前的文章曾经展示过,这里采用了一种新方式:

```C++
//根据字符串获取唯一ID
static std::size_t GetIndex(const char* code)
{
    static std::vector<std::string> codes;
    for (std::size_t i = 0; i < codes.size(); i++) {
        if (codes[i] == code) {
            return i;
        }
    }
    codes.emplace_back(code);
    return codes.size() - 1;
}

//根据类型名获取唯一ID,从0开始
template<typename T>
static std::size_t Index() {
    static auto r = GetIndex(typeid(T).name());
    return r;
}
```

基于这种技术,注册处的实现非常简单,同时也为`bind<I,T>`提供支持,对于指向类实例`I`这种指向其它类实例的提供`Proxy<I>`,并且`TypeIndex`指向真实类类型,实现如下:

```C++
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
```

存储类实例的类型为`Value<T>`,派生自`Proxy<T>`:

```C++
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
```

剩下的实现就非常简单了,不再赘述.
