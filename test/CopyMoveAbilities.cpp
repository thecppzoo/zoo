#include "zoo/meta/copy_and_move_abilities.h"

#include <type_traits>

namespace {

using namespace std;

struct Copyable: zoo::meta::CopyAndMoveAbilities<true> {};

#define SA(...) static_assert(__VA_ARGS__);

SA(is_nothrow_default_constructible_v<Copyable>)
SA(is_nothrow_copy_constructible_v<Copyable>)
SA(is_nothrow_move_constructible_v<Copyable>)

struct NonCopiable: zoo::meta::CopyAndMoveAbilities<false> {};
SA(is_nothrow_default_constructible_v<NonCopiable>)
SA(!is_copy_constructible_v<NonCopiable>);
SA(is_nothrow_move_constructible_v<NonCopiable>);

struct NonMovable: zoo::meta::CopyAndMoveAbilities<false, false> {};
SA(is_nothrow_default_constructible_v<NonMovable>)
SA(!is_copy_constructible_v<NonMovable>);
SA(!is_move_constructible_v<NonMovable>);

}
