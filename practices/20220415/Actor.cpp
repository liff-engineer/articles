#include "Actor.hpp"

namespace abc
{
    Broker::Broker() = default;
    void Broker::Handle(IPayload& payload, const char* code)
    {
        m_concurrentCount++;
        try {
            std::size_t i = 0;
            while (i < m_stubs.size()) {
                auto&& o = m_stubs[i++];
                if (!o.Accept(code)) continue;
                if (auto&& h = o.handler.lock()) {
                    h->Handle(payload, code);
                }
            }
        }
        catch (...) {
            m_concurrentCount--;
            throw;
        }
    }

    void Registry::Clear()
    {
        std::swap(m_entries, std::vector<Entry>{});
    }
}
