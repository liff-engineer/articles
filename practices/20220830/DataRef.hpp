/// 数据引用: 包装类中通过Set/Get接口暴露出的数据，以支持提取算法等公共操作

#pragma once
#include <functional>
#include <memory>

namespace abc
{
    template<typename T>
    class DataRef {
        std::function<T()> m_reader;
        std::function<bool(const T&)> m_writer;
    public:
        template<typename R, typename W>
        explicit DataRef(R&& r, W&& w)
            :m_reader(std::forward<R>(r)), m_writer(std::forward<W>(w)) {};

        explicit operator bool() const noexcept {
            return m_reader && m_writer;
        }

        T Get() {
            return m_reader();
        }

        bool Set(const T& obj) {
            return m_writer(obj);
        }
    };

    template<typename T>
    DataRef<T> MakeDataRef(T& obj) {
        return DataRef<T>{
            [&]()->T {
                return obj;
            },
                [&](const T& v)->bool {
                obj = v;
                return true;
            }
        };
    }

    template<typename T, typename U, typename R, typename W>
    DataRef<T> MakeDataRef(U& obj, R&& r, W&& w) {
        return DataRef<T>{
            [&, rf = std::forward<R>(r)]()->T {
                return rf(obj);
            },
                [&, wf = std::forward<W>(w)](const T& v)->bool {
                wf(obj, v);
                return true;
            }
        };
    }

    template<typename T>
    class IDataRef {
    public:
        virtual ~IDataRef() = default;

        virtual T Get() const = 0;
        virtual bool Set(const T& v) = 0;
    };
}
