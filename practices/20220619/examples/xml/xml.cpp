#include "pugixml.hpp"
#include "archive.h"
#include <sstream>

namespace
{
    static const auto text_archive_format = "xml";
}

class XmlArReader :public IArReader
{
public:
    pugi::xml_node node;

    struct XmlImpl {
        pugi::xml_node node;
        bool try_read(const char* m, Type type, void* v) const;
        bool try_read(const char* m, Type type, IReader& v) const;
    };
public:
    explicit XmlArReader(const pugi::xml_node n)
        :node(n) {};

    //查询接口
    std::size_t size() const noexcept override {
        return std::distance(node.begin(), node.end());
    }
protected:
    bool try_read_impl(const char* m, Type type, void* v) const override {
        return XmlImpl{ node }.try_read(m, type, v);
    }

    bool try_read_impl(const char* m, Type type, IReader& v) const override {
        return XmlImpl{ node }.try_read(m, type, v);
    }
};

class XmlArchive :public  IArchive
{
    std::shared_ptr<pugi::xml_document> doc;
    pugi::xml_node     node;
public:
    XmlArchive()
        :doc(std::make_shared<pugi::xml_document>())
    {
        node = doc->append_child();
    }

    XmlArchive(std::shared_ptr<pugi::xml_document> docArg, pugi::xml_node parent)
        :doc(docArg)
    {
        node = parent.append_child();
    }

    std::unique_ptr<IArchive> clone() const override {
        XmlArchive ar{};
        ar.doc = std::make_shared<pugi::xml_document>();
        ar.doc->reset(*doc.get());
        ar.node = ar.doc->first_child();
        return std::make_unique<XmlArchive>(std::move(ar));
    }

    const char* format() const noexcept {
        return text_archive_format;
    }

    bool open(const std::string& file)  override {
        auto result = doc->load_file(file.c_str());
        if (!result) {
            return false;
        }
        node = doc->first_child();
        return true;
    }

    bool save(const std::string& file) const override {
        //采用utf-8编码
        return doc->save_file(file.c_str(), "\t",1u,pugi::encoding_utf8);
    }

    std::string to_string() const override {
        std::stringstream oss;
        doc->save(oss);
        return  oss.str();
    }

    //查询接口
    std::size_t size() const noexcept override {
        return XmlArReader{ node }.size();
    }
protected:
    void try_write(const char* m, IWriter& writer) override {
        //Xml格式不支持数组
        if (!m)  return;
        XmlArchive ar{ doc,node };
        ar.node.set_name(m);
        writer.write(ar);
    }
    bool try_write(const char* m, IArchive& v) override {
        if (auto vp = dynamic_cast<XmlArchive*>(&v)) {
            auto n = m ? node.append_child(m) : node;
            n.append_copy(vp->doc->document_element());
            return true;
        }
        return false;
    }

    void try_write_impl(const char* m, Type type, const void* v) override {
        //Xml格式不支持数组,没有指定key则直接退出
        if (!m) return;
        switch (type) {
        case Type::Boolean:
            node.append_attribute(m).set_value(*static_cast<const bool*>(v));
            break;
        case Type::Integer:
            node.append_attribute(m).set_value(*static_cast<const int64_t*>(v));
            break;
        case Type::UInteger:
            node.append_attribute(m).set_value(*static_cast<const uint64_t*>(v));
            break;
        case Type::Number:
            node.append_attribute(m).set_value(*static_cast<const double*>(v));
            break;
        case Type::String:
        {
            auto vp = static_cast<const char*>(v);
            node.append_attribute(m).set_value(vp);
        }
        break;
        case Type::Nil:
            node.append_attribute(m);
            break;
        default:
            break;
        }
    }

    bool try_read_impl(const char* m, Type type, void* v) const override {
        return XmlArReader::XmlImpl{ node }.try_read(m, type, v);
    }

    bool try_read_impl(const char* m, Type type, IReader& v) const override {
        return XmlArReader::XmlImpl{ node }.try_read(m, type, v);
    }

    bool write_tag_impl(const char* tag) override {
        return node.set_name(tag);
    }
};

namespace
{
    struct ArchiveRegister
    {
        std::string code;

        ArchiveRegister(const char* format, std::unique_ptr<IArchive>(*ctor)()) :code(format) {
            IArchive::Register(format, ctor);
        }
        ~ArchiveRegister() {
            IArchive::Unregister(code.c_str());
        }

        template<typename T>
        static ArchiveRegister Make(const char* format) {
            return ArchiveRegister{ format,[]()->std::unique_ptr<IArchive> { return std::make_unique<T>(); } };
        }
    };
    static auto gRegister = ArchiveRegister::Make<XmlArchive>(text_archive_format);
}

bool XmlArReader::XmlImpl::try_read(const char* m, Type type, void* v) const
{
    //Xml格式不支持数组,因而只能根据key查找值
    if (!m) return false;
    auto obj = node.attribute(m);
    if (!obj) return false;
    switch (type) {
    case Type::Boolean:
    {
        *static_cast<bool*>(v) = obj.as_bool();
        return true;
    }
        break;
    case Type::Integer:
    {
        *static_cast<int64_t*>(v) = obj.as_llong();
        return true;
    }
        break;
    case Type::UInteger:
    {
        *static_cast<uint64_t*>(v) = obj.as_ullong();
        return true;
    }
        break;
    case Type::Number:
    {
        *static_cast<double*>(v) = obj.as_double();
        return true;
    }
        break;
    case Type::String:
    {
        *static_cast<std::string*>(v) = obj.as_string();
        return true;
    }
        break;
    case Type::Nil:
        return true;
    default:
        break;
    }
    return false;
}

bool XmlArReader::XmlImpl::try_read(const char* m, Type type, IReader& v) const
{
    switch (type) {
    case Type::Array:
        //Xml格式不支持数组
        break;
    case Type::Object: 
    {
        bool result = true;
        for (auto& n : node.children()) {
            v.read(n.name(), XmlArReader{ n });
        }
        return result;
    }
        break;
    case Type::Internal:
        if (!m) {
            return v.read(nullptr, XmlArReader{ node });
        }
        else if (auto n = node.child(m); n) {
            return v.read(nullptr, XmlArReader{ n });
        }
        break;
    default:
        break;
    }
    return false;
}
