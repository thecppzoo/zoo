#ifndef ZOO_TRAITS_SUPPORT_H
#define ZOO_TRAITS_SUPPORT_H

// Macro to generate _v variable from a trait
#define ZOO_PP_V_VARIABLE_OUT_OF_TRAIT(Trait) \
    template<typename T> \
    inline constexpr auto Trait##_v = Trait<T>::value;

#define ZOO_PP_MAKE_TRAIT(Name, ...) \
template <typename T> \
struct Name { \
    template <typename TypeToTriggerSFINAE> \
    static constexpr decltype( \
        __VA_ARGS__ \
        , std::true_type{} \
    ) test(int) { return {}; } \
    \
    template <typename> \
    static constexpr std::false_type test(...) { return {}; } \
    \
    static constexpr auto value = test<T>(0); \
}; \
ZOO_PP_V_VARIABLE_OUT_OF_TRAIT(Name)

#endif // ZOO_TRAITS_SUPPORT_H
