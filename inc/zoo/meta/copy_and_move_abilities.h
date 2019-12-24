#ifndef ZOO_META_COPYABLE_H
#define ZOO_META_COPYABLE_H

namespace zoo::meta {
template<bool, bool = true>
struct CopyAndMoveAbilities;

template<>
struct CopyAndMoveAbilities<true, true> {
    CopyAndMoveAbilities() = default;
    CopyAndMoveAbilities(const CopyAndMoveAbilities &) = default;

    // The default, or compiler generated move constructor is noexcept(true)
    // This code could make it explicit, but it is in general a better idea
    // to let the compiler calculate the noexceptness of copy and move
    // constructors, the programmers should be aware of this.
    // This code leaves it implicit because of the doctrine in the zoo
    // repository to let the compiler calculate as much as possible because
    // this policy minimizes oversights and subtle mistakes while keeping the
    // code clean of redundancies
    CopyAndMoveAbilities(CopyAndMoveAbilities &&) = default;

    CopyAndMoveAbilities &operator=(const CopyAndMoveAbilities &) = default;
    CopyAndMoveAbilities &operator=(CopyAndMoveAbilities &&) = default;
};

template<>
struct CopyAndMoveAbilities<true, false> {
    CopyAndMoveAbilities() = default;
    CopyAndMoveAbilities(const CopyAndMoveAbilities &) = default;
    CopyAndMoveAbilities(CopyAndMoveAbilities &&) = delete;
    CopyAndMoveAbilities &operator=(const CopyAndMoveAbilities &) = default;
};

template<>
struct CopyAndMoveAbilities<false, true> {
    CopyAndMoveAbilities() = default;
    CopyAndMoveAbilities(const CopyAndMoveAbilities &) = delete;
    CopyAndMoveAbilities(CopyAndMoveAbilities &&) = default;
    CopyAndMoveAbilities &operator=(CopyAndMoveAbilities &&) = default;
};

template<>
struct CopyAndMoveAbilities<false, false> {
    CopyAndMoveAbilities() = default;
    CopyAndMoveAbilities(const CopyAndMoveAbilities &) = delete;
    CopyAndMoveAbilities(CopyAndMoveAbilities &&) = delete;
};

}

#endif
