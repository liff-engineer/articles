#include "TypeRegistry.hpp"

namespace abc
{
    namespace v1
    {
        TypeRegistry* TypeRegistry::Get()
        {
            static TypeRegistry obj{};
            return std::addressof(obj);
        }

        int TypeRegistry::RegisterImpl(const char* TypeName)
        {
            int result = -1;
            for (auto&& name : m_names) {
                if (name == TypeName) {
                    return  result + 1;
                }
                result++;
            }
            m_names.emplace_back(TypeName);
            return (int)(m_names.size()-1);
        }
    }
}
