#ifndef FZ_FORWARD_LIKE_HPP
#define FZ_FORWARD_LIKE_HPP

#include <utility>

// From https://vittorioromeo.info/Misc/fwdlike.html

namespace fz {

template <class T, class U>
using apply_value_category_t = std::conditional_t<
	std::is_lvalue_reference_v<T>,
	std::remove_reference_t<U>&,
	std::remove_reference_t<U>&&
>;

template <class T, class U>
constexpr apply_value_category_t<T, U> forward_like(U&& u) noexcept
{
	return static_cast<apply_value_category_t<T, U>>(u);
}

}

#endif // FZ_FORWARD_LIKE_HPP
