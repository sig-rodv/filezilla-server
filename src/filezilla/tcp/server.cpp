#include "../tcp/server.hpp"

namespace fz::tcp {

server::server(server_context &context, logger_interface &logger, session::factory &session_factory)
	: event_handler(context.loop())
	, context_(context)
	, logger_(logger)
	, session_factory_(session_factory)
	, listeners_loop_(context.pool())
{
}

server::~server()
{
	logger_.log_u(logmsg::debug_debug, L"Destroying.");
	remove_handler();
	stop(true);
}

std::size_t server::end_sessions(const std::vector<session::id> &ids, int err)
{
	scoped_lock lock(mutex_);

	std::size_t num_ended = 0;

	// An empty list of peers ids means: disconnect all peers
	if (ids.empty()) {
		for (auto &s: sessions_) {
			++num_ended;
			s.second->shutdown(err);
		}
	}
	else {
		for (auto id: ids) {
			if (auto it = sessions_.find(id); it != sessions_.end()) {
				++num_ended;
				it->second->shutdown(err);
			}
		}
	}

	return num_ended;
}

util::locked_proxy<session> server::get_session(session::id id)
{
	mutex_.lock();

	session *s = [&]() -> session * {
		auto it = sessions_.find(id);
		if (it != sessions_.end())
			return it->second.get();

		return nullptr;
	}();

	return {s, &mutex_};
}

void server::start()
{
	scoped_lock lock(mutex_);

	if (is_serving_)
		return;

	is_serving_ = true;

	for (auto &l: listeners_)
		l.start();
}

void server::stop(bool destroy_all_sessions)
{
	// Session destruction must happen outside mutex, see also on_session_ended_event
	decltype(sessions_) sessions_to_destroy;

	scoped_lock lock(mutex_);

	if (!is_serving_)
		return;

	is_serving_ = false;

	logger_.log_u(logmsg::debug_debug, L"Stopping listeners.");
	for (auto &l: listeners_)
		l.stop();

	if (destroy_all_sessions) {
		logger_.log_u(logmsg::debug_debug, L"Destroying sessions.");
		sessions_to_destroy = std::move(sessions_);
	}
}

bool server::is_serving() const
{
	scoped_lock lock(mutex_);

	return is_serving_;
}

void server::operator ()(const event_base &ev)
{
	fz::dispatch<
		tcp::listener::connected_event,
		tcp::listener::status_changed_event,
		typename session::ended_event
	>(ev, this,
		&server::on_connected_event,
		&server::on_status_changed,
		&server::on_session_ended_event
	);
}

void server::on_connected_event(tcp::listener &listener, std::unique_ptr<socket> &socket)
{
	int error = 0;
	auto id = context_.next_session_id();
	auto session = session_factory_.make_session(*this, id, std::move(socket), listener.get_user_data(), error);

	if (session) {
		scoped_lock lock(mutex_);
		sessions_.insert({id, std::move(session)});
		num_sessions_ += 1;
	}
}

void server::on_status_changed(const listener &listener)
{
	scoped_lock lock(mutex_);
	session_factory_.listener_status_changed(listener);
}

void server::on_session_ended_event(session::id id, const channel::error_type &error)
{
	decltype(sessions_)::node_type extracted;

	{
		scoped_lock lock(mutex_);
		num_sessions_ -= 1;
		extracted = sessions_.extract(id);
	}

	if (!extracted)
		return;

	if (!error)
		logger_.log_u(logmsg::status, "Session %d ended gracefully.", id);
	else
	if (session_factory_.log_on_session_exit() && error != ECONNRESET)
		logger_.log_u(logmsg::error, "Session %d ended with error. Reason: %s.", id, socket_error_description(error.error()));
}

}
