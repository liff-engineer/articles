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

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <functional>

namespace abc
{
    //采用类似Flywidget的技术为字符串生成运行时索引,用来
    // Topic + Tag + Payload

    //1. node的创建、删除
    //2. node的连接
    //3. node的变化

    // 数据依赖关系图：
    // a) 用来建立依赖关系,譬如b要跟随a变化
    // b) 当某些数据变化时触发其它同步
    class DBGraph {
    public:
        std::size_t AddVectex(const char* topic) {
            vertexs.emplace_back(topic);
            return vertexs.size() - 1;
        }

        bool AddEdge(std::size_t src, std::size_t dst) {
            if (src >= vertexs.size() || dst >= vertexs.size())
                return false;
            edges[src].push_back(dst);
            return true;
        }

        template<typename Fn>
        bool AddTask(std::size_t src, Fn&& fn)
        {
            if (src >= vertexs.size())
                return false;
            tasks[src] = std::move(fn);
            return true;
        }


        //广度优先遍历
        void Traversal(std::size_t src) {
            if (src >= vertexs.size()) return;

            // 记录哪些被访问过
            std::vector<bool> visited{};
            visited.resize(vertexs.size(), false);

            //用队列存储待遍历节点
            std::queue<std::size_t> queue;
            
            //标记当前节点访问过,并压入待遍历队列
            visited[src] = true;
            queue.push(src);
            while (!queue.empty()) {
                //访问队列头
                auto n = queue.front();
                //std::cout << "visit node(" << vertexs.at(n) << ")\n";
                if (auto it = tasks.find(n); it != tasks.end()) {
                    it->second(this,
                        vertexs.at(n).c_str()
                    );
                }
                queue.pop();

                //确保关联边都压入待遍历队列
                if (auto it = edges.find(n); it != edges.end()) {
                    for (auto&& v : it->second) {
                        if (!visited[v]) {
                            visited[v] = true;
                            queue.push(v);
                        }
                    }
                }
            }
        }
    private:
        std::vector<std::string> vertexs;
        std::unordered_map<int, std::vector<int>> edges;
        std::unordered_map<int, std::function<void(DBGraph*,const char*)>> tasks;
    };
}

void test()
{
    using namespace abc;

    DBGraph g{};

    /// 数据集
    std::unordered_map<int, double> values;
    //计算a+b=c
    auto a = g.AddVectex("a");
    auto b = g.AddVectex("b");
    auto plus = g.AddVectex("+");
    //a+b 
    g.AddEdge(a, plus);
    g.AddEdge(b, plus);
    auto c = g.AddVectex("c");
    //a+b = c;
    g.AddEdge(plus,c);
    auto print = g.AddVectex("print");
    //print(c)
    g.AddEdge(c, print);

    /// DBGraph只负责触发,数据传递不在其职责范畴
    /// 此处手工处理数据传递
    for (auto n : { a,b,c,plus,print }) {
        g.AddTask(n, [](abc::DBGraph*, const char* topic) {
            std::cout << "visit node(" << topic << ")\n";
            });
    }

    g.AddTask(plus, [&, a, b, plus](auto g, auto topic) {
        std::cout << "visit node(" << topic << ")\n";
        if (values.count(a) <= 0 || values.count(b) <= 0) {
            return;
        }
        values[plus] = values.at(a) + values.at(b);
        });

    g.AddTask(c, [&, plus,c](auto g, auto topic) {
        std::cout << "visit node(" << topic << ")\n";
            if (values.count(plus)>0) {
                values[c] = values.at(plus);
            }
        });
    g.AddTask(print, [&, c, print](auto g, auto topic) {
        std::cout << "visit node(" << topic << ")\n";
        if (values.count(c) > 0) {
            values[print] = values.at(c);
            std::cout << "result:" << values[print] << "\n";
        }
        });

    /// 修改某些数据并触发整个关系重算
    for (auto&& [lhs, rhs] : std::vector<std::pair<double,double>>
        { 
            {1.414,10.0},
            {2.0,-12.0}
        }) {
        
        values[a] = lhs;
        g.Traversal(a);

        values[b] = rhs;
        g.Traversal(b);
    }
    //TODO FIXME 支持条件触发
    //TODO FIXME 支持更为丰富的topic,譬如结构为source.topic,source可以用?*代指
}


int main() {
    {
        test();
        return 0;
    }

    abc::DBGraph g{};

    //a,b,c,d
    //a->b
    //a->c
    //b->c
    //c->a
    //c->d
    //d->d
    auto a = g.AddVectex("a");
    auto b = g.AddVectex("b");
    auto c = g.AddVectex("c");
    auto d = g.AddVectex("d");
    g.AddEdge(a, b);
    g.AddEdge(a, c);
    g.AddEdge(b, c);
    g.AddEdge(c, a);
    g.AddEdge(c, d);
    g.AddEdge(d, d);
    for (auto n : { a,b,c,d }) {
        g.AddTask(n, [](abc::DBGraph*,const char* topic) {
            std::cout << "visit node(" << topic << ")\n";
            });
    }

    g.Traversal(c);

    return 0;
}
