#ifndef ZOO_VTABLE_POINTER_WRAPPER_H
#define ZOO_VTABLE_POINTER_WRAPPER_H


template<typename VirtualTable>
struct VTablePointerWrapper {
    const VirtualTable *pointer_;

    /// \brief from the vtable returns the entry corresponding to the affordance
    template<typename Affordance>
    const typename Affordance::VTableEntry *vTable() const noexcept {
        return //static_cast<const typename Affordance::VTableEntry *>(pointer_);
            pointer_->template upcast<Affordance>();
    }

    VTablePointerWrapper(const VirtualTable *p): pointer_(p) {}

    auto pointer() const noexcept { return pointer_; }

    void change(const VirtualTable *p) noexcept { pointer_ = p; }
};


#endif // ZOO_VTABLE_POINTER_WRAPPER_H

