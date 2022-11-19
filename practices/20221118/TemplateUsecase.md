[TOC]

# 模板应用-以注册场景为例

在命令设计模式的实践中，通常会遇到类似的设计实践：

1. 开发者抽象出命令`ICommand`的概念，并由命令管理器单例`CommandManager`进行管理；
2. 命令作为功能入口，被界面按钮等触发，界面管理使用的是命令代号，与命令解耦；
3. 添加新功能时实现命令，并注册到命令管理器中；
4. 实现插件时，通过全局（静态）变量达到加载时自动注册、卸载时自动移除的效果。

这里，将展示如何使用模板来助力类似场景的实现，示例涵盖：

- 单例、工厂、组合、命令等设计模式；
- 类、可变参数模板；
- 全局变量特性。

## 以注册存根应对移除注册的场景

这里以最简单的场景举例，假设命令`ICommand` 的代号为字符串`std::string`，命令管理器 `CommandManager` 提供`Destory`方法来移除注册，例如：

```C++
class CommandManager{
public:
    //由于命令管理器为单例,则Destory设计为静态方法比较合理
    static void Destory(std::string const&);
};
```

那么在移除注册时，需要：移除注册使用的参数`std::string`，移除注册的方法`Destory`，如果手工实现，类似于如下：

```C++
class CommandUnRegistar{
public:
    CommandUnRegistar(std::string code):m_code(std::move(code)){};
    ~CommandUnRegistar(){
        CommandManager::Destory(m_code);
        m_code = std::string{};
    }
private:
    std::string m_code{};
};
```

具体实现时要注意`CommandUnRegistar`行为类似于`std::unique_ptr`：

1. `CommandUnRegistar`需要提供默认构造函数，以方便作为类成员变量存储；
2. 需要删除`CommandUnRegistar`的拷贝构造、拷贝赋值函数，否则会多次无意间取消注册。

这里针对符合上述特征的场景，可以提供一个注册处存根`RegistryStub`的模板类，开发者只需要定义：

```C++
using ICommandStub = RegistryStub<CommandManager,std::string>;
```

注册时返回`ICommandStub`即可，通过对`ICommandStub`生命周期管理即可取消注册。

`RegistryStub.hpp`示例实现如下：

```C++
#pragma once
#include <utility>

namespace abc
{
    template<typename I, typename K = std::size_t>
    class RegistryStub {
    public:
        RegistryStub() = default;
        RegistryStub(K k) :m_stub(k) {};

        RegistryStub(RegistryStub const&) = delete;
        RegistryStub& operator=(RegistryStub const&) = delete;

        RegistryStub(RegistryStub&& other) noexcept
            :m_stub(std::exchange(other.m_stub, K{})) {};

        RegistryStub& operator=(RegistryStub&& other) noexcept
        {
            if (this != &other) {
                m_stub = std::exchange(other.m_stub, K{});
            }
            return *this;
        }

        ~RegistryStub() {
            I::Destory(std::exchange(m_stub, K{}));
        }

        K reset() {
            return std::exchange(m_stub, K{});
        }
    private:
        K m_stub{};
    };
}
```

## 命令工厂的实现示例

实际场景下的命令并没有那么简单，譬如某些命令构造需要参数，或者要求的时机下无法构造出命令实例；较为合适的做法是提供命令工厂，在合适的情况下构造出命令在注册。

这里假设存在一系列命令，构造时无需构造参数，并且以`std::string`区分不同的命令类，而且由单例来管理。通常会实现如下：

```C++
class ICommand{};
class CommandManager{
public:
    using Builder = std::function<std::unique_ptr<ICommand>()>;
    static CommandManager& Get();
    void add(std::string const& code,Builder&& builder);
    void remove(std::string const& code);
    std::unique_ptr<ICommand> create(std::string const& code);
private:
	std::map<std::string,Builder> m_builders;    
};
```

### 命令及工厂类接口

考虑到`CommandManager`是单例，它完全没必要向外暴露，可以将`add`、`remove`、`create`方法暴露到`ICommand`上作为静态方法，即`ICommand.hpp`实现为：

```C++
#pragma once
#include "RegistryStub.hpp"
#include <memory>
#include <string>
#include <functional>

class ICommand {
public:
    virtual ~ICommand() = default;
    ICommand& operator=(ICommand&&) = delete;
public:
    virtual void run() const = 0;
public:
    static std::unique_ptr<ICommand> Create(std::string const& code);
    static void Destory(std::size_t);

    template<typename T, typename K, typename... Args>
    static abc::RegistryStub<ICommand> Register(K&& k, Args&&... args) {
        return abc::RegistryStub<ICommand>{
            RegisterImpl(std::forward<K>(k), [&args...]()->std::unique_ptr<ICommand> {
                return std::make_unique<T>(std::forward<Args>(args)...);
                })
        };
    }
private:
    static std::size_t RegisterImpl(std::string const& code,
        std::function<std::unique_ptr<ICommand>()>&& builder
    );
};

using ICommandStub = abc::RegistryStub<ICommand>;
```

