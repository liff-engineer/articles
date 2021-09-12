#include <fstream>
#include <iomanip>
#include "JsonIO.hpp"

void JsonWriter::push(const abc::Payload& o, abc::IWriter::When when)
{
    //暂时只记录离开的时候
    if (when != abc::IWriter::When::leave)
        return;
    nlohmann::json j;
    for (auto& h : m_writers) {
        if (h.code != o.code || !h.writer)
            continue;
        if (h.writer(j, o)) {
            json.push_back(j);
        }
    }
}

std::vector<std::unique_ptr<abc::IReplayAction>> JsonReader::read(nlohmann::json const& json)
{
    std::vector<std::unique_ptr<abc::IReplayAction>> results;
    for (auto& j : json) {
        std::string code = j.at("code");

        for (auto& o : m_readers) {
            if (code != o.code || !o.reader)
                continue;
            auto r = o.reader(j);
            if (r) {
                results.emplace_back(std::move(r));
            }
        }
    }
    return results;
}

void SaveAndLoadEventHandler::on(nlohmann::json const& j, std::string const& filepath)
{
    std::ofstream ofs(filepath);
    ofs << std::setw(4) << j << std::endl;
}

nlohmann::json SaveAndLoadEventHandler::on(std::string const& filepath)
{
    nlohmann::json j;
    std::ifstream ifs(filepath);
    ifs >> j;
    return j;
}
