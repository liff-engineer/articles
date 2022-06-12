[TOC]

# 利用模板简化工厂模式实现

> 这里的工厂模式是指:根据key,创建出接口类的派生类实例.

工厂模式是用来负责对象实例创建的,这样使用方可以不关心具体实现,只需要使用即可,在软件设计中比较常用.当然它的实现也并不复杂,开发人员根据场景需要可以迅速实现一个能用的工厂.

而通过模板,开发者可以:

- 以非常少的代码来快速实现;
- `API`一致,易于学习维护;
- 通过修改模板实现,可立即为所有工厂类提供更为丰富的支持;
- 可通过统一方式管理所有工厂.

下面首先展示几种工厂需求场景,然后基于模板类的实现方式,最终是工厂模板类的设计.

## 工厂模式需求场景

假设有一个接口类`IReporter`:

```C++
class IReporter {
public:
    virtual ~IReporter() = default;
    virtual void Run() = 0;
};
```

希望能够提供工厂`ReporterFactory`,来提供如下基本支持:

- 能够以字符串为`key`来注册派生类;
- 根据`key`构造`IReporter`类实例;

而派生类构造时可能会需要构造参数,这时可能有不同场景诉求:

- 所有派生类构造参数一致;
- 根据`key`的不同,构造参数可能不一致.

针对构造参数的提供也会有不同诉求:

- 可以设置默认构造参数,构造时无需提供;
- 在构造`IReporter`类实例时提供构造参数来构造.

对于工厂本身,也会有不同诉求,譬如针对特定`key`,提供了默认实现,但是允许使用者替换掉,即:

> 向工厂注册时提供两个`key`:`k1`是构造实例时使用,`k2`是标识,注册后`k1`映射到`k2`;后续注册可以覆盖掉它.

对于某些场景,甚至不希望类实现直接派生自`IReporter`,而是通过一个`Wrapper`类将类型转换为`IReporter`派生类,例如:

```C++
struct IntReporter
{
    int iV;
    IntReporter(int v) :iV(v) {};
    void Run() {
        std::cout << "(IntReporter):" << iV << "\n";;
    }
};
```

通过模板类`ReporterImpl<T>`将`IntReporter`等类型转换为派生自`IReporter`:

```C++
//如果需要避免派生自IReporter可以考虑使用该包装类
template<typename T>
class ReporterImpl :public IReporter {
    T m_obj;
public:
    explicit ReporterImpl(T obj) :m_obj(std::move(obj)) {};

    void Run() override {
        m_obj.Run();
    }
};
```

## 基于模板类`FactoryBase<I,K>`的实现方式

这里提供了1个基本的模板类`FactoryBase<I,K>`,作为工厂实现的基类:

1. `I`代表接口类;
2. `K`代表`key`的类型.

根据不同场景,提供了两种基本的工厂实现:

1. `Factory<I,K>`:通过`K`创建`I`的实例;
2. `Factory2L<I,K1,K2=K1>`:通过`K1`创建`I`的实例,但是创建方法的标识为`K2`,注册时`K1`与`K2`建立映射关系.

### 基本的实现和使用示例

首先是基本的工厂实现:

```C++
//工厂定义
class ReporterFactory : public abc::Factory<IReporter, std::string>{};
```

如果假设派生自`IReporter`的类型通过`T::Code`指定了其`key`,则稍微调整一下,注册时就可以只注册类型:

```C++
//工厂定义
class ReporterFactory : public abc::Factory<IReporter, std::string> {
public:
    /// @brief 注册IReporter实现
    template<typename T, typename Arg = void>
    bool Register() {
        return Super::Register<T, Arg>(T::Code());
    }
};
```

`Factory<I,K>`提供了默认构造参数的设置和重置,也可以通过`Make`时传递构造参数来创建实例.

使用示例如下:

