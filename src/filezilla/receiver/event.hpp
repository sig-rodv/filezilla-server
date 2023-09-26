#ifndef FZ_RECEIVER_EVENT_HPP
#define FZ_RECEIVER_EVENT_HPP

#include <libfilezilla/event.hpp>
#include "../util/traits.hpp"

/*
 *  ***** !ATTENTION! *****
 *
 *  Putting this here temporarily, for lack of a better place.
 *
 *  When using the receivers, great care must be taken *NOT TO* std::move() objects into the lambda if they're also used in the async function call itself,
 *  as, by today's C++ standard, **order of evaluation of function arguments is unspecified** (as of c++17 interleaving is prohibited, but that doesn't help here).
 *
 *  Moving the receiver_handle is safe, because it's used only by async_receive(), which sits on the left side of
 *  operator>>(), which evaluates left-to-right: so first async_receive(r) takes place, then std::move(r).
 */

namespace fz {

class receiver_event_values_base
{
public:
	virtual ~receiver_event_values_base(){}
	virtual event_base *get_event() = 0;
};

template <typename... Values>
class receiver_event_values: public receiver_event_values_base
{
public:
	using tuple_type = std::tuple<Values...>;

	union {
		char dummy_;
		mutable tuple_type v_;
	};

	~receiver_event_values() override
	{
		if (!engaged_)
			return;

		v_.~tuple_type();
	}

protected:
	template <typename E>
	friend class receiver_handle;

	receiver_event_values() noexcept {}

private:
	bool engaged_{};

	// This *MUST* be called before sending the event, or else the receiver will access an uncostructed
	// v_, which is UB.
	// It's *impossible* to construct this event by third parties, because the default constructor is private,
	// so that's more a reminder to self than anything else.
	template <typename... Ts, std::enable_if_t<std::is_constructible_v<tuple_type, Ts...>>* = nullptr>
	void emplace(Ts &&... vs) noexcept(noexcept(tuple_type(std::forward<Ts>(vs)...)))
	{
		if (engaged_)
			v_.~tuple_type();

		new (&v_) tuple_type(std::forward<Ts>(vs)...);
		engaged_ = true;
	}
};

template<typename UniqueType, typename...Values>
class receiver_event final : public event_base, public receiver_event_values<Values...>
{
public:
	using values_type = receiver_event_values<Values...>;
	using unique_type = UniqueType;

	using values_type::values_type;

	static size_t type()
	{
		static size_t const v = get_unique_type_id(typeid(receiver_event*));
		return v;
	}

private:
	template <typename E>
	friend class receiver_handle;

	size_t derived_type() const override
	{
		return type();
	}

	event_base *get_event() override
	{
		return this;
	}
};

FZ_UTIL_TRAITS_MAKE_ACCESS_TEMPLATE(receiver_event)

template<typename UniqueType, typename E, typename... Values>
struct extend_receiver_event;

template<typename UniqueType, typename OtherUniqueType,  typename... OtherValues, typename... Values>
struct extend_receiver_event<UniqueType, receiver_event<OtherUniqueType, OtherValues...>, Values...>
{
	using type = receiver_event<UniqueType, OtherValues..., Values...>;
};

template<typename UniqueType, typename E, typename... Values>
using extend_receiver_event_t = typename extend_receiver_event<UniqueType, E, Values...>::type;

}

#endif // FZ_RECEIVER_EVENT_HPP
