#pragma once

#include "HashedTypeInfo.hpp"

namespace prefab
{
    //数据组件基类
    class IComponentBase
    {
    public:
        virtual ~IComponentBase() = default;

        //数据组件类型Code
        virtual HashedStringLiteral typeCode() const noexcept = 0;
        //实现类型Code
        virtual HashedStringLiteral implTypeCode() const noexcept = 0;
    };

    //数据组件类型T的接口类,可以通过模板特化定义T的真实接口类
    //!!每种类型只能有一份接口定义!!
    template<typename T>
    class IComponent :public IComponentBase
    {
    public:
        //T的类型Code实现
        HashedStringLiteral typeCode() const noexcept override {
            return HashedTypeNameOf<T>();
        }
    public:
        //T的可用接口
        virtual bool exist() const noexcept = 0;
        virtual T    view() const = 0;
        virtual bool remove() noexcept = 0;
        virtual bool assign(T const&) noexcept = 0;
        virtual T    replace(T const&) noexcept = 0;
    };
}
