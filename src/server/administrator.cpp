#include <clocale>

#include "../server/administrator.hpp"

#include "../filezilla/debug.hpp"

#include "../filezilla/hostaddress.hpp"
#include "../filezilla/logger/splitter.hpp"

#include "../filezilla/impersonator/util.hpp"
#include "../filezilla/util/username.hpp"
#include "../filezilla/util/io.hpp"

#include "administrator/notifier.hpp"
#include "administrator/update_checker.hpp"

struct administrator::session_data
{
	bool is_in_overflow{};
};

administrator::~administrator()
{
	ftp_server_.set_notifier_factory(fz::tcp::session::notifier::factory::none);

	ftp_server_.iterate_over_sessions({}, [](fz::ftp::session &s) mutable {
		auto notifier = static_cast<administrator::notifier *>(s.get_notifier());
		if (notifier)
			notifier->detach_from_administrator();

		return true;
	});

	splitter_logger_.remove_logger(*log_forwarder_);
}

auto administrator::operator()(administration::ban_ip &&v)
{
	auto &&[ip, family] = std::move(v).tuple();

	if (disallowed_ips_.add(ip, family)) {
		kick_disallowed_ips();
		return v.success();
	}

	return v.failure();
}

void administrator::connection(administration::engine::session *session, int err)
{
	if (session) {
		if (err)
			logger_.log_u(fz::logmsg::error, L"Administration client with ID %d attempted to connect from from %s, but failed with error %s.", session->get_id(), fz::join_host_and_port(session->peer_ip(), unsigned(session->peer_port())), fz::socket_error_description(err));
		else {
			logger_.log_u(fz::logmsg::status, L"Administration client with ID %d connected from %s", session->get_id(), fz::join_host_and_port(session->peer_ip(), unsigned(session->peer_port())));

			session->set_user_data(session_data{});

			session->enable_dispatching<administration::admin_login>(true);
			session->enable_sending<administration::admin_login::response>(true);
			session->set_max_buffer_size(administration::buffer_size_before_login);
		}
	}
}

void administrator::disconnection(administration::engine::session &s, int err)
{
	if (err)
		logger_.log_u(fz::logmsg::error, L"Administration client with ID %d disconnected with error %s", s.get_id(), fz::socket_error_description(err));
	else
		logger_.log_u(fz::logmsg::status, L"Administration client with ID %d disconnected without error", s.get_id());

	if (update_checker_)
		update_checker_->on_disconnection(s.get_id());
}

bool administrator::have_some_certificates_expired()
{
	auto s = server_settings_.lock();

	if (s->ftp_server.sessions().tls.cert.load_extra().expired())
		return true;

	if (s->admin.tls.cert.load_extra().expired())
		return true;

	return false;
}

void administrator::kick_disallowed_ips()
{
	std::vector<fz::tcp::session::id> ids;

	auto generate_ids = [&](const fz::tcp::session &s) {
		const auto &[addr, type] = s.get_peer_info();
		if (disallowed_ips_.contains(addr, type))
			ids.push_back(s.get_id());

		return true;
	};

	ftp_server_.iterate_over_sessions({}, std::cref(generate_ids));
	if (!ids.empty())
		ftp_server_.end_sessions(ids);
}

auto administrator::operator()(administration::end_sessions &&v)
{
	auto && [sessions] = std::move(v).tuple();

	auto num = ftp_server_.end_sessions(sessions);

	if(sessions.empty() || num > 0)
		return v.success(num);

	return v.failure();
}

void administrator::send_buffer_is_in_overflow(administration::engine::session &session)
{
	if (auto &sd = session.get_user_data<session_data>(); !sd.is_in_overflow) {
		sd.is_in_overflow = true;

		session.enable_sending<
			administration::session::user_name,
			administration::session::entry_open,
			administration::session::entry_close,
			administration::session::entry_written,
			administration::session::entry_read,
			administration::log,
			administration::listener_status
		>(false);

		session.send<administration::acknowledge_queue_full>();

		engine_logger_.log_u(fz::logmsg::debug_warning, L"Administrator: upload buffer has overflown! Silencing notifications until the client informs us it has exausted the queue.");
	}
}

std::unique_ptr<fz::tcp::session::notifier> administrator::make_notifier(fz::ftp::session::id id, const fz::datetime &start, const std::string &peer_ip, fz::address_type peer_address_type, fz::logger_interface &logger)
{
	return std::make_unique<notifier>(*this, id, start, peer_ip, peer_address_type, logger);
}

