#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include <optional>
#include <functional>

namespace abc
{
    // 数据索引
    struct DataIndex {
        int topic;
        int tag;
        const std::string* description;//调试用

        bool operator==(const DataIndex& rhs) const noexcept {
            return topic == rhs.topic && tag == rhs.tag;
        }

        // 主题索引,负责将topic转换为字符串,简化代码书写
        static std::pair<int,const std::string*> TopicIndexOf(const char* topic);

        static DataIndex Create(const std::string& topic, int tag) {
            auto v = TopicIndexOf(topic.c_str());
            return { v.first,tag,v.second };
        }
    };

}

namespace std {
    template<>
    struct hash<abc::DataIndex> {
        size_t operator()(abc::DataIndex const& o) const {
            std::size_t r = o.topic;
            r = r << 32;
            r += o.tag;
            return r;
        }
    };
}

namespace abc
{
    //注意,这里的算法有问题,广度优先遍历的同时,要确保多依赖Node在所有依赖就绪后执行
    //例如 n3同时依赖于n1和n2 ,则必须n1和n2执行完成后再执行n3
    class DRGraph {
    public:
        void AddEdge(DataIndex src, DataIndex dst);

        template<typename Fn>
        void AddTask(DataIndex src, Fn&& fn) {
            if (auto it = m_edges.find(src); it != m_edges.end()) {
                it->second.task = std::move(fn);
                return;
            }
            m_edges[src].task = std::move(fn);
        }

        void Traversal(DataIndex src);
    private:
        struct Edge {
            std::vector<DataIndex> observers;
            std::function<void()>  task;
        };
        std::unordered_map<DataIndex, Edge> m_edges;
    };

    struct  DataRepository {
        std::unordered_map<DataIndex, std::any> datas;

        template<typename T>
        void Set(DataIndex index, const T& v) {
            datas[index] = v;
        }

        template<typename T>
        T* Get(DataIndex index) {
            if (auto it = datas.find(index); it != datas.end()) {
                return std::any_cast<T>(&(it->second));
            }
            return nullptr;
        }

        inline void Reset(DataIndex index) {
            datas.erase(index);
        }
    };
}

