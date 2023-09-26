#ifndef FZ_TCP_LISTENER_HPP
#define FZ_TCP_LISTENER_HPP

#include <any>

#include <libfilezilla/event_handler.hpp>
#include <libfilezilla/socket.hpp>
#include <libfilezilla/logger.hpp>

#include "../tcp/address_info.hpp"

namespace fz::tcp {

class listener: private event_handler {
	struct connected_event_tag {};
	struct status_changed_event_tag {};

public:
	using connected_event = simple_event<connected_event_tag, listener &/*self*/, std::unique_ptr<socket>>;
	using status_changed_event = simple_event<status_changed_event_tag, const listener &/*self*/>;

	enum class status
	{
		started,          //!< Port is availabe, listening has effectively started.
		stopped,          //!< Listening has stopped
		retrying_to_start //!< Port is unavailable, will try to start repeteadly until it succeeds or is excplicitly stopped.
	};

	listener(thread_pool &pool, event_loop &loop, event_handler &target_handler, logger_interface &logger, address_info address_info, std::any user_data = {});
	~listener() override;

	const address_info &get_address_info() const;
	address_info &get_address_info();
	const std::any &get_user_data() const;
	std::any &get_user_data();
	status get_status() const;

	void start() const;
	void stop(bool remove_events = false) const;

private:
	void operator ()(const event_base &ev) override;
	void on_socket_event(fz::socket_event_source *, fz::socket_event_flag type, int error);
	void on_timer_event(timer_id const &);

	void try_listen() const;

private:
	thread_pool &pool_;
	event_handler &target_handler_;
	logger_interface &logger_;
	address_info address_info_;
	std::any user_data_;

	mutable status status_ = status::stopped;
	mutable std::unique_ptr<listen_socket> listen_socket_;
	mutable fz::timer_id timer_id_{};
	mutable fz::mutex mutex_;
};

inline bool operator <(const listener &lhs, const listener &rhs)
{
	return lhs.get_address_info() < rhs.get_address_info();
}

inline bool operator <(const listener &lhs, const tcp::address_info &rhs)
{
	return lhs.get_address_info() < rhs;
}

inline bool operator <(const tcp::address_info &lhs, const listener &rhs)
{
	return lhs < rhs.get_address_info();
}

}


#endif // FZ_TCP_LISTENER_HPP
