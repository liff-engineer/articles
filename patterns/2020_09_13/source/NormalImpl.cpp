#include <iostream>
#include <vector>
#include <memory>

class object_t
{
public:
    virtual ~object_t() = default;
    virtual void draw(std::ostream &, std::size_t) const = 0;
};

using document_t = std::vector<std::shared_ptr<object_t>>;

void draw(document_t const &doc, std::ostream &os, std::size_t position)
{
    os << std::string(position, ' ') << "<document>" << std::endl;
    for (const auto &e : doc)
    {
        e->draw(os, position + 2);
    }
    os << std::string(position, ' ') << "</document>" << std::endl;
}

class my_class_t : public object_t
{
public:
    my_class_t() = default;
    void draw(std::ostream &os, std::size_t position) const override
    {
        os << std::string(position, ' ') << "my_class_t" << std::endl;
    }
};

int main(int argc, char **argv)
{
    document_t document;
    document.emplace_back(std::make_shared<my_class_t>());
    draw(document, std::cout, 0);
    return 0;
}
