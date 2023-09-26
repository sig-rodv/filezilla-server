#ifndef FZ_RMP_ENGINE_FORWARDER_HPP
#define FZ_RMP_ENGINE_FORWARDER_HPP

#include "../../rmp/engine/dispatcher.hpp"
#include "../../rmp/engine/access.hpp"
#include "../../mpl/with_index.hpp"
#include "../../mpl/at.hpp"

namespace fz::rmp {

template <typename AnyMessage>
template <typename Implementation>
class engine<AnyMessage>::forwarder: public dispatcher
{
	template <typename V, typename = void>
	struct has_connection: std::false_type{};

	template <typename V>
	struct has_connection<V, std::void_t<decltype(std::declval<V>().connection(std::declval<session*>(), 0))>>: std::true_type{};

	template <typename V, typename = void>
	struct has_disconnection: std::false_type{};

	template <typename V>
	struct has_disconnection<V, std::void_t<decltype(std::declval<V>().disconnection(std::declval<session&>(), 0))>>: std::true_type{};

	template <typename V, typename = void>
	struct has_certificate_verification: std::false_type{};

	template <typename V>
	struct has_certificate_verification<V, std::void_t<decltype(std::declval<V>().certificate_verification(std::declval<session &>(), std::declval<tls_session_info>()))>>: std::true_type{};

public:
	forwarder(Implementation &implementation) noexcept
		: visitor_(implementation)
	{}

	void operator()(session &session, std::uint16_t index, serialization::binary_input_archive &l) override
	{
		mpl::with_index<any_message::size()>(index, [&](auto i) {
			using Message = mpl::at_c_t<typename any_message::messages, i>;
			access::template load_and_dispatch<Message>(visitor_, session, l);
		});
	}

	void connection(session *session, int err) override
	{
		if constexpr (has_connection<decltype (visitor_)>::value)
			visitor_.connection(session, err);
		else
			dispatcher::connection(session, err);
	}

	void disconnection(session &session, int err) override
	{
		if constexpr (has_disconnection<decltype (visitor_)>::value)
			visitor_.disconnection(session, err);
		else
			dispatcher::disconnection(session, err);
	}

	void certificate_verification(session &s, tls_session_info &&i) override
	{
		if constexpr (has_certificate_verification<decltype (visitor_)>::value)
			visitor_.certificate_verification(s, std::move(i));
		else
			dispatcher::certificate_verification(s, std::move(i));
	}

private:
	Implementation &visitor_;
};

}
#endif // FZ_RMP_ENGINE_FORWARDER_HPP
