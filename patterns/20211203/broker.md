# 支持`Actor`编程模型的同步消息机制设计

编程范式或者说编程模式是开发者思考、表达软件设计所采用的心智模型,不同的心智模型各有优劣,需要在应对不同场景时权衡. 正如面向对象设计中所有事物都是对象一样,`Actor model`概念也非常简单,代码由`Actor`构成,而之间的交互使用消息传递`Message passing`.

相较于`OOP`中接口的滥用导致各种耦合,`Actor`则是依赖于消息:消息驱动`Actor`创建、动作、销毁,通过传递消息构成整个结构.这就给开发者试图实现极具扩展性、复用性,又不希望存在较多耦合的设计提供了一些参考方向.

这里基于现实场景的需求,提供同步消息机制设计,以支持开发者采用类似`Actor`编程模型的方式开发软件.

## 需求分析

无论是事件机制、监听、观察、信号与槽,还是接口调用,从消息传递的视角来看,它需要考虑以下因素

- 参与的角色有多少
- 是否需要回复

无论是消息的发送者,还是接收者,都可能是1个,也可能是`n`个.

而根据是否需要回复,则分为:

1. 发布-订阅模式
2. 请求-回复模式

通过建立消息中间人`broker`的概念,可以应对发送者、接收者个数不确定的场景:

- 发送者可以持有`broker`,使得接收者只能拿到该`broker`才能接受消息,使得发送者限制到`1`个;
- 发送者可以发送消息到公共的`broker`,使得发送者可以扩展到`n`个.

而消息中间人也可以工作在发布-订阅模式、或者请求-回复模式,甚至是同时.以此来满足不同的需求.

## 设计

针对目标场景,存在以下概念:

| 模式      | 概念              | 说明                         |
| --------- | ----------------- | ---------------------------- |
| 发布-订阅 | 消息类型          | 传递的信息类型               |
| 发布-订阅 | 发布者`publisher` | 消息发出者                   |
| 发布-订阅 | 订阅者            | 消息接收者                   |
| 请求-回复 | 请求参数类型      | 发送请求时可能附加的参数类型 |
| 请求-回复 | 请求回复类型      | 要求返回的信息类型           |
| 请求-回复 | 客户端`client`    | 请求发出方                   |
| 请求-回复 | 服务端            | 请求回复方                   |

而消息机制无法替代开发者的业务模块,所以针对如下概念它要支持扩展:

- 消息类型
- 请求参数类型
- 请求回复类型

而针对如下概念它要做出限制：

- 订阅者
- 服务端

对于如下概念,它要提供通用实现,或支持:

- 发布者
- 客户端

因而设计核心类消息中间人`Broker`,具备以下特性:

1. 可以作为发布者的一部分,或者发布者的信息发布渠道;
2. 可以作为客户端的一部分,或者客户端的请求渠道;
3. 可以发布任意类型的消息;
4. 可以请求任意类型的回复,请求时的参数可选.

同时,对于订阅者、服务端的限制也局限为能够接受消息. 这里以`C++`的`lambda`表达式为载体,并且为了易用性,约定如下:

- 订阅者的成员函数`on`可以作为默认的消息接受函数
- 服务端的成员函数`reply`可以作为默认的请求回复函数

订阅者与`broker`建立订阅关系时,需要通过`broker::channel`方法获取其订阅的消息频道,然后就可以通过以下两种方式:

- 自由函数`subscribe`
- 辅助类`subscriber`:方便订阅者是某个类的情况

与之类似,服务端建立回复关系时,首先需要通过`broker::endpoint`方法获取其回复的请求端口,然后可以通过类似的两种方式:

- 自由函数`bind`
- 辅助类`binder`:方便服务端是某个类的情况

考虑到生命周期管理,即订阅者、服务端被销毁的情况,提供`broker::action_stub`概念,作为它们和`broker`建立关系的凭证/存根,提供释放,或者析构时自动释放的能力,以支持订阅者、服务端被销毁时自动移除.

总结一下,设计由以下类及自由函数构成:

| 要素                  | 类型 | 作用                                  |
| --------------------- | ---- | ------------------------------------- |
| 消息中间人`broker`    | 类   | 用来发送消息,或者请求回复             |
| `subscriber`          | 类   | 辅助订阅者与`broker`建立发布-订阅关系 |
| `binder`              | 类   | 辅助服务端与`broker`建立请求-回复关系 |
| `subscribe`           | 函数 | 建立发布-订阅关系                     |
| `bind`                | 函数 | 建立请求-回复关系                     |
| `broker::action_stub` | 类   | 持有可移除的发布-订阅、请求-回复关系  |

