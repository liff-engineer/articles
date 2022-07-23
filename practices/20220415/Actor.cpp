#include "Actor.hpp"
#include <sstream>
#include <Windows.h>

namespace abc
{
    static Broker::Reporter gBrokerReporter = [](const Broker::Action& event) {
        //记录初始化后第一次调用的时间点,用来计算ns级差值,来统计性能
        static auto mark = std::chrono::high_resolution_clock::now();
        std::stringstream oss;
        oss << "abc::broker::trace|"<< "broker(" << (std::uintptr_t)(event.broker) << ')';
        oss << '|' << event.type;
        oss << '|' << event.handlerCode << '(' << event.handler << ')';
        oss << '|' << event.message;
        oss << '|' << std::chrono::duration_cast<std::chrono::milliseconds>(event.timepoint.time_since_epoch()).count();
        oss << '|' << std::chrono::duration_cast<std::chrono::nanoseconds>(event.timepoint - mark).count();
        oss << '\n';

        OutputDebugStringA(oss.str().c_str());
    };


    Broker::Broker() = default;

    Broker::Reporter Broker::RegisterReporter(Reporter&& handler)
    {
        return std::exchange(gBrokerReporter, handler);
    }

    void Broker::handle(const Broker* source, IPayload& payload, const char* code)
    {
        Tracer log{ source,this,code };
        try {
            std::size_t i = 0;
            while (i < m_stubs.size()) {
                auto&& o = m_stubs[i++];
                if (!o.accept(code)) continue;
                if (auto&& h = o.handler.lock()) {
                    h->handle(source, payload, code);
                }
            }
        }
        catch (...) {
            throw;
        }
    }

    Broker::Tracer::Tracer(const Broker* source, std::uintptr_t address, const char* handlerCode, const char* code)
    {
        log = { source,0,address,handlerCode,code,std::chrono::high_resolution_clock::now() };
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
}
