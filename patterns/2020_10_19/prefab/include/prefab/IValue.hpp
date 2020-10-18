#pragma once
#include <utility>
#include <memory>
#include <stdexcept>
#include <vector>
#include <type_traits>
#include "HashedTypeInfo.hpp"

namespace prefab
{
    //存储任意类型值的接口
    class IValue {
    public:
        virtual ~IValue() = default;
        virtual HashedStringLiteral typeCode() const noexcept = 0;
        virtual HashedStringLiteral implTypeCode() const noexcept = 0;
    };

    //默认值实现
    template<typename T>
    class ValueImpl :public IValue
    {
        T impl;
    public:
        explicit ValueImpl(T&& v) :impl(std::move(v)) {};

        HashedStringLiteral typeCode() const noexcept override {
            return HashedTypeNameOf<T>();
        }

        HashedStringLiteral implTypeCode() const noexcept override {
            return HashedTypeNameOf<ValueImpl<T>>();
        }

        const T& value() const noexcept {
            return impl;
        }

        T& value() noexcept {
            return impl;
        }
    };


    //值语义的Value包装
    class Value {
    protected:
        std::unique_ptr<IValue> impl;
    public:
        Value() = default;
        template<typename T, std::enable_if_t<!std::is_same<std::remove_cv_t<T>, Value>::value>* = nullptr>
        Value(T&& v)
            :impl(std::make_unique<ValueImpl<T>>(std::move(v))) {};

        explicit operator bool() const noexcept {
            return impl != nullptr;
        }
        
        HashedStringLiteral typeCode() const noexcept {
            return impl->typeCode();
        }

        template<typename T>
        const T& as() const noexcept {
            return static_cast<const ValueImpl<T>*>(impl.get())->value();
        }
    };

    //要求,用在某些筛选型接口的附加条件上,也可以表达请求(这时类似于Command,用来触发命令)
    class Require:public Value
    {
    public:
        std::vector<std::pair<HashedStringLiteral, HashedStringLiteral>> remarks;
    public:
        Require() = default;
        template<typename T, std::enable_if_t<!std::is_same<std::remove_cv_t<T>, Require>::value>* = nullptr>
        Require(T && v)
            :Value(std::forward<T>(v)) {};

        Require&& remark(HashedStringLiteral key, HashedStringLiteral v)&& noexcept {
            remarks.emplace_back(std::make_pair(key, v));
            return std::move(*this);
        }

        bool has(HashedStringLiteral key) const noexcept {
            for (auto& o : remarks) {
                if (o.first == key) {
                    return true;
                }
                return false;
            }
        }

        HashedStringLiteral remark(HashedStringLiteral key) const noexcept {
            for (auto& o : remarks) {
                if (o.first == key) {
                    return o.second;
                }
            }
            return {};
        }
    };

}
