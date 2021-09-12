#pragma once

#include <functional>
#include <vector>
#include <string>
#include "nlohmann/json.hpp"
#include "Broker.hpp"

class JsonWriter :public abc::IWriter
{
public:
    struct Writer {
        abc::TypeCode code;
        std::function<bool(nlohmann::json&, abc::Payload const&)> writer;
    };
public:
    JsonWriter() = default;

    template<typename Arg, typename R>
    static bool DefaultWriter(nlohmann::json& j, Arg const& arg, R const& r) {
        j = {
            {"code",abc::TypeCodeOf<Arg,R>().literal},
            {"argument",arg},
            {"result",r}
        };
        return true;
    }

    template<typename Arg>
    static bool DefaultWriter(nlohmann::json& j, Arg const& arg) {
        j = {
            {"code",abc::TypeCodeOf<Arg>().literal},
            {"argument",arg}
        };
        return true;
    }

    template<typename Arg, typename R>
    void registerWriter() {
        m_writers.emplace_back(Writer{ abc::TypeCodeOf<Arg,R>(),
            [=](nlohmann::json& j,const abc::Payload& o)->bool {
                return DefaultWriter<Arg,R>(j,*static_cast<const Arg*>(o.arg),*static_cast<const R*>(o.result));
            } });
    }

    template<typename Arg>
    void registerWriter() {
        m_writers.emplace_back(Writer{ abc::TypeCodeOf<Arg>(),
            [=](nlohmann::json& j,const abc::Payload& o)->bool {
                return DefaultWriter<Arg>(j,*static_cast<const Arg*>(o.arg));
            } });
    }

    void push(const abc::Payload& o,abc::IWriter::When when) override;

    auto dump()const {
        return json.dump(4);
    }

    auto result() const {
        return json;
    }
private:
    std::vector<Writer> m_writers;
    nlohmann::json  json;
};

class JsonReader
{
public:
    struct Reader {
        std::string code;
        std::function<std::unique_ptr<abc::IReplayAction>(nlohmann::json const&)> reader;
    };
public:
    JsonReader() = default;

    template<typename Arg, typename R>
    void registerReader() {
        m_readers.emplace_back(Reader{ abc::TypeCodeOf<Arg,R>().literal,
            [](nlohmann::json const& j)->std::unique_ptr<abc::IReplayAction> {
                auto result = std::make_unique<abc::ReplayAction<Arg, R>>();
                j.at("argument").get_to(result->arg);
                j.at("result").get_to(result->expect);
                return result;
            } });
    }

    template<typename Arg>
    void registerReader() {
        m_readers.emplace_back(Reader{ abc::TypeCodeOf<Arg>().literal,
            [](nlohmann::json const& j)->std::unique_ptr<abc::IReplayAction> {
                auto result = std::make_unique<abc::ReplayAction<Arg,void>>();
                j.at("argument").get_to(result->arg);
                return result;
            } });
    }

    std::vector<std::unique_ptr<abc::IReplayAction>> read(nlohmann::json  const& json);

private:
    std::vector<Reader> m_readers;
};

struct SaveAndLoadEventHandler
{
    void on(nlohmann::json const& j, std::string const& filepath);
    nlohmann::json on(std::string const& filepath);
};
