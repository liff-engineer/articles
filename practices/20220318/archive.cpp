#include "meta.h"
#include "archive.h"
#include <iostream>
#include <tuple>

template<typename T>
struct writer<T, std::enable_if_t<meta<T>::value>>
{
    template<typename U,typename R>
    static void write(archive& ar, const T& obj, member<U, R> m) {
        //出现这种情况,说明开发者是抄写的代码,使用&Class::member时,忘记修改Class了
        static_assert(std::is_same_v<T,U>, "member declare invalid,check your code.");
        ar.write(m.name, (obj).*(m.mp));
    }

    template<typename U>
    static void write(archive& ar, const T& obj, U m) {
        //正常情况下都走上面的member<U,R>分支,一旦出现这个,说明
        //类型T的meta生成函数(默认为make_meta)返回了错误的内容
        static_assert(false, "meta info invalid,check your code.");
    }

    static archive save(const T& obj) {
        static auto class_meta = meta<T>::make();
        archive ar;
        ar.write("__class_id__", class_meta.first);
        std::apply
        ([&](auto&& ... args) 
            {
               (write(ar, obj, args), ...);
            }, class_meta.second
        );
        return ar;
    }
};

struct MyObject
{
    bool bV;
    int iV;
    double dV;
    std::string sV;

    archive save() const {
        archive ar;
        ar.write("__class_id__", "MyObject");
        ar.write("bV", bV);
        ar.write("iV", iV);
        ar.write("dV", dV);
        ar.write("sV", sV);
        return ar;
    }

    bool  read(archive::reader ar) {
        if (!ar.verify("__class_id__", "MyObject")) {
            return false;
        }
        ar.read_to("bV", bV);
        ar.read_to("iV", iV);
        ar.read_to("dV", dV);
        ar.read_to("sV", sV);
        return true;
    }

    //static constexpr auto make_meta() noexcept {
    //    return std::make_pair("MyObject",
    //        std::make_tuple(
    //            make_member("bV", &MyObject::bV),
    //            make_member("iV", &MyObject::iV),
    //            make_member("dV", &MyObject::dV),
    //            make_member("sV", &MyObject::sV)
    //        ));
    //}
};

template<>
struct writer<MyObject>
{
    static archive save(const MyObject& obj) {
        return obj.save();
    }
};

template<>
struct reader<MyObject>
{
    static bool read(archive::reader ar, MyObject& o) {
        return o.read(ar);
    }
};


//template<>
//struct meta<MyObject>: std::true_type
//{
//    static constexpr auto make() noexcept {
//        return MyObject::make_meta();
//    }
//};


struct MyComplexObject
{
    MyObject oV;
    std::vector<double> dVs;
};

template<>
struct reader<MyComplexObject>
{
    static bool read(archive::reader ar, MyComplexObject& o)
    {
        if (!ar.verify("__class_id__", "MyComplexObject")) {
            return false;
        }
        ar.read_to("oV", o.oV);
        auto reader = ar.get_reader("dVs");
        if (!reader) return false;
        o.dVs.reserve(reader.size());
        reader.foreach([&](archive::reader ar_obj) {
            o.dVs.push_back(double{});
            ar_obj.read_to(o.dVs.back());
            });
        return true;
    }
};


//template<>
//struct writer<MyComplexObject>
//{
//    static archive save(const MyComplexObject& obj) {
//        archive ar;
//        ar.write("__class_id__", "MyComplexObject");
//        ar.write("oV", obj.oV);
//        ar.write("dV",obj.dVs);
//        return ar;
//    }
//};

template<>
struct meta<MyComplexObject> : std::true_type
{
    static constexpr auto make() noexcept {
        return MAKE_META(MyComplexObject, oV, dVs);
    }
};

struct base
{
    virtual ~base() = default;
    virtual archive save() const = 0;
};

template<>
struct writer<base>
{
    static archive save(const base& obj) {
        return obj.save();
    }
};

//采用偏特化机制,要求使用T::make_meta提供meta信息
template<typename T>
struct meta<T, std::enable_if_t<std::is_base_of_v<base, T>>> : std::true_type
{
    static constexpr auto make() noexcept {
        return T::make_meta();
    }
};

