#include "Repo.hpp"
#include <string>
#include <vector>
#include <algorithm>

namespace abc
{
    namespace v1
    {
        namespace
        {
            class TypeRegistry {
            public:
                std::size_t Register(const char* code) {
                    std::size_t result{ 0 };
                    for (auto& v : m_codes) {
                        if (v == code) return result;
                        result++;
                    }
                    result = m_codes.size();
                    m_codes.emplace_back(code);
                    return result;
                }
            private:
                std::vector<std::string> m_codes;
            };
            static TypeRegistry gTypeRegistry{};
        }

        Repository::Repository(const Repository& other)
            :m_entitys(other.m_entitys)
        {
            m_components.clear();
            m_components.reserve(other.m_components.size());
            for (auto& o : other.m_components) {
                if (o) {
                    m_components.emplace_back(
                        o->clone()
                    );
                }
                else {
                    m_components.emplace_back();
                }
            }
        }

        Repository& Repository::operator=(const Repository& other)
        {
            if (std::addressof(other) != this) {
                m_entitys = other.m_entitys;
                m_components.clear();
                m_components.reserve(other.m_components.size());
                for (auto& o : other.m_components) {
                    if (o) {
                        m_components.emplace_back(
                            o->clone()
                        );
                    }
                    else {
                        m_components.emplace_back();
                    }
                }
            }
            return *this;
        }

        Repository::Repository(Repository&& other) noexcept
            :m_entitys(std::move(other.m_entitys)),
            m_components(std::move(other.m_components))
        {
        }

        Repository& Repository::operator=(Repository&& other)  noexcept
        {
            if (std::addressof(other) != this) {
                m_components = std::move(other.m_components);
                m_entitys = std::move(other.m_entitys);
            }
            return *this;
        }

        std::size_t Repository::IndexOf(const char* code) const
        {
            return gTypeRegistry.Register(code);
        }
    }
}
