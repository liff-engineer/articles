#pragma once
#include <string>
#include <vector>
#include <functional>
#include <compare>
#include <variant>

namespace abc
{
    struct Message {
    public:
        class Key {
        public:
            Key() = default;
            explicit Key(const char* literal);
            const char* c_str() const noexcept;
            auto operator<=>(const Key&)const = default;
        private:
            std::size_t index{ 0 };
        };

        using Value = std::variant<
            std::monostate,
            bool,
            std::int64_t,std::uint64_t,
            double,
            std::string
        >;
    public:
        Key   topic;
        Value tag;
        std::vector<std::pair<Key, Value>> payload;

        void broadcast();
        
        static std::size_t RegisterHandler(Key topic, std::function<void(Message&)> handler);
    };

    using StringTag = Message::Key;

    class MessageHandlerStub final {
    public:
        explicit MessageHandlerStub(std::size_t index)
            :m_index(index) {};
        ~MessageHandlerStub() noexcept;

        MessageHandlerStub(const MessageHandlerStub&) = delete;
        MessageHandlerStub& operator=(const MessageHandlerStub&) = delete;

        MessageHandlerStub(MessageHandlerStub&& other) noexcept
            :m_index{ std::exchange(other.m_index,std::size_t{}) } {};
        MessageHandlerStub& operator=(MessageHandlerStub&& other) noexcept {
            if (this != std::addressof(other)) {
                m_index = std::exchange(other.m_index, m_index);
            }
            return *this;
        }
    private:
        std::size_t m_index{};
    };
}