```C++
/// @brief IReporter实现示例
template<typename T>
class Reporter :public IReporter {
    T m_obj;
public:
    explicit Reporter(T obj) :m_obj(std::move(obj)) {};

    static std::string Code() {
        if constexpr (std::is_same_v<T, std::string>) {
            return "std::string";
        }
        else {
            return typeid(T).name();
        }
    }

    void Run() override {
        std::cout << "(" << Code() << "):" << m_obj << "\n";;
    }
};

int main() {

    ReporterFactory factory{};
    //注册
    factory.Register<Reporter<int>, int>();
    factory.Register<Reporter<double>, double>();
    factory.Register<Reporter<std::string>, std::string>();
    
    //设置默认构造参数
    factory.SetDefaultArgument("int", 1024);
    factory.SetDefaultArgument("double", 3.1415926);
    factory.SetDefaultArgument("std::string", std::string{"liff.engineer@gmail.com"});

    std::vector<std::unique_ptr<IReporter>> results;
    //使用默认构造参数构建类实例
    results.emplace_back(factory.Make("int"));
    results.emplace_back(factory.Make("double"));
    results.emplace_back(factory.Make("std::string"));
	
    //使用传递的构造参数构建类实例
    results.emplace_back(factory.Make("int", 256));
    results.emplace_back(factory.Make("double", 1.171));

    for (auto&& o : results) {
        if (o) {
            o->Run();
        }
    }
    return 0;
}
```

### 无派生实现和使用示例

如果有一系列类型虽然没有派生自`IReporter`,但是逻辑上满足`IReporter`的需求,即提供了`Run`方法,通过`ReporterImpl<T>`模板类包装即可,这里提供了扩展点,能够进行映射,方式如下:

```C++
//当类型T通过ReporterImpl提供IReporter派生类时可以注入,工厂就可以识别
template<typename T>
struct abc::IFactoryInject<T, IReporter> {
    using type = ReporterImpl<T>;
};
```

使用示例如下:

```C++
struct IntReporter
{
    int iV;
    IntReporter(int v) :iV(v) {};
    void Run() {
        std::cout << "(IntReporter):" << iV << "\n";;
    }
};

struct DoubleReporter
{
    double dV;
    DoubleReporter(double v) :dV(v) {};
    void Run() {
        std::cout << "(DoubleReporter):" << dV << "\n";;
    }
};

int main() {

    ReporterFactory factory{};
    //注册
    factory.Register<IntReporter, int>("Int");
    factory.Register<DoubleReporter, double>("Double");


    std::vector<std::unique_ptr<IReporter>> results;
	//以构造参数构造
    results.emplace_back(factory.Make("Int", 128));
    results.emplace_back(factory.Make("Double", 1.414));

    //由于没有提供构造参数,且没有设置默认构造参数,以下构造会失败
    results.emplace_back(factory.Make("Int"));
    results.emplace_back(factory.Make("Double"));
    for (auto&& o : results) {
        if (o) {
            o->Run();
        }
    }
    return 0;
}
```

## 工厂模板类的设计

工厂类除了工厂自身,还涉及到参数的存储和处理,这里首先定义两个基类:

- 工厂类基类`IFactory`:暂无用途,方便后续扩展;
- 参数基类`IArgument`:构造参数的基类.

```C++
class IFactory {//基类,方便后续统一管理所有工厂类
public:
    virtual ~IFactory() = default;
protected:
    struct IArgument {
        virtual ~IArgument() = default;
    };

    //表达接口I的构造相关内容
    template<typename I>
    struct ObjGen {
        const char* argCode; //构造参数类型
        std::unique_ptr<IArgument> arg;//默认构造参数
        std::unique_ptr<I>(*op)(const IArgument&);//构造方法
    };

    struct Impl;//为优化编译速度,避免用户读取接口时过于复杂,相关内容在该类中实现
};
```

之后就可以实现基本的工厂模板类`FactoryBase<I,K>`:

