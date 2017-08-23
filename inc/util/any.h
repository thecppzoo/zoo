#pragma once

#include <type_traits>
#include <new>

namespace zoo {

namespace detail {

template<int Size, int Alignment>
struct alignas(Alignment) AlignedStorage {
	char space[Size];
};

}

template<int Size, int Alignment = alignof(void *)>
struct AnyContainer {
	template<typename T>
	static constexpr bool value_semantics() {
		return alignof(T) <= Alignment && sizeof(T) <= Size;
	}

	template<
		typename T,
		typename Decayed = std::decay_t<T>,
		std::enable_if_t<std::is_copy_constructible<Decayed>::value, int> = 0
	>
	AnyContainer(T &&initializer);

    ~AnyContainer() { m_typeSwitch.destroy(m_space); }

protected:
    using Storage = detail::AlignedStorage<Size, Alignment>;

    struct TypeSwitch {
        virtual void copy(Storage &to, Storage &from) {}
        virtual void move(Storage &to, Storage &&from) {}
        virtual void destroy(Storage &what) {}
    };

    template<typename T>
    struct Value: TypeSwitch {
        /*void copy(Storage &to, Storage &from) override {
            auto fromPointer = reinterpret_cast<T *>(from.space);
            new(to.space) T(*fromPointer);
        }

        void move(Storage &to, Storage &&from) override {
            auto fromPointer = reinterpret_cast<T *>(from.space);
            new(to.space) T(std::move(*fromPointer));
        }*/

        void destroy(Storage &what) override {
            auto pointer = reinterpret_cast<T *>(&what);
            pointer->~T();
        }
    };

    template<typename T>
    struct Referential: TypeSwitch {
        void destroy(Storage &what) override {
            auto pointer = reinterpret_cast<T *>(what.space);
            delete pointer;
        }
    };

    Storage m_space;
    TypeSwitch m_typeSwitch;
};

using Any = AnyContainer<8>;

template<int Size, int Alignment>
template<
    typename T,
    typename Decayed,
    std::enable_if_t<std::is_copy_constructible<Decayed>::value, int>
>
AnyContainer<Size, Alignment>::AnyContainer(T &&initializer) {
    if(alignof(Decayed) <= Alignment && sizeof(Decayed) <= Size) {
        new(m_space.space) Decayed{std::forward<T>(initializer)};
        new(&m_typeSwitch) Value<Decayed>{};
    } else {
        reinterpret_cast<Decayed *&>(m_space.space) =
            new Decayed{std::forward<T>(initializer)};
        new(&m_typeSwitch) Referential<Decayed>{};
    }
};

}