注意这里的 `Register`实现：

```C++
template<typename T, typename K, typename... Args>
static abc::RegistryStub<ICommand> Register(K&& k, Args&&... args) {
    return abc::RegistryStub<ICommand>{
        RegisterImpl(std::forward<K>(k), [&args...]()->std::unique_ptr<ICommand> {
            return std::make_unique<T>(std::forward<Args>(args)...);
            })
    };
}
```

通过可变参数模板、完美转发以及`lambda`表达式，可以让用户在注册命令`Builder`时，指定特定命令类型，并附加对应的构造参数。

### 注册处样板

命令工厂这种形态，在其它场景也有类似使用，基本上是`Key-Value`对存储，且`Key`不能重复：有时`Key`是特定的，而也有默认的情况，这里提供`RegistryTemplate`模板来表达类似概念：

```C++
template<typename... Ts>
class RegistryTemplate;
```

那么针对命令工厂这种`Key-Value`，可以实现如下模板类：

```C++
template<typename K, typename T>
class RegistryTemplate<std::pair<K, T>> {
public:
    template<typename U>
    std::size_t add(U&& k, T&& v) {
        //TODO 提供比较判定,来覆盖k对应的v
        m_elements.emplace_back(std::make_pair(std::forward<U>(k), std::move(v)));
        return m_elements.size();
    }

    template<typename U>
    void remove(U&& k)
    {
        for (auto& e : m_elements) {
            if (e.first == k) {
                e.first = K{};
                e.second = T{};
            }
        }
    }

    void remove(std::size_t k) {
        if (k == 0 || k > m_elements.size()) return;
        m_elements[k - 1] = std::make_pair(K{}, T{});
    }


    auto begin() const { return m_elements.begin(); }
    auto end() const { return m_elements.end(); }
private:
    std::vector<std::pair<K, T>> m_elements;
};
```

这样类似的场景直接使用`RegistryTemplate`即可,上述命令工厂定义如下:

```C++
class CommandFactory :public
    abc::RegistryTemplate<
        std::pair<std::string, std::function<std::unique_ptr<ICommand>()>>
    >
{
public:
    static CommandFactory& Get() {
        static CommandFactory object{};
        return object;
    }
};
```

而`ICommand`上的接口实现就非常简单了：

```C++
std::unique_ptr<ICommand> ICommand::Create(std::string const& code)
{
    for (auto& e : CommandFactory::Get()) {
        if (e.first == code) {
            return e.second();
        }
    }
    return {};
}

void ICommand::Destory(std::size_t k)
{
    CommandFactory::Get().remove(k);
}

std::size_t ICommand::RegisterImpl(std::string const& code,
                                   std::function<std::unique_ptr<ICommand>()>&& builder)
{
    return CommandFactory::Get().add(code, std::move(builder));
}
```

### 使用示例

最简单的`CommandA.cpp`：

```C++
#include "ICommand.hpp"
#include <iostream>

class CommandA :public ICommand {
public:
    void run() const override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

namespace
{
    //利用全局变量生命周期,自动注册和取消注册
    auto stub{ ICommand::Register<CommandA>("CommandA") };
}
```

以及等价的`CommandB.cpp`：

```C++
#include "ICommand.hpp"
#include <iostream>

class CommandB :public ICommand {
public:
    void run() const override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

namespace
{
    auto stub{ ICommand::Register<CommandB>("CommandB") };
}
```

同时`RegistryStub`也支持同时注册多个，例如`Commands.cpp`：

```C++
#include "ICommand.hpp"
#include <iostream>

class MyCommand :public ICommand {
    std::string m_code;
public:
    explicit MyCommand(std::string code)
        :m_code(std::move(code)) {};

    void run() const override {
        std::cout<<"["<<m_code<<"]" << __FUNCSIG__ << "\n";
    }
};

namespace
{
    ICommandStub stubs[]{
        ICommand::Register<MyCommand>("MyCommand-X","CommandX"),
        ICommand::Register<MyCommand>("MyCommand-Y","CommandY"),
        ICommand::Register<MyCommand>("MyCommand-Z","CommandZ")
    };
}
```

可以看到，之前的`Register`实现支持存储构造参数，以应对相同`Command`类，而构造参数不同，以形成不同功能的情况。

`ICommand`使用实例如下：

```C++
for (auto& code : {"CommandA","CommandB","MyCommand-X","MyCommand-Y" ,"MyCommand-Z" }) {
    if (auto v = ICommand::Create(code)) {
        v->run();
    }
}
```

运行结果如下：

```bash
void __cdecl CommandA::run(void) const
void __cdecl CommandB::run(void) const
[CommandX]void __cdecl MyCommand::run(void) const
[CommandY]void __cdecl MyCommand::run(void) const
[CommandZ]void __cdecl MyCommand::run(void) const
```

