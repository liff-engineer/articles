[TOC]

# 采用数据集抽象的应用程序模型设计思路

在《一种应用程序模型的设计思路-以节点编辑器为例》中介绍过一种结构化的模型表达方法，即拆分出如下基本要素：

- 实体 `Entity`
- 值对象 `ValueObject`或数据组件 `Component`
- 存储库 `Repository`
- 工作单元 `UnitOfWork`

这里在此基础上更进一步，来看看是否能够更加简洁、更容易与现有代码结合。

## 原有设计思路的问题

### 1. 存储库设计/代码重复

针对每种实体，均需要提供存储库 `Repository`，这些设计与实现大同小异。

### 2. 不易用

其中采用了各种标识符，而不是直接使用实体指针，导致需要编写大量代码来获取想要的信息， 如果能够使用指针，则代码量会大幅减少，例如节点`Node`：

```C++
class Node{
private:
    const Rule* m_rule;//节点的规则
};
```

通过成员变量`m_rule`，可以直接获取节点的输入、输出端口，如果用其标识符`ruleCode`，则：

1. 先获取规则存储库`RuleRepository`；
2. 然后根据`ruleCode`查询规则信息；
3. 从规则信息上获取输入、输出端口。

### 3. 较难与现有设计整合

通常开发者不是在全新的代码基上工作，怎么和现有设计/实现整合是比较麻烦的事情。采用这种结构化的模型表达方法固然清晰，但是和已有代码不好融合。

### 4. 性能隐患

每次需要获取实体某些信息，就要根据标识符去查找，即使查找算法性能再高，也不如直接访问成员变量，获取对象指针来得直接迅速。

## 改进方向

考虑到上述问题，新的设计思路从如下角度进行调整：

- 在设计中采用指针，以解决不易用、性能隐患等问题，同时提供指针有效性应对的方案；
- 对实体及存储库进行归一化设计，可以基于场景分析形成几种存储库实现，以解决设计/代码重复问题；
- 移除值对象 `ValueObject`及工作单元`UnitOfWork`要素，减少设计约束，以更好地和现有设计融合。

## 数据集抽象

这里依然保留实体`Entity`要素，由于值对象 `ValueObject`用来构成实体，设计中交由开发者自行考虑。从实现角度来看，实体有着比较明显的特点：无论实体的标识符是整数抑或字符串，它在应用程序运行过程中是需要一个类实例来表达的，即运行时可以用类实例的指针作为标识符；而存储库`Repository`核心目的是存储实体，可以理解为实体容器。

将某种实体视作一种数据，那么存储库即为数据集，而应用程序模型则是由不同且互相关联的数据集构成。

| 要素   | 新概念     | 说明                             |
| ------ | ---------- | -------------------------------- |
| 实体   | 实体/数据  | 由标识符、内存地址构成           |
| 存储库 | 数据集     | 容器语义，由标识符、内存地址构成 |
| 模型   | 数据集集合 | 容器语义，标识符可选             |

### 数据要素说明

譬如节点`Node`，无论开发者如何设计，它需要整数作为其标识符：

```C++
class Node {
private:
    unsigned m_id;//标识符
    EntityRef<const Rule*> m_rule;//规则
    Point m_position;
};
```

### 数据集要素说明

当构造出新的`Node`实例，就需要分配新的`id`，这时的节点数据集可以采用如下方式表达：

```C++
class NodeContainer{
    unsigned next; //下一个id
	std::vector<std::unique_ptr<Node>> nodes;    //所有node实例
};
```

即每个`Node`实例都由其标识符`m_id`，内存地址`std::unqiue_ptr<Node>`构成。

### 数据集集合要素说明

而模型包含了规则数据集和节点数据集，则其设计可以是：

```c++
//数据集抽象
class IContainer{};
//规则数据集
class RuleContainer:public IContainer{};
//节点数据集
class NodeContainer:public IContainer{};

class DataSetCluster{
private:
    //下一个数据集标识
    unsigned next;
    //不同的数据集
    std::vector<std::unique_ptr<IContainer>> datasets;
};
```

当然，数据集又可以由数据集构成，这里不再赘述。

### 指针的使用

由上述说明可知，在应用程序运行时，实体除了由标识符来识别，也有对应的内存地址，这两者一一匹配，开发者完全可以将其作为标识符使用；在设计不同的实体时，如果它依赖其它实体，完全可以直接使用其内存地址。

不过指针有效性是比较麻烦的事情，有可能某个实体已经被删除了，这时指针是悬挂的，有两种互不冲突的应对策略：

1. 在数据集上提供方法来判断指针是否已经失效；
2. 通过机制或编码保证指针要么有效，要么被置空。

当然也可以采用智能指针，例如`std::shared_ptr`或`std::weak_ptr`。

### 归一化

