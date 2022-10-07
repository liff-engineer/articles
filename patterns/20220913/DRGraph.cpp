#include "DRGraph.hpp"
#include <unordered_set>
#include <queue>
#include <list>

namespace abc
{
    std::pair<int, const std::string*> DataIndex::TopicIndexOf(const char* topic)
    {
        static std::list<std::string> codes;
        int result{};
        for (auto&& code : codes) {
            if (code == topic) {
                return std::make_pair(result,&code);
            }
            result++;
        }
        codes.emplace_back(topic);
        return std::make_pair(result, &codes.back());
    }

    void DRGraph::AddEdge(DataIndex src, DataIndex dst)
    {
        m_edges[src].observers.emplace_back(dst);
    }

    void DRGraph::Traversal(DataIndex src)
    {
        std::unordered_set<DataIndex> visited{};

        std::queue<DataIndex> queue;

        visited.insert(src);
        queue.push(src);

        auto visit = [&](DataIndex v) {
            if (auto it = m_edges.find(v); it != m_edges.end())
            {
                if (it->second.task) {
                    it->second.task();
                }

                for (auto&& v : it->second.observers) {
                    if (visited.count(v) != 1) {
                        visited.insert(v);
                        queue.push(v);
                    }
                }
            }
        };

        while (!queue.empty()) {
            auto n = queue.front();
            queue.pop();
            visit(n);
            if (n.tag >= 0) {
                n.tag = -1;
                visit(n);
            }
        }
    }

}
