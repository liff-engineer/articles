#pragma once

#include "members.hpp"
#include <iostream>

template<typename T>
void print_struct_info() {
    static_assert(abc::members<T>::value, "need abc::members<T> implement");
    abc::members<T>::for_each([&](auto& m) {
        std::cout << typeid(T).name() << " member:" << m.name << ",offset:" << m.offset << "\n";
        });
}

template<typename T>
void print_enum_info() {
    static_assert(abc::members<T>::value && std::is_enum<T>::value, "need abc::members<T> implement");
    abc::members<T>::for_each([&](auto m,auto name) {
        std::cout << typeid(T).name() <<" " << name << "=" << static_cast<std::underlying_type_t<decltype(m)>>(m) << "\n";
        });
}

template<typename T, std::enable_if_t<abc::members<T>::value, int>* = nullptr>
void print(T const& o) {
    abc::members<T>::for_each([&](auto& m) {
        std::cout << typeid(T).name() << " member:" << m.name << ",offset:" << m.offset << ",value:" << o.*(m.mp) << "\n";
        });
}

template<typename T, std::enable_if_t<!abc::members<T>::value, int>* = nullptr>
void print(T const& o) {
    std::cout << o << "\n";
}