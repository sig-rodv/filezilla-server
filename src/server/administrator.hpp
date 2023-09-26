#ifndef ADMINISTRATOR_HPP
#define ADMINISTRATOR_HPP

#include <libfilezilla/thread_pool.hpp>
#include <libfilezilla/event_loop.hpp>

#include "../filezilla/logger/modularized.hpp"
#include "../filezilla/logger/splitter.hpp"

#include "../filezilla/ftp/server.hpp"
#include "../filezilla/tcp/automatically_serializable_binary_address_list.hpp"

#include "../server/administration.hpp"
#include "../filezilla/rmp/engine/forwarder.hpp"
#include "../filezilla/rmp/engine/server.hpp"

#include "../filezilla/util/xml_archiver.hpp"

#include "../filezilla/acme/daemon.hpp"

#include "../filezilla/util/invoke_later.hpp"

#ifdef HAVE_CONFIG_H
#	include "config.hpp"
#endif

class administrator
	: private administration::engine::forwarder<administrator>
	, private fz::tcp::session::notifier::factory
{
public:
	administrator(fz::tcp::server_context &context,
				  fz::event_loop_pool &loop_pool,
				  fz::logger::file &file_logger,
				  fz::logger::splitter &nonsession_logger,
				  fz::ftp::server &ftp_server,
				  fz::tcp::automatically_serializable_binary_address_list &disallowed_ips,
				  fz::tcp::automatically_serializable_binary_address_list &allowed_ips,
				  fz::authentication::autobanner &autobanner,
				  fz::authentication::file_based_authenticator &authenticator,
				  fz::util::xml_archiver<server_settings> &server_settings,
				  fz::acme::daemon &acme,
				  const server_config_paths &config_paths);

	~administrator() override;

	void reload_config();

private:
	void set_protocols_options(server_settings::protocols_options &&opts);
	void set_logger_options(fz::logger::file::options &&opts);
	void set_groups_and_users(fz::authentication::file_based_authenticator::groups &&groups, fz::authentication::file_based_authenticator::users &&users);
	void set_ftp_options(fz::ftp::server::options &&opts);
	void set_admin_options(server_settings::admin_options &&opts);
	void set_acme_options(server_settings::acme_options &&opts);
	void set_ip_filters(fz::tcp::binary_address_list &&disallowed, fz::tcp::binary_address_list &&allowed, bool save);
	void set_updates_options(fz::update::checker::options &&opts);

private:
	friend administration::engine::access;

	void send_buffer_is_in_overflow(administration::engine::session &session) override;

	auto operator()(administration::ban_ip &&);
	auto operator()(administration::end_sessions &&v);
	auto operator()(administration::session::solicit_info &&v, administration::engine::session &session);
	auto operator()(administration::get_groups_and_users &&v);
	auto operator()(administration::set_groups_and_users &&v);
	auto operator()(administration::get_ip_filters &&v);
	auto operator()(administration::set_ip_filters &&v);
	auto operator()(administration::get_ftp_options &&v);
	auto operator()(administration::set_ftp_options &&v);
	auto operator()(administration::get_protocols_options &&v);
	auto operator()(administration::set_protocols_options &&v);
	auto operator()(administration::get_admin_options &&v);
	auto operator()(administration::set_admin_options &&v);
	auto operator()(administration::get_logger_options &&v);
	auto operator()(administration::set_logger_options &&v);
	auto operator()(administration::get_acme_options &&v);
	auto operator()(administration::set_acme_options &&v);
	auto operator()(administration::acknowledge_queue_full::response &&v, administration::engine::session &session);
	auto operator()(administration::admin_login &&v, administration::engine::session &session);
	auto operator()(administration::generate_selfsigned_certificate &&v);
	auto operator()(administration::upload_certificate &&v);
	auto operator()(administration::get_extra_certs_info &&v);

	auto operator()(administration::get_acme_terms_of_service &&v, administration::engine::session &session);
	auto operator()(administration::generate_acme_account &&v, administration::engine::session &session);
	auto operator()(administration::restore_acme_account &&v, administration::engine::session &session);
	auto operator()(administration::generate_acme_certificate &&v, administration::engine::session &session);

#if !defined(WITHOUT_FZ_UPDATE_CHECKER)
	auto operator()(administration::set_updates_options &&v);
	auto operator()(administration::get_updates_options &&v);
	auto operator()(administration::retrieve_update_raw_data::response &&v, administration::engine::session &session);
	auto operator()(administration::solicit_update_info &&v);
#endif

	void connection(administration::engine::session *, int err) override;
	void disconnection(administration::engine::session &, int) override;

private:
	template <typename Command>
	auto acme_error(administration::engine::session::id id, const std::string &cmd);
	bool have_some_certificates_expired();
	void kick_disallowed_ips();

private:
	class log_forwarder;
	class notifier;
	class update_checker;

	std::unique_ptr<fz::tcp::session::notifier> make_notifier(fz::ftp::session::id id, const fz::datetime &start, const std::string &peer_ip, fz::address_type peer_address_type, fz::logger_interface &logger) override;

	void listener_status(const fz::tcp::listener &listener) override;

	bool handle_new_admin_settings();

private:
	fz::tcp::server_context &server_context_;
	fz::logger::file &file_logger_;
	fz::logger::splitter &splitter_logger_;

	fz::logger::modularized engine_logger_;
	fz::logger::modularized logger_;

	fz::event_loop_pool &loop_pool_;
	fz::ftp::server &ftp_server_;
	fz::tcp::automatically_serializable_binary_address_list &disallowed_ips_;
	fz::tcp::automatically_serializable_binary_address_list &allowed_ips_;
	fz::authentication::autobanner &autobanner_;
	fz::authentication::file_based_authenticator &authenticator_;
	fz::util::xml_archiver<server_settings> &server_settings_;
	fz::acme::daemon &acme_;
	const server_config_paths &config_paths_;
	std::unique_ptr<log_forwarder> log_forwarder_;
	std::unique_ptr<update_checker> update_checker_;

	fz::util::invoker_handler invoke_later_;

	administration::engine::server admin_server_;

private:
	struct session_data;
};

// Instantiate the loading and dispatching code externally,
// so that compilation time is reduced and the assembler on mingw doesn't complain about the string table being overflown.

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_ftp_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_ftp_options);

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_ip_filters);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_ip_filters);

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_groups_and_users);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_groups_and_users);

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_admin_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_admin_options);

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_protocols_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_protocols_options);

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_logger_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_logger_options);

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_acme_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_acme_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_acme_terms_of_service);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::generate_acme_account);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::restore_acme_account);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::generate_acme_certificate);

FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::generate_selfsigned_certificate);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::upload_certificate);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_extra_certs_info);

#if !defined(WITHOUT_FZ_UPDATE_CHECKER)
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::get_updates_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::set_updates_options);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::solicit_update_info);
FZ_RMP_INSTANTIATE_EXTERNALLY_DISPATCHING_FOR(administration::engine, administrator, administration::retrieve_update_raw_data::response);
#endif

#endif // ADMINISTRATOR_HPP
