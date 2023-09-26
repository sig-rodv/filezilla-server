#ifndef FT_FTP_SERVER_HPP
#define FT_FTP_SERVER_HPP

#include <libfilezilla/socket.hpp>
#include <libfilezilla/thread_pool.hpp>
#include <libfilezilla/rate_limiter.hpp>

#include "../logger/modularized.hpp"
#include "../authentication/authenticator.hpp"
#include "../tcp/server.hpp"
#include "../tcp/address_list.hpp"
#include "../ftp/session.hpp"
#include "../util/options.hpp"


namespace fz::ftp {

class session;

class server final: private event_handler, private tcp::session::factory::base
{
public:
	struct address_info: tcp::address_info
	{
		session::tls_mode tls_mode = session::tls_mode::allow_tls;

		template <typename Archive>
		void serialize(Archive &ar)
		{
			tcp::address_info::serialize(ar);

			ar(FZ_NVP_O(tls_mode));
		}
	};

	struct options: util::options<options, server> {
		opt<std::vector<address_info>>    listeners_info  = o();
		opt<session::options>             sessions        = o();
		opt<commander::welcome_message_t> welcome_message = o();

		options(){}
	};

	server(tcp::server_context &context, event_loop_pool &loop_pool,
		   logger_interface &nonsession_logger, logger_interface &session_logger,
		   authentication::authenticator &authenticator,
		   rate_limit_manager &rate_limit_manager,
		   tcp::address_list &disallowed_ips, tcp::address_list &allowed_ips,
		   authentication::autobanner &autobanner,
		   port_manager &port_manager,
		   options opts = {});

	~server() override;

	void start();
	void stop();

	/// Refuse connections with a message, if the message is non-empty.
	/// If the message is empty, connections aren't refused.
	void refuse_connections(std::string_view refuse_message);

	//! Set server options at runtime
	//! \param opts the options
	void set_options(options opts);

	//! Set the data socket buffer sizes
	void set_data_buffer_sizes(std::int32_t receive = -1, std::int32_t send = -1);

	//! Set the timeouts
	void set_timeouts(const duration &login_timeout, const duration &activity_timeout);

	//! Kicks the given session out
	//! \param id the session to kick out
	std::size_t end_sessions(const std::vector<session::id> &ids);

	//! Iterates over the active sessions.
	//! \param ids is the list of sessions ids to use as a filter. If empty, then all sessions are iterated over.
	//! \param func is a functor that gets invoked over each of the iterated over sessions. \return false to make the iteration stop.
	//! \returns the number of all available sessions, regardless of how many were iterated over.
	template <typename Func, std::enable_if_t<std::is_invocable_v<Func, ftp::session&>>* = nullptr>
	size_t iterate_over_sessions(const std::vector<session::id> &ids, Func && func);

	//! \returns the number of sessions currently active
	std::size_t get_number_of_sessions() const;

	void set_notifier_factory(session::notifier::factory &nf);

private:
	fz::mutex mutex_{true};

	thread_pool &pool_;
	logger::modularized nonsession_logger_;
	logger::modularized session_logger_;
	authentication::authenticator &authenticator_;
	fz::rate_limit_manager &rate_limit_manager_;
	authentication::autobanner::with_events autobanner_;
	port_manager &port_manager_;

	options opts_;

	std::int32_t receive_buffer_size_ = -1;
	std::int32_t send_buffer_size_    = -1;
	fz::duration login_timeout_       = {};
	fz::duration activity_timeout_    = {};

	std::string refuse_message_;

	tcp::server tcp_server_;

	session::notifier::factory *notifier_factory_{&session::notifier::factory::none};

private:
	std::unique_ptr<tcp::session> make_session(event_handler &target_handler, event_loop &loop, tcp::session::id session_id, std::unique_ptr<socket> socket, const std::any &user_data, int &error) override;
	void listener_status_changed(const tcp::listener &listener) override;
	bool log_on_session_exit() override;

private:
	void operator()(const event_base &ev) override;
	void on_banned_event(const std::string &address, address_type type);
};

template <typename Func, std::enable_if_t<std::is_invocable_v<Func, ftp::session&>>*>
size_t server::iterate_over_sessions(const std::vector<session::id> &ids, Func && func)
{
	return tcp_server_.iterate_over_sessions(ids, [&func](tcp::session &tcp_session) {
		auto &s = static_cast<ftp::session &>(tcp_session);
		return func(s);
	});
}

inline std::size_t server::get_number_of_sessions() const
{
	return tcp_server_.get_number_of_sessions();
}

}

#endif
