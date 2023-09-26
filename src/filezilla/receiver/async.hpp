#ifndef FZ_RECEIVER_ASYNC_HPP
#define FZ_RECEIVER_ASYNC_HPP

#include "handle.hpp"

namespace fz {

template <typename ReentrancyTag>
class async_receiver_maker
{
	template <typename F>
	struct holder {
		holder(const receiving_context &h, F && f) noexcept
			: h_(h)
			, f_(std::forward<F>(f))
		{}

		template <typename E>
		operator receiver_handle<E> () &&
		{
			return { h_, std::move(f_), ReentrancyTag{} };
		}

	private:
		const receiving_context &h_;
		F f_;
	};

	receiving_context h_;

public:
	async_receiver_maker(enabled_for_receiving_base *h) noexcept
		: h_(h->get_receiving_context())
	{}

	async_receiver_maker(enabled_for_receiving_base &h) noexcept
		: h_(h.get_receiving_context())
	{}

	template <typename E>
	async_receiver_maker(const receiver_handle<E> &h) noexcept
		: h_(h.h_)
	{}

	template <typename F>
	holder<F> operator >>(F && f)
	{
		return { h_, std::forward<F>(f) };
	}
};

using async_receive = async_receiver_maker<receiver_detail::non_reentrant_tag>;
using async_reentrant_receive = async_receiver_maker<receiver_detail::reentrant_tag>;

class async_handler final: public util::invoker_handler, public enabled_for_receiving<async_handler>
{
public:
	using util::invoker_handler::invoker_handler;
	~async_handler() override
	{
		stop_receiving();
		remove_handler();
	}
};

}
#endif // FZ_RECEIVER_ASYNC_HPP
