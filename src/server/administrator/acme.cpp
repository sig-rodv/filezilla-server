#include "../administrator.hpp"

auto administrator::operator()(administration::set_acme_options &&v)
{
	auto &&[opts] = std::move(v).tuple();

	set_acme_options(std::move(opts));

	server_settings_.save_later();

	return v.success();
}

auto administrator::operator()(administration::get_acme_options &&v)
{
	auto s = server_settings_.lock();

	return v.success(s->acme, acme_.load_extra_account_info(s->acme.account_id));
}

void administrator::set_acme_options(server_settings::acme_options &&opts)
{
	auto server_settings = server_settings_.lock();

	server_settings->acme = std::move(opts);
	acme_.set_how_to_serve_challenges(server_settings->acme.how_to_serve_challenges);
}

template <typename Command>
auto administrator::acme_error(administration::engine::session::id id, const std::string &cmd)
{
	return [this, id = id, cmd = cmd] (const std::string &error) {
		logger_.log_u(fz::logmsg::error, L"Error processing %s: %s", cmd, error);
		auto s = admin_server_.get_session(id);
		if (!s)
			return;

		s->send(Command::failure(error));
	};
}

#define ACME_ERROR(id, cmd) acme_error<administration::cmd>(id, #cmd)

auto administrator::operator()(administration::get_acme_terms_of_service &&v, administration::engine::session &session)
{
	auto [directory] = v.tuple();

	acme_.get_terms_of_service(fz::uri(directory),
		[this, id = session.get_id()] (const std::string &terms){
			auto s = admin_server_.get_session(id);
			if (!s)
				return;

			s->send(administration::get_acme_terms_of_service::success(terms));
		},
		ACME_ERROR(session.get_id(), get_acme_terms_of_service)
	);
}



auto administrator::operator()(administration::generate_acme_account &&v, administration::engine::session &session)
{
	auto &&[directory, contacts, terms_of_service_agreed] = std::move(v).tuple();

	if (!terms_of_service_agreed) {
		session.send(v.failure("You must agree to the terms of service."));
		return;
	}

	acme_.create_account(fz::uri(directory), contacts,
		[this, id = session.get_id(), directory = directory, contacts = contacts] (const std::string &account_id) {
			auto s = admin_server_.get_session(id);
			if (!s)
				return;

			s->send(administration::generate_acme_account::success(account_id, acme_.load_extra_account_info(account_id)));
		},
		ACME_ERROR(session.get_id(), generate_acme_account)
	);
}

auto administrator::operator()(administration::restore_acme_account &&v, administration::engine::session &session)
{
	auto &&[account_id, extra] = std::move(v).tuple();

	acme_.restore_account(account_id, extra,
		[this, id = session.get_id()] () {
			auto s = admin_server_.get_session(id);
			if (!s)
				return;

			s->send(administration::restore_acme_account::success());
		},
		ACME_ERROR(session.get_id(), restore_acme_account)
	);
}

auto administrator::operator()(administration::generate_acme_certificate &&v, administration::engine::session &session)
{
	auto &&[reqid, how_to_serve_challenges, info] = std::move(v).tuple();

	acme_.create_certificate(info.account_id, how_to_serve_challenges, info.hostnames, fz::duration::from_milliseconds(0),
		[this, id = session.get_id(), reqid = reqid] (fz::securable_socket::cert_info &&info) {
			auto s = admin_server_.get_session(id);
			if (!s)
				return;

			s->send(administration::generate_acme_certificate::success(reqid, std::move(info), info.load_extra()));
		},
		[this, id = session.get_id(), reqid = reqid] (const fz::securable_socket::cert_info &info) {
   			 auto s = admin_server_.get_session(id);
  			  if (!s)
    		    return;

   			 s->send(administration::generate_acme_certificate::success(reqid, info, info.load_extra()));
}
		}
	);
}

FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::get_acme_options);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::set_acme_options);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::get_acme_terms_of_service);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::generate_acme_account);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::restore_acme_account);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::generate_acme_certificate);
