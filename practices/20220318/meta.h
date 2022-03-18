#pragma once
#include <type_traits>
#include <tuple>

template<typename T, typename R>
struct member
{
    const char* name;
    R T::* mp;
};

template<typename T, typename R>
constexpr member<T, R> make_member(const char* name, R T::* mp) {
    return { name,mp };
}

/// @brief 注意偏特化该类,一定要派生自std::true_type
template<typename T, typename E = void>
struct meta : std::false_type {};

//偏特化的书写形式类似如下
//template<typename T>
//struct meta<T, std::enable_if_t<std::is_class_v<T>>> :std::true_type
//{
//    static constexpr auto make() noexcept {
//        return T::make_meta();
//    }
//};

//计算宏参数个数:最多支持10个?
#define VFUNC_NARGS_IMPL_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VFUNC_NARGS_IMPL(args)   VFUNC_NARGS_IMPL_ args
#define VFUNC_NARGS(...)   VFUNC_NARGS_IMPL((__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))

//通用的可变参数版本宏
#define VFUNC_IMPL_(name,n)  name##n
#define VFUNC_IMPL(name,n)  VFUNC_IMPL_(name,n)
#define VFUNC_(name,n) VFUNC_IMPL(name,n)
#define VFUNC_EXPAND(x, y) x y 
#define VFUNC(func,...) VFUNC_EXPAND(VFUNC_(func,VFUNC_NARGS(__VA_ARGS__)) , (__VA_ARGS__))

#define MAKE_META_1(CLASS)   \
    std::make_pair(#CLASS,std::make_tuple()) 

#define MAKE_META_2(CLASS,M1)  \
    std::make_pair(#CLASS,   \
        std::make_tuple(make_member(#M1, &CLASS::M1)))  

#define MAKE_META_3(CLASS,M1,M2) \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2) \
    ));

#define MAKE_META_4(CLASS,M1,M2,M3)  \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2), \
            make_member(#M3, &CLASS::M3) \
        ));

#define MAKE_META_5(CLASS,M1,M2,M3,M4) \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2), \
            make_member(#M3, &CLASS::M3), \
            make_member(#M4, &CLASS::M4) \
        ));

#define MAKE_META_6(CLASS,M1,M2,M3,M4,M5) \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2), \
            make_member(#M3, &CLASS::M3), \
            make_member(#M4, &CLASS::M4), \
            make_member(#M5, &CLASS::M5) \
        ));

#define MAKE_META_7(CLASS,M1,M2,M3,M4,M5,M6) \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2), \
            make_member(#M3, &CLASS::M3), \
            make_member(#M4, &CLASS::M4), \
            make_member(#M5, &CLASS::M5), \
            make_member(#M6, &CLASS::M6) \
        ));

#define MAKE_META_8(CLASS,M1,M2,M3,M4,M5,M6,M7) \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2), \
            make_member(#M3, &CLASS::M3), \
            make_member(#M4, &CLASS::M4), \
            make_member(#M5, &CLASS::M5), \
            make_member(#M6, &CLASS::M6), \
            make_member(#M7, &CLASS::M7) \
        ));

#define MAKE_META_9(CLASS,M1,M2,M3,M4,M5,M6,M7,M8) \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2), \
            make_member(#M3, &CLASS::M3), \
            make_member(#M4, &CLASS::M4), \
            make_member(#M5, &CLASS::M5), \
            make_member(#M6, &CLASS::M6), \
            make_member(#M7, &CLASS::M7), \
            make_member(#M8, &CLASS::M8) \
        ));

#define MAKE_META_10(CLASS,M1,M2,M3,M4,M5,M6,M7,M8,M9) \
    std::make_pair(#CLASS, \
        std::make_tuple(   \
            make_member(#M1, &CLASS::M1), \
            make_member(#M2, &CLASS::M2), \
            make_member(#M3, &CLASS::M3), \
            make_member(#M4, &CLASS::M4), \
            make_member(#M5, &CLASS::M5), \
            make_member(#M6, &CLASS::M6), \
            make_member(#M7, &CLASS::M7), \
            make_member(#M8, &CLASS::M8), \
            make_member(#M9, &CLASS::M9) \
        ));

#define MAKE_META(...) VFUNC(MAKE_META_,__VA_ARGS__)
