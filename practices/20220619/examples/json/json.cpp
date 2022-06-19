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

    struct Impl {
        const  nlohmann::json* j;
        bool try_read(const char* m, Type type, void* v) const;
        bool try_read(const char* m, Type type, IReader& v) const;
    };
public:
    explicit JsonArReader(const nlohmann::json* json)
        :j(json) {};

    std::size_t size() const noexcept override {
        return j->size();
    }
protected:
    bool try_read_impl(const char* m, Type type, void* v) const override {
        return Impl{ j }.try_read(m, type, v);
    }

    bool try_read_impl(const char* m, Type type, IReader& v) const override {
        return Impl{ j }.try_read(m, type, v);
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
protected:
    void try_write(const char* m, IWriter& writer) override {
        JsonArchive ar{};
        writer.write(ar);
        if (m) {
            j[m] = std::move(ar.j);
        }
        else {
            j.emplace_back(std::move(ar.j));
        }
    }
    bool try_write(const char* m, IArchive& v) override {
        if (auto vp = dynamic_cast<JsonArchive*>(&v)) {
            if (m) {
                j[m] = std::move(vp->j);
            }
            else
            {
                j.emplace_back(std::move(vp->j));
            }
            return true;
        }
        return false;
    }
    void try_write_impl(const char* m, Type type, const void* v) override {
        auto write = [&](auto& obj) {
            if (m) {
                j[m] = obj;
            }
            else {
                j.push_back(obj);
            }
        };

        switch (type) {
        case Type::Boolean:
            write(*static_cast<const bool*>(v));
            break;
        case Type::Integer:
            write(*static_cast<const int64_t*>(v));
            break;
        case Type::UInteger:
            write(*static_cast<const uint64_t*>(v));
            break;
        case Type::Number:
            write(*static_cast<const double*>(v));
            break;
        case Type::String:
        {
            auto vp = static_cast<const char*>(v);
            if (m) {
                j[m] = vp;
            }
            else {
                j.push_back(vp);
            }
        }
        break;
        case Type::Nil:
            write(*static_cast<const nullptr_t*>(v));
            break;
        default:
            break;
        }
    }
    bool try_read_impl(const char* m, Type type, void* v) const override {
        return JsonArReader::Impl{ &j }.try_read(m, type, v);
    }

    bool try_read_impl(const char* m, Type type, IReader& v) const override {
        return JsonArReader::Impl{ &j }.try_read(m, type, v);
    }
};

bool JsonArReader::Impl::try_read(const char* m, Type type, void* v) const
{
    auto obj = j;
    if (m) {
        auto it = j->find(m);
        if (it == j->end()) return false;
        obj = std::addressof(*it);
    }
    auto str = j->dump();
    auto str1 = obj->dump();
    switch (type) {
    case Type::Boolean:
        if (obj->is_boolean()) {
            obj->get_to(*static_cast<bool*>(v));
            return true;
        }
        break;
    case Type::Integer:
        if (obj->is_number_integer()) {
            obj->get_to(*static_cast<int64_t*>(v));
            return true;
        }
        break;
    case Type::UInteger:
        if (obj->is_number_unsigned()) {
            obj->get_to(*static_cast<uint64_t*>(v));
            return true;
        }
        break;
    case Type::Number:
        if (obj->is_number()) {
            obj->get_to(*static_cast<double*>(v));
            return true;
        }
        break;
    case Type::String:
        if (obj->is_string()) {
            obj->get_to(*static_cast<std::string*>(v));
            return true;
        }
        break;
    case Type::Nil:
        return obj->is_null();
    default:
        break;
    }
    return false;
}

bool JsonArReader::Impl::try_read(const char* m, Type type, IReader& v) const
{
    switch (type) {
    case Type::Array:
        if (j->is_array()) {
            bool result = true;
            for (auto&& o : *j) {
                v.read(nullptr, JsonArReader{ std::addressof(o) });
            }
            return result;
        }
        break;
    case Type::Object:
        if (j->is_object()) {
            bool result = true;
            for (auto& [key, val] : j->items()) {
                v.read(key.c_str(), JsonArReader{ std::addressof(val) });
            }
            return result;
        }
        break;
    case Type::Internal:
        if (!m) {
            return v.read(nullptr, JsonArReader{ j });
        }
        else if (auto it = j->find(m); it != j->end()) {
            return v.read(nullptr, JsonArReader{ std::addressof(*it) });
        }
        break;
    default:
        break;
    }
    return false;
}


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


/// @brief 存档实现注册用
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

namespace
{
    static auto gRegister = ArchiveRegister::Make<JsonArchive>(text_archive_format);

    static auto gBSONRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::BSON>>(JsonBinaryFormat<BinaryJsonType::BSON>::archive_format);
    static auto gCBORRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::CBOR>>(JsonBinaryFormat<BinaryJsonType::CBOR>::archive_format);
    static auto gMessagePackRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::MessagePack>>(JsonBinaryFormat<BinaryJsonType::MessagePack>::archive_format);
    static auto gUBJSONRegister = ArchiveRegister::Make<BinaryJsonArchive<BinaryJsonType::UBJSON>>(JsonBinaryFormat<BinaryJsonType::UBJSON>::archive_format);
}