template<typename T>
struct Base :public base
{
    archive save() const override {
        return writer<T>::save(*static_cast<const T*>(this));
    }
};

struct Class1 final :public Base<Class1>
{
    MyObject oV;
    std::string sV;

    Class1(const MyObject& o,std::string s)
        :oV(o),sV(s) {}

    //采用make_meta机制实现
    //archive save() const override;
    //{
    //    archive ar;
    //    ar.write("__class_id__", "Class1");
    //    ar.write("oV", oV);
    //    ar.write("sV", sV);
    //    return ar;
    //}

    static auto make_meta() noexcept {
        return MAKE_META(Class1, oV, sV);
    }
};

//常规机制实现?
struct Class2 final :public base
{
    MyComplexObject oV;

    Class2(const MyComplexObject& o)
        :oV(o) {}

    archive save() const override {
        archive ar;
        ar.write("__class_id__", "Class2");
        ar.write("oV", oV);
        return ar;
    }
};

struct  Class3
{
    std::unique_ptr<base> o;
};

template<>
struct writer<Class3>
{
    static archive save(const Class3& obj) {
        archive ar;
        ar.write("__class_id__", "Class3");
        ar.write("ptr", obj.o);
        return ar;
    }
};

struct TestObject
{
    bool bV;
    int iV;
    double dV;
    std::string sV;

    static constexpr auto make_meta() noexcept {
        //return std::make_pair("TestObject",
        //    std::make_tuple(
        //        make_member("bV", &TestObject::bV),
        //        make_member("iV", &TestObject::iV),
        //        make_member("dV", &TestObject::dV),
        //        make_member("sV", &TestObject::sV)
        //    ));
        return MAKE_META(TestObject, bV, iV, dV, sV);
    }
};

template<>
struct meta<TestObject> :std::true_type {
    static constexpr auto make() noexcept {
        return TestObject::make_meta();
    }
};


int main()
{
    {
        TestObject obj{};
        obj.bV = true;
        obj.iV = 1024;
        obj.dV = 3.1415;
        obj.sV = "liff.engineer@gmail.com";

        archive ar_object{};
        ar_object.write(obj);
        auto str_object = ar_object.to_string();

    }

    //常规的结构体/类型
    MyObject obj{};
    obj.bV = true;
    obj.iV = 1024;
    obj.dV = 3.1415;
    obj.sV = "liff.engineer@gmail.com";

    archive ar_object{};
    ar_object.write(obj);
    auto str_object = ar_object.to_string();
    std::cout << "\nobject\n";
    std::cout << str_object;

    {
        //读取测试
        archive ar{};
        ar.write("obj", obj);

        auto reader = ar.get_reader("obj");
        if (reader) {
            MyObject result{};
            result.read(reader);
        }
    }

    //复杂结构体
    MyComplexObject obj1{};
    obj1.oV = obj;
    obj1.dVs = { 1.414,3.14,1.7 };

    archive ar_complex_object{};
    ar_complex_object.write("obj1", obj1);
    auto str_complex_object = ar_complex_object.to_string();
    std::cout << "\ncomplex_object\n";
    std::cout << str_complex_object;


    MyComplexObject obj2{};
    ar_complex_object.get_reader("obj1").read_to(obj2);

    //容器
    std::vector<MyObject> values;
    values.push_back(obj);
    values.push_back(obj);
    values.push_back(obj);

    archive ar_values;
    ar_values.write(values);
    auto str_values = ar_values.to_string();
    std::cout << "\nvalues\n";
    std::cout << str_values;

    std::map<std::string, MyObject> map;
    map["O1"] = obj;
    map["O2"] = obj;
    map["O3"] = obj;

    archive ar_map;
    ar_map.write(map);
    auto str_map = ar_map.to_string();
    std::cout << "\nmap\n";
    std::cout << str_map;

    //基类指针
    Class3 o3;
    o3.o = std::make_unique<Class1>( obj, "liff-b@glodon.com" );
    Class3 o4;
    o4.o = std::make_unique<Class2>( obj1 );
    
    archive ar_base_class;
    ar_base_class.write("o3", o3);
    ar_base_class.write("o4", o4);
    auto str_base_class = ar_base_class.to_string();
    std::cout << "\nbase class\n";
    std::cout << str_base_class;
    return 0;
}