```C++
template<typename I, typename K>
class FactoryBase :public IFactory {
protected://全部声明成保护,作为公共基类,公开接口由派生类实现

    //注册,指定构造参数为Arg
    template<typename T, typename Arg>
    bool RegisterImpl(K key);

    //创建类实例,分为有参数版、无参数版
    template<typename Arg>
    std::unique_ptr<I> MakeImpl(const K& key, const Arg& arg) const;

    std::unique_ptr<I> MakeImpl(const K& key) const;

    //设置默认构造参数
    template<typename Arg>
    bool SetArgumentImpl(const K& key, Arg&& arg);

    //重置默认构造参数
    bool ResetArgumentImpl(const K& key);
protected:
    //存储了构造方法
    std::unordered_map<K, ObjGen<I>> m_builders;
};
```

在此基础上,实现特定场景的工厂类就非常简单了,譬如`Factory<I,K>`:

```C++
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
```

具有别名的工厂`Factory2L<I,K1,K2>`实现只是附加了`map<K1,K2>`:

```C++
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
```

## 工厂模板类如何处理构造参数

在真实场景下,类的构造参数可能会有很多个,而且类型也比较复杂,尝试去处理类具有多个构造参数的场景得不偿失,这里只拆分界定为两种情况:

- 无构造参数,即类型`T`支持以`T{}`构造;
- 有`1`个构造参数,即类型`T`支持以`T{arg}`构造.

对于构造参数有多个的情况可以聚合成类型`Arg`来应对,这样就极大地简化了实现和使用方式.

前文提到构造参数提供了基类`IArgument`,从它会产生两种派生类:

1. `Proxy<Arg>`:派生自`IArgument`,用来处理构造参数;
2. `Argument<Arg>`:派生自`Proxy<Arg>`,用来存储构造参数`Arg`.

针对`MakeImpl`接口的两种形态:

```C++
//创建类实例,分为有参数版、无参数版
template<typename Arg>
std::unique_ptr<I> MakeImpl(const K& key, const Arg& arg) const;

std::unique_ptr<I> MakeImpl(const K& key) const;
```

分别对应`Proxy<Arg>`和`Proxy<void>`,在调用`ObjGen<I>`来构造时,可以根据参数来区分进行分别处理:

- 参数为`Proxy<void>`:采用默认构造参数,无默认构造参数则返回空(**如果类型原本就没有构造参数,注册时就会提供默认构造参数**);
- 参数为`Proxy<Arg>`:采用传递过来的构造参数.

`Proxy<Arg>`和`Argument<Arg>`实现类似如下:

```C++
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
```

如此而来,构造实现如下:

```C++
template<typename I, typename Arg>
static std::unique_ptr<I> Make(const ObjGen<I>& gen, const Proxy<Arg>& arg) {
    if constexpr (std::is_same_v<Arg, void>) {//采用默认构造参数构造
        if (gen.arg) return gen.op(*gen.arg.get());
        return {};
    }
    else {
        //首先保证参数类型一致,然后使用传入的参数构造
        static const auto code = typeid(Arg).name();
        if (std::strcmp(code, gen.argCode) != 0) return {};
        return gen.op(arg);
    }
}
```

注册时创建`ObjGen<I>`方式如下:

```C++
template<typename I, typename T, typename Arg>
static ObjGen<I> ObjGenOf() {
    //参数为void时提供默认构造参数
    std::unique_ptr<IArgument> arg{};
    if constexpr (std::is_same_v<Arg, void>) {
        arg = std::make_unique<Proxy<Arg>>();
    }

    return ObjGen<I>{typeid(Arg).name(), std::move(arg),
        [](const IArgument& arg)->std::unique_ptr<I> {
            //找到真实要创建的类型
            using U = std::conditional_t<std::is_base_of_v<I, T>, T, IFactoryInject<T, I>::type>;
            if constexpr (std::is_same_v<Arg, void>) {
                return std::make_unique<U>();//无参数版构造
            }
            else
            {
                //有参数版构造,类型正确性可以由外部保证,这里直接static_cast转换
                return std::make_unique<U>(*(static_cast<const Proxy<Arg>*>(&arg)->vp));
            }
        }
    };
}
```



