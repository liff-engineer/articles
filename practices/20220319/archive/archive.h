//archive定义,以支持内存结构/文档的序列化与反序列化
//开发者可以根据archive要求,提供自己的存档实现,
//且需支持如下基本类型:
//- 布尔Boolean
//- 整数Integer
//- 浮点数Number
//- 字符串String
//- 字节流Binary(如果对应格式不支持,可以考虑使用base64等将二进制转换为字符串)
//- 空值Nil
//以及以下复合结构(通过API实现): 
//- 数组Array
//- 字典Map(以字符串为key)

#pragma once
#include <type_traits>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "meta.h"

/// @brief 复杂类型T的存档实现
template<typename T, typename E = void>
struct ArAdapter;

/// @brief 基本类型T的存档实现(直接映射为存档支持的基本类型)
template<typename T, typename E = void>
struct ArBasicAdapter :std::false_type {};

class IArReader {
public:
    //避免使用std::function
    struct IVisitor {
        virtual void accept(IArReader&) = 0;
    };

    template<typename Fn>
    struct Visitor final :public IVisitor {
        Fn fn;
        explicit Visitor(Fn&& fn) :fn(std::move(fn)) {};
        void accept(IArReader& ar) override { fn(ar); }
    };

    struct IKVVisitor {
        virtual void accept(const char*, IArReader&) = 0;
    };

    template<typename Fn>
    struct KVVisitor final :public IKVVisitor {
        Fn fn;
        explicit KVVisitor(Fn&& fn) :fn(std::move(fn)) {};
        void accept(const char* key, IArReader& ar) override { fn(key, ar); }
    };

    struct IReader {
        virtual bool read(const IArReader&) = 0;
    };

    /// @brief 使用该实现来读取复杂类型对象信息
    template<typename T>
    struct Reader final :public IReader
    {
        T* obj{};
        explicit Reader(T& o) :obj(std::addressof(o)) {};
        bool read(const IArReader& ar) override {
            return ArAdapter<T>::read(ar, *obj);
        }
    };
public:
    virtual ~IArReader() = default;

    //查询接口
    virtual std::size_t size() const noexcept = 0;

    virtual void visit(IVisitor& op) const = 0;
    virtual void visit(IKVVisitor& op) const = 0;

    template<typename Fn>
    std::enable_if_t<std::is_invocable_v<Fn, IArReader&>, void> visit(Fn&& fn) const {
        Visitor<Fn> op{ std::forward<Fn>(fn) };
        visit(op);
    }

    template<typename Fn>
    std::enable_if_t<std::is_invocable_v<Fn, const char*, IArReader&>, void> visit(Fn&& fn) const {
        KVVisitor<Fn> op{ std::forward<Fn>(fn) };
        visit(op);
    }

    //数组读取,基本类型
    virtual bool read(bool& v) const = 0;
    virtual bool read(int64_t& v) const = 0;
    virtual bool read(uint64_t& v) const = 0;
    virtual bool read(double& v) const = 0;
    virtual bool read(std::string& v) const = 0;
    virtual bool read(std::vector<char>& v) const = 0;
    virtual bool read() const = 0;

    //字典读取,基本类型
    virtual bool read(const char* member, bool& v) const = 0;
    virtual bool read(const char* member, int64_t& v) const = 0;
    virtual bool read(const char* member, uint64_t& v) const = 0;
    virtual bool read(const char* member, double& v) const = 0;
    virtual bool read(const char* member, std::string& v) const = 0;
    virtual bool read(const char* member, std::vector<char>& v) const = 0;
    virtual bool read(const char* member) const = 0;

    inline bool verify(const char* member, const char* v) const {
        std::string result{};
        if (read(member, result)) {
            return result == v;
        }
        return false;
    }

    template<typename T>
    bool  read(const char* member, T& v) const {
        if constexpr (ArBasicAdapter<T>::value) {
            return ArBasicAdapter<T>::read(*this, member, v);
        }
        else {
            return try_read(member, Reader<T>{ v });
        }
    }

