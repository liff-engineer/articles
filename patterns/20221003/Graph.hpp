#pragma once
#include <string>
#include <vector>
#include <memory>

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

class Document;

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

class DataSetCluster;
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
