#include "Graph.hpp"


int GraphStore::AddNode(std::string code)
{
    static int id = 0;
    auto node = Node{ id++,code };
    doc.nodes[node.id] = node;

    //发出通知
    notifyer.notify(*this, NodeCreated{ node.id });
    return node.id;
}

int GraphStore::AddEdge(int srcNode, std::string srcNodePort, int dstNode, std::string sdtNodePort)
{
    static int id = 0;
    auto edge = Edge{ id++,srcNode,srcNodePort,dstNode,sdtNodePort };
    doc.edges[edge.id] = edge;

    //发出通知
    notifyer.notify(*this, EdgeCreated{ edge.id });
    return edge.id;
}

void GraphStateStore::AddNodeState(int node)
{
    auto nodeIt = graph->doc.nodes.find(node);
    auto repo = graph->conceptRepo;
    auto conceptIt = repo->concepts.find(nodeIt->second.code);
    
    NodeState state;
    state.node = node;
    int x = 0;
    int y = 0;

    state.center = { x,y };
    state.description = conceptIt->second.description;
    {
        int xoffset = -100;
        int dy = 10;
        for (auto& port : conceptIt->second.inPorts) {
            state.inPorts.emplace_back(NodePortState{
                port.code,port.description,Point{x+xoffset,y+dy}
                });
            dy += 10;
        }
    }
    {
        int xoffset = 100;
        int dy = 10;
        for (auto& port : conceptIt->second.outPorts) {
            state.outPorts.emplace_back(NodePortState{
               port.code,port.description,Point{x + xoffset,y + dy}
                });
        }
    }

    this->states.nodeStates[node] = state;
    notifyer.publish(NodeCreated{ node });
}

void GraphStateStore::AddEdgeState(int edge)
{
    auto edgeIt = this->graph->doc.edges.find(edge);
    
    EdgeState state{};
    state.edge = edge;
    {
        auto nodeIt = states.nodeStates.find(edgeIt->second.srcNode);
        for (auto& port : nodeIt->second.outPorts) {
            if (port.code == edgeIt->second.srcNodePort) {
                state.srcCenter = port.center;
            }
        }
    }
    {
        auto nodeIt = states.nodeStates.find(edgeIt->second.dstNode);
        for (auto& port : nodeIt->second.inPorts) {
            if (port.code == edgeIt->second.dstNodePort) {
                state.dstCenter = port.center;
            }
        }
    }

    this->states.edgeStates[edge] = state;

    notifyer.publish(EdgeCreated{ edge });
}

void NodeObserver::update(GraphStore& store, const NodeCreated& e)
{
    //节点创建时GraphStore不需要做任何一致性处理,直接传递出去即可
    store.notifyer.publish(e);
}

void NodeObserver::update(GraphStore& store, const NodeRemoved& e)
{
    //节点移除时要连带移除依赖它的边
    std::vector<int> edges{};
    for (auto&& [k,edge] : store.doc.edges) {
        if (edge.srcNode == e.id) {
            edges.push_back(k);
        }
        if (edge.dstNode == e.id) {
            edges.push_back(k);
        }
    }

    //通知边要被删除
    for (auto&& k : edges) {
        store.notifyer.publish(EdgeRemoved{ k });
        //删除边
        store.doc.edges.erase(k);
    }

    //通知节点要被删除
    store.notifyer.publish(e);
}

void NodeStateObserver::update(GraphStateStore& store, const NodeCreated& e)
{
    store.AddNodeState(e.id);
}

void NodeStateObserver::update(GraphStateStore& store, const NodeRemoved& e)
{
    //先通知外部,要删了,然后再删除
    store.notifyer.publish(e);
    store.states.nodeStates.erase(e.id);
}

void NodeStateObserver::update(GraphStateStore& store, const NodeCenterChanged& e)
{
    //遍历所有edge,同步更新
    std::vector<int> edges;
    for (auto&& [k, edge] : store.graph->doc.edges) {
        if (edge.srcNode == e.id || edge.dstNode == e.id) {
            //更新edge的位置
            edges.push_back(k);
        }
    }

    for (auto&& k : edges) {
        if (auto it = store.states.edgeStates.find(k); it != store.states.edgeStates.end()) {
            //刷新位置,发出信号
            store.notifyer.publish(EdgeStateChanged{ k });
        }
    }
}

void EdgeStateObserver::update(GraphStateStore& store, const EdgeCreated& e)
{
    store.AddEdgeState(e.id);
}

void EdgeStateObserver::update(GraphStateStore& store, const EdgeRemoved& e)
{
    store.notifyer.publish(e);
    store.states.edgeStates.erase(e.id);
}

