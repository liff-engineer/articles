#include "Graph.hpp"

class IDataSet {
public:
    virtual ~IDataSet() = default;
};

template<typename T>
struct EntitySet final : public IDataSet, Container<T> {
    unsigned next{1};
    std::vector<std::pair<unsigned, std::unique_ptr<T>>> elements;

    unsigned create() {
        elements.emplace_back(std::make_pair(next++, std::unique_ptr<T>{}));
        return elements.back().first;
    }

    EntityRef<T*> assign(unsigned id, std::unique_ptr<T> obj) {
        //element是有序的,可以考虑二分查找
        for (auto& [i, v] : elements) {
            if (i == id) {
                v = std::move(obj);
                return { id,v.get() };
            }
        }
        return {};
    }

    EntityRef<T*> find(unsigned id) noexcept override {
        for (auto& [i, v] : elements) {
            if (i == id) {
                return { id,v.get() };
            }
        }
        return {};
    }

    EntityRef<const T*> find(unsigned id) const noexcept override {
        for (auto& [i, v] : elements) {
            if (i == id) {
                return { id,v.get() };
            }
        }
        return {};
    }

    bool contains(unsigned id) const noexcept  override {
        return find(id).operator bool();
    }

    bool destory(unsigned id) override {
        for (auto& [i, v] : elements) {
            if (i == id) {
                v.reset();
                return true;
            }
        }
        return false;
    }
};

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

EntityRef<Rule*> Rule::Create(Document* doc, std::string code, std::string description, std::vector<Port>&& ports)
{
    auto repo = doc->cluster()->get<Rule>();
    //检查code是否存在
    for (auto& [i, v] : repo->elements) {
        if (v && v->code() == code) {
            return {};
        }
    }
    auto id = repo->create();
    return repo->assign(id, std::make_unique<Rule>(Rule{ id,code,description,std::move(ports) }));
}

EntityRef<Node*> Node::Create(Document* doc, EntityRef<const Rule*> rule, Point position)
{
    auto repo = doc->cluster()->get<Node>();
    auto id = repo->create();
    return repo->assign(id, std::make_unique<Node>(Node{ id,rule,position }));
}

EntityRef<Connect*> Connect::Create(Document* doc, EntityRef<const Node*> src, std::string srcPortCode, EntityRef<const Node*> dst, std::string dstPortCode)
{
    auto repo = doc->cluster()->get<Connect>();
    auto id = repo->create();
    return repo->assign(id, std::make_unique<Connect>(Connect{ id,src,srcPortCode,dst,dstPortCode }));
}

Document::Document()
    :m_cluster(std::make_unique<DataSetCluster>())
{}

Document::~Document() = default;

Container<Rule>* Document::GetView(Tag<Rule>)
{
    return m_cluster->get<Rule>();
}

Container<Node>* Document::GetView(Tag<Node>)
{
    return m_cluster->get<Node>();
}

Container<Connect>* Document::GetView(Tag<Connect>)
{
    return m_cluster->get<Connect>();
}