void administrator::listener_status(const fz::tcp::listener &listener)
{
	if (admin_server_.get_number_of_sessions() < 1)
		return;

	admin_server_.broadcast<administration::listener_status>(fz::datetime::now(), listener.get_address_info(), listener.get_status());
}

bool administrator::handle_new_admin_settings()
{
	auto server_settings = server_settings_.lock();

	auto &admin = server_settings->admin;

	bool enable_local_ipv4 = admin.local_port != 0;
	bool enable_local_ipv6 = admin.local_port != 0;

	std::vector<fz::rmp::address_info> address_info_list;

	if (admin.password) {
		static constexpr fz::hostaddress::ipv4_host local_ipv4 = *fz::hostaddress("127.0.0.1", fz::hostaddress::format::ipv4).ipv4();
		static constexpr fz::hostaddress::ipv6_host local_ipv6 = *fz::hostaddress("::1", fz::hostaddress::format::ipv6).ipv6();

		for (auto &i: admin.additional_address_info_list) {
			if (i.port != admin.local_port)
				continue;

			if (fz::hostaddress r(i.address, fz::hostaddress::format::ipv4); r.is_valid()) {
				if (*r.ipv4() == local_ipv4 || *r.ipv4() == fz::hostaddress::ipv4_host{})
					enable_local_ipv4 = false;
			}
			else
			if (fz::hostaddress r(i.address, fz::hostaddress::format::ipv6); r.is_valid()) {
				if (*r.ipv6() == local_ipv6 || *r.ipv6() == fz::hostaddress::ipv6_host{})
					enable_local_ipv6 = false;
			}
		}

		address_info_list.reserve(admin.additional_address_info_list.size() + enable_local_ipv4 + enable_local_ipv6);

		std::copy(admin.additional_address_info_list.begin(), admin.additional_address_info_list.end(), std::back_inserter(address_info_list));
	}
	else
	if (!admin.additional_address_info_list.empty()) {
		logger_.log_u(fz::logmsg::debug_warning, L"A list of listener is specified, but no valid password is set: this is not supported. Ignoring the provided listeners.");
	}

	if (enable_local_ipv4)
		address_info_list.push_back({{"127.0.0.1", admin.local_port}, true});

	if (enable_local_ipv6)
		address_info_list.push_back({{"::1", admin.local_port}, true});

	admin_server_.set_listen_address_infos(address_info_list);

	if (admin.tls.cert)
		admin.tls.cert.set_root_path(config_paths_.certificates());

	admin_server_.set_security_info(admin.tls);

	if (address_info_list.empty()) {
		logger_.log_u(fz::logmsg::debug_warning, L"No listeners were enabled. Will not serve!");
		return false;
	}

	return true;
}


auto administrator::operator()(administration::session::solicit_info &&v, administration::engine::session &session)
{
	auto && [session_ids] = std::move(v).tuple();

	// We cap the number of sessions to be retrieved to a given maximum, to avoid stalling the server.
	constexpr size_t max_num_info = 10'000;

	ftp_server_.iterate_over_sessions(session_ids, [&session, sent_so_far = std::size_t(0)](fz::ftp::session &s) mutable {
		if (sent_so_far == max_num_info)
			return false;

		auto notifier = static_cast<administrator::notifier *>(s.get_notifier());
		if (notifier)
			notifier->send_session_info(session);

		return true;
	});
}

auto administrator::operator()(administration::admin_login &&v, administration::engine::session &session)
{
	auto [password] = std::move(v).tuple();

	if (server_settings_.lock()->admin.password.verify(password)) {
		bool unsafe_ptrace_scope = false;
		#ifdef __linux__
			unsafe_ptrace_scope = fz::trimmed(fz::util::io::read(fzT("/proc/sys/kernel/yama/ptrace_scope")).to_view()) == "0";
		#endif
		session.send(v.success(
			fz::util::fs::native_format,
			unsafe_ptrace_scope,
			fz::impersonator::can_impersonate(),
			fz::util::get_current_user_name(),
			fz::get_network_interfaces(),
			have_some_certificates_expired(),
			fz::hostaddress::any_is_equivalent,
			!server_settings_.lock()->ftp_server.sessions().pasv.host_override.empty(),
			fz::build_info::version,
			fz::build_info::host
		));

		// Enable all messages
		session.enable_dispatching(true);
		session.enable_sending(true);

		// Except for the login ones, sice we're now logged in.
		session.enable_dispatching<administration::admin_login>(false);
		session.enable_sending<administration::admin_login::response>(false);

		session.set_max_buffer_size(administration::buffer_size_after_login);

		// Send ftp sessions info to the admin session
		operator()(administration::session::solicit_info{}, session);

		if (update_checker_) {
			// Send the last available update info
			session.send<administration::update_info>(update_checker_->get_last_checked_info(), update_checker_->get_next_check_dt());
		}

		return;
	}

	session.send(v.failure());
}


