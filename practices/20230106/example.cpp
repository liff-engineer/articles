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
            msg.send();
        }
    }
    ~ExecuteRecorder()noexcept {
        if (!m_target.empty()) {
            abc::Message{ gExecuteStopTopic,m_target }
            .send();
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
struct MessageHandler {
    void operator()(abc::Message& msg) {
        std::cout << msg.topic.data() << ":" << msg.tag << "\n";
    }
};

abc::MessageHandlerStub stubs[]{
    abc::MessageHandlerStub{abc::Message::RegisterHandler(gExecuteStartTopic,gExecuteStartTopic,MessageHandler{})},
    abc::MessageHandlerStub{abc::Message::RegisterHandler(gExecuteStopTopic,gExecuteStopTopic,MessageHandler{})}
};

int main() {
    {
        ExecuteRecorder recorder{ __FUNCSIG__ };
    }

    return 0;
}
