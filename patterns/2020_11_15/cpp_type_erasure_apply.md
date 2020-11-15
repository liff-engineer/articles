# C++的类型擦除及其应用

可扩展性是我们衡量设计的一个重要指标,在C++这种强类型语言中,想要应对扩展性需求,虚接口是最常用的方法.

但是读过很多书的你肯定会说,继承这种方式会带来耦合,有没有别的办法?

是的,在C++中有技术能够满足你的需求,这种技术被称之为类型擦除.

## 什么是类型擦除

类型擦除顾名思义,就是将类型信息抹除掉. 以最典型的存储场景为例,如果希望声明的某个变量存储的值类型可变:

```C++
//定义变量var
Var var;

var = 1; //存储整数
var = 1.414;//存储浮点数
var = "liff.engineer@gmail.com";//存储字符串
var = Article("长不胖的Garfield","C++的类型擦除及应用");
```

从上述示例可以看到,`var`变量的类型为`Var`,实际上却可以存储各种类型的值.即把值的类型抹除了.

## 和虚接口方式的对比

虚接口的实现通常如下:

```C++
class IVar{
public:
    virtual ~IVar()=default;
};

class IntgerVar:public IVar
{
    int m_var;
public:
 	IntgerVar(int v):m_var(v){};
};
```

然后才可以以类似如下方式使用:

```C++
//定义变量
std::shared_ptr<IVar> var;

var = std::make_shared<IntgerVar>(1);
var = std::make_shared<FloatVar>(1.414);
var = std::make_shared<StringVar>("liff.engineer@gmail.com");
var = std::make_shared<ArticleVar>(Article("长不胖的Garfield","C++的类型擦除及应用"));
```

由此可以看出来,类型擦除的方式具备以下优点:

- 新类型无需派生自接口,无依赖/耦合
- 使用方便,简洁易懂
- 无需关注生命周期

## 应用场景

满足可扩展需求的前提是要满足功能性需求,譬如可以存取任意类型,具备一些功能接口,以下从几个示例来看一下应用:

### 以事件驱动架构中的事件表示为例

在以事件驱动为核心的架构中,不同事件的表示和存储是基本的要求.譬如在Qt中,如果要定义新的事件类型,则需要派生自`QEvent`,并且还要申请事件类型编码:

```C++
class MyEvent:public QEvent {
public:
    static const QEvent::Type myType = static_cast<QEvent::Type>(2000);//为事件申请的编号
    MyEvent():QEvent(myType){};
    //事件负载
    MyPayload payload;
};
```

使用时如下:

```C++
void handleEvent(QEvent* event)
{
    //判断类型是否为MyEvent
    if(event->type() == MyEvent::myType){
        auto realEvent = static_cast<MyEvent*>(event);
        //执行命令
       	execute(realEvent->payload);
    }
}
```

而采用类型擦除技术设计的类型在实现上述代码时如下:

```C++
//按照常规方式定义即可
struct MyEvent{
  MyPayload payload;  
};
```

使用如下:

```C++
void handleEvent(Event* event)
{
    //判断类型是否为MyEvent
    if(event->is<MyEvent>()){
        //执行命令
       	execute(event->as<MyEvent>().payload);
    }
}
```

注意这时的`Event`定义类似如下:

```C++
//基于类型擦除技术设计的事件类
class Event {
public:
    //判断是否是某类型
    template<typename T>
    bool is() const noexcept;
    
    //获取某类型值
    template<typename T>
    const T& as() const;//const版本,不能修改值,注意类型不一致会抛出异常
    
    template<typename T>
    T& as();//可以修改值,注意类型不一致会抛出异常
};
```

### 以命令模式为例

传统的命令模式设计如下:

```C++
class ICommand{
public:
    virtual ~ICommand()=default;
    virtual void execute() = 0;
};

class XXXCommand:public ICommand{
public:
    void execute() override{
        //实现
    }
};

std::vector<std::shared_ptr<ICommand>> createCommand(){
    std::vector<std::shared_ptr<ICommand>> results;
    results.emplace_back(std::make_shared<XXXCommand>());
    return results;
}

void process(std::vector<std::shared_ptr<ICommand>> const& commands){
    for(auto& e:commands){
        e->execute();
    }
}
```

使用类型擦除技术实现的命令模式如下:

```C++
//类形式的命令
class XXXCommand{
public:
    void execute(){
        //实现
    }  
};

//可调用对象的命令适配
class Commander{
    std::function<void()> fn;
public:  
    void execute(){
        fn();
    }
};

std::vector<Command> createCommands(){
    std::vector<Command> results;
    results.emplace_back(XXXCommand());
    results.emplack_back(Commander(GlobalFunction)); 
    results.emplack_back(Commander([](){
        //lambda body
    }));    
    return results;
}

void process(std::vector<Command> const& commands){
    for(auto& e:commands){
        e.execute();
    }
}
```

可以看到,使用类型擦除,你可以按照场景选择合适的实现方式,而无需关注使用者的附加要求.这在设计通用性组件时非常有用,因为各个平台/框架对于其上的组件有不同的约束.

## 总结

基于类型擦除技术,能够在满足扩展性的同时减少依赖,达到“高内聚,低耦合,易扩展”的效果.