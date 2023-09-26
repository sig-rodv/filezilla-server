#include "listener.hpp"
#include "hostaddress.hpp"
#include "../remove_event.hpp"
#include "../util/parser.hpp"

fz::tcp::listener::listener(fz::thread_pool &pool, event_loop &loop, event_handler &target_handler, logger_interface &logger, address_info address_info, std::any user_data)
	: event_handler{loop}
	, pool_{pool}
	, target_handler_{target_handler}
	, logger_{logger}
	, address_info_(address_info)
	, user_data_(std::move(user_data))
{
}

fz::tcp::listener::~listener() {
	stop(true);
	remove_handler();
}

const fz::tcp::address_info &fz::tcp::listener::get_address_info() const
{
	return address_info_;
}

fz::tcp::address_info &fz::tcp::listener::get_address_info()
{
	return address_info_;
}


const std::any &fz::tcp::listener::get_user_data() const
{
	return user_data_;
}

std::any &fz::tcp::listener::get_user_data()
{
	return user_data_;
}

fz::tcp::listener::status fz::tcp::listener::get_status() const
{
	fz::scoped_lock lock(mutex_);

	return status_;
}

void fz::tcp::listener::start() const
{
	fz::scoped_lock lock(mutex_);

	if (!listen_socket_) {
		socket_base::socket_t fd = -1;
		util::parseable_range r(address_info_.address);

		if (match_string(r, "file_descriptor:") && parse_int(r, fd) && eol(r)) {
			int error = 0;
			listen_socket_ = listen_socket::from_descriptor(socket_descriptor(fd), pool_, error, static_cast<event_handler *>(const_cast<listener *>(this)));
			if (listen_socket_) {
				logger_.log_u(logmsg::status, L"Listening on %s (%s).", address_info_.address, listen_socket_->local_ip());
				status_ = status::started;
			}
			else
				logger_.log_u(logmsg::error, L"Couldn't create listener from %s. Reason: %s.", address_info_.address, socket_error_description(error));
		}
		else {
			listen_socket_ = std::make_unique<listen_socket>(pool_, static_cast<event_handler *>(const_cast<listener *>(this)));
			try_listen();
		}
	}
}

void fz::tcp::listener::stop(bool remove_events) const
{
	fz::scoped_lock lock(mutex_);

	status_ = status::stopped;

	if (listen_socket_) {
		const_cast<listener*>(this)->stop_timer(timer_id_);
		listen_socket_.reset();

		if (remove_events)
			fz::remove_events<connected_event, status_changed_event>(&target_handler_, *this);
		else
			target_handler_.send_event<status_changed_event>(*this);
	}
}

void fz::tcp::listener::operator ()(const fz::event_base &ev) {

	{
		fz::scoped_lock lock(mutex_);

		if (status_ == status::stopped)
			return;
	}

	fz::dispatch<
		socket_event,
		timer_event
	>(ev, this,
		&listener::on_socket_event,
		&listener::on_timer_event
	);
}


void fz::tcp::listener::on_socket_event(fz::socket_event_source *, fz::socket_event_flag type, int error) {
	if (type == fz::socket_event_flag::connection) {
		if (error) {
			logger_.log_u(fz::logmsg::error, L"[%s] Error during connection on %s. Reason: %s.", join_host_and_port(address_info_.address, address_info_.port), socket_error_description(error), error);
			return;
		}

		auto socket = listen_socket_->accept(error);

		if (!socket) {
			logger_.log_u(fz::logmsg::error, L"Failed to accept new connection on %s. Reason: %s.", join_host_and_port(address_info_.address, address_info_.port), socket_error_description(error), error);
			return;
		}

		target_handler_.send_event<connected_event>(*this, std::move(socket));
   }
}

void fz::tcp::listener::on_timer_event(const fz::timer_id &) {
	try_listen();
}

void fz::tcp::listener::try_listen() const {
	constexpr static int retry_time = 1;

	int error = !listen_socket_->bind(address_info_.address) ? EBADF : listen_socket_->listen(fz::address_type::unknown, (int)address_info_.port);

	timer_id_ = {};

	if (error) {
		logger_.log_u(logmsg::error, L"Couldn't bind on %s. Reason: %s. Retrying in %d seconds.", join_host_and_port(address_info_.address, address_info_.port), socket_error_description(error), retry_time);
		timer_id_ = const_cast<listener *>(this)->add_timer(fz::duration::from_seconds(retry_time), true);

		if (status_ != status::retrying_to_start) {
			status_ = status::retrying_to_start;
			target_handler_.send_event<status_changed_event>(*this);
		}
	}
	else {
		logger_.log_u(logmsg::status, L"Listening on %s.", join_host_and_port(address_info_.address, address_info_.port));
		status_ = status::started;
		target_handler_.send_event<status_changed_event>(*this);
	}
}
