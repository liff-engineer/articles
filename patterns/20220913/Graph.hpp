#pragma once

#include "DRGraph.hpp"

class IProto {
public:
    virtual ~IProto() = default;
    virtual const std::string& code() const noexcept = 0;
    virtual const std::vector<std::string>& inputs() const noexcept = 0;
    virtual const std::vector<std::string>& outputs() const noexcept = 0;
    virtual void run(abc::DataRepository& repo,int id) const noexcept = 0;

    static abc::DataIndex SetupDR(abc::DRGraph& dr,IProto& proto,int id);
};

struct ProtoRepository {
    std::unordered_map<std::string, std::unique_ptr<IProto>> protos;

    void SetupDemo();
};

class Node;
class Connect;


class Graph {
public:
    ProtoRepository protoRepo;
    abc::DRGraph   drGraph;
    abc::DataRepository dbRepo;
public:
    int AddNode(const std::string& proto);
    int AddConnect(int srcNode, const std::string& srtNodePort, int dstNode, const std::string& dstNodePort);

    void SetupDemo();

    void DemoSetA(double a);
    void DemoSetB(double b);
private:
    //计算函数?
    void run(int node);
private:
    friend class Node;
    friend class Connect;

    struct ConnectData {
        int srcNode;
        std::string srcNodePort;
        int dstNode;
        std::string dstNodePort;
    };

    struct NodeData {
        std::string proto;
    };

    enum class ElementType {
        Node,
        Edge
    };
    
    std::vector<ElementType> m_types;
    std::unordered_map<int, NodeData> m_nodes;
    std::unordered_map<int, ConnectData> m_edges;
};
