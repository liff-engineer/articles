# 一种支持界面调用函数的实现方法

在使用`C++`开发大型桌面应用的场景下,需要频繁的修改函数调用参数并执行,通过编码方式修改会给开发者带来困扰,因为需要重新编译.

那么是可以在函数执行前弹出界面来调整参数,然后再执行的.这意味着针对每个参数不同的函数都需要提供相应界面,会增加开发者的工作量.

是否能够提供一种函数对象,以用来设计通用的界面,完成相应场景的需求? 这里展示一种实现方法.

## 场景分析

如果想要设计通用的界面,则就需要通用的函数对象和函数参数:

```C++
class IArgument{
public:
    virtual ~IArgument()=default;
	virtual  std::string name() const noexcept = 0;
    virtual  Type  type() const noexcept = 0;
};

class Argument:public IArgument {
public:    
    T  value() const noexcept;
    void setValue(T v) noexcept;
};

class ICallback{
public:    
    virtual ~ICallback()=default;
    virtual void execute() = 0;
    virtual std::string name() = 0;
    virtual std::vector<IArgument*> argument() noexcept = 0;
};
```

通过全局的`ICallback`仓库,可以获取所有可调用函数,根据名称得到函数对象`ICallback`,又能够获取参数对象`IArgument`,界面修改后执行`ICallback`的`execute`即可.

界面的主要工作量在于针对不同的参数类型,提供相应的界面显示和设置.

## 实现思路

1. 提供参数基类`IArgument`
2. 提供通用的参数实现`Argument`
3. 提供参数包实现`Arguments`
4. 提供函数基类`ICallback`
5. 使用`Arguments`实现通用的函数类`Callback`

这里假设有个函数,接收1个整数`iV`、1个浮点数`dV`、1个字符串`sV`作为输入参数,执行时打印到命令行,则代码类似如下:

```C++
Callback<int, double, std::string> cb([](int iV,double dV,std::string sV) {
    //输出到命令行
    std::cout << "iV:" << iV << "\ndV:" << dV << "\nsV:" << sV << "\n";
},{"iV","dV","sV"});//参数名称

//提供默认的参数
cb.args.set<0, int>(1024);
cb.args.set<1, double>(3.1415926);
cb.args.set<2, std::string>("liff.engineer@gmail.com");

//执行函数
cb();
```

那么这个`Callback`使用方式如下:

```C++
void callback_example(ICallback& cb)
{
    //查询参数
    auto iV = cb.argumentAs<int>("iV");
    auto dV = cb.argumentAs<double>("dV");
    auto sV = cb.argumentAs<std::string>("sV");
    if (iV) {
        std::cout << "iV:" << iV.value() << "\n";
    }
    if (dV) {
        std::cout << "dV:" << dV.value() << "\n";
    }
    if (sV) {
        std::cout << "sV:" << sV.value() << "\n";
    }

    //执行
    cb.execute();

    //修改参数
    cb.setArgument("iV", 123456);
    cb.setArgument("dV", 1.717);
    cb.setArgument("sV", std::string{"Garfield"});
    //执行
    cb.execute();
}
```

下面来看一看是如何一步步实现的.

## 参数接口`IArgument`及其实现`Argument`

为了简单期间,这里只展示部分实现方法,首先看一下参数接口`IArgument`:

```C++
class IArgument
{
public:
    virtual ~IArgument() = default;
    
    //判断是哪种参数类型
    template<typename T>
    bool is() const noexcept;
    
    //转换为哪种参数类型的参数
    template<typename T>
    T& as() & noexcept;
};
```

然后是通用的参数实现`Argument<T>`,通过模板实现,支持任意可复制的参数类型:

```C++
template<typename T>
class Argument final:public IArgument
{
public:
    T v;
    Argument(T v_arg)
        :v(std::move(v_arg)) {};
};
```

这样之前`IArgument`的两个模板函数就可以通过如下方式实现:

```C++
//简单期间,这里使用RTTI做类型判断
template<typename T>
bool IArgument::is() const noexcept{
    return dynamic_cast<const Argument<T>*>(this) != nullptr;
}

//直接使用静态cast,有效性判断使用is
template<typename T>
T& IArgument::as() & noexcept {
    return static_cast<Argument<T>*>(this)->v;
}
```

这样对于示例中的函数,其函数参数可以结合`Argument`与`std::tuple`来定义:

```C++
std::tuple<Argument<int>,Argument<double>,Argument<std::string>> args;
```

## 通用参数包实现`Arguments`

对于函数的输入参数,通常格式和类型都不固定,可以提供函数参数包`Arguments<Ts...>`,来应对各种需求,譬如之前示例的函数,其参数包定义如下:

```C++
//定义参数包
Arguments<int, double, std::string> args;

//调整参数值
args.set<0, int>(1024);
args.set<1, double>(3.1415926);
args.set<2, std::string>("liff.engineer@gmail.com");

//重置参数
args.reset<0>();

//获取参数
auto v = args.at(1)->as<double>();

args.set<0, int>(8192);
```

即,参数包需要按照参数顺序和类型定义,修改时由于类型确定,稍微麻烦,不过可以提供`IArgument`方式修改.其定义如下:

```C++
template<typename... Ts>
class Arguments {
    //参数存储,这里采用std::optional使得参数可以不存在,使用时需小心
    std::tuple<std::optional<Argument<Ts>>...> m_values;
    
    //参数指针存储,用来提供IArgument
    std::array<IArgument*, sizeof...(Ts)> m_pointers{};
public:
    Arguments() {
        //默认参数为空,指针也为空
        m_pointers.fill(nullptr);
    };
    //其它成员函数实现
    
    template<std::size_t I>
    IArgument* at() noexcept {
        //根据参数位置获取参数接口
        return std::get<I>(m_pointers);
    }

    IArgument* at(std::size_t i) {
        //根据参数位置获取参数接口
        return m_pointers.at(i);
    }
    //其它成员函数实现    
};
```

