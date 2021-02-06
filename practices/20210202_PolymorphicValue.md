# C++中值语义多态的一种实现方法

C++中的运行时多态通常是采用虚函数实现的,譬如命令模式,通常定义如下接口:

```C++
class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void execute() const noexcept = 0;
};
```

之后就需要通过指针来使用了.指针这种属于引用语义,涉及到生命周期管理,最终会面临各种复杂场景.例如组合模式:

```C++
class CompositeCommand:public ICommand
{
    std::vector<std::shared_ptr<ICommand>> m_children;
public:
    explicit  CompositeCommand(std::vector<std::shared_ptr<ICommand>> && commands)
        :m_children(std::move(commands)){}
    
    void execute() const noexcept override{
        for(auto& e:m_children){
            e->execute();
        }
    }
};
```

针对某些场景,这种写法很难写出完全正确的,譬如深度复制的实现. 

那么多态能否写成值语义的呢? 可以像整数、字符串一样操作,譬如:

```C++
using Command = XXXX<ICommand>;
class CompositeCommand:public ICommand
{
    std::vector<Command> m_children;
public:    
    explicit  CompositeCommand(std::vector<Command> && commands)
        :m_children(std::move(commands)){}    
    void execute() const noexcept override{
        for(auto& e:m_children){
            e->execute();
        }
    }    
};

void example(Command const& cmd){
    Command cmd1 = cmd;//直接复制
    cmd1->execute();
}
```

## 动机

- 降低心智负担:值语义,无指针,将开发者从生命周期管理中解放出来;
- 避免内存申请:基于**SBO**,在小对象情况下可以避免内存申请;

## 使用样例

假设我们有一个接口`ITask`如下:

```C++
class ITask
{
public:
    virtual ~ITask() = default;
    virtual void execute() const noexcept = 0;
};
```

这时我可以定义`Task`为接口`ITask`的值语义类型:

```C++
using Task = PolymorphicValue<ITask>;
```

有一系列派生类,演示样例如下:

```C++
template <typename T>
class PrintTask : public ITask
{
    T v;
public:
    PrintTask(T arg)
        : v(std::move(arg)){};

    void execute() const noexcept override
    {
        std::cout << v << "\n";
    }
};

template<typename T>
PrintTask(T)->PrintTask<T>;
```

然后就可以以普通值的方式存储、复制、使用:

```C++
int main(int argc, char **argv)
{
    //task的构造
    std::vector<Task> tasks;
    tasks.emplace_back(PrintTask{1.414});
    tasks.emplace_back(PrintTask{1024});
    tasks.emplace_back(PrintTask{std::string{"liff.engineer@gmail.com"}});
    
    //const版本构造
    const PrintTask t{ true };
    tasks.emplace_back(t);
    
    //task的调用和复制
    std::vector<Task> task1s;
    for (auto const &task : tasks)
    {
        task1s.push_back(task);
        task->execute();
    }
    
    //task的调用
    for (auto const &task : task1s)
    {
        task->execute();
    }
	
    //拷贝赋值
    Task  pt = PrintTask{ 1.414 };
    Task pt1 = pt;
    return 0;
}
```

## 实现方法

