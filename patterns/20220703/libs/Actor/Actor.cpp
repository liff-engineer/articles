#include "Actor.h"
#include <iostream>

namespace abc {

    ActorFactory::ActorFactory() = default;

    std::vector<std::string> ActorFactory::Codes()
    {
        std::vector<std::string> results;
        for (auto&& obj : m_builders) {
            results.emplace_back(obj.first);
        }
        return results;
    }

    std::unique_ptr<IActor> ActorFactory::Make(const std::string& code)
    {
        auto it = m_builders.find(code);
        if (it != m_builders.end()) {
            return it->second();
        }
        return nullptr;
    }

    class InnerTestActor final:public IActor {
    public:
        void Launch() override {
            std::cout << "InnerTestActor::Launch\n";
        }
    };

    ActorFactory* gActorFactory()
    {
        static ActorFactory obj;
        {//测试代码,注册内建Actor
            obj.Register<InnerTestActor>("InnerTestActor");
        }
        return std::addressof(obj);
    }
}
