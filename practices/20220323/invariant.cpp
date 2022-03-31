#include "Notifyer.hpp"
#include <iostream>
#include <vector>

struct DataChanged {
    int iV;
};

struct Report {
    std::string key;
    explicit Report(std::string str)
        :key(std::move(str)) {};

    void on(DataChanged const& e) const {
        std::cout << key << " >>DataChanged:" << e.iV << "\n";
    }
};

class Document
{
    std::vector<std::shared_ptr<Report>> m_reporters;
public:
    abc::Notifyer<Document> notifyer;

    void createReport(const std::string& key) {
        m_reporters.emplace_back(std::make_shared<Report>(key));
    }

    template<typename Fn>
    void foreachReport(Fn& fn) const {
        for (auto&& v : m_reporters) {
            fn(v.get());
        }
    }

    template<typename E>
    void notify(const E& e) {
        notifyer.notify(*this, e);
    }

    template<typename E>
    void publish(const E& e) {
        notifyer.publish(e);
    }

    template<typename T, typename E>
    void registerObserver() {
        notifyer.registerObserver<T, E>();
    }
};

template<>
struct abc::Observer<Document, Report, DataChanged> {
    void update(Document& doc, DataChanged const& e) {
        doc.foreachReport([&](const Report* obj) { obj->on(e); });
        
        doc.publish(e);
    }
};


struct Viewer {

    void on(DataChanged const& e) {
        std::cout << "Viewer receive datachanged message\n";
    }
};

int main(int argc, char** argv) {

    Document doc{};
    doc.registerObserver<Report, DataChanged>();
    doc.createReport("abc");

    doc.notify(DataChanged{ 1024 });

    doc.createReport("hehe");
    doc.notify(DataChanged{ 256 });

    Viewer viewer{};
    auto stub =  doc.notifyer.subscribe<DataChanged>(viewer);

    doc.notify(DataChanged{ 128 });
    return 0;
}
