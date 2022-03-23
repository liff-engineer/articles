#include "Notifyer.h"
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
    Notifyer<Document> m_notifyer;
    std::vector<std::shared_ptr<Report>> m_reporters;
public:
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
        m_notifyer.notify(*this, e);
    }

    template<typename T,typename E>
    void emplaceObserver() {
        m_notifyer.emplaceObserver<T, E>();
    }
};

template<>
struct Observer<Document, Report, DataChanged> {
    void operator()(const Document& doc, DataChanged const& e) {
        doc.foreachReport([&](const Report* obj) { obj->on(e); });
    }
};


int main()
{
    Document doc{};
    doc.emplaceObserver<Report,DataChanged>();
    doc.createReport("abc");
    
    doc.notify(DataChanged{ 1024 });

    doc.createReport("hehe");
    doc.notify(DataChanged{ 256 });
    return 0;
}
