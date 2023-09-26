#include <libfilezilla/jws.hpp>
#include <libfilezilla/local_filesys.hpp>

#include "../util/io.hpp"
#include "../string.hpp"
#include "../logger/scoped.hpp"

#include "daemon.hpp"

namespace fz::acme
{

namespace {

	auto retry_delay = fz::duration::from_minutes(5);
	auto maximum_allowed_time_difference_from_acme_server = fz::duration::from_days(1);

}

daemon::daemon(thread_pool &pool, event_loop &loop, logger_interface &logger, tls_system_trust_store &trust_store)
	: fz::event_handler(loop)
	, logger_(logger, "ACME Daemon")
	, client_(pool, loop, logger, trust_store)
{
}

daemon::~daemon()
{
	remove_handler();
}

void daemon::set_root_path(const util::fs::native_path &root_path)
{
	fz::scoped_lock lock(mutex_);
	root_path_ = root_path;

	try_to_renew_expiring_certs();
}

void daemon::set_how_to_serve_challenges(const serve_challenges::how &how_to_serve_challenges)
{
	fz::scoped_lock lock(mutex_);
	how_to_serve_challenges_ = how_to_serve_challenges;

	try_to_renew_expiring_certs();
}

void daemon::set_certificate_used_status(securable_socket::cert_info ci, bool used)
{
	if (auto acme = ci.acme()) {
		fz::scoped_lock lock(mutex_);

		std::sort(acme->hostnames.begin(), acme->hostnames.end());

		auto it = std::find_if(used_certs_.begin(), used_certs_.end(), [acme](const securable_socket::cert_info &ci) {
			return std::tie(ci.acme()->account_id, ci.acme()->hostnames) == std::tie(acme->account_id, acme->hostnames);
		});

		if (it != used_certs_.end()) {
			if (used)
				return;

			used_certs_.erase(it);
		}
		else {
			used_certs_.push_back(std::move(ci));
		}

		try_to_renew_expiring_certs();
	}
}

void daemon::get_terms_of_service(const fz::uri &directory, daemon::get_terms_of_service_handler terms_handler, daemon::error_handler error_handler)
{
	auto id = client_.get_terms_of_service(directory, *this);
	if (!id) {
		if (error_handler)
			error_handler("Could not execute acme::client::get_terms_of_service.");
		return;
	}

	fz::scoped_lock lock(mutex_);
	id2handlers_[id] = terms_handlers{ std::move(terms_handler), std::move(error_handler) };
}

void daemon::create_account(const uri &directory, const std::vector<std::string> &contacts, daemon::create_account_handler account_handler, daemon::error_handler error_handler)
{
	fz::scoped_lock lock(mutex_);

	if (!root_path_.is_absolute()) {
		if (error_handler)
			error_handler("acme::daemon: root path is not absolute.");
		return;
	}

	auto id = client_.get_account(directory, contacts, create_jwk(), false, *this);
	if (!id) {
		if (error_handler)
			error_handler("Could not execute acme::client::get_account.");
		return;
	}

	id2handlers_[id] = acct_handlers{ std::move(account_handler), std::move(error_handler) };
}

void daemon::restore_account(const std::string &account_id, const extra_account_info &extra, restore_account_handler restore_handler, error_handler error_handler)
{
	if (!root_path_.is_absolute()) {
		if (error_handler)
			error_handler("acme::daemon: root path is not absolute.");
		return;
	}

	if (!extra.save(root_path_, account_id)) {
		if (error_handler)
			error_handler("acme::daemon: failed restoring account.");
		return;
	}

	if (restore_handler)
		restore_handler();

	return;
}

void daemon::create_certificate(
	const std::string &account_id,
	const serve_challenges::how &how_to_serve_challenges,
	const std::vector<std::string> &hosts,
	fz::duration allowed_max_server_time_difference,
	daemon::create_certificate_handler cert_handler, daemon::error_handler error_handler
)
{
	fz::scoped_lock lock(mutex_);

	if (!root_path_.is_absolute()) {
		if (error_handler)
			error_handler("acme::daemon: root path is not absolute.");
		return;
	}

	extra_account_info extra = load_extra_account_info(account_id);
	if (!extra)
		error_handler("acme::deaemon: could not read or parse account info");

	auto id = client_.get_certificate(fz::uri(extra.directory), extra.jwk, hosts, how_to_serve_challenges, allowed_max_server_time_difference, *this);
	if (!id) {
		if (error_handler)
			error_handler("Could not execute acme::client::get_account.");
		return;
	}

	id2handlers_[id] = cert_handlers{ std::move(cert_handler), std::move(error_handler) };
}

extra_account_info daemon::load_extra_account_info(const std::string &account_id) const
{
	return extra_account_info::load(root_path_, account_id);
}

void daemon::on_terms(acme::client::opid_t id, std::string &terms)
{
	get_terms_of_service_handler th;

	{
		fz::scoped_lock lock(mutex_);
		if (auto it = id2handlers_.find(id); it != id2handlers_.end()) {
			th = std::move(std::get_if<terms_handlers>(&it->second)->first);

			id2handlers_.erase(it);
		}
	}

	if (th)
		th(terms);
}

void daemon::on_certificate(acme::client::opid_t id, fz::uri &, std::string &kid, std::vector<std::string> &hosts, std::string &key, std::string &certs)
{
	create_certificate_handler ch;
	error_handler eh;

	fz::scoped_lock lock(mutex_);

	if (auto it = id2handlers_.find(id); it != id2handlers_.end()) {
		ch = std::move(std::get_if<cert_handlers>(&it->second)->first);
		eh = std::move(std::get_if<cert_handlers>(&it->second)->second);

		id2handlers_.erase(it);
	}

	securable_socket::cert_info ci = securable_socket::acme_cert_info{ std::move(kid), std::move(hosts) };
	ci.set_root_path(root_path_);

	if (!fz::mkdir(ci.key_path().parent(), true, mkdir_permissions::cur_user_and_admins)) {
		if (eh)
			eh("Could not create certificate directory.");

		return;
	}

	auto written = util::io::write(file(ci.key_path(), file::writing, file::empty | file::current_user_and_admins_only), key);
	if (!written) {
		if (eh)
			eh("Could not write cert key to file.");

		return;
	}

	written = util::io::write(file(ci.certs_path(), file::writing, file::empty | file::current_user_and_admins_only), certs);
	if (!written) {
		if (eh)
			eh("Could not write certs chain to file.");

		return;
	}

	if (ch)
		ch(std::move(ci));

	try_to_renew_expiring_certs_at(fz::datetime::now());
}

void daemon::on_error(acme::client::opid_t id, const std::string &error)
{
	error_handler h;

	{
		fz::scoped_lock lock(mutex_);
		if (auto it = id2handlers_.find(id); it != id2handlers_.end()) {
			h = std::visit([](auto &handlers) { return std::move(handlers.second); }, it->second);

			id2handlers_.erase(it);
		}
	}

	if (h)
		h(error);
}

void daemon::on_account(acme::client::opid_t id, fz::uri &directory, std::string &kid, std::pair<json, json> &jwk, json &object)
{
	create_account_handler ah;
	error_handler eh;

	fz::scoped_lock lock(mutex_);

	if (auto it = id2handlers_.find(id); it != id2handlers_.end()) {
		ah = std::move(std::get_if<acct_handlers>(&it->second)->first);
		eh = std::move(std::get_if<acct_handlers>(&it->second)->second);

		id2handlers_.erase(it);
	}

	json info;
	info["directory"] = std::move(directory.to_string());
	info["kid"] = kid;
	info["jwk"]["priv"] = std::move(jwk.first);
	info["jwk"]["pub"] = std::move(jwk.second);
	info["contact"] = std::move(object["contact"]);
	info["createdAt"] = std::move(object["createdAt"]);

	securable_socket::cert_info ci = securable_socket::acme_cert_info{ kid, {} };
	ci.set_root_path(root_path_);

	auto account_dir = ci.key_path().parent().parent();
	if (auto res = fz::mkdir(account_dir, true, mkdir_permissions::cur_user_and_admins); !res) {
		if (eh)
			eh(fz::sprintf("Could not create account directory \"%s\", error: %d", account_dir.str(), res.error_));

		return;
	}

	auto account_info = account_dir / fzT("account.info");

	if (fz::local_filesys::get_size(account_info) != -1) {
		if (eh)
			eh("Account file already exists. This is likely due to a very unlikely MD5 clash. Try again, you should be luckier.");

		return;
	}

	auto written = util::io::write(file(account_info, file::writing, file::empty | file::current_user_and_admins_only), info.to_string());
	if (!written) {
		if (eh)
			eh("Could not write account data to file.");

		return;
	}

	if (ah)
		ah(std::move(kid));
}

void daemon::on_timer(timer_id)
{
	if (renewal_dt_ > datetime::now())
		return try_to_renew_expiring_certs_at(renewal_dt_);

	try_to_renew_expiring_certs();
}

void daemon::operator()(const event_base &ev)
{
	fz::dispatch<
		acme::client::terms_of_service_event,
		acme::client::account_event,
		acme::client::error_event,
		acme::client::certificate_event,
		timer_event
	>(ev, this,
		&daemon::on_terms,
		&daemon::on_account,
		&daemon::on_error,
		&daemon::on_certificate,
		&daemon::on_timer
	  );
}

void daemon::try_to_renew_expiring_certs_at(datetime dt)
{
	renewal_dt_ = dt;

	stop_timer(timer_id_);

	auto delta = dt - datetime::now();

	if (delta > duration::from_days(1))
		delta = duration::from_days(1);
	else
	if (delta > duration::from_minutes(1))
		delta = duration::from_minutes(1);

	timer_id_ = add_timer(delta, true);
}

void daemon::try_to_renew_expiring_certs()
{
	static auto get_hostnames = [](const auto &alt_subjects) {
		std::vector<std::string> hostnames;
		hostnames.reserve(alt_subjects.size());

		for (const auto &s: alt_subjects)
			if (s.is_dns)
				hostnames.push_back(s.name);

		return hostnames;
	};

	fz::scoped_lock lock(mutex_);

	if (!how_to_serve_challenges_ || !root_path_.is_absolute())
		return;

	std::string account_id_of_cert_to_renew;
	std::vector<std::string> domains_of_cert_to_renew;
	util::fs::native_path dir_of_cert_to_renew;
	auto next_renew_date = fz::datetime(0, datetime::milliseconds) + fz::duration::from_milliseconds(std::numeric_limits<int64_t>::max());

	for (auto &account_dir: only_dirs(root_path_ / fzT("acme"))) {
		auto account_info_s = util::io::read(file(account_dir / fzT("account.info"), file::reading));
		auto account_id = fz::json::parse(account_info_s.to_view())["kid"].string_value();
		if (account_id.empty())
			continue;

		for (auto &cert_dir: only_dirs(account_dir)) {
			const auto &cert_pem = cert_dir / fzT("cert.pem");

			bool in_use = false;
			for (auto &ci: used_certs_) {
				ci.set_root_path(root_path_);
				if (ci.certs_path() == cert_pem) {
					in_use = true;
					break;
				}
			}

			if (!in_use) {
				logger_.log_u(logmsg::debug_verbose, "Certificate %s is not currently in use, skipping it", cert_pem.str());
				continue;
			}

			auto certs = fz::load_certificates_file(cert_pem, true, true, nullptr);
			if (certs.empty())
				continue;

			auto last_error = fz::local_filesys::get_modification_time(cert_dir / fzT(".last_error"));
			auto renew_date = last_error
				? last_error + retry_delay
				: certs[0].get_activation_time() + fz::duration::from_milliseconds((certs[0].get_expiration_time() - certs[0].get_activation_time()).get_milliseconds()/3*2);

			if (renew_date < next_renew_date) {
				next_renew_date = renew_date;
				account_id_of_cert_to_renew = account_id;
				domains_of_cert_to_renew = get_hostnames(certs[0].get_alt_subject_names());
				dir_of_cert_to_renew = cert_dir;
			}
		}
	}

	if (!account_id_of_cert_to_renew.empty()) {
		logger_.log_u(logmsg::status, L"Next certificate to be renewed is registered with the account [%s], for the domains [%s].", account_id_of_cert_to_renew, fz::join<std::string>(domains_of_cert_to_renew));

		if (next_renew_date <= datetime::now()) {
			logger_.log_u(logmsg::status, L"Starting renewal of certificate NOW.");

			create_certificate(account_id_of_cert_to_renew, how_to_serve_challenges_, domains_of_cert_to_renew, maximum_allowed_time_difference_from_acme_server,
				[this, dir_of_cert_to_renew, domains_of_cert_to_renew, account_id_of_cert_to_renew](const auto &){
					logger_.log_u(logmsg::status, L"Finished renewal of certificate for the domains [%s], registered with the account [%s]. SUCCESS.", fz::join<std::string>(domains_of_cert_to_renew), account_id_of_cert_to_renew);

					fz::remove_file(dir_of_cert_to_renew / fzT(".last_error"));
					try_to_renew_expiring_certs_at(datetime::now());
				},

				[this, dir_of_cert_to_renew, domains_of_cert_to_renew, account_id_of_cert_to_renew](const auto &error) {
					logger_.log_u(logmsg::error, L"Finished renewal of certificate for the domains [%s], registered with the account [%s]. FAILED.", fz::join<std::string>(domains_of_cert_to_renew), account_id_of_cert_to_renew);
					logger_.log_u(logmsg::error, L"Retrying in %d seconds.", retry_delay.get_seconds());

					util::io::write(file(dir_of_cert_to_renew / fzT(".last_error"), file::writing, file::empty | file::current_user_and_admins_only), error);
					try_to_renew_expiring_certs_at(datetime::now());
				}
			);
		}
		else {
			logger_.log_u(logmsg::status, L"It will be renewed on the date [%s].", next_renew_date.get_rfc822());
			try_to_renew_expiring_certs_at(next_renew_date);
		}
	}
}


}
