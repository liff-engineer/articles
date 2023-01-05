#include "message.hpp"

namespace abc
{
    struct StringTagRegistry final {
        std::vector<std::string> elements;

        StringTagRegistry()
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

        static StringTagRegistry& Get() {
            static StringTagRegistry object{};
            return object;
        }
    };

    struct MessageHandlerRegistry final {
        struct Handler {
            StringTag code;
            StringTag topic;
            std::function<void(Message&)> op;
        };

        std::vector<Handler> elements;

        StringTag add(StringTag code, StringTag topic, std::function<void(Message&)>&& handler) {
            for (auto& e : elements) {
                if (e.code == code) {
                    e.topic = topic;
                    e.op = std::move(handler);
                    return code;
                }
            }
            elements.emplace_back(Handler{ code,topic,std::move(handler) });
            return code;
        }
        
        bool remove(StringTag code) noexcept {
            for (auto& e : elements) {
                if (e.code == code) {
                    std::swap(e, Handler{});
                    return true;
                }
            }
            return false;
        }

        static MessageHandlerRegistry& Get() {
            static MessageHandlerRegistry object{};
            return object;
        }

        void handle(Message& msg) const noexcept {
            std::size_t n = elements.size();
            try
            {
                for (auto i = std::size_t{}; i < n; i++) {
                    auto&& e = elements[i];
                    if (e.topic == StringTag{})
                        continue;
                    if (e.topic == msg.topic && e.op) {
                        e.op(msg);
                    }
                }
            }
            catch (...) {
                ;//
            }
        }
    };

    StringTag::StringTag(const char* literal)
        :index{ StringTagRegistry::Get().at(literal)}
    {
    }

    const char* StringTag::data() const noexcept
    {
        return  StringTagRegistry::Get().at(index);
    }

    void Message::send() noexcept
    {
        MessageHandlerRegistry::Get().handle(*this);
    }

    StringTag Message::RegisterHandler(StringTag code, StringTag topic, std::function<void(Message&)> handler)
    {
        return MessageHandlerRegistry::Get().add(code, topic, std::move(handler));
    }

    MessageHandlerStub::~MessageHandlerStub() noexcept
    {
        MessageHandlerRegistry::Get().remove(m_code);
    }
}
