#pragma once

#include <vector>
#include <string>
#include <unordered_map>

struct NodeConcept {
    struct Port {
        std::string code;
        std::string description;
    };
    std::string code;
    std::string description;
    std::vector<Port> inPorts;
    std::vector<Port> outPorts;
};

struct Node {
    int id;
    std::string code;
};

struct Edge {
    int id;
    int srcNode;
    std::string srcNodePort;
    int dstNode;
    std::string dstNodePort;
};

//节点只有添加、移除两种变化
struct NodeCreated {
    int id;
};

struct NodeRemoved {
    int id;
};

//边有添加、移除、修改三种变化
struct EdgeCreated {
    int id;
};
struct EdgeRemoved {
    int id;
};
struct EdgeChanged {
    int id;
};

struct Point {
    int x;
    int y;
};

//节点有中心位置,可能会发生变化
//从而引发端口变化,进而引发边的位置变化
struct NodeCenterChanged {
    int id;
};

struct EdgeStateChanged {
    int id;
};

struct NodePortState {
    std::string code;
    std::string description;
    Point  center;
};

struct NodeState {
    int node;
    Point center;
    std::string description;
    std::vector<NodePortState> inPorts;
    std::vector<NodePortState> outPorts;
};

struct EdgeState {
    int edge;
    Point srcCenter;
    Point dstCenter;
};

struct NodeConceptRepo {
    std::unordered_map<std::string, NodeConcept> concepts;
};

struct Graph
{
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, Edge> edges;
};

struct GraphState
{
    std::unordered_map<int, NodeState> nodeStates;
    std::unordered_map<int, EdgeState> edgeStates;
};


#include "Notifyer.hpp"

struct GraphStore {
    NodeConceptRepo* conceptRepo;
    Graph doc;
    abc::Notifyer<GraphStore> notifyer;

    int AddNode(std::string code);
    void RemoveNode(int id);
    int AddEdge(int srcNode,std::string srcNodePort, int dstNode, std::string sdtNodePort);
    void RemoveEdge(int id);
    void Modify(int id, Edge edgeInfo);
};

struct GraphStateStore {
    GraphStore* graph;
    GraphState states;
    abc::Notifyer<GraphStateStore> notifyer;
    abc::SubscribeStub subscribeStub;

    //订阅GraphStore投递的信息
    template<typename E>
    void on(const E& e) {
        notifyer.notify(*this, e);
    }

    void  AddNodeState(int node);
    void  AddEdgeState(int edge);
};

//Graph的一致性处理
struct NodeObserver
{
    void update(GraphStore& store, const NodeCreated& e);
    void update(GraphStore& store, const NodeRemoved& e);
};

//边的变化并不影响Graph内部一致性,需要向外通知
struct EdgeObserver
{
    template<typename E>
    void update(GraphStore& store, const E& e) {
        store.notifyer.publish(e);
    }
};

//GraphState的一致性处理
struct NodeStateObserver
{
    void update(GraphStateStore& store, const NodeCreated& e);
    void update(GraphStateStore& store, const NodeRemoved& e);
    void update(GraphStateStore& store, const NodeCenterChanged& e);
};

struct EdgeStateObserver
{
    void update(GraphStateStore& store, const EdgeCreated& e);
    void update(GraphStateStore& store, const EdgeRemoved& e);
    void update(GraphStateStore& store, const EdgeChanged& e);
};


struct ObserverRegister
{
    void Register(GraphStore& store);
    void Register(GraphStateStore& store);

    void Subscribe(GraphStore& graph,GraphStateStore& state);
};


void TestGraphApp();
