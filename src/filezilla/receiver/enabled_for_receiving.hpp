#ifndef FZ_RECEIVER_ENABLED_FOR_RECEIVING_HPP
#define FZ_RECEIVER_ENABLED_FOR_RECEIVING_HPP

#include <cassert>
#include <libfilezilla/event_handler.hpp>

#include "../shared_context.hpp"

namespace fz {

using receiving_context = shared_context<event_handler>;

class enabled_for_receiving_base: private enabled_for_shared_context<event_handler>
{
public:
	virtual ~enabled_for_receiving_base() override = 0;

	const receiving_context &get_receiving_context() const
	{
		return get_shared_context();
	}

	explicit operator bool() const
	{
		return bool(get_shared_context());
	}

protected:
	void stop_receiving()
	{
		stop_sharing_context();
	}

private:
	template <typename T>
	friend class enabled_for_receiving;

	enabled_for_receiving_base(event_handler &eh)
		: enabled_for_shared_context(eh)
	{}
};

template <typename H>
class enabled_for_receiving: public enabled_for_receiving_base
{
protected:
	enabled_for_receiving()
		: enabled_for_receiving_base(static_cast<event_handler &>(static_cast<H &>(*this)))
	{
		static_assert(std::is_base_of_v<enabled_for_receiving, H>, "H must be derived from enabled_for_receiving<H>");
		static_assert(std::is_base_of_v<event_handler, H>, "H must be derived from event_handler");
	}
};

}
#endif // ENABLED_FOR_RECEIVING_HPP