在这种抽象中，对实体的要求非常少，考虑到数据集及外部使用的要求，无非是：

1. 标识符及其分配机制；
2. 实体的构造。

如果实体没有标识符，仅仅使用内存地址来区分，则对应数据集可以设计为:

```C++
struct EntitySet{
    std::vector<std::unique_ptr<E>> elements;
};
```

如果实体的标识符为整数，则对应数据集可以设计为：

```C++
struct EntitySet{
    unsigned next;
    std::vector<std::unique_ptr<E>> elements;
};
```

考虑到设计的一致性及性能考虑，可以统一约定实体标识符为无符号整数，并将标识符存储起来，任何数据集统一做如下设计：

```C++
struct EntitySet{
    unsigned next{1};
    std::vector<std::pair<unsigned,std::unique_ptr<E>>> elements;
};
```

- 实体标识符为无符号整数，必要时可以存储指针地址，从而适应无标识符的情况；
- 采用`vector`，通过标识符`unsigned`排序，能够最大化遍历、查找性能。

以此类推，数据集集合的设计也是类似：

```C++
struct DataSetCluster{
    unsigned next{1};
    std::vector<std::pair<unsigned,std::unique_ptr<IContainer>>> datasets;
};
```

通过这种归一化的设计，无论是某个数据集，还是某条数据，均可以用整数表达，譬如:

```C++
using  DataSetKey = unsigned;

struct EntityKey{
    DataSetKey   dataset;
    unsigned     id;
};
```

### 与现有设计融合

根据前面的描述，这种设计思路对开发者现有代码的要求非常少：

1. 开发者已有实体设计与实现：提供对应的数据集实现即可；
2. 开发者已有存储库设计与实现：调整存储库基类，使其能够集成到数据集集合即可。
3. 开发者已有模型设计：将模型调整为数据集集合形态即可。

## 设计示例

这里根据数据集抽象和设计思路，对节点编辑器的模型重新设计。

### 基础结构

 这里假设所有实体均为其分配无符号整数作为标识符，提供`EntityRef`来同时存储其标识符和内存地址：

```C++
template<typename T>
struct EntityRef {
    static_assert(std::is_pointer_v<T>, "std::is_pointer_v<T>");
    using const_t = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>;

    unsigned id;
    T        vp;

    explicit operator bool() const noexcept {
        return (id != 0) && (vp);
    }

    operator EntityRef<const_t>() const noexcept {
        return { id,vp };
    }

    const_t operator->() const noexcept { return vp; }
    T operator->() noexcept { return vp; }
};
```

然后提供数据集抽象`Container<T>`：

```C++
class IContainer {
public:
    virtual ~IContainer() = default;
    virtual bool destory(unsigned id) = 0;
    virtual bool contains(unsigned id) const noexcept = 0;
};

template<typename T>
class Container :public IContainer {
public:
    bool destory(EntityRef<const T*> vp) {
        return destory(vp.id);
    }

    bool contains(EntityRef<const T*> vp) const noexcept {
        return contains(vp.id);
    }

    virtual EntityRef<T*> find(unsigned id) noexcept = 0;
    virtual EntityRef<const T*> find(unsigned id) const noexcept = 0;
};
```

之后提供数据集默认实现，以及数据集集合：

```C++
//数据集抽象
class IDataSet {
public:
    virtual ~IDataSet() = default;
};

//数据集默认实现
template<typename T>
struct EntitySet final : public IDataSet, Container<T> {
    unsigned next{1};
    std::vector<std::pair<unsigned, std::unique_ptr<T>>> elements;
};

//数据集集合
class DataSetCluster {
public:
    template<typename T>
    EntitySet<T>* get() {
        for (auto& o : m_datasets) {
            if (o.code == typeid(T).name()) {
                return dynamic_cast<EntitySet<T>*>(o.v.get());
            }
        }
        m_datasets.emplace_back(DataSet{ typeid(T).name(),next++,
            std::make_unique<EntitySet<T>>()
            });
        return dynamic_cast<EntitySet<T>*>(m_datasets.back().v.get());
    }
private:
    unsigned next{1};
    struct DataSet {
        std::string code;
        unsigned id;
        std::unique_ptr<IDataSet> v;
    };

    std::vector<DataSet> m_datasets;
};
```

### 节点编辑器模型实现

规则实体`Rule`：

```C++
enum class PortType {
    in,
    out,
};

struct Port {
    std::string code;
    std::string description;
    PortType type;
};

class Rule {
public:
    unsigned id() const noexcept { return m_id; }
    const std::string& code() const noexcept { return m_code; }

    const std::vector<Port>& ports() const noexcept { return m_ports; }
    std::vector<Port>& ports() noexcept { return m_ports; }
public:
    static EntityRef<Rule*> Create(Document* doc, std::string code, std::string description, std::vector<Port>&& ports);
private:
    explicit Rule(unsigned id, std::string code, std::string description, std::vector<Port> ports)
        :m_id(id), m_code(code), m_description(description), m_ports(ports) {};
private:
    unsigned    m_id;
    std::string m_code;
    std::string m_description;
    std::vector<Port> m_ports;
};
```