    template<typename T>
    bool  read(T& v) const {
        if constexpr (ArBasicAdapter<T>::value) {
            return ArBasicAdapter<T>::read(*this, v);
        }
        else {
            return try_read(Reader<T>{ v });
        }
    }
protected:
    virtual bool       try_read(const char* member, IReader& reader) const = 0;
    virtual bool       try_read(IReader& reader) const = 0;
};

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

    //数组写入,基本类型
    virtual void write(bool v) = 0;
    virtual void write(int64_t v) = 0;
    virtual void write(uint64_t v) = 0;
    virtual void write(double v) = 0;
    virtual void write(const char* v) = 0;
    virtual void write(const std::vector<char>& v) = 0;
    virtual void write(nullptr_t v) = 0;


    //字典写入,基本类型
    virtual void write(const char* member, bool v) = 0;
    virtual void write(const char* member, int64_t v) = 0;
    virtual void write(const char* member, uint64_t v) = 0;
    virtual void write(const char* member, double v) = 0;
    virtual void write(const char* member, const char* v) = 0;
    virtual void write(const char* member, const std::vector<char>& v) = 0;
    virtual void write(const char* member, nullptr_t v) = 0;

    //扩展基本类型
    inline void write(const std::string& v) {
        write(v.c_str());
    }
    inline void write(const char* member, const std::string& v) {
        write(member, v.c_str());
    }

    //复合类型处理
    template<typename T>
    void  write(const char* member, const T& v) {
        // 如果类型T可以按照内建类型操作,则无需申请writer
        if constexpr (ArBasicAdapter<T>::value) {
            ArBasicAdapter<T>::write(*this, member, v);
        }
        else
        {
            try_write(member, Writer<T>{v});
        }
    }

    template<typename T>
    void  write(const T& v) {
        // 如果类型T可以按照内建类型操作,则无需申请writer
        if constexpr (ArBasicAdapter<T>::value) {
            ArBasicAdapter<T>::write(*this, v);
        }
        else
        {
            try_write(Writer<T>{v});
        }
    }

    //STL中提供的复合类型,当然也可以通过类型偏特化机制处理
    template<typename T>
    void write(const char* member, const std::unique_ptr<T>& v) {
        write(member, v ? (*v.get()) : nullptr);
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

    virtual void try_write(const char* member, IWriter& writer) = 0;
    virtual void try_write(IWriter& writer) = 0;
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

/// @brief IArchive的值语义封装
class Archive
{
    std::unique_ptr<IArchive> m_impl;
public:
    explicit Archive(std::unique_ptr<IArchive> ar)
        :m_impl(std::move(ar)) {};

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

    IArchive* get() noexcept {
        return m_impl.get();
    }

    const IArchive* get() const noexcept {
        return m_impl.get();
    }

    template<typename T>
    void write(const char* member, T&& v) {
        m_impl->write(member, std::forward<T>(v));
    }

    template<typename T>
    void write(T&& v) {
        m_impl->write(std::forward<T>(v));
    }

    template<typename T>
    bool read(const char* member, T& v) {
        return m_impl->read(member, v);
    }

    template<typename T>
    bool read(T& v) {
        return m_impl->read(v);
    }
};

template<typename T>
struct ArBasicAdapter<T, std::enable_if_t<std::is_signed_v<T>>> :std::true_type
{
    static int64_t as(const T& v) { return static_cast<int64_t>(v); }

    static bool read(const IArReader& ar, const char* member, T& v) {
        int64_t rv{};
        if (ar.read(member, rv)) {
            v = static_cast<T>(rv);
            return true;
        }
        return false;
    }

    static bool read(const IArReader& ar, T& v) {
        int64_t rv{};
        if (ar.read(rv)) {
            v = static_cast<T>(rv);
            return true;
        }
        return false;
    }

    static void write(IArchive& ar, const char* member, const T& v) {
        ar.write(member, as(v));
    }

    static void write(IArchive& ar, const T& v) {
        ar.write(as(v));
    }
};

template<typename T>
struct ArBasicAdapter<T, std::enable_if_t<std::is_unsigned_v<T>>> :std::true_type
{
    static uint64_t as(const T& v) { return static_cast<uint64_t>(v); }

    static bool read(const IArReader& ar, const char* member, T& v) {
        uint64_t rv{};
        if (ar.read(member, rv)) {
            v = static_cast<T>(rv);
            return true;
        }
        return false;
    }

    static bool read(const IArReader& ar, T& v) {
        uint64_t rv{};
        if (ar.read(rv)) {
            v = static_cast<T>(rv);
            return true;
        }
        return false;
    }

    static void write(IArchive& ar, const char* member, const T& v) {
        ar.write(member, as(v));
    }

    static void write(IArchive& ar, const T& v) {
        ar.write(as(v));
    }
};

template<typename T>
struct ArAdapter<T, std::enable_if_t<Meta<T>::value>> {

    template<typename U, typename R>
    static void write(IArchive& ar, const T& obj, Member<U, R> m) {
        //出现这种情况,说明开发者是抄写的代码,使用&Class::member时,忘记修改Class了
        static_assert(std::is_same_v<T, U>, "member declare invalid,check your code.");
        ar.write(m.name, (obj).*(m.mp));
    }

    template<typename U, typename R>
    static void read(const IArReader& ar, T& obj, Member<U, R> m) {
        //出现这种情况,说明开发者是抄写的代码,使用&Class::member时,忘记修改Class了
        static_assert(std::is_same_v<T, U>, "member declare invalid,check your code.");
        ar.read(m.name, (obj).*(m.mp));
    }

    template<typename U>
    static void write(IArchive& ar, const T& obj, U m) {
        //正常情况下都走上面的member<U,R>分支,一旦出现这个,说明
        //类型T的meta生成函数(默认为make_meta)返回了错误的内容
        static_assert(false, "meta info invalid,check your code.");
    }

    template<typename U>
    static void read(const IArReader& ar, T& obj, U m) {
        //正常情况下都走上面的member<U,R>分支,一旦出现这个,说明
        //类型T的meta生成函数(默认为make_meta)返回了错误的内容
        static_assert(false, "meta info invalid,check your code.");
    }

    static void write(IArchive& ar, const T& v) {
        static auto meta = Meta<T>::Make();
        ar.write("__class_id__", meta.first);
        std::apply([&](auto&& ... args)
            {
                (write(ar, v, args), ...);
            }, meta.second
        );
    }

    static bool read(const IArReader& ar, T& v) {
        static auto meta = Meta<T>::Make();
        if (ar.verify("__class_id__", meta.first)) {
            std::apply([&](auto&& ... args)
                {
                    (read(ar, v, args), ...);
                }, meta.second
            );
            return true;
        }
        return false;
    }
};


template<typename... Ts>
struct ArAdapter<std::vector<Ts...>> {

    static void write(IArchive& ar, const std::vector<Ts...>& v) {
        for (auto& o : v) {
            ar.write(o);
        }
    }

    static bool read(const IArReader& ar, std::vector<Ts...>& v) {
        v.reserve(v.size() + ar.size());
        ar.visit([&](IArReader& o) {
            v.resize(v.size() + 1);
            o.read(v.back());
            });
        return true;
    }
};

template<typename K, typename... Ts>
struct ArAdapter<std::map<K, Ts...>, std::enable_if_t<std::is_integral_v<K>>> {

    static void write(IArchive& ar, const std::map<K, Ts...>& items) {
        for (auto&& [k, v] : items) {
            ar.write(std::to_string(k).c_str(), v);
        }
    }

    static bool read(const IArReader& ar, std::map<K, Ts...>& items) {
        ar.visit([&](const char* key, const IArReader& o) {
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
struct ArAdapter<std::map<std::string, Ts...>> {

    static void write(IArchive& ar, const std::map<std::string, Ts...>& items) {
        for (auto&& [k, v] : items) {
            ar.write(k.c_str(), v);
        }
    }

    static bool read(const IArReader& ar, std::map<std::string, Ts...>& items) {
        ar.visit([&](const char* key, const IArReader& o) {
            o.read(items[key]);
            });
        return true;
    }
};
