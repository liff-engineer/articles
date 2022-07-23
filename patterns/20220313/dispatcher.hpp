#pragma once

#include <type_traits>
#include <string_view>
#include <vector>
#include <cassert>
#include <memory>

namespace abc
{
    //编译期类型ID机制可参考 https://stackoverflow.com/a/56600402
    using type_code = std::string_view;

    template<typename... Ts>
    constexpr type_code type_code_of() noexcept {
#ifdef __clang__
        return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
        return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
        return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
    }

    template<typename T>
    const void* address_of(const T& obj) {
        if constexpr (!std::is_polymorphic_v<T> || std::is_final_v<T>) {
            return &obj;
        }
        else
            return dynamic_cast<const void*>(&obj);
    }

    template<typename T>
    void* address_of(T& obj) {
        if constexpr (!std::is_polymorphic_v<T> || std::is_final_v<T>) {
            return &obj;
        }
        else
            return dynamic_cast<void*>(&obj);
    }

    //设定分发系统构成的基本要素为:Action;Actor
    //由Action触发Actor,Actor同时支持发出Actor
    //Action为普通的Struct,Actor为类型,包含签名为on(Action act)的处理函数
    

    //struct actor_action_enter {};
    //struct actor_action_leave {};

    struct actor_action_log
    {
        std::size_t number; //编号
        type_code action;//执行的Action
        type_code decorator;//修饰:Enter、Leave、Post
    };

    struct trace
    {
        std::vector<actor_action_log> logs;
    };



    class dispatcher
    {
        ////要执行的动作
        //struct handler
        //{
        //    void* actor; //actor
        //    void (*op)(void*, const void*, type_code);//actor,action_payload,action payload type
        //};

        //struct actor_action_base
        //{
        //    virtual void on(const void* action, type_code code) = 0;
        //};

        //template<typename T, typename Act>
        //struct actor final :public actor_action_base
        //{
        //    T* obj{};

        //    void on(const void* action, type_code code) override {
        //        constexpr auto require_code = type_code_of<Act>();
        //        assert(require_code == code);
        //        obj->on(*static_cast<const Act*>(action));
        //    }
        //};


        struct action_base {
            
        };

        //struct action_stub
        //{
        //    void* actor;
        //    type_code actor_code;
        //    type_code action_code;
        //    void (*op)(void*, const void*, type_code);
        //};

        struct action_stub
        {
            type_code code;
            std::weak_ptr<action_base> action;
        };


    public:

        template<typename T>
        void dispatch(const T& action)
        {

        }
    };
}
// 需求列表
// - action的发送:dispatcher
// - action执行的记录:dispatcher
// - action的执行:actor
// - actor的生命周期管理:dispatcher

//简化场景,actor仅为函数体,以规避actor的生命周期管理
//避免actor持有状态导致复杂化
