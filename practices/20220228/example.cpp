#include <QtXml/QDomDocument>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QDebug>


void before() {
    QFile file("example.xml");
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        qDebug() << "Already opened or there is another issue";
        file.close();
    }
    QTextStream qts(&file);

    QDomDocument doc;

    QDomElement root = doc.createElement("StudentList");
    doc.appendChild(root);
    auto result = doc.toString().toStdWString();
    QDomElement student = doc.createElement("student");
    student.setAttribute("id", "1");
    student.setAttribute("name", "Burak");
    student.setAttribute("number", "1111");
    root.appendChild(student);

    student = doc.createElement("student");
    student.setAttribute("id", "2");
    student.setAttribute("name", "Hamdi");
    student.setAttribute("number", "2222");
    root.appendChild(student);

    student = doc.createElement("student");
    student.setAttribute("id", "3");
    student.setAttribute("name", "TUFAN");
    student.setAttribute("number", "33333");
    root.appendChild(student);

    student = doc.createElement("student");
    student.setAttribute("id", "4");
    student.setAttribute("name", "Thecodeprogram");
    student.setAttribute("number", "4444");
    root.appendChild(student);

    qts << doc.toString();
}

namespace xml_util
{
    template<typename K,typename V>
    auto Attr(K&& k, V&& v) {
        return std::make_pair(k, v);
    }

    template<typename T>
    struct Comment_t {
        T v;
    };

    template<typename T>
    auto Comment(T&& v) {
        return Comment_t<std::remove_cv_t<T>>{std::forward<T>(v)};
    }

    class Element
    {
        QDomElement  m_element;
    public:
        explicit Element(QDomDocument& doc, QString const& tag)
        {
            m_element = doc.createElement(tag);
            doc.appendChild(m_element);
        }

        explicit Element(QDomNode parent, QString const& tag)
        {
            m_element = parent.ownerDocument().createElement(tag);
            parent.appendChild(m_element);
        }

        /// @brief 创建子element
        /// @return 返回自身
        template<typename... Args>
        Element& AddChild(QString const& tag, Args&&... args) {
            auto result = Element(m_element, tag);
            result.SetAttributes(std::forward<Args>(args)...);
            return *this;
        }

        /// @brief 创建子element
        /// @return 返回子element
        template<typename... Args>
        Element CreateChild(QString const& tag, Args&&... args) {
            auto result = Element(m_element, tag);
            result.SetAttributes(std::forward<Args>(args)...);
            return result;
        }

        /// @brief 设置属性
        template<typename T, typename... Args>
        void SetAttributes(T&& attr, Args&&... args) {
            SetAttributeImpl(std::forward<T>(attr));
            if constexpr (sizeof...(Args) > 0) {
                SetAttributes(std::forward<Args>(args)...);
            }
        }
    private:
        template<typename T>
        void SetAttributeImpl(Comment_t<T>&& o) {
            m_element.appendChild(m_element.ownerDocument().createComment(o.v));
        }

        template<typename K, typename V>
        void SetAttributeImpl(std::pair<K, V>&& o) {
            m_element.setAttribute(o.first, o.second);
        }
    };
}

void after() {
    using namespace xml_util;

    QFile file("example_after.xml");
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        qDebug() << "Already opened or there is another issue";
        file.close();
    }
    QTextStream qts(&file);

    QDomDocument doc;
    auto root = Element(doc, "StudentList");
    root.SetAttributes(Comment("just comment"));
    auto result = doc.toString().toStdWString();
    root.AddChild("student",
        Attr("id", "1"),
        Attr("name", "Burak"),
        Attr("number", "1111"));

    root.CreateChild("student",
            Attr("id", "2"),
            Attr("name", "Hamdi"),
            Attr("number", "2222")
        )
        .AddChild("info",
            Attr("iV",10),
            Attr("bV",true),
            Attr("dV",1.414),
            Attr("sV","@#$@#!%")
            )
        ;

    root.AddChild("student",
        Attr("id", "3"),
        Attr("name", "TUFAN"),
        Attr("number", "33333"));

    root.AddChild("student",
        Attr("id", "4"),
        Attr("name", "Thecodeprogram"),
        Attr("number", "4444"));

    qts << doc.toString();
}

int main() {

    before();
    after();

    return 0;
}
