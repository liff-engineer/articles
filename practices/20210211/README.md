# 自注册工厂模式的C++实现

工厂模式在软件设计中非常常见,譬如领域驱动设计中使用工厂来进行领域对象的创建.工厂模式实现并不复杂,但是要支持各种需求场景却并非易事,这里展示一种自注册的工厂模板通用实现.该实现取自[Unforgettable Factory Registration](http://www.nirfriedman.com/2018/04/29/unforgettable-factory/),进行了部分调整.

## 支持特性

- 通用工厂实现
- 支持多个构造参数
- 自动注册
- 禁止非自注册派生
- 可配置标识和负载获取

## 示例

假设有一系列命令,接口如下:

```C++
class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void execute() const noexcept = 0;
};
```

希望为其提供工厂模式,构造参数为`std::string`,派生类自动注册,且能够自动生成标识,可以配置附加的`order`信息.

首先,使用`CRTP`来提供接口的工厂:

```C++
class ICommand :public Factory<ICommand,std::string>{
  //...  
};
```

其中`Factory<I,...Args>`的`I`为接口类型,`Args`为工厂模式创建接口实例时的参数,通常为派生类构造函数参数.

然后,指定`ICommand`的标识和负载:

```C++
class ICommand :public Factory<ICommand,std::string>
{
public:
    //形式为Identify<Payload_t>
    //Payload_t为负载类型,如果没有负载,则直接使用Identify<>即可
    using Identify_t = Identify<int>;
};
```

工厂默认使用`std::string`作为标识符,鉴于`ICommand`需要附加的`order`信息,则标识类型为`Identify<int>`:

```C++
struct Identify<int>{
  	std::string key;
    int         payload;
};
```

然后指定标识和负载的获取方式:

```C++
//移除typeid(T).name()中附带的class
//例如PrintCommand 对应的typeid(T).name()为 'class PrintCommand'
//期望的标识为'PrintCommand'
std::string trimClass(std::string v) {
    auto  i = v.find("class");
    if (i != v.npos) {
        i += strlen("class");
    }
    return v.substr(v.find_first_not_of(' ', i));
}
class ICommand :public Factory<ICommand,std::string>
{
public:
    using Identify_t = Identify<int>;

    //接口类必须提供如下静态函数,来指定如何获取标识和负载
    template<typename T>
    static Identify_t Id() {
        Identify_t result;
        result.key = typeid(T).name();
        result.key = trimClass(result.key);
		
        //负载的获取方法为获得派生类T的静态成员变量order
        result.payload = T::order;
        return result;
    }
};
```

接口完整定义如下:

```C++
std::string trimClass(std::string v) {
    auto  i = v.find("class");
    if (i != v.npos) {
        i += strlen("class");
    }
    return v.substr(v.find_first_not_of(' ', i));
}

class ICommand :public Factory<ICommand,std::string>
{
public:
    using Identify_t = Identify<int>;

    template<typename T>
    static Identify_t Id() {
        Identify_t result;
        result.key = typeid(T).name();
        result.key = trimClass(result.key);

        result.payload = T::order;
        return result;
    }
public:
    //接口类构造函数接收Key作为参数,可以避免其它类直接派生自ICommand
    ICommand(Key) {};
    virtual ~ICommand() = default;
    virtual void execute() const noexcept = 0;
};
```

然后看一下如何书写一个派生类`PrintCommand`:

```C++
//必须派生自I::Registrar<D>
//I为接口类,D为派生类型,这样才能自动注册
class PrintCommand :public ICommand::Registrar<PrintCommand>
{
    std::string m_msg;
public:
    //构造函数要满足接口的构造要求
    PrintCommand(std::string msg):m_msg(msg) {};
    void execute() const noexcept override {
        std::cout << "Print:"<<m_msg<<"\n";
    }

public:
    //之前指定了接口的负载获取方式,派生类要实现,否则编译报错
    static const int order = 1;
};
```

另一个派生类实现:

```C++
class ReportCommand :public ICommand::Registrar<ReportCommand>
{
    std::string m_msg;
public:
    ReportCommand(std::string msg) :m_msg(msg) {};
    void execute() const noexcept override {
        std::cout << "Report:" << m_msg << "\n";
    }

public:
    static const int order = 2;
};
```

使用示例如下:

```C++
int main(int argc, char** argv) {

    //接口的Visit可以遍历工厂中存储的信息,
    //对于无负载的内容e.key为标识,e.creator为创建函数
    //对于有负载的内容e.payload为负载
    ICommand::Visit([](auto& e) {
        std::cout << e.key << "\n";
        std::cout << e.payload << "\n";
        });
	
    //Make接口可以接收标识,构造参数从而构造出对应的接口实例
    ICommand::Make("PrintCommand", "what?")->execute();
    return 0;
}
```

输出如下:

```bash
PrintCommand
1
ReportCommand
2
Print:what?
```

## 实现

这里将工厂模式实现为以下几部分:

1. 标识和负载处理
2. 工厂实现
3. 自注册实现

### 标识和负载处理

要求接口类提供`Identidy_t`来指定标识和负载信息,然后提供`static Identidy_t Id<T>()`来指定如何获取标识及负载.

首先定义标识:

```C++
template<typename T = void>
struct Identidy;
```

其中`T`为负载类型,那么无负载和有负载情况如下:

```C++
template<>
struct Identify{
  	std::string key;  
};

template<typename T>
struct Identidy{
  	std::string key;  
    T           payload;
};
```

这样就可以通过如下方式得到接口及其派生类的标识信息:

```C++
template<typename T,typename I>
I::Identify_t demo_creatorOf(){
    return I::Id<T>();
}
```

接口类声明如下:

```C++
class I
{
public:
  	using Identify_t = Identidy<Payload_t>;  
    
    template<typename T>
    static Identify_t Id(){
        Identify_t result;
        result.key = /*...*/;
        //如果有payload
        result.payload = /*...*/; 
        return result;
    };
};
```

### 工厂实现

对于工厂来讲,要提供创建方法的存储和获取,然后利用`CRTP`和接口声明整合到一起.

对于接口`I`,除了前文所属的`Identify_t`,还需要存储创建方法,这里选择派生自`Identify_t`:

```C++
template<typename I, typename... Args>
class Factory {
public:
    //Creator存储了标识、负载,以及创建函数creator
    struct Creator :I::Identify_t
    {
        Creator() = default;
        Creator(typename I::Identify_t arg)
            :I::Identify_t(arg) {};

        std::unique_ptr<I>(*creator)(Args...) = nullptr;
    };
};
```

然后提供存储:

```C++
template<typename I, typename... Args>
class Factory {
public:
    //...
protected:
    Factory() = default;

    static auto& creators() {
        static std::unordered_map<std::string, Creator> d;
        return d;
    }
};
```

提供`Make`接口来创建对象:

```C++
template<typename I, typename... Args>
class Factory {
public:
	//...
    //如果没有对应的k则直接抛出异常
    template<typename... Ts>
    static std::unique_ptr<I> Make(std::string const& k, Ts... args) {
        return creators().at(k).creator(std::forward<Ts>(args)...);
    }
protected:
    static auto& creators() {
        static std::unordered_map<std::string, Creator> d;
        return d;
    }
};
```

提供`Visit`接口来访问工厂信息:

```C++
template<typename I, typename... Args>
class Factory {
public:
    template<typename Fn>
    static void Visit(Fn&& fn) {
        for (auto& [k, e] : creators()) {
            fn(e);
        }
    }
protected:
    Factory() = default;

    static auto& creators() {
        static std::unordered_map<std::string, Creator> d;
        return d;
    }
};
```

为了避免开发者直接派生`I`,这里提供保护的`Key`让`I`构造函数接收`Key`类型:

```C++
template<typename I, typename... Args>
class Factory {
public:
    //...
protected:
    class Key {
        Key() {};

        template<typename T>
        friend struct Registrar;
    };

    Factory() = default;
	//...
};
```

接口`I`构造函数声明如下:

```C++
class I:Factory<I>
{
public:
    I(Key){};
};
```

如果直接使用以下方式派生自`I`,则编译报错:

```C++
class D:public I{
public:
    D(){};//报错
};
```

### 自注册实现

在工厂中实现`Registrar`,包含静态变量`registered`,然后用静态函数初始化该变量,在初始化过程中注册到工厂的`creators`里:

```C++
template<typename I, typename... Args>
class Factory {
public:
    //...
    
    template<typename T>
    struct Registrar :I {
        friend T;

        static bool AutoRegister() {
            //通过I的Id获取Creator,然后初始化构造函数
            Creator creator = I::Id<T>();
            creator.creator = [](Args... args)->std::unique_ptr<I> {
                return std::make_unique<T>(std::forward<Args>(args)...);
            };
            //保存到工厂的creators中
            Factory::creators()[creator.key] = creator;
            return true;
        }
        static bool registered;
    private:
        Registrar() :I(Key{}) {
            //避免优化掉静态变量初始化动作
            (void)registered;
        };
    };    
    //...
};

//静态变量的初始化,通过调用AutoRegister实现自注册
template<typename I, typename... Args>
template<typename T>
bool Factory<I, Args...>::Registrar<T>::registered = 
    Factory<I, Args...>::Registrar<T>::AutoRegister();
```

### 完整实现

`factory.hpp`的内容如下:

```C++
#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <type_traits>

//http://www.nirfriedman.com/2018/04/29/unforgettable-factory/
template<typename I, typename... Args>
class Factory {
public:
    template<typename T = void>
    struct Identify;

    template<>
    struct Identify<void> {
        std::string key;
    };

    template<typename T>
    struct Identify {
        std::string key;
        T           payload;
    };

    struct Creator :I::Identify_t
    {
        Creator() = default;
        Creator(typename I::Identify_t arg)
            :I::Identify_t(arg) {};

        std::unique_ptr<I>(*creator)(Args...) = nullptr;
    };

    template<typename... Ts>
    static std::unique_ptr<I> Make(std::string const& k, Ts... args) {
        return creators().at(k).creator(std::forward<Ts>(args)...);
    }

    template<typename Fn>
    static void Visit(Fn&& fn) {
        for (auto& [k, e] : creators()) {
            fn(e);
        }
    }

    template<typename T>
    struct Registrar :I {
        friend T;

        static bool AutoRegister() {
            Creator creator = I::Id<T>();
            creator.creator = [](Args... args)->std::unique_ptr<I> {
                return std::make_unique<T>(std::forward<Args>(args)...);
            };
            Factory::creators()[creator.key] = creator;
            return true;
        }
        static bool registered;
    private:
        Registrar() :I(Key{}) {
            (void)registered;
        };
    };
protected:
    class Key {
        Key() {};

        template<typename T>
        friend struct Registrar;
    };

    Factory() = default;

    static auto& creators() {
        static std::unordered_map<std::string, Creator> d;
        return d;
    }
};

template<typename I, typename... Args>
template<typename T>
bool Factory<I, Args...>::Registrar<T>::registered = Factory<I, Args...>::Registrar<T>::AutoRegister();
```

示例`example.cpp`内容如下:

```C++
#include "factory.hpp"
#include <iostream>


std::string trimClass(std::string v) {
    auto  i = v.find("class");
    if (i != v.npos) {
        i += strlen("class");
    }
    return v.substr(v.find_first_not_of(' ', i));
}

class ICommand :public Factory<ICommand,std::string>
{
public:
    using Identify_t = Identify<int>;

    template<typename T>
    static Identify_t Id() {
        Identify_t result;
        result.key = typeid(T).name();
        result.key = trimClass(result.key);

        result.payload = T::order;
        return result;
    }
public:
    ICommand(Key) {};
    virtual ~ICommand() = default;
    virtual void execute() const noexcept = 0;
};

class PrintCommand :public ICommand::Registrar<PrintCommand>
{
    std::string m_msg;
public:
    PrintCommand(std::string msg):m_msg(msg) {};
    void execute() const noexcept override {
        std::cout << "Print:"<<m_msg<<"\n";
    }

public:
    static const int order = 1;
};

class ReportCommand :public ICommand::Registrar<ReportCommand>
{
    std::string m_msg;
public:
    ReportCommand(std::string msg) :m_msg(msg) {};
    void execute() const noexcept override {
        std::cout << "Report:" << m_msg << "\n";
    }

public:
    static const int order = 2;
};

//class TCommand :public ICommand
//{
//    std::string m_msg;
//public:
//    TCommand(std::string msg) :m_msg(msg) {};
//    void execute() const noexcept override {
//        std::cout << "Report:" << m_msg << "\n";
//    }
//};

int main(int argc, char** argv) {

    ICommand::Visit([](auto& e) {
        std::cout << e.key << "\n";
        std::cout << e.payload << "\n";
        });

    ICommand::Make("PrintCommand", "what?")->execute();


    return 0;
}
```