auto administrator::operator ()(administration::acknowledge_queue_full::response &&, administration::engine::session &session)
{
	if (auto &sd = session.get_user_data<session_data>(); sd.is_in_overflow) {
		sd.is_in_overflow = false;

		engine_logger_.log_u(fz::logmsg::debug_warning, L"Administrator: upload buffer has been emptied by the client! Enabling notifications again.");

		session.enable_sending<
			administration::session::user_name,
			administration::session::entry_open,
			administration::session::entry_close,
			administration::session::entry_written,
			administration::session::entry_read,
			administration::log,
			administration::listener_status
		>(true);

		// Send ftp sessions info to the admin session
		operator()(administration::session::solicit_info{}, session);
	}
}

/**********************************************************************/

void administrator::reload_config()
{
	invoke_later_([this] {
		fz::authentication::file_based_authenticator::groups groups;
		fz::authentication::file_based_authenticator::users users;
		fz::tcp::binary_address_list disallowed_ips, allowed_ips;
		server_settings server_settings;

		int err = 0;

		if (!err)
			err = authenticator_.load_into(groups, users);

		if (!err)
			err = server_settings_.load_into(server_settings);

		if (!err)
			err = disallowed_ips_.load_into(disallowed_ips);

		if (!err)
			err = allowed_ips_.load_into(allowed_ips);

		if (err) {
			logger_.log_u(fz::logmsg::error, L"Failed reloading configuration. Reason: %s.", fz::socket_error_description(err));
			return;
		}

		set_groups_and_users(std::move(groups), std::move(users));
		set_logger_options(std::move(server_settings.logger));
		set_ftp_options(std::move(server_settings.ftp_server));
		set_protocols_options(std::move(server_settings.protocols));
		set_admin_options(std::move(server_settings.admin));
		set_acme_options(std::move(server_settings.acme));
		set_ip_filters(std::move(disallowed_ips), std::move(allowed_ips), false);
		set_updates_options(std::move(server_settings.update_checker));

		logger_.log_u(fz::logmsg::status, L"Successfully reloaded configuration.");
	});
}

administrator::administrator(fz::tcp::server_context &context,
							 fz::event_loop_pool &loop_pool,
							 fz::logger::file &file_logger,
							 fz::logger::splitter &splitter_logger,
							 fz::ftp::server &ftp_server,
							 fz::tcp::automatically_serializable_binary_address_list &disallowed_ips,
							 fz::tcp::automatically_serializable_binary_address_list &allowed_ips,
							 fz::authentication::autobanner &autobanner,
							 fz::authentication::file_based_authenticator &authenticator,
							 fz::util::xml_archiver<server_settings> &server_settings,
							 fz::acme::daemon &acme,
							 const server_config_paths &config_paths)
	: forwarder<administrator>(*this)
	, server_context_(context)
	, file_logger_(file_logger)
	, splitter_logger_(splitter_logger)
	, engine_logger_(file_logger, "Administration Server")
	, logger_(splitter_logger, "Administration Server")
	, loop_pool_(loop_pool)
	, ftp_server_(ftp_server)
	, disallowed_ips_(disallowed_ips)
	, allowed_ips_(allowed_ips)
	, autobanner_(autobanner)
	, authenticator_(authenticator)
	, server_settings_(server_settings)
	, acme_(acme)
	, config_paths_(config_paths)
	, log_forwarder_(new log_forwarder(*this, 0))
	#if !defined(WITHOUT_FZ_UPDATE_CHECKER)
		, update_checker_(new update_checker(*this, config_paths.update() / fz::build_info::toString<fz::native_string>(fz::build_info::flavour) / fzT("cache"), server_settings_.lock()->update_checker))
	#endif
	, invoke_later_(context.loop())
	, admin_server_(context, *this, engine_logger_)
{
	log_forwarder_->set_all(fz::logmsg::type(~0));

	splitter_logger_.add_logger(*log_forwarder_);

	if (handle_new_admin_settings()) {
		ftp_server_.set_notifier_factory(*this);

		admin_server_.start();
	}

	if (update_checker_)
		update_checker_->start();
}

