#include "Actor.hpp"
#include <chrono>

namespace abc
{
    //利用栈追踪通过broker的消息流
    thread_local const Broker* gCurrentBroker = nullptr;

    //需要追踪并记录如下信息
    //> id:用来追踪整个调用链路
    //> broker:当前由哪个broker管理
    //> actor:当前由哪个actor处理
    //> actor description:actor的描述
    //> timestamp in milliseconds:当前时间戳
    //> enter or leave:是进入还是退出,用来形成调用链
    //> time lost in nanoseconds:leave时耗费的时间

    void example() {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto s = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto start = std::chrono::high_resolution_clock::now();
        auto finish = std::chrono::high_resolution_clock::now();

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    }

    Broker::Broker() = default;
    void Broker::handle(IPayload& payload, const char* code)
    {
        gCurrentBroker = this;

        m_concurrentCount++;
        try {
            std::size_t i = 0;
            while (i < m_stubs.size()) {
                auto&& o = m_stubs[i++];
                if (!o.accept(code)) continue;
                if (auto&& h = o.handler.lock()) {
                    h->handle(payload, code);
                }
            }
            m_concurrentCount--;
        }
        catch (...) {
            m_concurrentCount--;
            throw;
        }
    }

    void Registry::clear()
    {
        std::swap(m_entries, std::vector<Entry>{});
    }
}