节点实体`Node`：

```C++
struct Point {
    double x;
    double y;
};

class Node {
public:
    unsigned id() const noexcept { return m_id; }
    EntityRef<const Rule*> rule() const noexcept { return m_rule; }

    const Point& position() const noexcept { return m_position; }
    Point& position() noexcept { return m_position; }
public:
    static EntityRef<Node*> Create(Document* doc, EntityRef<const Rule*> rule, Point position);
private:
    explicit Node(unsigned id, EntityRef<const Rule*> rule, Point position)
        :m_id(id), m_rule(rule), m_position(position) {};
private:
    unsigned m_id;
    EntityRef<const Rule*> m_rule;
    Point m_position;
};
```

连接实体`Connect`：

```C++
class Connect {
public:
    unsigned id() const noexcept { return m_id; }

    EntityRef<const Node*> src() const noexcept { return m_src; }
    EntityRef<const Node*> dst() const noexcept { return m_dst; }
    const std::string& srcPortCode() const noexcept { return m_srcPortCode; }
    const std::string& dstPortCode() const noexcept { return m_dstPortCode; }

    void setSrc(EntityRef<const Node*> node, std::string portCode) {
        m_src = node;
        m_srcPortCode = portCode;
    }

    void setDst(EntityRef<const Node*> node, std::string portCode) {
        m_dst = node;
        m_dstPortCode = portCode;
    }

public:
    static EntityRef<Connect*> Create(Document* doc, EntityRef<const Node*> src, std::string srcPortCode, EntityRef<const Node*> dst, std::string dstPortCode);
private:
    explicit Connect(unsigned id, EntityRef<const Node*> src, std::string srcPortCode, EntityRef<const Node*> dst, std::string dstPortCode)
        :m_id(id), m_src(src), m_srcPortCode(srcPortCode), m_dst(dst), m_dstPortCode(dstPortCode) {};
private:
    unsigned m_id;
    EntityRef<const Node*> m_src;
    std::string m_srcPortCode;
    EntityRef<const Node*> m_dst;
    std::string m_dstPortCode;
};
```

文档模型：

```C++
class Document {
public:
    Document();
    ~Document();

    template<typename T, typename... Args>
    EntityRef<T*> Create(Args&&... args) {
        return T::Create(this, std::forward<Args>(args)...);
    }

    template<typename T>
    Container<T>* View() {
        return GetView(Tag<T>{});
    }

    DataSetCluster* cluster() { return m_cluster.get(); }
protected:
    template<typename T>
    struct Tag {};
    Container<Rule>* GetView(Tag<Rule>);
    Container<Node>* GetView(Tag<Node>);
    Container<Connect>* GetView(Tag<Connect>);
private: 
    std::unique_ptr<DataSetCluster> m_cluster;
};
```

### 节点模型使用示例

```C++
#include "Graph.hpp"

int main()
{
    Document doc{};
	//创建规则
    auto r1 = doc.Create<Rule>("var", "variable",
        std::vector<Port>{
        Port{ "out","variable output",PortType::out }
    });

    //创建节点
    auto n1 = doc.Create<Node>(r1, Point{1.0,2.0});

    //查询规则
    auto rules = doc.View<Rule>();
    auto r2 = rules->find(1);
    
    //查询节点
    auto nodes = doc.View<Node>();
    auto n2 = nodes->find(1);
    return 0;
}
```

### 示例总结

从示例可以看到：

- 不再有存储库的设计和实现，只需要设计实体即可；
- 对象中使用了指针，可以直接通过指针获取信息，不再需要书写查找代码；
- 文档对象还可以继续扩充，通过一些调整，完全能够不修改文档代码来添加新的实体类型。

## 总结

通过对实体概念和现实实现的综合，这里将整个模型设计拆分成实体、实体数据集两种要素，由此形成三层结构：实体、实体数据集（另一个层面的实体）、数据集集合（另一个层面的数据集）。

实体交由开发者自由定义，数据集可以采用如下表达：

```C++
template<typename E>
struct EntitySet {
  	unsigned next{1};
    std::vector<std::pair<unsigned,std::unique_ptr<E>>> elements;
};
```

以此可以以简洁清晰的方式应对应用程序模型设计，并为后续设计带来更多可能性。

> 模型的表达可以通过该方式解决，但是模型之间的数据一致性是比较难处理的，通过归一化的设计，也可以有更简洁的方案来实现，后续将在《基于图的数据一致性保证机制》中阐述。





 

