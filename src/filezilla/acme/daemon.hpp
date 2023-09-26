#ifndef FZ_ACME_DAEMON_HPP
#define FZ_ACME_DAEMON_HPP

#include <libfilezilla/uri.hpp>
#include <libfilezilla/thread_pool.hpp>
#include <libfilezilla/event_loop.hpp>
#include <libfilezilla/tls_system_trust_store.hpp>

#include "../logger/modularized.hpp"

#include "cert_info.hpp"

#include "client.hpp"

namespace fz::acme
{

class daemon: private fz::event_handler
{
public:
	daemon(thread_pool &pool, event_loop &loop, logger_interface &logger, tls_system_trust_store &trust_store);
	~daemon() override;

	void set_root_path(const util::fs::native_path &root_path);
	void set_how_to_serve_challenges(const serve_challenges::how &how_to_serve_challenges);

	void set_certificate_used_status(securable_socket::cert_info ci, bool used);

	using error_handler = std::function<void(std::string error)>;
	using get_terms_of_service_handler = std::function<void(std::string terms_of_service)>;
	using create_account_handler = std::function<void(std::string account_id)>;
	using restore_account_handler = std::function<void()>;
	using create_certificate_handler = std::function<void(securable_socket::cert_info ci)>;

	void get_terms_of_service(const fz::uri &directory, get_terms_of_service_handler, error_handler);
	void create_account(const fz::uri &directory, const std::vector<std::string> &contacts, create_account_handler, error_handler);
	void restore_account(const std::string &account_id, const extra_account_info &extra, restore_account_handler, error_handler);
	void create_certificate(const std::string &account_id, const serve_challenges::how &how_to_serve_challenges, const std::vector<std::string> &hosts, fz::duration allowed_max_server_time_difference, create_certificate_handler, error_handler);

	extra_account_info load_extra_account_info(const std::string &account_id) const;

private:
	util::fs::native_path root_path_;
	serve_challenges::how how_to_serve_challenges_;

	logger::modularized logger_;
	acme::client client_;

private:
	void on_terms(acme::client::opid_t, std::string &terms);
	void on_certificate(acme::client::opid_t, uri &, std::string &kid, std::vector<std::string> &hosts, std::string &key, std::string &certs);
	void on_error(acme::client::opid_t i, const std::string &error);
	void on_account(acme::client::opid_t id, fz::uri &directory, std::string &kid, std::pair<json, json> &jwk, json &object);
	void on_timer(timer_id id);

	void operator()(const event_base &ev) override;

	void try_to_renew_expiring_certs_at(fz::datetime dt);
	void try_to_renew_expiring_certs();

	fz::mutex mutex_;

	struct terms_handlers: std::pair<get_terms_of_service_handler, error_handler> { using pair::pair; };
	struct acct_handlers: std::pair<create_account_handler, error_handler> { using pair::pair; };
	struct cert_handlers: std::pair<create_certificate_handler, error_handler> { using pair::pair; };

	using any_handlers = std::variant<
		terms_handlers,
		acct_handlers,
		cert_handlers
	>;

	std::map<acme::client::opid_t, any_handlers> id2handlers_;
	timer_id timer_id_{};
	datetime renewal_dt_{};

	std::list<securable_socket::cert_info> used_certs_;
};

}
#endif // FZ_ACME_DAEMON_HPP
