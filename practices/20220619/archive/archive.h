//archive定义,以支持内存结构/文档的序列化与反序列化
//开发者可以根据archive要求,提供自己的存档实现,
//且需支持如下基本类型:
//- 布尔Boolean
//- 整数Integer
//- 浮点数Number
//- 字符串String
//- 字节流Binary(不提供支持,使用者可以考虑使用base64等将二进制转换为字符串)
//- 空值Nil
//以及以下复合结构(通过API实现): 
//- 数组Array
//- 字典Dict(以字符串为key)

#pragma once
#include <type_traits>
#include <string>
#include <vector>
#include <memory>
#include <map>

template<typename T, typename E = void>
struct ArAdapter :std::false_type {
    template<typename Ar>
    static void write(Ar& ar, const T& v) {
        return ArWrite(&ar, v);
    }

    template<typename Ar>
    static bool read(const Ar& ar, T& v) {
        return ArRead(&ar, v);
    }
};

class IArReader {
public:
    virtual ~IArReader() = default;

    //查询接口
    virtual std::size_t size() const noexcept = 0;

    inline  bool read(const char* m, bool& v) const {
        return try_read_impl(m, Type::Boolean, &v);
    }
    inline  bool read(const char* m, int64_t& v) const {
        return try_read_impl(m, Type::Integer, &v);
    }
    inline  bool read(const char* m, uint64_t& v) const {
        return try_read_impl(m, Type::UInteger, &v);
    }
    inline  bool read(const char* m, double& v) const {
        return try_read_impl(m, Type::Number, &v);
    }
    inline  bool read(const char* m, std::string& v) const {
        return try_read_impl(m, Type::String, &v);
    }
    inline  bool read(const char* m) const {
        nullptr_t v{};
        return try_read_impl(m, Type::Nil, &v);
    }

    inline bool read() const { return read(nullptr); }

    template<typename T>
    bool  read(T& v) const { return read(nullptr, v); }

    template<typename Fn>
    std::enable_if_t<std::is_invocable_v<Fn, const IArReader&>, bool> read(Fn&& fn) const {
        return try_read_impl(nullptr, Type::Array, ArrayReader<Fn>{std::forward<Fn>(fn)});
    }

    template<typename Fn>
    std::enable_if_t<std::is_invocable_v<Fn, const char*, const IArReader&>, bool> read(Fn&& fn) const {
        return try_read_impl(nullptr, Type::Object, ObjectReader<Fn>{std::forward<Fn>(fn)});
    }

    template<typename T>
    bool  read(const char* m, T& v) const {
        if constexpr (ArAdapter<T>::value) {
            return ArAdapter<T>::read(*this, m, v);
        }
        else {
            return try_read_impl(m, Type::Internal, Reader<T>{v});
        }
    }

    template<typename T>
    bool  read_as(T& v) const {
        return ArAdapter<T>::read(*this, v);
    }
protected:
    enum class Type {
        Boolean,  //布尔量
        Integer,  //整数
        UInteger, //无符号整数
        Number,   //数值
        String,   //字符串
        Nil,      //空值
        Array,    //数组
        Object,   //对象
        Internal,
    };

    struct IReader {
        virtual bool read(const char* m, const IArReader& ar) = 0;
    };

    template<typename T>
    struct Reader final :public IReader
    {
        T* obj{};
        explicit Reader(T& o) :obj(std::addressof(o)) {};
        bool read(const char* m, const IArReader& ar) override {
            return ArAdapter<T>::read(ar, *obj);
        }
    };

    template<typename Fn>
    struct ArrayReader final :public IReader {
        Fn fn;
        explicit ArrayReader(Fn&& fn) :fn(std::move(fn)) {};
        bool read(const char* m, const IArReader& ar) override {
            fn(ar);
            return true;
        }
    };

    template<typename Fn>
    struct ObjectReader final :public IReader {
        Fn fn;
        explicit ObjectReader(Fn&& fn) :fn(std::move(fn)) {};
        bool read(const char* m, const IArReader& ar) override {
            fn(m, ar);
            return true;
        }
    };
    virtual bool try_read_impl(const char* m, Type type, void* v) const = 0;
    virtual bool try_read_impl(const char* m, Type type, IReader& v) const = 0;
};

class Archive;
class IArchive :public IArReader
{
public:
    static void    Unregister(const char* format);
    static bool    Register(const char* format, std::unique_ptr<IArchive>(*ctor)());

    template<typename T>
    static bool    Register(const char* format) {
        return Register(format, []() { return std::make_unique<T>(); });
    }
public:
    virtual ~IArchive() = default;

    virtual std::unique_ptr<IArchive> clone() const = 0;

    //存档格式
    virtual const char* format() const noexcept = 0;

    //文件IO
    virtual bool open(const std::string& file) = 0;
    virtual bool save(const std::string& file) const = 0;

    //调试支持(非必须,仅针对文本格式的存档)
    virtual std::string to_string() const = 0;

    inline void write(const char* m, bool v) {
        return try_write_impl(m, Type::Boolean, &v);
    }
    inline void write(const char* m, int64_t v) {
        return try_write_impl(m, Type::Integer, &v);
    }
    inline void write(const char* m, uint64_t v) {
        return try_write_impl(m, Type::UInteger, &v);
    }
    inline void write(const char* m, double v) {
        return try_write_impl(m, Type::Number, &v);
    }
    inline void write(const char* m, const char* v) {
        return try_write_impl(m, Type::String, v);
    }
    inline void write(const char* m, nullptr_t v) {
        return try_write_impl(m, Type::Nil, nullptr);
    }
    inline void write(const char* m) {
        return try_write_impl(m, Type::Nil, nullptr);
    }

