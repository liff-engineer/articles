#pragma once
#include <string>
#include <vector>
#include <functional>

namespace abc
{
    class StringTag {
    public:
        StringTag() = default;
        explicit StringTag(const char* literal);

        const char* data() const noexcept;
        std::string str() const {
            return data();
        }

        bool operator==(const StringTag& other) const noexcept {
            return other.index == index;
        }
        bool operator!=(const StringTag& other) const noexcept {
            return other.index != index;
        }
    private:
        std::size_t index{ 0 };
    };

    struct Message {
        StringTag   topic;
        std::string tag;
        std::vector<std::pair<StringTag, std::string>> payload;

        void send() noexcept;
        
        static StringTag  RegisterHandler(StringTag code, StringTag topic, std::function<void(Message&)> handler);
    };

    class MessageHandlerStub final {
    public:
        explicit MessageHandlerStub(StringTag code)
            :m_code(code) {};
        ~MessageHandlerStub() noexcept;

        MessageHandlerStub(const MessageHandlerStub&) = delete;
        MessageHandlerStub& operator=(const MessageHandlerStub&) = delete;

        MessageHandlerStub(MessageHandlerStub&&) noexcept = default;
        MessageHandlerStub& operator=(MessageHandlerStub&&) noexcept = default;
    private:
        StringTag m_code;
    };
}
