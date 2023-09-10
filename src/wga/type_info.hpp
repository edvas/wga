//
// Created by edvas on 9/10/23.
//

#ifndef WGA_TYPE_INFO_HPP
#define WGA_TYPE_INFO_HPP

#include <typeinfo>

namespace wga {
    template<typename T>
    auto type_name(const T &) -> const char * {
#if defined(__GXX_RTTI) || defined(__CPPRTTI)
        return typeid(T).name();
#else
        return "(no rtti)";
#endif
    }
}

#endif //WGA_TYPE_INFO_HPP
