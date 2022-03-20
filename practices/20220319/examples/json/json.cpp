#include "archive.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iomanip>
#include <iterator>

namespace
{
    static const auto text_archive_format = "json";
}

class JsonArReader :public IArReader
{
public:
    const  nlohmann::json* j;
public:
    explicit JsonArReader(const nlohmann::json* json)
        :j(json) {};

    //查询接口
    std::size_t size() const noexcept override {
        return j->size();
    }

    void visit(IVisitor& op) const override {
        for (auto&& o : *j) {
            JsonArReader reader{ std::addressof(o)};
            op.accept(reader);
        }
    }
    
    void visit(IKVVisitor& op) const override {
        for (auto& [key, val] : j->items()) {
            JsonArReader reader{ std::addressof(val) };
            op.accept(key.c_str(),reader);
        }
    }

    //数组读取,基本类型
    bool read(bool& v) const override {
        if (j->is_boolean()) {
            j->get_to(v);
            return true;
        }
        return false;
    }

    bool read(int64_t& v) const override {
        if (j->is_number_integer()) {
            j->get_to(v);
            return true;
        }
        return false;
    }

    bool read(uint64_t& v) const override {
        if (j->is_number_unsigned()) {
            j->get_to(v);
            return true;
        }
        return false;
    }

    bool read(double& v) const override {
        if (j->is_number()) {
            j->get_to(v);
            return true;
        }
        return false;
    }

    bool read(std::string& v) const override {
        if (j->is_string()) {
            j->get_to(v);
            return true;
        }
        return false;
    }

    bool read(std::vector<char>& v) const override {
        //TODO FIXME
        return false;
    }

    bool read() const override {
        return j->is_null();
    }