## `API`说明

### `broker`

发布消息:

```C++
template<typename T>
void publish(T const& msg);
```

发送请求,需提供回复结果的引用:

```C++
template<typename T,typename R>
void request(T const& msg,R& result);
template<typename R>
void request(R& result);
```
发送请求,需提供回复结果处理类,该类需提供`R& alloc()`以及`value()`两个函数,可参考代码中的`response<R>`实现:

```C++
template<typename T,typename Op>
auto request(T const& msg,Op& op)->decltype(op.value());
template<typename Op>
auto request(Op& op)->decltype(op.value());
```
获取消息发布频道:

```C++
template<typename T>
channel_stub<T> channel();
```
获取请求回复端口,请求参数为空时`T=void`:
```C++
template<typename T,typename R>
endpoint_stub<T,R> endpoint();
```

### `broker::action_stub`

析构时自动销毁与`broker`建立的订阅、回复关系.

追加关系:

```C++
action_stub& operator+=(action_stub&& other) noexcept;
```

释放关系:

```C++
void release();
```

### `subscribe`与`subscriber`

使用`subscribe`时需提供`broker`的`channel`以及回调函数:

```C++
template<typename T, typename Fn>
broker::action_stub subscribe(T& source, Fn&& fn);
```

使用`subscriber`则需要提供订阅类的引用:

```C++
template<typename T>
class subscriber {
	T* m_obj{};
public:
	explicit subscriber(T& obj);
```

`subscriber`提供一系列的`subscribe`接口,与自由函数版的不同之处在于其第一个参数为订阅类指针.

当不提供回调函数时,采用订阅类的成员函数`on`.

### `bind`与`binder`

使用`bind`时需要提供`broker`的`endpoint`以及回调函数:

```C++
template<typename T, typename R, typename Fn>
broker::action_stub bind(broker::endpoint_stub<T, R>& ep, Fn&& fn) ;
```

使用`binder`则需要提供回复类的引用:

```C++
template<typename T>
class binder {
	T* m_obj{};
public:
	explicit binder(T& obj);
```

`binder`提供一系列的`bind`接口,与自由函数版的不同之处在于其第一个参数为回复类指针.

当不提供回调函数时,采用回复类的成员函数`reply`.

## 参考实现

[https://github.com/liff-engineer/articles/blob/master/patterns/20211203/broker.hpp](https://github.com/liff-engineer/articles/blob/master/patterns/20211203/broker.hpp).

需要支持`C++17`的编译器,可以通过调整以支持`C++14`,即`Visual Studio 2015`.

## 使用示例

```C++
struct Actor
{
    broker::action_stub stub;
    void launch(broker& client) {
        //binder的使用
        //这里使用了成员函数reply
        stub += binder(*this).bind(client.endpoint<int, double>());
    }

    void reply(int query, double& reply) {
        reply *= query;
    }
};

void example() {
    broker client{};
	//bind的使用
    auto stub = bind(client.endpoint<int, double>(),
        [](auto&& query, auto&& reply) {
            if (query == 1) {
                reply = 1.414;
            }
            else if (query == 2) {
                reply = 1.73;
            }
            else {
                reply = 3.1415926;
            }
        });
    Actor actor{};
    actor.launch(client);

    {//跳出作用域自动取消关系
        auto astub = bind(client.endpoint<int, double>(), 
            [](auto&& query, auto&& reply) 
            { reply = query * 2.0; });
        double dV = {};
        client.request(10, dV);
        std::cout << "responce:" << dV << "\n";
        dV = {};
        client.request(1, dV);
        std::cout << "responce:" << dV << "\n";
    }
    
    stub += bind(client.endpoint<int, double>(), 
        [](auto&& query, auto&& reply) 
        { reply = query * 2.0; });
    //回复合并类,这里的v1和v2是vector<double>
    auto v1 = client.request(2, response<double>{});
    auto v2 = client.request(10, response<double>{});
}
```

## 总结

开发者的设计是由认知决定的,该以何种方式去认识、去思考? 或许需要多打开一下视野,尝试不同的方式.



