#include <cassert>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

template <typename T>
void draw(const T &obj, std::ostream &os, std::size_t position)
{
    os << std::string(position, ' ') << obj << std::endl;
}

class object_t
{
public:
    template <typename T>
    object_t(T x) : self_(std::make_shared<model<T>>(std::move(x)))
    {
    }

    friend void draw(const object_t &obj, std::ostream &os, std::size_t position)
    {
        obj.self_->drawImpl(os, position);
    }

private:
    struct concept_t
    {
        virtual ~concept_t() = default;
        virtual void drawImpl(std::ostream &, std::size_t) const = 0;
    };

    template <typename T>
    struct model : concept_t
    {
        T data_;

        model(T x) : data_{std::move(x)} {};

        void drawImpl(std::ostream &os, std::size_t position) const override
        {
            draw(data_, os, position);
        }
    };

    std::shared_ptr<const concept_t> self_;
};

using document_t = std::vector<object_t>;
void draw(document_t const &doc, std::ostream &os, std::size_t position)
{
    os << std::string(position, ' ') << "<document>" << std::endl;
    for (const auto &e : doc)
    {
        draw(e, os, position + 2);
    }
    os << std::string(position, ' ') << "</document>" << std::endl;
}

class my_class_t
{
public:
    my_class_t() = default;
};

void draw(const my_class_t &, std::ostream &os, std::size_t position)
{
    os << std::string(position, ' ') << "my_class_t" << std::endl;
}

int main(int argc, char **argv)
{
    document_t document;
    document.emplace_back(std::string("hello"));
    document.emplace_back(my_class_t());
    draw(document, std::cout, 0);
    return 0;
}
