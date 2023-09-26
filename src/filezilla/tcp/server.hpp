#ifndef FZ_TCP_SERVER_HPP
#define FZ_TCP_SERVER_HPP

#include <unordered_map>
#include <set>

#include <libfilezilla/event_handler.hpp>
#include <libfilezilla/logger.hpp>

#include "../tcp/listener.hpp"
#include "../tcp/session.hpp"

namespace fz::tcp {

class server_context {
public:
	server_context(thread_pool &pool, event_loop &loop) : pool_(pool), loop_(loop) {}

	thread_pool& pool() const { return pool_; }
	event_loop& loop() const { return loop_; }

	session::id next_session_id() {
		scoped_lock lock(mutex_);
		return ++last_session_id_;
	}

private:
	thread_pool &pool_;
	event_loop &loop_;
	session::id last_session_id_{};
	fz::mutex mutex_;
};

class server: protected event_handler
{
public:
	server(server_context &context, logger_interface &logger, session::factory &session_factory);
	~server() override;

	void start();
	void stop(bool destroy_all_sessions = false);
	bool is_serving() const;

	template <typename It, typename Sentinel, typename UserDataFunc>
	auto set_listen_address_infos(It begin, Sentinel end, const UserDataFunc &f) -> decltype(static_cast<const tcp::address_info&>(*begin), begin != end, void());

	template <typename It, typename Sentinel>
	auto set_listen_address_infos(It begin, Sentinel end) -> decltype(static_cast<const tcp::address_info&>(*begin), begin != end, void());

	//! Iterates over ALL the active peers.
	//! \param func is a functor that gets invoked over each of the iterated over peers. \return false to make the iteration stop.
	//! \returns the number of all iterated over peers.
	template <typename Func, std::enable_if_t<std::is_invocable_v<Func, session&>>* = nullptr>
	size_t iterate_over_sessions(Func && func);

	//! Iterates over the active peers.
	//! \param ids is the list of peers ids to use as a filter. If empty, then all peers are iterated over.
	//! \param func is a functor that gets invoked over each of the iterated over peers. \return false to make the iteration stop.
	//! \returns the number of all available peers, regardless of how many were iterated over.
	template <typename Func, std::enable_if_t<std::is_invocable_v<Func, session&>>* = nullptr>
	size_t iterate_over_sessions(const std::vector<session::id> &ids, Func && func);

	//! Disconnect the active sessions.
	//! \param ids is the list of sessions ids to use as a filter. If empty, then all sessions are disconnected.
	std::size_t end_sessions(const std::vector<session::id> &ids = {}, int err = 0);

	//! \returns the number of sessions currently active
	std::size_t get_number_of_sessions() const;

	//! \returns a locked proxy to a session with the specifid id, if it exists.
	//! Careful: the session list will be locked for as long as the returned locked proxy is alive.
	util::locked_proxy<session> get_session(session::id id);

private:
	void operator ()(const event_base &ev) override;
	void on_connected_event(tcp::listener &listener, std::unique_ptr<fz::socket> &socket);
	void on_status_changed(const fz::tcp::listener &listener);
	void on_session_ended_event(session::id, const channel::error_type &);

	server_context &context_;
	logger_interface &logger_;
	session::factory &session_factory_;
	event_loop listeners_loop_;

	mutable fz::mutex mutex_;

	std::unordered_map<session::id, std::unique_ptr<session>> sessions_{};
	std::set<tcp::listener, std::less<void>> listeners_{};
	bool is_serving_{};

	std::atomic<std::size_t> num_sessions_{};
};

template <typename Func, std::enable_if_t<std::is_invocable_v<Func, session&>>*>
inline size_t server::iterate_over_sessions(Func && func)
{
	scoped_lock lock(mutex_);

	for (auto &[id, s]: sessions_)
		if (!func(*s))
			break;

	return sessions_.size();
}

template <typename Func, std::enable_if_t<std::is_invocable_v<Func, session&>>*>
inline size_t server::iterate_over_sessions(const std::vector<session::id> &ids, Func && func)
{
	scoped_lock lock(mutex_);

	// An empty list of peers ids means: iterate over all available peers
	if (ids.empty())
		return iterate_over_sessions(func);

	for (auto id: ids) {
		auto it = sessions_.find(id);
		if (it != sessions_.end())
			if (!func(*it->second))
				break;
	}

	return sessions_.size();
}

inline std::size_t server::get_number_of_sessions() const
{
	return num_sessions_;
}

template <typename It, typename Sentinel, typename UserDataFunc>
auto server::set_listen_address_infos(It begin, Sentinel end, const UserDataFunc &f) -> decltype(static_cast<const tcp::address_info&>(*begin), begin != end, void())
{
	std::set<tcp::listener, std::less<void>> new_listeners;

	scoped_lock lock(mutex_);

	for (; begin != end; ++begin) {
		const auto &info = *begin;

		auto it = listeners_.find(info);

		std::any user_data;
		if constexpr (!std::is_same_v<void, decltype(f(info))>)
			user_data = f(info);

		if (it != listeners_.end()) {
			auto &&node = listeners_.extract(it);

			node.value().get_user_data() = std::move(user_data);

			new_listeners.insert(std::move(node));
		}
		else {
			it = new_listeners.emplace(context_.pool(), listeners_loop_, static_cast<event_handler&>(*this), logger_, info, std::move(user_data)).first;

			if (is_serving_)
				it->start();
		}
	}

	std::swap(new_listeners, listeners_);

	// Manually inform the factory that some listeners are being shut down,
	// because listener's destructor doesn't do it. If it did, then the message
	// would arrive when the destructor already exited, causing a crash when accessing the now-dead listeners internals.
	for (auto &l: new_listeners) {
		l.stop(true);
		session_factory_.listener_status_changed(l);
	}
}

template <typename It, typename Sentinel>
auto server::set_listen_address_infos(It begin, Sentinel end) -> decltype(static_cast<const tcp::address_info&>(*begin), begin != end, void())
{
	return set_listen_address_infos(begin, end, [](const tcp::address_info &){});
}

}

#endif // FZ_TCP_SERVER_HPP
