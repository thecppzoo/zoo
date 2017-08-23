#pragma once

#include <type_traits>

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

protected:
	detail::AlignedStorage<Size, Alignment> m_space;
};

using Any = AnyContainer<8>;

}

