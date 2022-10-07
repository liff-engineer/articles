#include "Graph.hpp"

int main()
{
    Document doc{};

    auto r1 = doc.Create<Rule>("var", "variable",
        std::vector<Port>{
        Port{ "out","variable output",PortType::out }
    });

    auto n1 = doc.Create<Node>(r1, Point{1.0,2.0});

    auto rules = doc.View<Rule>();
    auto r2 = rules->find(1);

    auto nodes = doc.View<Node>();
    auto n2 = nodes->find(1);
    return 0;
}
