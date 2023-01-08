#include "message.hpp"

abc::StringTag gExecuteStartTopic{ "execute.start" };
abc::StringTag gExecuteStopTopic{ "execute.stop" };

class ExecuteRecorder final {
public:
    ExecuteRecorder() = default;
    explicit ExecuteRecorder(std::string target)
        :m_target{ std::move(target) } {
        if (!m_target.empty()) {
            abc::Message msg{ gExecuteStartTopic,m_target };
            msg.broadcast();
        }
    }
    ~ExecuteRecorder()noexcept {
        if (!m_target.empty()) {
            abc::Message{ gExecuteStopTopic,m_target }.broadcast();
        }
    };
    ExecuteRecorder(const ExecuteRecorder&) = delete;
    ExecuteRecorder& operator=(const ExecuteRecorder&) = delete;

    ExecuteRecorder(ExecuteRecorder&&)noexcept = default;
    ExecuteRecorder& operator=(ExecuteRecorder&&)noexcept = default;
private:
    std::string m_target;
};

#include <iostream>
class MessageHandler {
public:
    MessageHandler()
    {
        //stubs.reserve(2);
        stubs.emplace_back(abc::Message::RegisterHandler(gExecuteStartTopic, 
            [&](auto& msg) { this->handle(msg); }));
        stubs.emplace_back(abc::Message::RegisterHandler(gExecuteStopTopic,
            [&](auto& msg) { this->handle(msg); }));
    }

    void handle(abc::Message& msg) {
        std::cout << msg.topic.c_str() << ":" << std::get<std::string>(msg.tag) << "\n";
    }
private:
    std::vector<abc::MessageHandlerStub> stubs;
};

namespace
{
    static MessageHandler gMessageHandler{};
}

int main() {
    ExecuteRecorder recorder{ __FUNCSIG__ };
    return 0;
}
