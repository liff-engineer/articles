#include "Actor.hpp"
#include <sstream>
#include <Windows.h>

namespace abc
{
    static Broker::Reporter gBrokerReporter = [](const Broker::Action& event) {
        //记录初始化后第一次调用的时间点,用来计算ns级差值,来统计性能
        static auto mark = std::chrono::high_resolution_clock::now();
        std::stringstream oss;
        oss << "abc::broker::trace|";
        oss << event.source.id;
        oss << '|' << "broker(" << (std::uintptr_t)(event.source.broker) << ')';
        oss << '|' << event.actor.code << '(' << event.actor.address << ')';
        oss << '|' << event.type;
        oss << '|' << event.message;
        oss << '|' << std::chrono::duration_cast<std::chrono::milliseconds>(event.timepoint.time_since_epoch()).count();
        oss << '|' << std::chrono::duration_cast<std::chrono::nanoseconds>(event.timepoint - mark).count();
        oss << '\n';

        OutputDebugStringA(oss.str().c_str());
    };

    Broker::Tracer::Tracer(Action::Header source, Action::Actor actor, const char* code)
    {
        log = { source,0,actor,code,std::chrono::high_resolution_clock::now() };
        if (gBrokerReporter) {
            gBrokerReporter(log);
        }
    }

    Broker::Tracer::~Tracer() noexcept
    {
        log.type = 1;
        log.timepoint = std::chrono::high_resolution_clock::now();
        try {
            if (gBrokerReporter) {
                gBrokerReporter(log);
            }
        }
        catch (...) {};
    }

    Broker::Broker() = default;

    Broker::Reporter Broker::RegisterReporter(Reporter&& handler)
    {
        return std::exchange(gBrokerReporter, handler);
    }

    void Broker::handle(Action::Header source, IPayload& payload, const char* code)
    {
        static unsigned id = 0;
        if (source.broker == nullptr) {
            source.broker = this;
            source.id = id++;
        }

        Tracer log{ source,{(std::uintptr_t)this,description.c_str()},code };
        m_concurrentCount++;
        try {
            std::size_t i = 0;
            while (i < m_stubs.size()) {
                auto&& o = m_stubs[i++];
                if (!o.accept(code)) continue;
                if (auto&& h = o.handler.lock()) {
                    h->handle(source, payload, code);
                }
            }
            m_concurrentCount--;
        }
        catch (...) {
            m_concurrentCount--;
            throw;
        }
    }
}
