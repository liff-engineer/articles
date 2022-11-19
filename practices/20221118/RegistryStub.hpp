#pragma once
#include <utility>

namespace abc
{
    template<typename I, typename K = std::size_t>
    class RegistryStub {
    public:
        RegistryStub() = default;
        RegistryStub(K k) :m_stub(k) {};

        RegistryStub(RegistryStub const&) = delete;
        RegistryStub& operator=(RegistryStub const&) = delete;

        RegistryStub(RegistryStub&& other) noexcept
            :m_stub(std::exchange(other.m_stub, K{})) {};

        RegistryStub& operator=(RegistryStub&& other) noexcept
        {
            if (this != &other) {
                m_stub = std::exchange(other.m_stub, K{});
            }
            return *this;
        }

        ~RegistryStub() {
            I::Destory(std::exchange(m_stub, K{}));
        }

        K reset() {
            return std::exchange(m_stub, K{});
        }
    private:
        K m_stub{};
    };
}
