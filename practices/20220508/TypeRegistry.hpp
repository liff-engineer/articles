#pragma once
#include <type_traits>
#include <string>
#include <vector>

#ifdef TYPE_REGISTRY_EXPORT
#define  TYPE_REGISTRY_API __declspec(dllexport)
#else
#define  TYPE_REGISTRY_API __declspec(dllimport)
#endif


namespace abc
{
    //常规的类型ID生成方式
    namespace v0
    {
        using TypeCode = std::string;
        template<typename T>
        TypeCode Of() {
            return typeid(T).name();
        }
    }


    namespace v1
    {
        //全局唯一ID
        using TypeCode = int;
        class TYPE_REGISTRY_API TypeRegistry final {
        public:
            static TypeRegistry* Get();

            template<typename T>
            int Register() {
                static auto name = typeid(T).name();
                return RegisterImpl(name);
            }
        private:
            TypeRegistry() = default;
            int RegisterImpl(const char* TypeName);
        private:
            std::vector<std::string> m_names;
        };

        template<typename T>
        TypeCode Of() {
            return TypeRegistry::Get()->Register<T>();
        }
    }
}
