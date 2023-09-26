#ifndef FZ_MPL_FOR_EACH_HPP
#define FZ_MPL_FOR_EACH_HPP

#include "arity.hpp"
#include "at.hpp"
#include "identity.hpp"

namespace fz::mpl {

namespace detail {

	template <typename Seq, typename F, std::size_t... Is>
	void for_each(F && f, const std::index_sequence<Is...>&)
	{
		// If F returns a boolean, stop iterating if the return is false.
		if constexpr ((std::is_invocable_r_v<bool, F, identity<typename at_c<Seq, Is>::type>> && ...))
			(std::forward<F>(f)(identity<typename at_c<Seq, Is>::type>{}) && ...);
		else
			(std::forward<F>(f)(identity<typename at_c<Seq, Is>::type>{}), ...);
	}
}

template <typename Seq, typename F>
void for_each(F && f)
{
	detail::for_each<Seq>(std::forward<F>(f), typename arity<Seq>::indices());
}

}


#endif // FZ_MPL_FOR_EACH_HPP
