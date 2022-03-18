#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "nlohmann/json.hpp"

/// @brief 通过偏特化提供序列化实现:针对复杂的类型
template<typename T, typename E = void>
struct writer;

template<typename T,typename E = void>
struct reader;

class archive
{
    nlohmann::json j;
public:

//写入接口
    void write(const char* member, bool v) {
        j[member] = v;
    }
    void write(const char* member, int v) {
        j[member] = v;
    }
    void write(const char* member, double v) {
        j[member] = v;
    }
    void write(const char* member, const char* v) {
        j[member] = v;
    }
    void write(const char* member, const std::string& v) {
        j[member] = v;
    }
    void write(const char* member, archive&& ar) {
        j[member] = std::move(ar.j);
    }

    template<typename T>
    void write(const char* member, std::unique_ptr<T> const& v){
        if (v)  write(member, writer<T>::save(*v.get()));
    }

    template<typename T>
    void write(const char* member, const T& obj) {
        return write(member, writer<T>::save(obj));
    }

    void write(archive&& ar) {
        j.emplace_back(std::move(ar.j));
    }

    template<typename T>
    void write(const T& obj) {
        return write(writer<T>::save(obj));
    }

    void write(bool v) {
        j.push_back(v);
    }
    void write(int v) {
        j.push_back(v);
    }
    void write(double v) {
        j.push_back(v);
    }
    void write(const char* v) {
        j.emplace_back(v);
    }
    void write(const std::string& v) {
        j.emplace_back(v);
    }

//读取接口
    class reader
    {
        const nlohmann::json* j;
    public:
        explicit reader(const nlohmann::json* json)
            :j(json){}
        
        bool read_to(bool& v) const {
            if (j->is_boolean()) {
                j->get_to(v);
                return true;
            }
            return false;
        }
        bool read_to(int& v) const {
            if (j->is_number_integer()) {
                j->get_to(v);
                return true;
            }
            return false;
        }
        bool read_to(double& v) const {
            if (j->is_number()) {
                j->get_to(v);
                return true;
            }
            return false;
        }

        bool read_to(std::string& v) const {
            if (j->is_string()) {
                j->get_to(v);
                return true;
            }
            return false;
        }

        template<typename T>
        bool read_to(T& v) const {
            return ::reader<T>::read(*this, v);
        }

        template<typename T>
        bool read_to(const char* member, T& v) const {
            if (auto ar = get_reader(member)) {
                return ar.read_to(v);
            }
            return false;
        }

        bool verify(const char* member, const char* v) const {
            if (auto it = j->find(member); it != j->end()) {
                return v == it->get<std::string>();
            }
            return false;
        }

        explicit operator bool() const noexcept {
            return j != nullptr;
        }

        reader get_reader(const char* member) const {
            if (auto it = j->find(member); it != j->end()) {
                return reader(std::addressof(*it));
            }
            return reader{ nullptr };
        }

        std::size_t size() const noexcept {
            return j->size();
        }

        //设计接口时Op为:std::function<void(reader)>
        template<typename Op>
        void foreach(Op&& op) const {
            for (auto&& o : *j) {
                op(reader{ std::addressof(o) });
            }
        }

        //设计接口时Op为:std::function<void(const std::string&,reader)>
        template<typename Op>
        void foreach_members(Op&& op) const {
            for (auto& [key, val] : j->items()) {
                op(key, val);
            }
        }
    };

    //获取读取器
    reader get_reader(const char* member) {
        if (auto it = j.find(member); it != j.end()) {
            return reader(std::addressof(*it));
        }
        return reader{nullptr};
    }
public:
    std::string to_string() {
        return j.dump(4);
    }
};

/// @brief std::vector容器实现的示例
/// @tparam ...Ts 
template<typename... Ts>
struct writer<std::vector<Ts...>>
{
    static archive save(const std::vector<Ts...>& values) {
        archive ar;
        for (auto& o : values) {
            ar.write(o);
        }
        return ar;
    }
};

/// @brief std::map<std::string,T>的实现,通用实现还待商榷
/// @tparam ...Ts 
template<typename... Ts>
struct writer<std::map<std::string, Ts...>>
{
    static archive save(const std::map<std::string, Ts...>& values) {
        archive ar;
        for (auto& obj : values) {
            ar.write(obj.first.c_str(), obj.second);
        }
        return ar;
    }
};
