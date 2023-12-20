#include <type_traits>
#include <new>
#include <utility>

template<typename T, typename... Args>
constexpr auto Constructible_v = std::is_constructible_v<T, Args...>;

template<typename Q>
struct ATemplate {
    alignas(alignof(Q)) char space_[sizeof(Q)];

    template<typename T>
    constexpr static auto FitsInSpace_v = sizeof(T) <= sizeof(Q);

    template<typename T, typename... Args>
    std::enable_if_t<
        FitsInSpace_v<T>
        &&
            #ifndef TRIGGER_MSVC_SFINAE_BUG
            bool(
            #endif
            Constructible_v<T, Args...>
            #ifndef TRIGGER_MSVC_SFINAE_BUG
            )
            #endif
        ,
        T *
    >
    sfinaeFunction(Args &&...args) {
        T *rv = new(static_cast<void *>(space_)) T(std::forward<Args>(args)...); 
        return rv;
    }

};

auto triggerError(ATemplate<void *[2]> &m) {
    return m.sfinaeFunction<void *>(nullptr);
}

int main(int, const char *[]) { return 0; }
