/// 用来保证数据一致性的数据集依赖关系图
/// 适用于某些数据要跟随其它数据变化
/// 1) 数据作为图上node
/// 2) 数据跟随变化被视为图上node的edge
/// 3) 当某数据变化,通过遍历图来全部刷新
/// 
/// 可以提供如下特性：
/// a) 使用条件触发，用来中断传播
/// b) 提供子图，用来表达相对独立，又有关系的不同数据集DataSet
/// c) 性能优化?
/// 
/// 图上node的构成可能为：
/// 1)主题topic：可以设定主题格式，譬如?*等通配符来表达不关注数据源是谁,只要发生就触发
/// 2)标签tag：通常代表数据源，譬如不同的实体
/// 3)负载payload：可能存在，用来直接传递给其它节点?

#include "DRGraph.hpp"
#include "Graph.hpp"
#include <iostream>

void TestDRGraph()
{
    Graph graph{};
    //{//接收Node创建消息
    //    auto di = abcd::DataIndex::Create("GraphView.Node.Refresh", -1);
    //    graph.drGraph.AddTask(di, []() {
    //        std::cout << "Node Created\n";
    //        });
    //    graph.drGraph.AddEdge(abcd::DataIndex::Create(
    //        "Graph.Node.Create", -1
    //    ),
    //       di);
    //}
    //{//接收Connect创建消息
    //    auto di = abcd::DataIndex::Create("GraphView.Connect.Refresh", -1);
    //    graph.drGraph.AddTask(di, []() {
    //        std::cout << "Connect  Created\n";
    //        });
    //    graph.drGraph.AddEdge(abcd::DataIndex::Create(
    //        "Graph.Connect.Create", -1
    //    ),
    //        di);
    //}
    //a+b=c
    //print(c)
    graph.SetupDemo();

    graph.DemoSetA(1.0);
    graph.DemoSetB(2.0);
    graph.DemoSetA(11.345);
    graph.DemoSetB(1.414);

}


int main() {
    TestDRGraph();
    return 0;
}
