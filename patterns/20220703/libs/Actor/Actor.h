#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include "ActorApi.h"

namespace abc
{
    class ACTOR_EXPORT IActor {
    public:
        virtual ~IActor() = default;
        virtual void Launch() = 0;
    };

    class ACTOR_EXPORT ActorFactory final {
        std::unordered_map<std::string,std::function<std::unique_ptr<IActor>()>> m_builders;
    public:
        ActorFactory();
        std::vector<std::string> Codes();
        std::unique_ptr<IActor>  Make(const std::string& code);

        template<typename Fn>
        bool Register(const std::string& code, Fn&& fn) {
            auto it = m_builders.find(code);
            if (it != m_builders.end()) {
                return false;
            }
            m_builders[code] = std::move(fn);
            return true;
        }

        template<typename T>
        bool Register(const std::string& code) {
            return Register(code, []() {
                return std::make_unique<T>();
                });
        }
    };

    ACTOR_EXPORT ActorFactory* gActorFactory();
}