提供`set`和`reset`接口用来填充/修改参数值:

```C++
template<typename... Ts>
class Arguments {
public:
    //设置参数值
    template<std::size_t I,typename T>
    void set(T v)
    {
        if (IArgument* arg = std::get<I>(m_pointers)) {
            if (!arg->is<T>()) {
                throw std::invalid_argument("invalid setting");
            }
            arg->as<T>() = v;
        }
        else
        {
            //更新值并刷新指针
            std::get<I>(m_values) = v;
            std::get<I>(m_pointers) = std::get<I>(m_values).operator->();
        }
    }
	
    //重置参数值
    template<std::size_t I>
    void reset()
    {
        std::get<I>(m_pointers) = nullptr;
        std::get<I>(m_values).reset();
    }
};
```

由于`m_pointers`存储的就是`m_values`中的指针,一旦值发生变化,就要刷新`m_pointers`对应内容,提供`refresh`:

```C++
template<typename... Ts>
class Arguments {
public:
    //其它实现
    
    //刷新某个函数
    template<std::size_t I>
    void refresh() noexcept {
        if (std::get<I>(m_values)) {
            //获取值地址并更新指针
            std::get<I>(m_pointers) = std::get<I>(m_values).operator->();
        }
    }

    template<std::size_t ...Is>
    void refreshImpl(std::index_sequence<Is...> is) noexcept
    {
        (refresh<Is>(), ...);
    }

    void refresh()
    {
        refreshImpl(std::make_index_sequence<sizeof...(Ts)>{});
    }
};
```

当函数调用时,需要获取存储在参数包的原始信息,即`std::tuple<int,double,std::string>`,然后转换成`(int iV,double dV,std::string sV)`,以此来驱动函数,所以提供`as`接口将其转换成原始参数:

```C++
template<typename... Ts>
class Arguments {
public:
    //其它实现
    
    template<std::size_t ...Is>
    std::tuple<Ts...> asImpl(std::index_sequence<Is...> is) const
    {
        return std::make_tuple(std::get<Is>(m_values).value().v...);
    }

    decltype(auto) as() const {
        return asImpl(std::make_index_sequence<sizeof...(Ts)>{});
    }
};
```

这样通用的参数包就实现完成了,下面来看函数的实现.

## 函数接口`ICallback`

接口需要包含执行和参数访问、设置等内容,其定义如下:

```C++
class ICallback
{
public:
    virtual ~ICallback() = default;
	//执行函数
    virtual void execute() = 0;
    //根据名称获取参数
    virtual IArgument* argument(std::string_view name) noexcept = 0;

    //设置参数
    template<typename T>
    void setArgument(std::string_view name, T v) {
        auto arg = argument(name);
        if (arg && arg->is<T>()) {
            arg->as<T>() = v;
        }
    }

    //获取参数
    template<typename T>
    std::optional<T> argumentAs(std::string_view name) noexcept {
        auto arg = argument(name);
        if (arg && arg->is<T>()) {
            return arg->as<T>();
        }
        return {};
    }
};
```

## 通用函数实现`Callback`

`Callback`只需要利用`Arguments`,保存函数地址,并添加参数名称,提供出`ICallback`所需接口实现即可:

```C++
template<typename ... Ts>
class Callback:public ICallback
{
    //与参数一一对应的名称
    std::array<std::string_view, sizeof...(Ts)> m_names;
    //函数地址,可以替换成function或者继续派生,将类实例及其成员函数绑定成为Callback
    void(*m_addr)(Ts...) = nullptr;
public:
    //参数列表
    Arguments<Ts...> args;
public:
    Callback() = default;
    Callback(void(*f)(Ts...), std::array<std::string_view, sizeof...(Ts)> names)
        :m_addr(f), m_names(names) {};
	
    explicit operator bool() const noexcept {
        return m_addr != nullptr;
    }
	//作为仿函数调用
    void operator()() {
        std::apply(m_addr, args.as());
    }
	
    //ICallback的执行接口
    void execute()  override {
        std::apply(m_addr, args.as());
    }
     
    //ICallback的参数接口
    IArgument* argument(std::string_view name) noexcept override {
        for (std::size_t i = 0; i < m_names.size(); i++) {
            if (m_names[i] == name) {
                return args.at(i);
            }
        }
        return nullptr;
    }
};
```

`Callback`对应于示例,使用方式如下:

```C++
//参数包
Arguments<int, double, std::string> args;
args.set<0, int>(1024);
args.set<1, double>(3.1415926);
args.set<2, std::string>("liff.engineer@gmail.com");

//函数对象
Callback<int, double, std::string> cb([](int iV,double dV,std::string sV) {
    std::cout << "iV:" << iV << "\ndV:" << dV << "\nsV:" << sV << "\n";
    },
    {"iV","dV","sV"});

//利用之前的参数包,这里复制完要刷新
cb.args = args;
cb.args.refresh();

//执行函数,修改参数,再执行
cb();
cb.args.set<1, double>(1.414);
cb();
```

## 总结

以上只是原理性的简单实现,真实场景会略显复杂,需要酌情修改.

通过这种实现方法,可以将函数转换成可检视、编辑参数,并且能够调用的对象,再通过相对通用的界面/命令行交互,就可以在不修改代码的情况下重新执行某些代码片段了.