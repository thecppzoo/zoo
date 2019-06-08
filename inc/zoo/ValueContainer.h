//
//  ValueContainer.h
//  ZooTests
//
//  Created by Eduardo Madrid on 6/7/19.
//  Copyright © 2019 Eduardo Madrid. All rights reserved.
//

#ifndef ValueContainerCRT_h
#define ValueContainerCRT_h

#include <typeinfo>

namespace zoo {

template<typename Derived, typename IAC, typename ValueType>
struct ValueContainerCRT {
    Derived *d() noexcept { return static_cast<Derived *>(this); }
    const Derived *d() const noexcept {
        return const_cast<ValueContainerCRT *>(this)->d();
    }

    ValueType *thy() { return reinterpret_cast<ValueType *>(d()->m_space); }
    
    ValueContainerCRT(typename IAC::NONE) {}
    
    template<typename... Args>
    ValueContainerCRT(Args &&... args) {
        new(d()->m_space) ValueType{std::forward<Args>(args)...};
    }
    
    void destroy() noexcept { thy()->~ValueType(); }
    
    void copy(IAC *to) { new(to) Derived{*thy()}; }
    
    void move(IAC *to) {
        new(to) Derived{std::move(*thy())};
    }
    
    void moveAndDestroy(IAC *to) noexcept {
        ValueContainerCRT::move(to);
        ValueContainerCRT::destroy();
    }
    
    void *value() noexcept { return thy(); }
    
    const std::type_info &type() const noexcept {
        return typeid(ValueType);
    }
};

template<typename Derived, typename IAC, typename ValueType>
struct ReferentialContainerCRT {
    Derived *d() noexcept { return static_cast<Derived *>(this); }
    const Derived *d() const noexcept {
        return const_cast<ReferentialContainerCRT *>(this)->d();
    }

    ValueType **pThy() {
        return reinterpret_cast<ValueType **>(d()->m_space);
    }
    
    ValueType *thy() { return *pThy(); }
    
    ReferentialContainerCRT(typename IAC::NONE) {}
    
    template<typename... Values>
    ReferentialContainerCRT(Values &&... values) {
        *pThy() = new ValueType{std::forward<Values>(values)...};
    }
    
    ReferentialContainerCRT(typename IAC::NONE, ValueType *ptr) {
        *pThy() = ptr;
    }
    
    void destroy() noexcept { delete thy(); }
    
    void copy(IAC *to) { new(to) Derived{*thy()}; }
    
    void transferPointer(IAC *to) {
        new(to) Derived{IAC::None, thy()};
    }
    
    void move(IAC *to) noexcept {
        new(to) Derived{IAC::None, thy()};
        new(d()) IAC;
    }
    
    void moveAndDestroy(IAC *to) noexcept {
        transferPointer(to);
    }
    
    void *value() noexcept { return thy(); }
    
    const std::type_info &type() const noexcept {
        return typeid(ValueType);
    }
};

}
#endif /* ValueContainerCRT_h */
