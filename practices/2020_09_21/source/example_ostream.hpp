#pragma once

#include <ostream>
#include "example.hpp"

inline std::ostream  &operator<<(std::ostream &os, MyType v){
    os << "MyType{";
    switch(v){
        case MyType::A: os << "A";break;
        case MyType::B: os << "B";break;
        case MyType::C: os << "C";break;
    }
    os << "}";
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const Address &v) {
    os << "Address{";
    os << "zipcode = "<<v.zipcode;    
    os << ",detail = "<<v.detail;   
    os << "}";
    return os;  
}
