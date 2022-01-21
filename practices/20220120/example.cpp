#include "iterator_facade.h"
#include <vector>


class MyObject
{
    std::vector<double> m_values;
public:
    explicit MyObject(std::initializer_list<double> list)
        :m_values(list)
    {}

    inline std::size_t size() const noexcept {
        return m_values.size();
    }

    double* at(std::size_t i) noexcept {
        if (i >= m_values.size()) return nullptr;
        return std::addressof(m_values[i]);
    }

    const double* at(std::size_t i) const noexcept {
        if (i >= m_values.size()) return nullptr;
        return std::addressof(m_values[i]);
    }

    struct iterator :abc::iterator_facade<iterator, std::random_access_iterator_tag, double>
    {
        iterator() = default;
        explicit iterator(MyObject* obj, std::size_t i) :m_obj(obj), m_index(i) {};

        double& operator*() const noexcept { return *(m_obj->at(m_index)); }

        iterator& advance(std::ptrdiff_t n) noexcept { m_index += n; return *this; }
        std::ptrdiff_t distance_to(const iterator& rhs) const noexcept { return m_index - rhs.m_index; }
    private:
        MyObject* m_obj{};
        std::ptrdiff_t m_index{};
    };

    struct const_iterator :abc::iterator_facade<iterator, std::random_access_iterator_tag, double>
    {
        const_iterator() = default;
        explicit const_iterator(const MyObject* obj, std::size_t i) :m_obj(obj), m_index(i) {};

        const double& operator*() const noexcept { return *(m_obj->at(m_index)); }

        const_iterator& advance(std::ptrdiff_t n) noexcept { m_index += n; return *this; }
        std::ptrdiff_t distance_to(const const_iterator& rhs) const noexcept { return m_index - rhs.m_index; }
    private:
        const MyObject* m_obj{};
        std::ptrdiff_t m_index{};
    };

    iterator begin() noexcept {
        return iterator{ this,0 };
    }

    const_iterator begin() const noexcept {
        return const_iterator{ this,0 };
    }

    iterator end() noexcept {
        return iterator{ this,this->size() };
    }

    const_iterator end() const noexcept {
        return const_iterator{ this,this->size() };
    }
};

int main(int argc, char** argv) {

    MyObject obj{ 1,3,5,7,9 };
    std::vector<double> results;
    std::copy(obj.begin(), obj.end(), std::back_inserter(results));

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        results.push_back(*it);
    }

    for (auto&& v1 : obj) {
        results.push_back(v1);
    }

    return 0;
}
