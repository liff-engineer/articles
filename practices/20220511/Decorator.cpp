#include <type_traits>
#include <cassert>
#include <memory>
#include <iostream>

template<typename T, typename... Args>
bool Is(Args&&... args) {
    return T::Is(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
T   As(Args&&... args) {
    return T{ std::forward<Args>(args)... };
}

//测试用接口
class IValue {
public:
    virtual ~IValue() = default;
};

namespace impl {
    class MyObjectImpl :public IValue {
        int m_iV{};
        double m_dV{};
    public:
        int iV() const { return m_iV; }
        double dV() const { return m_dV; }
        void iV(int v) { m_iV = v; }
        void dV(double v) { m_dV = v; };
    };
}

namespace v1 {

    /// @brief 外部API
    class EntityProxy {
    public:
        EntityProxy() = default;
        explicit EntityProxy(IValue* value)
            :m_v(value)
        {
            assert(m_v != nullptr);
        }

        //虽然有虚函数,但是应用场景是有保证的,因而把5件套全实现了
        EntityProxy(const EntityProxy& other) = default;
        EntityProxy& operator=(const EntityProxy& other) = default;
        EntityProxy(EntityProxy&& other) noexcept = default;
        EntityProxy& operator=(EntityProxy&& other) noexcept = default;
        virtual ~EntityProxy() = default;

        explicit  operator bool() const noexcept {
            return IsValidImpl();
        }

        template<typename T>
        T  As() const {
            return T{ m_v };
        }
    protected:
        IValue* m_v{};
        virtual bool IsValidImpl() const noexcept {
            return m_v != nullptr;
        }
    };


    class MyObjectProxy : public EntityProxy {
    public:
        static impl::MyObjectImpl* To(IValue* v) {
            return dynamic_cast<impl::MyObjectImpl*>(v);
        }
        static bool Is(IValue* v) {
            return To(v) != nullptr;
        }
    public:
        using EntityProxy::EntityProxy;
        MyObjectProxy() = default;
        explicit MyObjectProxy(IValue* v)
            :EntityProxy(v)
        {
            //这个函数有副作用,实际实现时采用抛出异常的策略
            m_impl = To(v);
            assert(m_impl != nullptr);
        }

        int iV() const {
            return m_impl->iV();
        }
        double dV() const {
            return m_impl->dV();
        }
        void iV(int v) {
            m_impl->iV(v);
        }
        void dV(double v) {
            m_impl->dV(v);
        }
    protected:
        impl::MyObjectImpl* m_impl{};
        bool IsValidImpl() const noexcept override {
            return EntityProxy::IsValidImpl() && (m_impl != nullptr);
        }
    };


    /////////////////////////////////////////////////
    ///  以下是基于派生方式扩展API以及对象类型

    class MyObjectAddApiProxy :public  MyObjectProxy {
    public:
        using MyObjectProxy::MyObjectProxy;

        std::pair<int, double> Values() const {
            return std::make_pair(iV(), dV());
        }

        void setValues(int iv, double dv) {
            iV(iv);
            dV(dv);
        }
    };

    class MyObjectAddCheckProxy :public MyObjectProxy {
    public:
        static bool Is(const MyObjectProxy& obj) {
            return obj.iV() > 256;
        }
        static bool Is(IValue* v) {
            if (!MyObjectProxy::Is(v)) return false;
            return Is(MyObjectProxy{ v });
        }
    public:
        MyObjectAddCheckProxy() = default;
        explicit MyObjectAddCheckProxy(IValue* v)
            :MyObjectProxy(v) {
            assert(Is(*this));
        };
    protected:
        bool IsValidImpl() const noexcept override {
            return MyObjectProxy::IsValidImpl() && (Is(*this));
        }
    };

    /////////////////////////////////////////////////
    ///  以下是基于组合方式扩展API以及对象类型

    class MyObjectAddApiDecorator {
        MyObjectProxy m_obj;
    public:
        static bool Is(IValue* v) {
            return MyObjectProxy::Is(v);
        }
    public:
        MyObjectAddApiDecorator() = default;

        explicit MyObjectAddApiDecorator(IValue* v)
            :m_obj(v) {};

        MyObjectProxy* operator ->() noexcept {
            return std::addressof(m_obj);
        }

        const MyObjectProxy* operator ->() const noexcept {
            return std::addressof(m_obj);
        }

        explicit operator bool() const noexcept {
            return m_obj.operator bool();
        }

        std::pair<int, double> Values() const {
            return std::make_pair(m_obj.iV(), m_obj.dV());
        }

        void setValues(int iv, double dv) {
            m_obj.iV(iv);
            m_obj.dV(dv);
        }
    };

    class MyObjectAddCheckDecorator {
        MyObjectProxy m_obj;
    public:
        static bool Is(const MyObjectProxy& obj) {
            return obj.iV() > 256;
        }
        static bool Is(IValue* v) {
            if (!MyObjectProxy::Is(v)) return false;
            return Is(MyObjectProxy{ v });
        }
    public:
        MyObjectAddCheckDecorator() = default;
        explicit MyObjectAddCheckDecorator(IValue* v)
            :m_obj(v) {
            assert(Is(m_obj));
        }

        MyObjectProxy* operator ->() noexcept {
            return std::addressof(m_obj);
        }

        const MyObjectProxy* operator ->() const noexcept {
            return std::addressof(m_obj);
        }

        explicit operator bool() const noexcept {
            return m_obj && Is(m_obj);
        }
    };

    //Proxy和Decorator还是有相对明显的差别的
    //这个差别可以以写法论(继承、组合)
    //也可以以目的论(转发动作、扩展能力)
    //
    //以目的论较为贴合使用场景,这里以目的论,除了EntityProxy
    //其它都应该算Decorator

    void test(IValue* v) {
        if (!Is<MyObjectProxy>(v)) {
            std::cout << "!Is<MyObject>\n";
            return;
        }
        auto obj = As<MyObjectProxy>(v);
        std::cout << obj.iV() << "\n";
        std::cout << obj.dV() << "\n";
        obj.iV(128);
        obj.dV(obj.dV() * 2);
        std::cout << obj.iV() << "\n";
        std::cout << obj.dV() << "\n";

        //测试代理方式-附加API
        {
            auto r = obj.As<MyObjectAddApiProxy>();
            auto values = r.Values();
            std::cout << "(" << values.first << "," << values.second << ")" << "\n";
        }
        //测试修饰器方式-附加API
        {
            auto r = obj.As<MyObjectAddApiDecorator>();
            std::cout << "(" << r->iV() << "," << r->dV() << ")" << "\n";
        }

        if (!Is<MyObjectAddApiProxy>(v)) {
            std::cout << "!Is<MyObjectAddApiProxy>\n";
        }
        else
        {
            std::cout << "Is<MyObjectAddApiProxy>\n";
        }

        if (!Is<MyObjectAddCheckProxy>(v)) {
            std::cout << "!Is<MyObjectAddCheckProxy>\n";
        }
        else
        {
            std::cout << "Is<MyObjectAddCheckProxy>\n";
        }

        if (!Is<MyObjectAddCheckDecorator>(v)) {
            std::cout << "!Is<MyObjectAddCheckDecorator>\n";
        }
        else
        {
            std::cout << "Is<MyObjectAddCheckDecorator>\n";
        }
    }

    void RunTest(IValue& obj)
    {

        EntityProxy e1{};
        MyObjectAddApiProxy e2{};
        MyObjectAddApiProxy e2c = e2;
        MyObjectAddCheckProxy e3{};

        test(&obj);
    }
}

#include "Proxy.hpp"
namespace v2
{
    class ObjectProxy :public abc::Proxy<IValue*> {
    public:
        static impl::MyObjectImpl* To(IValue* v) {
            return dynamic_cast<impl::MyObjectImpl*>(v);
        }
        static bool Is(IValue* v) {
            return To(v) != nullptr;
        }
    public:
        using Super::Super;
        ObjectProxy() = default;
        explicit ObjectProxy(IValue* v)
            :Super(v), m_impl(To(v))
        {}

        int iV() const {
            return m_impl->iV();
        }
        double dV() const {
            return m_impl->dV();
        }

        void iV(int v) {
            m_impl->iV(v);
        }
        void dV(double v) {
            m_impl->dV(v);
        }
    protected:
        impl::MyObjectImpl* m_impl{};
        bool IsValidImpl() const noexcept override {
            return Super::IsValidImpl() && (m_impl != nullptr);
        }
    };

    /////////////////////////////////////////////////
    ///  以下是基于派生方式扩展API以及对象类型

    class ObjectAddApiProxy :public  ObjectProxy {
    public:
        using ObjectProxy::ObjectProxy;

        std::pair<int, double> Values() const {
            return std::make_pair(iV(), dV());
        }

        void setValues(int iv, double dv) {
            iV(iv);
            dV(dv);
        }
    };

    class MyObjectAddCheckProxy :public ObjectProxy {
    public:
        static bool Is(const ObjectProxy& obj) {
            return obj.iV() > 256;
        }
        static bool Is(IValue* v) {
            if (!ObjectProxy::Is(v)) return false;
            return Is(ObjectProxy{ v });
        }
    public:
        MyObjectAddCheckProxy() = default;
        explicit MyObjectAddCheckProxy(IValue* v)
            :ObjectProxy(v) {
            assert(Is(*this));
        };
    protected:
        bool IsValidImpl() const noexcept override {
            return ObjectProxy::IsValidImpl() && (Is(*this));
        }
    };

    /////////////////////////////////////////////////
    ///  以下是基于组合方式扩展API以及对象类型

    class MyObjectAddApiDecorator :public abc::Proxy<ObjectProxy> {
    public:
        static bool Is(IValue* v) {
            return ObjectProxy::Is(v);
        }
    public:
        MyObjectAddApiDecorator() = default;
        explicit MyObjectAddApiDecorator(IValue* v)
            :Super(ObjectProxy(v)) {};

        std::pair<int, double> Values() const {
            return std::make_pair(Get().iV(), Get().dV());
        }

        void setValues(int iv, double dv) {
            Get().iV(iv);
            Get().dV(dv);
        }
    };

    void test(IValue* v) {
        if (!Is<ObjectProxy>(v)) {
            std::cout << "!Is<MyObject>\n";
            return;
        }
        auto obj = As<ObjectProxy>(v);
        std::cout << obj.iV() << "\n";
        std::cout << obj.dV() << "\n";
        obj.iV(128);
        obj.dV(obj.dV() * 2);
        std::cout << obj.iV() << "\n";
        std::cout << obj.dV() << "\n";

        //测试代理方式-附加API
        {
            auto r = ObjectAddApiProxy(v);
            auto values = r.Values();
            std::cout << "(" << values.first << "," << values.second << ")" << "\n";
        }
        //测试修饰器方式-附加API
        {
            auto r = MyObjectAddApiDecorator(v);
            std::cout << "(" << r->iV() << "," << r->dV() << ")" << "\n";
        }

        if (!Is<ObjectAddApiProxy>(v)) {
            std::cout << "!Is<MyObjectAddApiProxy>\n";
        }
        else
        {
            std::cout << "Is<MyObjectAddApiProxy>\n";
        }

        if (!Is<MyObjectAddCheckProxy>(v)) {
            std::cout << "!Is<MyObjectAddCheckProxy>\n";
        }
        else
        {
            std::cout << "Is<MyObjectAddCheckProxy>\n";
        }

        if (!Is<MyObjectAddApiDecorator>(v)) {
            std::cout << "!Is<MyObjectAddCheckDecorator>\n";
        }
        else
        {
            std::cout << "Is<MyObjectAddCheckDecorator>\n";
        }
    }

    void RunTest(IValue& obj)
    {
        ObjectAddApiProxy e2{};
        ObjectAddApiProxy e2c = e2;
        MyObjectAddCheckProxy e3{};

        test(&obj);
    }
}

int main() {
    impl::MyObjectImpl obj{};
    obj.iV(1024);
    obj.dV(3.1415);
    v1::RunTest(obj);
    v2::RunTest(obj);
    return 0;
}
