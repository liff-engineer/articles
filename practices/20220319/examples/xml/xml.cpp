#include "pugixml.hpp"
#include "archive.h"
#include <sstream>

namespace
{
    static const auto text_archive_format = "xml";
#if 1
    static const auto Node = "Node";
    static const auto KeyAttr = "Key";
    static const auto ValueAttr = "Value";
#else
    static const auto Node = "N";
    static const auto KeyAttr = "K";
    static const auto ValueAttr = "V";
#endif
}

class XmlArReader :public IArReader
{
public:
    pugi::xml_node node;
public:
    explicit XmlArReader(const pugi::xml_node n)
        :node(n) {};

    //查询接口
    std::size_t size() const noexcept override {
        return std::distance(node.begin(), node.end());
    }

    void visit(IVisitor& op) const override {
        for (auto& n : node.children()) {
            XmlArReader reader{ n };
            op.accept(reader);
        }
    }
    void visit(IKVVisitor& op) const override {
        for (auto& n : node.children()) {
            XmlArReader reader{ n };
            op.accept(n.attribute(KeyAttr).as_string(), reader);
        }
    }

    bool read(bool& v) const override {
        if (auto attr = node.attribute(ValueAttr)) {
            v = attr.as_bool();
            return true;
        }
        return false;
    }

    bool read(int64_t& v) const  override {
        if (auto attr = node.attribute(ValueAttr)) {
            v = attr.as_llong();
            return true;
        }
        return false;
    }

    bool read(uint64_t& v) const  override {
        if (auto attr = node.attribute(ValueAttr)) {
            v = attr.as_ullong();
            return true;
        }
        return false;
    }

    bool read(double& v) const override {
        if (auto attr = node.attribute(ValueAttr)) {
            v = attr.as_double();
            return true;
        }
        return false;
    }

    bool read(std::string& v) const override {
        if (auto attr = node.attribute(ValueAttr)) {
            v = attr.as_string();
            return true;
        }
        return false;
    }

    bool read(std::vector<char>& v) const override {
        //TODO FIXME
        return false;
    }

    bool read() const override {
        return node.attribute(ValueAttr);
    }

    bool read(const char* member, bool& v) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            if (auto attr = n.attribute(ValueAttr)) {
                v = attr.as_bool();
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, int64_t& v) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            if (auto attr = n.attribute(ValueAttr)) {
                v = attr.as_llong();
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, uint64_t& v) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            if (auto attr = n.attribute(ValueAttr)) {
                v = attr.as_ullong();
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, double& v) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            if (auto attr = n.attribute(ValueAttr)) {
                v = attr.as_double();
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, std::string& v) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            if (auto attr = n.attribute(ValueAttr)) {
                v = attr.as_string();
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, std::vector<char>& v) const override {
        //TODO FIXME
        return false;
    }

    bool read(const char* member) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            return true;
        }
        return false;
    }
protected:
    bool       try_read(const char* member, IReader& reader) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            XmlArReader ar{ n };
            return reader.read(ar);
        }
        return false;
    }
    
    bool       try_read(IReader& reader) const override {
        return reader.read(*this);
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
        node = doc->append_child(Node);
    }

    XmlArchive(std::shared_ptr<pugi::xml_document> docArg, pugi::xml_node parent)
        :doc(docArg)
    {
        node = parent.append_child(Node);
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

    void write(bool v) override {
        node.append_child(Node).append_attribute(ValueAttr).set_value(v);
    }

    void write(int64_t v) override {
        node.append_child(Node).append_attribute(ValueAttr).set_value(v);
    }
    void write(uint64_t v) override {
        node.append_child(Node).append_attribute(ValueAttr).set_value(v);
    }

    void write(double v) override {
        node.append_child(Node).append_attribute(ValueAttr).set_value(v);
    }

    void write(const char* v) override {
        node.append_child(Node).append_attribute(ValueAttr).set_value(v);
    }

    void write(const std::vector<char>& v)  override {
        //TODO FIXME
    }

    void write(nullptr_t v) override {
        node.append_child(Node).append_attribute(ValueAttr);
    }

    template<typename T>
    void writeImpl(const char* member, T&& v) {
        auto result = node.append_child(Node);
        result.append_attribute(KeyAttr).set_value(member);
        result.append_attribute(ValueAttr).set_value(v);
    }

    void writeImpl(const char* member, nullptr_t v) {
        auto result = node.append_child(Node);
        result.append_attribute(KeyAttr).set_value(member);
    }

    void write(const char* member, bool v)  override {
        writeImpl(member, v);
    }

    void write(const char* member, int64_t v) override {
        writeImpl(member, v);
    }

    void write(const char* member, uint64_t v) override {
        writeImpl(member, v);
    }

    void write(const char* member, double v) override {
        writeImpl(member, v);
    }

    void write(const char* member, const char* v) override {
        writeImpl(member, v);
    }

    void write(const char* member, const std::vector<char>& v) override {
        //TODO FIXME
    }

    void write(const char* member, nullptr_t v) override {
        writeImpl(member, v);
    }

    //查询接口
    std::size_t size() const noexcept override {
        return XmlArReader{ node }.size();
    }

    void visit(IVisitor& op) const override {
        return XmlArReader{ node }.visit(op);
    }
    void visit(IKVVisitor& op) const override {
        return XmlArReader{ node }.visit(op);
    }

    bool read(bool& v) const override {
        return XmlArReader{ node }.read(v);
    }
    bool read(int64_t& v) const  override {
        return XmlArReader{ node }.read(v);
    }
    bool read(uint64_t& v) const  override {
        return XmlArReader{ node }.read(v);
    }

    bool read(double& v) const override {
        return XmlArReader{ node }.read(v);
    }

    bool read(std::string& v) const override {
        return XmlArReader{ node }.read(v);
    }

    bool read(std::vector<char>& v) const override {
        return XmlArReader{ node }.read(v);
    }

    bool read() const override {
        return XmlArReader{ node }.read();
    }

    bool read(const char* member, bool& v) const override {
        return XmlArReader{ node }.read(member, v);
    }

    bool read(const char* member, int64_t& v) const override {
        return XmlArReader{ node }.read(member, v);
    }

    bool read(const char* member, uint64_t& v) const override {
        return XmlArReader{ node }.read(member, v);
    }

    bool read(const char* member, double& v) const override {
        return XmlArReader{ node }.read(member, v);
    }

    bool read(const char* member, std::string& v) const override {
        return XmlArReader{ node }.read(member, v);
    }

    bool read(const char* member, std::vector<char>& v) const override {
        return XmlArReader{ node }.read(member,v);
    }

    bool read(const char* member) const override {
        return XmlArReader{ node }.read(member);
    }
protected:
    bool       try_read(const char* member, IReader& reader) const override {
        if (auto n = node.find_child_by_attribute(KeyAttr, member)) {
            XmlArReader ar{ n };
            return reader.read(ar);
        }
        return false;
    }

    bool       try_read(IReader& reader) const override {
        return reader.read(*this);
    }

    void try_write(const char* member, IWriter& writer) override {
        XmlArchive ar{ doc,node };
        ar.node.append_attribute(KeyAttr).set_value(member);
        writer.write(ar);
    }

    void try_write(IWriter& writer) override {
        XmlArchive ar{ doc,node };
        writer.write(ar);
    }
};

namespace
{
    static auto gRegister = ArchiveRegister::Make<XmlArchive>(text_archive_format);
}