基本的实现方法受[Inheritance Without Pointers](https://www.fluentcpp.com/2021/01/29/inheritance-without-pointers/)启发,也有相应的C++标准提案[P201R2- A polymorphic value-type for C++](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0201r2.pdf),部分参数实现在[Github-polymorphic_value](https://github.com/jbcoe/polymorphic_value).至于应用场景和详细背景说明参见[std::polymorphic_value + Duck Typing = Type Erasure](https://foonathan.net/2020/01/type-erasure/).

### 存储处理

在C++17中,提供了`std::any`,可以用来存储任意可复制类型,我们可以使其它来完成派生类的存储:

```C++
class CommandImpl:public ICommand{
    //...
};

void demo(){
    //创建一个派生类
    std::any holder = CommandImpl();
    //按值复制
    std::any copy = holder;
}
```

### 获取接口

真实使用时需要从中取出`ICommand`,这个需要知道原始类型,所以我们通过函数实现:

```C++
ICommand* as(std::any& holder){
    CommandImpl* impl = std::any_cast<CommandImpl>(&holder);
    return impl;
}
```

鉴于该函数与原始类型有关,我们在创建时初始化对应函数:

```C++
template<typename I>
struct PolymorphicValue
{
	template<typename T>
    PolymorphicValue(T v)
    	:holder(v)
    {
    	getter = [](std::any& o) ->I*{
         	return std::any_cast<T>(&o);   
        };     
    };    
private:
    std::any holder;
    I*  (*getter)(std::any&);
};
```

### 提供`->`访问

现在解决了存储和接口获取的问题,就需要通过运算符重载实现类似智能指针那种访问效果:

```C++
template<typename I>
struct PolymorphicValue{
    //...
    
    I* operator->() {
        return getter(holder);
    }
    I& operaor->(){
        return *getter(holder);
    }
    //...
};
```

### 处理构造函数

构造函数要能够正确处理自身和派生类,所以需要用一些模板技巧来实现:

```C++
template<typename I>
class PolymorphicValue{
{
public:
    template<typename T, std::enable_if_t<std::is_base_of_v<I, std::remove_reference_t<T>>>* = nullptr>
    PolymorphicValue(T&& v)
    :holder(std::move(v)),
    getter([](std::any& o)->I* {  return std::any_cast<T>(&o); })
    {}

    template<typename T, std::enable_if_t<std::is_base_of_v<I, std::remove_reference_t<T>>>* = nullptr>
    PolymorphicValue(T const& v)
    : holder(v),
    getter([](std::any& o)->I* {  return std::any_cast<T>(&o); })
    {}
};
```

### `const`版本

上述只是实现了`I*`,通常为保证正确性,还需要实现`const I*`版本,这里可以通过新增相应函数来完成:

```C++
template<typename I>
class PolymorphicValue
{
    template<typename T>
    PolymorphicValue(T v)
    	:getter1([](std::any const& o)->const I*{ return std::any_cast<T>(&o);})
        {};
  
    const I* operator->() const {
        return getter1(holder);
    }
public:
    const I* (*getter1)(std::any const&);
};    
```

## 存在的问题

上述实现虽然能够达到效果,但是存在两个问题:

1. 开发者体验不好

   当你开启调试模式希望看看目前究竟用的是哪个派生类时,由于`std::any`的原因,是没办法看到具体派生类信息的.

2. 性能隐患

   当尝试着访问接口时,首先要执行函数调用获取接口指针,这一步多做了.

## 改进后的实现

这里通过为`PolymorphicValue`类型提供`I*`的成员来解决,一旦存储的值发生变化,就刷新该成员.大概结构如下:

```C++
template<typename I>
class PolymorphicValue
{
public:
  	template<typename T>
    PolymorphicValue(T const& v)
    	:holder(v),
    	refresh([](std::any& o)->I*{ return std::any_cast<T>(&o);})
    {
        pointer = refresh(holder);
    }
    
    I* operator->() noexcept{
        return pointer;
    }
    const I* operator->() const noexcept{
        return pointer;
    }
private:
    std::any holder;
    I*   (*refresh)(std::any&) = nullptr;
    I*   pointer = nullptr;
};    
```

这样就可以在调试时直接查看`I* pointer`的具体类型,并且访问时没有函数调用.



不过这种实现要相对复杂许多,需要正确处理每一次的`holder`变动.即构造函数、拷贝构造、赋值构造等等均需要重写.参考实现如下:

```C++
#pragma once
#include <any>
#include <type_traits>

template<typename I>
class PolymorphicValue
{
public:
    template<typename T, std::enable_if_t<std::is_base_of_v<I, std::remove_reference_t<T>>>* = nullptr>
    PolymorphicValue(T&& v)
        :holder(std::move(v)),
        refresh([](std::any& o)->I* {  return std::any_cast<T>(&o); })
    {
        static_assert(std::is_base_of_v<I, std::remove_cv_t<T>>);
        pointer = refresh(holder);
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<I, std::remove_reference_t<T>>>* = nullptr>
    PolymorphicValue(T const& v)
        : holder(v),
        refresh([](std::any& o)->I* {  return std::any_cast<T>(&o); })
    {
        static_assert(std::is_base_of_v<I, std::remove_cv_t<T>>);
        pointer = refresh(holder);
    }

    PolymorphicValue(PolymorphicValue const& other)
        :holder(other.holder),
        refresh(other.refresh) {
        pointer = refresh(holder);
    }

    PolymorphicValue(PolymorphicValue&& other) noexcept
        :holder(std::exchange(other.holder, {})),
        refresh(std::exchange(other.refresh, nullptr))
    {
        if (refresh != nullptr) {
            pointer = refresh(holder);
        }
        other.pointer = nullptr;
    }

    PolymorphicValue& operator=(PolymorphicValue const& other)
    {
        return *this = PolymorphicValue(other);
    }

    PolymorphicValue& operator=(PolymorphicValue&& other) noexcept
    {
        holder = std::exchange(other.holder, {});
        refresh = std::exchange(other.refresh, nullptr);
        if (refresh) {
            pointer = refresh(holder);
        }
        other.pointer = nullptr;
    }

    void swap(PolymorphicValue& other) noexcept {
        using std::swap;
        swap(holder, other.holder);
        swap(refresh, other.refresh);
        if (refresh) {
            pointer = refresh(holder);
        }
        if (other.refresh) {
            other.pointer = other.refresh(other.holder);
        }
    }

    explicit operator bool() const noexcept {
        return pointer != nullptr;
    }

    I* operator->() noexcept {
        return pointer;
    }

    I& operator*() noexcept {
        return *pointer;
    }

    const I* operator->() const noexcept {
        return pointer;
    }

    const I& operator*() const noexcept {
        return *pointer;
    }
private:
    std::any holder;
    I* (*refresh)(std::any&) = nullptr;
    I* pointer = nullptr;
};
```

## 调试可视化配置`PolymorphicValue.natvis`

`Visual Studio`提供了`Nativs`框架来支持调试时定制显示内容.我们添加`PolymorphicValue.natvis`文件使得开发者调试时可以直接看到内部的`I* pointer`信息.文件内容如下:

```xml
<?xml version="1.0" encoding="utf-8"?> 
<AutoVisualizer xmlns="http://schemas.microsoft.com/vstudio/debugger/natvis/2010">
  <Type Name="PolymorphicValue&lt;*&gt;">
    <Expand>
      <Item Name="[value]">*pointer</Item>
    </Expand>
  </Type>
</AutoVisualizer>
```

如果你使用的是`CMake`,则可以直接将该文件添加到目标`source`上,调试时就可用了:

```cmake
add_executable(demo)
target_sources(demo
    PRIVATE demo.cpp PolymorphicValue.natvis
)
```

## 总结

在`Modern C++`中,非常鼓励使用值语义来编写程序.因为生命周期管理一直是逃不开的坎儿,值语义的书写方式将开发者从生命周期管理中解放出来,专注于业务逻辑的实现.

现在多态写法也可以通过上述按普通值来进行操作,结合移动语义,能够写出更为简洁易懂的代码.