    inline void write(const char* m, Archive& v);
    inline void write(Archive& v) {
        return write(nullptr, v);
    }

    inline void write(const std::string& v) {
        write(v.c_str());
    }
    inline void write(const char* m, const std::string& v) {
        write(m, v.c_str());
    }

    template<typename T>
    void  write(const char* m, const T& v) {
        if constexpr (ArAdapter<T>::value) {
            ArAdapter<T>::write(*this, m, v);
        }
        else
        {
            try_write(m, Writer<T>{v});
        }
    }

    template<typename T>
    void  write(const T& v) { write(nullptr, v); }

    template<typename T>
    void write(const char* m, const std::unique_ptr<T>& v) {
        write(m, v ? (*v.get()) : nullptr);
    }

    template<typename T>
    void write_as(const T& v) {
        ArAdapter<T>::write(*this, v);
    }

    bool write_tag(const char* tag) {
        if (std::strcmp(format(), "xml") == 0) {
            return write_tag_impl(tag);
        }
        return false;
    }
protected:
    struct IWriter {
        virtual void write(IArchive& ar) = 0;
    };

    template<typename T>
    struct Writer final :public IWriter {
        const T* obj;
        explicit Writer(const T& o) :obj(std::addressof(o)) {};
        void write(IArchive& ar) override {
            ArAdapter<T>::write(ar, *obj);
        }
    };

    virtual void try_write(const char* m, IWriter& writer) = 0;
    virtual bool try_write(const char* m, IArchive& v) = 0;
    virtual void try_write_impl(const char* m, Type type, const void* v) = 0;
    virtual bool write_tag_impl(const char* tag) { return false; };
};

class Archive final {
    friend class IArchive;
    std::unique_ptr<IArchive> m_impl;
public:
    Archive() = default;
    explicit Archive(const char* format);

    Archive(const Archive& other) {
        if (other) {
            m_impl = other->clone();
        }
    }

    Archive(Archive&& other) noexcept = default;

    Archive& operator=(const Archive& other) {
        if (this != std::addressof(other)) {
            if (other) {
                m_impl = other->clone();
            }
        }
        return *this;
    }

    Archive& operator=(Archive&& other) noexcept = default;

    explicit operator bool() const noexcept {
        return m_impl != nullptr;
    }

    IArchive* operator->() noexcept {
        return m_impl.get();
    }

    const IArchive* operator->() const noexcept {
        return m_impl.get();
    }
};

inline void IArchive::write(const char* m, Archive& v)
{
    if (!v.m_impl) return;
    if (try_write(m, *v.m_impl.get())) {
        v.m_impl.reset();
    }
}

template<typename T>
struct ArAdapter<T, std::enable_if_t<std::is_signed_v<T>>> :std::true_type
{
    static int64_t as(const T& v) { return static_cast<int64_t>(v); }

    static bool read(const IArReader& ar, const char* m, T& v) {
        int64_t rv{};
        if (ar.read(m, rv)) {
            v = static_cast<T>(rv);
            return true;
        }
        return false;
    }

    static void write(IArchive& ar, const char* m, const T& v) {
        ar.write(m, as(v));
    }
};

template<typename T>
struct ArAdapter<T, std::enable_if_t<std::is_unsigned_v<T>>> :std::true_type
{
    static uint64_t as(const T& v) { return static_cast<uint64_t>(v); }

    static bool read(const IArReader& ar, const char* m, T& v) {
        uint64_t rv{};
        if (ar.read(m, rv)) {
            v = static_cast<T>(rv);
            return true;
        }
        return false;
    }

    static void write(IArchive& ar, const char* m, const T& v) {
        ar.write(m, as(v));
    }
};

template<typename... Ts>
struct ArAdapter<std::vector<Ts...>> :std::false_type {

    static void write(IArchive& ar, const std::vector<Ts...>& v) {
        for (auto& o : v) {
            ar.write(o);
        }
    }

    static bool read(const IArReader& ar, std::vector<Ts...>& v) {
        v.reserve(v.size() + ar.size());
        ar.read([&](const IArReader& o) {
            v.resize(v.size() + 1);
            o.read(v.back());
            });
        return true;
    }
};

template<typename K, typename... Ts>
struct ArAdapter<std::map<K, Ts...>, std::enable_if_t<std::is_integral_v<K>>> :std::false_type {

    static void write(IArchive& ar, const std::map<K, Ts...>& items) {
        for (auto&& [k, v] : items) {
            ar.write(std::to_string(k).c_str(), v);
        }
    }

    static bool read(const IArReader& ar, std::map<K, Ts...>& items) {
        ar.read([&](const char* key, const IArReader& o) {
            if constexpr (std::is_unsigned_v<K>) {
                K k = (K)std::stoull(key);
                o.read(items[k]);
            }
            else
            {
                K k = (K)std::stoll(key);
                o.read(items[k]);
            }
            });
        return true;
    }
};

template<typename... Ts>
struct ArAdapter<std::map<std::string, Ts...>> :std::false_type {

    static void write(IArchive& ar, const std::map<std::string, Ts...>& items) {
        for (auto&& [k, v] : items) {
            ar.write(k.c_str(), v);
        }
    }

    static bool read(const IArReader& ar, std::map<std::string, Ts...>& items) {
        ar.read([&](const char* key, const IArReader& o) {
            o.read(items[key]);
            });
        return true;
    }
};
