#include "Graph.hpp"
#include <iostream>

using namespace abc;
namespace demo
{
    //var,+,=,print
    class Proto:public IProto {
    protected:
        std::string m_code;
        std::vector<std::string> m_inputs;
        std::vector<std::string> m_outputs;
    public:
        const std::string& code() const noexcept final {
            return m_code;
        }
        const std::vector<std::string>& inputs() const noexcept final {
            return m_inputs;
        }
        const std::vector<std::string>& outputs() const noexcept  final {
            return m_outputs;
        }
    };

    class VarProto :public Proto
    {
    public:
        VarProto()
        {
            m_code = "Var";
            m_inputs = { "In" };
            m_outputs = { "Result" };
        }

        void run(abc::DataRepository& repo, int id) const noexcept override {
            //std::cout << m_code << " " << id << "\n";
            DataIndex in = DataIndex::Create("Var::In",id);
            DataIndex result = DataIndex::Create("Var::Result", id);
            auto vp = repo.Get<double>(in);
            if (vp) {
                repo.Set(result, *vp);
            }
            else
            {
                repo.Reset(result);
            }
        }
    };

    class PlusProto :public Proto
    {
    public:
        PlusProto()
        {
            m_code = "+";
            m_inputs = { "Lhs","Rhs" };
            m_outputs = { "Result" };
        }

        void run(abc::DataRepository& repo, int id) const noexcept override {
            //std::cout << m_code << " " << id << "\n";
            DataIndex lhs = DataIndex::Create("+::Lhs", id); 
            DataIndex rhs = DataIndex::Create("+::Rhs", id);
            DataIndex result = DataIndex::Create("+::Result", id);
            auto vlhs = repo.Get<double>(lhs);
            auto vrhs = repo.Get<double>(rhs);
            if (vlhs && vrhs ) {
                repo.Set(result, *vlhs+*vrhs);
            }
            else
            {
                repo.Reset(result);
            }
        }
    };

    class PrintProto :public Proto
    {
    public:
        PrintProto()
        {
            m_code = "Print";
            m_inputs = { "In" };
        }

        void run(abc::DataRepository& repo, int id) const noexcept override {
            //std::cout << m_code << " " << id << "\n";
            DataIndex in = DataIndex::Create("Print::In", id);
            auto vp = repo.Get<double>(in);
            if (vp) {
                std::cout << *vp << "\n";
            }
        }
    };
}


abc::DataIndex IProto::SetupDR(abc::DRGraph& dr, IProto& proto, int id)
{
    DataIndex obj = DataIndex::Create(proto.code(), id);
    //节点输入依赖于自身
    for (auto& input : proto.inputs()) {
        dr.AddEdge(
            DataIndex::Create(proto.code() + "::" + input, id),
            obj
        );
    }
    //输出依赖于自身
    for (auto& output : proto.outputs()) {
        dr.AddEdge(obj,
            DataIndex::Create(proto.code() + "::" + output, id));
    }
    return obj;
}


void ProtoRepository::SetupDemo()
{
    {
        auto r = std::make_unique<demo::VarProto>();
        protos[r->code()] = std::move(r);
    }
    {
        auto r = std::make_unique<demo::PlusProto>();
        protos[r->code()] = std::move(r);
    }
    {
        auto r = std::make_unique<demo::PrintProto>();
        protos[r->code()] = std::move(r);
    }
}



int Graph::AddNode(const std::string& proto)
{
    m_types.emplace_back(ElementType::Node);
    auto index = static_cast<int>(m_types.size() - 1);
    m_nodes[index].proto = proto;
    {//建立数据关系
        auto it =  protoRepo.protos.find(proto);
        if (it != protoRepo.protos.end()) {
            auto obj = IProto::SetupDR(drGraph, *it->second.get(), index);
            drGraph.AddTask(obj, [=]() {
                this->run(index);
                });
        }
    }
    {//发出Node创建通知
        drGraph.Traversal(
            DataIndex::Create(
                "Graph.Node.Create", 
                index
            )
        );
    }
    return index;
}

int Graph::AddConnect(int srcNode, const std::string& srtNodePort, int dstNode, const std::string& dstNodePort)
{
    m_types.emplace_back(ElementType::Edge);
    auto index = static_cast<int>(m_types.size() - 1);
    m_edges[index] = ConnectData{ srcNode,srtNodePort ,dstNode,dstNodePort };

    {//建立数据关系

        DataIndex src = DataIndex::Create(
                m_nodes[srcNode].proto+"::"+ srtNodePort, srcNode
            );
        
        DataIndex dst = DataIndex::Create(
                m_nodes[dstNode].proto + "::" + dstNodePort,dstNode
        );

        //创建连接DI,负责更新数据,一旦连接的目标修改,则需要重新建立
        //Task和Edge
        DataIndex con = DataIndex::Create("Connect", index);
        drGraph.AddTask(con, [&,con,src,dst]() {
            //std::cout << *con.description << " " << con.tag << "\n";
            if (auto vp = dbRepo.Get<double>(src)) {
                dbRepo.Set(dst, *vp);
            }
            else {
                dbRepo.Reset(dst);
            }
            });
        drGraph.AddEdge(src, con);
        drGraph.AddEdge(con, dst);
    }
    {//发出Connect创建通知
        drGraph.Traversal(
            DataIndex::Create(
                "Graph.Connect.Create",
                index
            )
        );
    }
    return index;
}

void Graph::SetupDemo()
{
    protoRepo.SetupDemo();

    auto a = AddNode("Var");
    auto b = AddNode("Var");
    auto c = AddNode("Var");
    auto plus = AddNode("+");
    auto print = AddNode("Print");

    //a+b
    AddConnect(a, "Result", plus, "Lhs");
    AddConnect(b, "Result", plus, "Rhs");
    //=c
    AddConnect(plus, "Result", c, "In");
    //print(c)
    AddConnect(c, "Result",print, "In");
}

void Graph::DemoSetA(double a)
{
    //a的索引为0
    DataIndex ai = DataIndex::Create("Var::In", 0);
    dbRepo.Set(ai, a);
    //更新值后重算
    drGraph.Traversal(ai);
}

void Graph::DemoSetB(double b)
{
    //b的索引为0
    DataIndex bi = DataIndex::Create("Var::In", 1);
    dbRepo.Set(bi, b);
    //更新值后重算
    drGraph.Traversal(bi);
}

void Graph::run(int node)
{
    auto proto = m_nodes[node].proto;
    protoRepo.protos.at(proto)->run(dbRepo,node);
}
