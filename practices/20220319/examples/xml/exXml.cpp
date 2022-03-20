#include "archive.h"

struct MyObject
{
    bool bV;
    int iV;
    double dV;
    std::string sV;
    std::vector<int> iVs;
    std::map<int, std::string> kVs;
};

struct MyComplexObject
{
    MyObject oV;
    std::vector<double> dVs;
};


#if 1
//template<>
//struct Meta<MyObject>:std::true_type
//{
//    static constexpr auto Make() noexcept {
//        return MakeMeta("MyObject",
//            MakeMember("bV", &MyObject::bV),
//            MakeMember("iV", &MyObject::iV),
//            MakeMember("dV", &MyObject::dV),
//            MakeMember("sV", &MyObject::sV),
//            MakeMember("iVs", &MyObject::iVs),
//            MakeMember("kVs", &MyObject::kVs)
//            );
//    }
//};
template<>
struct Meta<MyObject> :std::true_type
{
    static constexpr auto Make() noexcept {
        return MAKE_META(MyObject, bV, iV, dV, sV, iVs, kVs);
    }
};
#else
template<>
struct ArAdapter<MyObject>
{
    static void write(IArchive& ar, const MyObject& obj) {
        ar.write("bV", obj.bV);
        ar.write("iV", obj.iV);
        ar.write("dV", obj.dV);
        ar.write("sV", obj.sV);
        ar.write("iVs", obj.iVs);
        ar.write("kVs", obj.kVs);
    }

    static bool read(IArReader& ar, MyObject& obj) {
        ar.read("bV", obj.bV);
        ar.read("iV", obj.iV);
        ar.read("dV", obj.dV);
        ar.read("sV", obj.sV);
        ar.read("iVs", obj.iVs);
        ar.read("kVs", obj.kVs);
        return true;
    }
};
#endif

#if 1
template<>
struct Meta<MyComplexObject> :std::true_type
{
    static constexpr auto Make() noexcept {
        return MakeMeta("MyComplexObject",
            MakeMember("oV", &MyComplexObject::oV),
            MakeMember("dVs", &MyComplexObject::dVs)
        );
    }
};
#else 
template<>
struct ArAdapter<MyComplexObject>
{
    static void write(IArchive& ar, const MyComplexObject& obj) {
        ar.write("oV", obj.oV);
        ar.write("dVs", obj.dVs);
    }
    static bool read(IArReader& ar, MyComplexObject& obj) {
        ar.read("oV", obj.oV);
        ar.read("dVs", obj.dVs);
        return true;
    }
};
#endif

void test_load_xml(std::string const& file) {
    auto ar = Archive("xml");
    ar->open(file);

    MyObject obj{};
    ar->read("MyObject", obj);
    MyComplexObject obj1{};
    ar->read("MyComplexObject", obj1);

    if (obj1.dVs.empty()) {

    }
}

void test_archive_clone(const Archive& other)
{
    Archive ar{ other };

    MyObject obj{};
    ar.read("MyObject", obj);
    MyComplexObject obj1{};
    ar.read("MyComplexObject", obj1);
    if (obj1.dVs.empty()) {

    }
}

int main() {

    MyObject obj{};
    obj.bV = false;
    obj.iV = 1024;
    obj.dV = 3.1415926;
    obj.sV = "liff.engineer@gmail.com";
    obj.iVs = { 1,3,5,7,9 };
    obj.kVs = { {1,"liff.xian@gmail.com"},{2,"liff.cpplang@gmail.com"} };
    
    MyComplexObject obj1{};
    obj1.oV = obj;
    obj1.dVs = { 1.1,2.2,3.3,4.4 };

    auto ar = Archive("xml");
    ar->write("MyComplexObject", obj1);
    ar->write("MyObject", obj);
    auto result = ar->to_string();
    ar->save("archive.xml");

    test_archive_clone(ar);

    test_load_xml("archive.xml");
    return 0;
}
