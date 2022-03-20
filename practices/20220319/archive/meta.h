#pragma once
#include <type_traits>
#include <tuple>

/// @brief 类型T的成员变量信息
template<typename T,typename R>
struct Member {
    const char* name;
    R T::* mp;
};

/// @brief 创建成员变量信息辅助函数
template<typename T,typename R>
constexpr Member<T, R> MakeMember(const char* name, R T::* mp) {
    return { name,mp };
}

/// @brief 类型T的元信息头部,可以根据具体类型定制
//template<typename T,typename E = void>
//struct MetaHeader {
//    const char* name;
//
//    constexpr const char* ClassName() {
//        return name;
//    }
//
//    static constexpr MetaHeader Header(const char* name) noexcept {
//        return MetaHeader{ name };
//    }
//};

/// @brief 注意偏特化该类,一定要派生自std::true_type
template<typename T, typename E = void>
struct Meta : std::false_type {};

//偏特化的书写形式类似如下
//template<typename T>
//struct Meta<T, std::enable_if_t<std::is_class_v<T>>> :std::true_type
//{
//    static constexpr auto Get() noexcept {
//        return T::MakeMeta();
//    }
//};

template<typename... Args>
constexpr auto MakeMeta(const char* name, Args&&... args) {
    return std::make_pair(name, std::make_tuple(
        std::forward<Args>(args)...
    ));
}

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

#define MAKE_MEMBERS_1(CLASS)     #CLASS
#define MAKE_MEMBERS_2(CLASS,M1)   MAKE_MEMBERS_1(CLASS),MakeMember(#M1,&CLASS::M1)
#define MAKE_MEMBERS_3(CLASS,M1,M2)   MAKE_MEMBERS_2(CLASS,M1),MakeMember(#M2,&CLASS::M2)
#define MAKE_MEMBERS_4(CLASS,M1,M2,M3)   MAKE_MEMBERS_3(CLASS,M1,M2),MakeMember(#M3,&CLASS::M3)
#define MAKE_MEMBERS_5(CLASS,M1,M2,M3,M4)   MAKE_MEMBERS_4(CLASS,M1,M2,M3),MakeMember(#M4,&CLASS::M4)
#define MAKE_MEMBERS_6(CLASS,M1,M2,M3,M4,M5)   MAKE_MEMBERS_5(CLASS,M1,M2,M3,M4),MakeMember(#M5,&CLASS::M5)
#define MAKE_MEMBERS_7(CLASS,M1,M2,M3,M4,M5,M6)   MAKE_MEMBERS_6(CLASS,M1,M2,M3,M4,M5),MakeMember(#M6,&CLASS::M6)
#define MAKE_MEMBERS_8(CLASS,M1,M2,M3,M4,M5,M6,M7)   MAKE_MEMBERS_7(CLASS,M1,M2,M3,M4,M5,M6),MakeMember(#M7,&CLASS::M7)
#define MAKE_MEMBERS_9(CLASS,M1,M2,M3,M4,M5,M6,M7,M8)   MAKE_MEMBERS_8(CLASS,M1,M2,M3,M4,M5,M6,M7),MakeMember(#M8,&CLASS::M8)
#define MAKE_MEMBERS_10(CLASS,M1,M2,M3,M4,M5,M6,M7,M8,M9)   MAKE_MEMBERS_9(CLASS,M1,M2,M3,M4,M5,M6,M7,M8),MakeMember(#M9,&CLASS::M9)

#define MAKE_MEMBERS(...) VFUNC(MAKE_MEMBERS_,__VA_ARGS__)

#define MAKE_META(...)  MakeMeta(MAKE_MEMBERS(__VA_ARGS__))
