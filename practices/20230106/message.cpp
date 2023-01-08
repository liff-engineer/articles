#include "message.hpp"

namespace abc
{
    struct MessageKeyRegistry final {
        std::vector<std::string> elements;

        MessageKeyRegistry()
            :elements{ std::string{} } {};

        std::size_t at(const char* literal) {
            for (auto i = std::size_t{}; i < elements.size(); i++) {
                if (elements[i] == literal)
                    return i;
            }
            elements.emplace_back(literal);
            return elements.size() - 1;
        }

        const char* at(std::size_t index) const noexcept {
            if (index < elements.size()) {
                return elements[index].c_str();
            }
            return nullptr;
        }

        static MessageKeyRegistry& Get() {
            static MessageKeyRegistry object{};
            return object;
        }
    };

    struct MessageHandlerRegistry final {
        struct Handler {
            StringTag topic;
            std::function<void(Message&)> op;
        };

        std::vector<Handler> elements;

        std::size_t add(StringTag topic, std::function<void(Message&)>&& handler) {
            elements.emplace_back(Handler{ topic,std::move(handler) });
            return elements.size();
        }
        
        bool remove(std::size_t index) noexcept {
            if (index == 0 || index > elements.size())
                return false;
            elements[index - 1] = Handler{};
            return true;
        }

        static MessageHandlerRegistry& Get() {
            static MessageHandlerRegistry object{};
            return object;
        }

        void handle(Message& msg) const {
            std::size_t n = elements.size();
            for (auto i = std::size_t{}; i < n; i++) {
                auto&& e = elements[i];
                if (e.topic == StringTag{})
                    continue;
                if (e.topic == msg.topic && e.op) {
                    e.op(msg);
                }
            }
        }
    };

    Message::Key::Key(const char* literal)
        :index{ MessageKeyRegistry::Get().at(literal) }
    {
    }

    const char* Message::Key::c_str() const noexcept
    {
        return  MessageKeyRegistry::Get().at(index);
    }

    void Message::broadcast()
    {
        MessageHandlerRegistry::Get().handle(*this);
    }

    std::size_t Message::RegisterHandler(Key topic, std::function<void(Message&)> handler)
    {
        return MessageHandlerRegistry::Get().add(topic, std::move(handler));
    }

    MessageHandlerStub::~MessageHandlerStub() noexcept
    {
        if (m_index) {
            MessageHandlerRegistry::Get().remove(m_index);
        }
    }
}