    //字典读取,基本类型
    bool read(const char* member, bool& v) const override {
        if (auto it = j->find(member); it != j->end()) {
            if (it->is_boolean()) {
                it->get_to(v);
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, int64_t& v) const override {
        if (auto it = j->find(member); it != j->end()) {
            if (it->is_number_integer()) {
                it->get_to(v);
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, uint64_t& v) const override {
        if (auto it = j->find(member); it != j->end()) {
            if (it->is_number_unsigned()) {
                it->get_to(v);
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, double& v) const override {
        if (auto it = j->find(member); it != j->end()) {
            if (it->is_number()) {
                it->get_to(v);
                return true;
            }
        }
        return false;
    }

    bool read(const char* member, std::string& v) const override {
        if (auto it = j->find(member); it != j->end()) {
            if (it->is_string()) {
                it->get_to(v);
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
        if (auto it = j->find(member); it != j->end()) {
            return it->is_null();
        }
        return false;
    }
protected:
    bool       try_read(const char* member, IReader& reader) const override {
        if (auto it = j->find(member); it != j->end()) {
            JsonArReader ar{ std::addressof(*it) };
            return reader.read(ar);
        }
        return false;
    }
    bool       try_read(IReader& reader) const override {
        return reader.read(*this);
    }
};

class JsonArchive :public IArchive
{
protected:
    nlohmann::json j;
public:
    std::unique_ptr<IArchive> clone() const override {
        JsonArchive result{};
        result.j = j;
        return std::make_unique<JsonArchive>(std::move(result));
    }

    const char* format() const noexcept override {
        return text_archive_format;
    }

    bool open(const std::string& file) override {
        std::ifstream ifs(file);
        ifs >> j;
        return true;
    }

    bool save(const std::string& file) const override {
        std::ofstream ofs(file);
        ofs << std::setw(4) << j << std::endl;
        return true;
    }

    std::string to_string() const override {
        return j.dump(4);
    }

    std::size_t size() const noexcept  override {
        return j.size();
    }

    void visit(IVisitor& op) const override {
        JsonArReader{ &j }.visit(op);
    }
    void visit(IKVVisitor& op) const override {
        JsonArReader{ &j }.visit(op);
    }

    void write(bool v) override {
        j.push_back(v);
    }
    void write(int64_t v) override {
        j.push_back(v);
    }

    void write(uint64_t v) override {
        j.push_back(v);
    }
    void write(double v) override {
        j.push_back(v);
    }

    void write(const char* v) override {
        j.push_back(v);
    }

    void write(const std::vector<char>& v)  override {
        //TODO FIXME
    }

    void write(nullptr_t v) override {
        j.push_back(nullptr);
    }

    //数组读取,基本类型
    bool read(bool& v) const override {
        return JsonArReader{ &j }.read(v);
    }

    bool read(int64_t& v) const override {
        return JsonArReader{ &j }.read(v);
    }

    bool read(uint64_t& v) const override {
        return JsonArReader{ &j }.read(v);
    }

    bool read(double& v) const override {
        return JsonArReader{ &j }.read(v);
    }

    bool read(std::string& v) const override {
        return JsonArReader{ &j }.read(v);
    }

    bool read(std::vector<char>& v) const override {
        //TODO FIXME
        return false;
    }

    bool read() const override  {
        return j.is_null();
    }

    void write(const char* member, bool v)  override {
        j[member] = v;
    }

    void write(const char* member, int64_t v) override {
        j[member] = v;
    }

    void write(const char* member, uint64_t v) override {
        j[member] = v;
    }

    void write(const char* member, double v) override {
        j[member] = v;
    }

    void write(const char* member, const char* v) override {
        j[member] = v;
    }

    void write(const char* member, const std::vector<char>& v) override {
        //TODO FIXME
    }

    void write(const char* member, nullptr_t v) override {
        j[member] = nullptr;
    }

    bool read(const char* member, bool& v) const override {
        return JsonArReader{ &j }.read(member,v);
    }

    bool read(const char* member, int64_t& v) const override {
        return JsonArReader{ &j }.read(member, v);
    }

    bool read(const char* member, uint64_t& v) const override {
        return JsonArReader{ &j }.read(member, v);
    }

    bool read(const char* member, double& v) const override {
        return JsonArReader{ &j }.read(member, v);
    }

    bool read(const char* member, std::string& v) const override {
        return JsonArReader{ &j }.read(member, v);
    }
    
    bool read(const char* member, std::vector<char>& v) const override {
        //TODO FIXME
        return false;
    }
    
    bool read(const char* member) const override {
        return JsonArReader{ &j }.read(member);
    }
protected:
    bool       try_read(const char* member, IReader& reader) const override {
        if (auto it = j.find(member); it != j.end()) {
            JsonArReader ar{ std::addressof(*it) };
            return reader.read(ar);
        }
        return false;
    }
    bool       try_read(IReader& reader) const override {
        return reader.read(*this);
    }

    void try_write(const char* member, IWriter& writer) override {
        JsonArchive ar{};
        writer.write(ar);
        j[member] = std::move(ar.j);
    }

    void try_write(IWriter& writer) override {
        JsonArchive ar{};
        writer.write(ar);
        j.emplace_back(std::move(ar.j));
    }
};

enum class BinaryJsonType
{
    BSON,
    CBOR,
    MessagePack,
    UBJSON
};

template<BinaryJsonType T>
struct JsonBinaryFormat;

template<>
struct JsonBinaryFormat<BinaryJsonType::BSON> {
    static constexpr auto archive_format = "BSON";

    static auto  read(const std::vector<uint8_t>& buffer) {
        return nlohmann::json::from_bson(buffer);
    }

    static auto  write(const nlohmann::json& j) {
        return nlohmann::json::to_bson(j);
    }
};

template<>
struct JsonBinaryFormat<BinaryJsonType::CBOR> {
    static constexpr auto archive_format = "CBOR";

    static auto  read(const std::vector<uint8_t>& buffer) {
        return nlohmann::json::from_cbor(buffer);
    }
    static auto  write(const nlohmann::json& j) {
        return nlohmann::json::to_cbor(j);
    }
};

template<>
struct JsonBinaryFormat<BinaryJsonType::MessagePack> {
    static constexpr auto archive_format = "MessagePack";

    static auto  read(const std::vector<uint8_t>& buffer) {
        return nlohmann::json::from_msgpack(buffer);
    }
    static auto  write(const nlohmann::json& j) {
        return nlohmann::json::to_msgpack(j);
    }
};

template<>
struct JsonBinaryFormat<BinaryJsonType::UBJSON> {
    static constexpr auto archive_format = "UBJSON";

    static auto  read(const std::vector<uint8_t>& buffer) {
        return nlohmann::json::from_ubjson(buffer);
    }
    static auto  write(const nlohmann::json& j) {
        return nlohmann::json::to_ubjson(j);
    }
};

template<BinaryJsonType T>
class BinaryJsonArchive :public  JsonArchive {
public:
    std::unique_ptr<IArchive> clone() const override {
        BinaryJsonArchive<T> result{};
        result.j = j;
        return std::make_unique<BinaryJsonArchive<T>>(std::move(result));
    }

    const char* format() const noexcept override {
        return JsonBinaryFormat<T>::archive_format;
    }

    bool open(const std::string& file) override {
        std::ifstream ifs(file, std::ios::binary);
        ifs.seekg(0, ifs.end);
        auto len = ifs.tellg();
        ifs.seekg(0, ifs.beg);

        std::vector<std::uint8_t> buffer(len);
        ifs.read((char*)(buffer.data()), len);
        j = JsonBinaryFormat<T>::read(buffer);
        return true;
    }

    bool save(const std::string& file) const override {
        std::ofstream ofs(file,std::ios::binary);
        auto buffer = JsonBinaryFormat<T>::write(j);
        ofs.write((const char*)(buffer.data()), buffer.size());
        return true;
    }
};

namespace
{
    static auto gRegister = ArchiveRegister::Make<JsonArchive>(text_archive_format);

    static auto gBSONRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::BSON>>(JsonBinaryFormat<BinaryJsonType::BSON>::archive_format);
    static auto gCBORRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::CBOR>>(JsonBinaryFormat<BinaryJsonType::CBOR>::archive_format);
    static auto gMessagePackRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::MessagePack>>(JsonBinaryFormat<BinaryJsonType::MessagePack>::archive_format);
    static auto gUBJSONRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::UBJSON>>(JsonBinaryFormat<BinaryJsonType::UBJSON>::archive_format);
}