## 流程扩展的组合模式实现示例

与之类似，假设有个流程扩展`IExtension`，也需要注册与取消注册的支持，它定义如下：

```C++
class IExtension {
public:
    virtual ~IExtension() = default;
    IExtension& operator=(IExtension&&) = delete;
public:
    virtual void doX() = 0;
    virtual void doY() = 0;
};    
```

### 流程管理类接口

与命令工厂类类似，并考虑到这些`Extension`最终表现与单个`IExtension`一致，即组合模式，它可以定义成如下形式：

```C++
class IExtension {
public:
    virtual ~IExtension() = default;
    IExtension& operator=(IExtension&&) = delete;
public:
    virtual void doX() = 0;
    virtual void doY() = 0;
public:
    static IExtension& Get();
    static void Destory(std::size_t);

    template<typename T,typename... Args>
    static abc::RegistryStub<IExtension> Register(Args&&... args) {
        return abc::RegistryStub<IExtension>{
            RegisterImpl(std::make_unique<T>(std::forward<Args>(args)...))
        };
    }
private:
    static std::size_t RegisterImpl(std::unique_ptr<IExtension> e);
};

using IExtensionStub = abc::RegistryStub<IExtension>;
```

### 注册处样板

这种场景则可以用如下模板表示：

```C++
template<typename T>
class RegistryTemplate<std::unique_ptr<T>> {
public:
    std::size_t add(std::unique_ptr<T> e) {
        m_elements.emplace_back(std::move(e));
        return m_elements.size();
    }

    void        remove(const T* vp) {
        for (auto& e : m_elements) {
            if (e.get() == vp) {
                e = nullptr;
            }
        }
    }

    void        remove(std::size_t k) {
        if (k == 0 || k > m_elements.size()) return;
        m_elements[k - 1] = nullptr;
    }

    auto begin() const { return m_elements.begin(); }
    auto end() const { return m_elements.end(); }
private:
    std::vector<std::unique_ptr<T>> m_elements;
};
```

然后扩展管理类可以实现如下：

```C++
class Extension :public 
    abc::RegistryTemplate<
        std::unique_ptr<IExtension>
    >,
    public IExtension
{
public:
    static Extension& Get() {
        static Extension object{};
        return object;
    }

    void doX() override {
        for (auto& e : *this) {
            e->doX();
        }
    }
    
    void doY() override {
        for (auto& e : *this) {
            e->doY();
        }
    }
};
```

而`IExtension`的静态接口实现也非常简单：

```C++
IExtension& IExtension::Get()
{
    return Extension::Get();
}

void IExtension::Destory(std::size_t k)
{
    return Extension::Get().remove(k);
}

std::size_t IExtension::RegisterImpl(std::unique_ptr<IExtension> e)
{
    return Extension::Get().add(std::move(e));
}
```

### 使用示例

对于`ExtensionAB.cpp`：

```C++
#include "IExtension.hpp"
#include <iostream>

class ExtensionA :public IExtension {
public:
    void doX() override {
        std::cout << __FUNCSIG__ << "\n";
    }
    void doY() override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

class ExtensionB :public IExtension {
public:
    void doX() override {
        std::cout << __FUNCSIG__ << "\n";
    }
    void doY() override {
        std::cout << __FUNCSIG__ << "\n";
    }
};

namespace
{
    IExtensionStub stubs[]{
        IExtension::Register<ExtensionA>(),
        IExtension::Register<ExtensionB>()
    };
}
```

另外就是含构造参数的`Extensions.cpp`：

```C++
#include "IExtension.hpp"
#include <iostream>
#include <string>

class MyExtension :public IExtension {
    std::string  m_code;
public:
    explicit MyExtension(std::string code)
        :m_code(std::move(code))
    {}

    void doX() override {
        std::cout << "(" << m_code << ")" << __FUNCSIG__ << "\n";
    }
    void doY() override {
        std::cout << "(" << m_code << ")" << __FUNCSIG__ << "\n";
    }
};

namespace
{
    IExtensionStub stubs[]{
        IExtension::Register<MyExtension>("E1"),
        IExtension::Register<MyExtension>("E2")
    };
}
```

使用示例如下：

```C++
auto&& ext = IExtension::Get();
ext.doX();
ext.doY();
```

运行效果如下：

```bash
void __cdecl ExtensionA::doX(void)
void __cdecl ExtensionB::doX(void)
(E1)void __cdecl MyExtension::doX(void)
(E2)void __cdecl MyExtension::doX(void)
void __cdecl ExtensionA::doY(void)
void __cdecl ExtensionB::doY(void)
(E1)void __cdecl MyExtension::doY(void)
(E2)void __cdecl MyExtension::doY(void)
```

## 其它

通过模板技术，可以将识别出的相似场景提供模板实现，以减少代码书写、保证实现质量、统一实现方式。

