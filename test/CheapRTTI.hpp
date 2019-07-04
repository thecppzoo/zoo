//
//  CheapRTTI.hpp
//
//  Created by Eduardo Madrid on 7/3/19.
//

#ifndef ZOO_CheapRTTI_hpp
#define ZOO_CheapRTTI_hpp

#include <vector>

namespace zoo {

using RegisteredType = void *(*)(void *);

inline std::vector<RegisteredType> &cheapTypeRegistry() {
    static auto singleton = new std::vector<RegisteredType>;
    return *singleton;
}

inline auto cheapRTTIRegistration() {
    auto &reg = cheapTypeRegistry();
    auto rv = reg.size();
    reg.resize(rv + 1);
    return rv;
}

}

#endif /* CheapRTTI_hpp */