void EdgeStateObserver::update(GraphStateStore& store, const EdgeChanged& e)
{
    //重算位置?并发出变化信号
    store.notifyer.publish(EdgeStateChanged{ e.id });
}


template<typename E>
struct abc::Observer<GraphStore, Node, E>
{
    void update(GraphStore& store, const E& e) {
        NodeObserver{}.update(store, e);
    }
};

template<typename E>
struct abc::Observer<GraphStore, Edge, E>
{
    void update(GraphStore& store, const E& e) {
        EdgeObserver{}.update(store, e);
    }
};

template<typename E>
struct abc::Observer<GraphStateStore, NodeState, E>
{
    void update(GraphStateStore& store, const E& e) {
        NodeStateObserver{}.update(store, e);
    }
};

template<typename E>
struct abc::Observer<GraphStateStore, EdgeState, E>
{
    void update(GraphStateStore& store, const E& e) {
        EdgeStateObserver{}.update(store, e);
    }
};

void ObserverRegister::Register(GraphStore& store)
{
    store.notifyer.registerObserver<Node, NodeCreated>();
    store.notifyer.registerObserver<Node, NodeRemoved>();

    store.notifyer.registerObserver<Edge, EdgeCreated>();
    store.notifyer.registerObserver<Edge, EdgeChanged>();
    store.notifyer.registerObserver<Edge, EdgeRemoved>();
}


void ObserverRegister::Register(GraphStateStore& store)
{
    store.notifyer.registerObserver<NodeState, NodeCreated>();
    store.notifyer.registerObserver<NodeState, NodeRemoved>();
    store.notifyer.registerObserver<NodeState, NodeCenterChanged>();

    store.notifyer.registerObserver<EdgeState, EdgeCreated>();
    store.notifyer.registerObserver<EdgeState, EdgeRemoved>();
    store.notifyer.registerObserver<EdgeState, EdgeChanged>();
}

void ObserverRegister::Subscribe(GraphStore& graph, GraphStateStore& state)
{
    //State模块要订阅node的创建和移除；edge的创建、移除和变化事件
    state.graph = std::addressof(graph);
    state.subscribeStub = graph.notifyer.subscribe<NodeCreated>(state);
    state.subscribeStub += graph.notifyer.subscribe<NodeRemoved>(state);
    state.subscribeStub += graph.notifyer.subscribe<EdgeCreated>(state);
    state.subscribeStub += graph.notifyer.subscribe<EdgeRemoved>(state);
    state.subscribeStub += graph.notifyer.subscribe<EdgeChanged>(state);
}

#include <iostream>

struct TestObserver {
    abc::SubscribeStub stub;


    template<typename E>
    void on(const E& e) {
        std::cout << "event:" << typeid(E).name() << "\n";
    }
};


void TestGraphApp()
{
    NodeConceptRepo conceptRepo;
    {
        NodeConcept number{};
        number.code = "number";
        number.description = "number";
        number.inPorts.emplace_back(
            NodeConcept::Port{
                "input","input"
            }
        );
        number.outPorts.emplace_back(
            NodeConcept::Port{
                "value","value"
            }
        );
        conceptRepo.concepts[number.code] = number;
    }
    {
        NodeConcept number{};
        number.code = "plus";
        number.description = "+";
        number.inPorts.emplace_back(
            NodeConcept::Port{
                "lhs","lhs"
            }
        );
        number.inPorts.emplace_back(
            NodeConcept::Port{
                "rhs","rhs"
            }
        );
        number.outPorts.emplace_back(
            NodeConcept::Port{
                "result","result"
            }
        );
        conceptRepo.concepts[number.code] = number;
    }

    GraphStore graph;
    {
        graph.conceptRepo = std::addressof(conceptRepo);
    }
    GraphStateStore state;

    {//建立观察与订阅
        ObserverRegister helper;
        helper.Register(graph);
        helper.Register(state);
        helper.Subscribe(graph, state);
    }
    TestObserver ob;
    {//订阅观察者
        ob.stub = graph.notifyer.subscribe<NodeCreated>(ob);
        ob.stub += graph.notifyer.subscribe<EdgeCreated>(ob);
    }

    //创建图: a+b = c
    {
        auto a = graph.AddNode("number");
        auto b = graph.AddNode("number");
        auto op = graph.AddNode("plus");
        auto c = graph.AddNode("number");

        auto a_op = graph.AddEdge(a, "value", op, "lhs");
        auto b_op = graph.AddEdge(b, "value", op, "rhs");
        auto op_c = graph.AddEdge(op, "result", c, "input");
    }
    
    if (state.states.edgeStates.empty())
    {
        std::cout << "failed\n";
    }
}
