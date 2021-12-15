# `ECS`架构模式中的`EC`实现及应用

在游戏行业中应用的`ECS`架构模式,其设计理念和思路很有意思.虽然不一定适用于其它业务场景,但是它对模型的表达,却可以借鉴.

这里首先简要描述一下`ECS`架构模式,然后阐述用`EC`来表达模型的优势,最终展示一种实现方式,以及它可能的应用场景.

## 什么是`ECS`架构模式?

`ECS`即`Entity Component System`的简写,它由三种概念组合而成:

-  实体`Entity`:用来区分不同的对象或实例,其概念与领域驱动设计中的一致,通常实现为整数;
- 数据组件`Component`:表达某种类型的值,通常会聚合对象的属性来构成,其概念与领域驱动设计中的值对象一致;
- 系统`System`:业务逻辑实现.

如果说面向对象设计中,对象由数据/状态、操作组合而成,区分不同对象时采用`ID`成员/属性,那么`ECS`就是把对象的内容打散了:

- 对象的属性/数据/状态,按照业务场景组合形成普通的结构体,即`Component`;
- 围绕对象属性的操作,拆分成不同的`System`;
- 用来区分不同对象/实例的`ID`属性等,被抽象为存粹的整数型`ID`.

这种方式,能够达成如下效果:

- 可以通过为实体赋予不同的数据组件,来表达不同类型的对象,而不再限制为某种类对象;
- 数据组件可以任意组合,从而驱动不同的业务场景;
- 业务逻辑只存在与系统中.

## 采用`EC`表达模型的优势

即使不使用`ECS`架构模式,使用`EC`来表达模式同样具备非常明显的优势:

- 通用实现;
- 组合式设计;
- 极强的扩展能力;
- 采用合适的实现,可以具备极强的性能表现.

## `EC`实现`API`设计

面向对象设计时,开发者经常会定义某一类型,然后在为其定义管理类,以管理这种类型的实例.例如:

```C++
class MyClass{};

class MyClassManager{
public:
    //创建类实例
    MyClass* Create();
    //销毁类实例
    void     Destory(MyClass*);
    //查找类实例
    MyClass* Find(std::string const& name);
    //查找多个类实例
    std::vector<MyClass*> Search();
};
```

较好的实践是由专门的存储类来管理实例,其它业务逻辑封装到服务中,即采用存储库`Repository`模式来管理类实例.

因而这里提供的`EC`实现接口包含三种概念:

| 概念                | 作用                            |
| ------------------- | ------------------------------- |
| 存储库`Repository`  | 存储实体                        |
| 实体`Entity`        | 表达某个对象,可以获取其上的数据 |
| 数据组件`Component` | 存粹的数据定义                  |

首先来看一下存储库`Repository`的接口:

| 接口                              | 说明                                      |
| --------------------------------- | ----------------------------------------- |
| `entity_view<> create()`          | 创建实体                                  |
| `void destory(int id)`            | 销毁实体                                  |
| `entity_view<Ts...> find(int id)` | 根据`id`查找实体                          |
| `each<Ts...>(Fn&& fn)`            | 遍历所有实体,不保证实体具有所有的数据组件 |
| `foreach<Ts...>(Fn&& fn)`         | 遍历所有包含指定数据组件类型的实体        |

由于实体`Entity`由存储库`Repository`管理,实体提供`entity_view<Ts...>`来访问、操作实体信息:

| 接口                          | 说明                                    |
| ----------------------------- | --------------------------------------- |
| `int id()`                    | 获取实体的标识符                        |
| `bool exist<T>()`             | 释放存在类型`T`的数据                   |
| `void assign(Args&& ...args)` | 为实体设置指定数据,支持多种类型同时设置 |
| `T& view<T>()`                | 查看某种数据,非`const`版本              |
| `const T& view<T>()`          | 查看某种数据,`const`版本                |
| `void remove<T>()`            | 移除某种数据                            |

当开发者使用`entity_view<>`时,它可以操作任意类型的数据;而一旦指定了所包含的数据组件类型,则只能操作限定的类型.这两种形态可以互相转换,譬如`entity_view<>`可以转换为`entity_view<DataType1,DataType2>()`.

如果能够确定要访问的数据组件类型,建议指定好,效率相对会快一点.

## 参考实现及示例

参考实现位于[https://gitee.com/liff-engineer/articles/tree/master/20211215](https://gitee.com/liff-engineer/articles/tree/master/20211215).

使用示例如下:

```C++
//定义实体的存储库
repository repo{};
{
    //创建实体
    auto e = repo.create();
    //赋值
    e.assign(1024, std::string{ "liff.engineer@gmail.com" });
}
{
    //创建实体,注意返回值修改了
    repository::entity_view<double,std::string> e = repo.create();
    //赋值
    e.assign(3.1415926, std::string{ "liff.engineer@gmail.com" });
}
    
std::vector<int> is{};
std::vector<std::string> ss{};
//遍历所有实体,注意e的类型可以为任意的repository::entity_view<Ts...>
repo.each<int, std::string>([&](auto const& e) {
    //检查int数据是否存在
    if (e.exist<int>()) {
        //存在则读取
        is.emplace_back(e.view<int>());
    }
    if (e.exist<std::string>()) {
        ss.emplace_back(e.view<std::string>());
    }
});

//遍历所有包含double,std::string两种类型数据的实体
//注意其回调函数接口依次是标识符、double、std::string
//不需要再通过entity_view读取
repo.foreach<double,std::string>([&](int id,double dv,auto&& v) {
    ss.emplace_back(v);
});
```

## 应用场景

在使用`Qt`实现软件界面时,会面临很多界面需求,这就需要开发者频繁地实现相似的代码;当希望设计通用的界面组件时,开发者一般是通过定义接口并派生的方式,最终实现怪物一般的接口类,使得使用者无从下手.

这里可以考虑借用上述`EC`实现,以填充数据的方式来进行扩展,让使用者可以在不了解`Qt`的前提下通过调整数据类,来驱动呈现不同的界面;由于是解耦设计,界面实现可以按需调整,相互影响可以最小化.

譬如,无论是普通的菜单栏,还是`Ribbon`界面,在定义层面是一致的,完全可以以数据结构来表达,通过填充不同的数据改变界面呈现.这里暂不展开描述,后续将会呈现相关示例.



