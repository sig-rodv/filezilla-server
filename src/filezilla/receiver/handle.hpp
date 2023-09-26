#ifndef FZ_RECEIVER_HANDLE_HPP
#define FZ_RECEIVER_HANDLE_HPP

#include <libfilezilla/event_handler.hpp>

#include "../util/invoke_later.hpp"
#include "../mpl/prepend.hpp"

#include "event.hpp"
#include "detail.hpp"
#include "enabled_for_receiving.hpp"

namespace fz {

/*
 * receiver_handle<E> only accepts E's which are specializations of receiver_event<>.
 * A receiver_event<> is like a simple_event<>, and has a v_ tuple member which can be used by fz::dispatch,
 * but the v_ member isn't constructed, even if the event itself is, until the event is sent.
 *
 * The event E can only be constructed by receiver_handle<E>, and it can thus only be sent by receiver_handle<E>,
 * via its call operator.
 *
 * This, coupled with the fact that receiver_handle<E> is a movable-only type, has the implication that
 * the address of the event it contains can be thought of as the unique id of the asynchronous operation that was started.
 *
 * This is needed in order to know to which async operation does the event correspond to, both by a normal event handler
 * that is handling those responses, and by any other module that wants to offload the operation to some other processes/machines:
 * such modules would have to gather the unique id and pass it along the offloaded request, whose response would contain
 * the very same unique id
 */

class receiver_handle_base
{
public:
	virtual ~receiver_handle_base()
	{
		delete v_;
	}

	receiver_handle_base() = default;
	receiver_handle_base(const receiver_handle_base &) = delete;
	receiver_handle_base &operator=(const receiver_handle_base &) = delete;

	receiver_handle_base(receiver_handle_base &&rhs) noexcept
		: h_(std::move(rhs.h_))
		, v_(rhs.v_)
	{
		rhs.v_ = nullptr;
	}

	receiver_handle_base &operator=(receiver_handle_base &&rhs) noexcept
	{
		// It's UB to do *this = std::move(*this)
		h_ = std::move(rhs.h_);

		v_ = rhs.v_;
		rhs.v_ = nullptr;

		return *this;
	}

	explicit operator bool() const
	{
		return h_ && v_;
	}

protected:
	mutable receiving_context h_;
	mutable receiver_event_values_base *v_{};

	receiver_handle_base(const receiving_context &h, receiver_event_values_base *v)
		: h_(h)
		, v_(v)
	{}
};

template <typename E = receiver_event<void>>
class receiver_handle final: public receiver_handle_base
{
	using values_type = typename E::values_type;

	static_assert(trait::is_receiver_event_v<E>, "Parameter E must be a receiver_event<>");

	template <typename ReentrancyTag>
	friend class async_receiver_maker;

	struct async_caller_base: util::invoker_event, values_type {};

	template <typename F>
	struct async_caller: async_caller_base
	{
		async_caller(F && f)
			: f_(std::forward<F>(f))
		{
		}

		void operator()() const override
		{
			auto &self = *const_cast<async_caller *>(this);
			std::apply(self.f_, self.values_type::v_);
		}

		event_base *get_event() override
		{
			return this;
		}

		static async_caller* make(F && f)
		{
			constexpr bool CanCompile = receiver_detail::is_invocable_with_tuple_v<F, typename E::tuple_type>;

			if constexpr (CanCompile)
				return new async_caller(std::forward<F>(f));
			else {
				static_assert(CanCompile, "Callable's parameters don't match with the event.");
				return nullptr;
			}
		}

		F f_;
	};

	template <typename F>
	struct async_reentrant_caller: async_caller_base
	{
		async_reentrant_caller(const receiving_context &h, F && f)
			: h_(h)
			, f_(std::forward<F>(f))
		{
		}

		void operator()() const override
		{
			auto &self = *const_cast<async_reentrant_caller *>(this);

			std::apply([this](auto &&...args) {
				receiver_handle<E>(h_, std::move(f_), receiver_detail::reentrant_tag{}).reentrant_call<async_reentrant_caller>(std::forward<decltype(args)>(args)...);
			}, self.values_type::v_);
		}

		event_base *get_event() override
		{
			return this;
		}

		receiving_context h_;
		mutable F f_;

		static async_reentrant_caller* make(const receiving_context &h, F && f)
		{
			constexpr bool CanCompile = receiver_detail::is_invocable_with_tuple_v<F, mpl::prepend_t<typename E::tuple_type, receiver_handle<E>>>;

			if constexpr (CanCompile)
				return new async_reentrant_caller(h, std::forward<F>(f));
			else {
				static_assert(CanCompile, "Callable's parameters don't match with the event.");
				return nullptr;
			}
		}
	};

	template <typename F>
	receiver_handle(const receiving_context &h, F && f, receiver_detail::non_reentrant_tag)
		: receiver_handle_base(h, async_caller<F>::make(std::forward<F>(f)))
	{
	}

	template <typename F>
	receiver_handle(const receiving_context &h, F && f, receiver_detail::reentrant_tag)
		: receiver_handle_base(h, async_reentrant_caller<F>::make(h, std::forward<F>(f)))
	{
	}

	template <typename C, typename... Args>
	void reentrant_call(Args &&... args) &&
	{
		auto c = static_cast<C*>(v_);
		c->f_(std::move(*this), std::forward<Args>(args)...);
	}

public:
	using receiver_handle_base::receiver_handle_base;

	receiver_handle(enabled_for_receiving_base *h)
		: receiver_handle_base(h->get_receiving_context(), new E)
	{
	}

	receiver_handle(enabled_for_receiving_base &h)
		: receiver_handle_base(h.get_receiving_context(), new E)
	{
	}

	std::uintptr_t get_id() const {
		return std::uintptr_t(v_);
	}

	// One shot invoker. Once you activate this, subsequent calls will just be no-ops. Be advised.
	template <typename... Args>
	auto operator()(Args &&... args) const -> decltype(typename E::tuple_type(std::forward<Args>(args)...), void())
	{
		if (!v_)
			return;

		if (auto &&h = h_.lock()) {
			static_cast<values_type*>(v_)->emplace(std::forward<Args>(args)...);
			h->send_event(v_->get_event());
		}

		v_ = nullptr;
	}
};

}
#endif // FZ_RECEIVER_HANDLE_HPP
