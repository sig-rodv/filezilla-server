#include "../administrator.hpp"

auto administrator::operator()(administration::set_ftp_options &&v)
{
	auto &&[opts] = std::move(v).tuple();

	set_ftp_options(std::move(opts));

	server_settings_.save_later();

	return v.success();
}

auto administrator::operator()(administration::get_ftp_options &&v)
{
	auto && [export_cert] = v.tuple();

	auto s = server_settings_.lock();

	auto &&ftp_server = export_cert ? fz::ftp::server::options(s->ftp_server) : s->ftp_server;

	if (export_cert)
		ftp_server.sessions().tls.cert = s->ftp_server.sessions().tls.cert.generate_exported();

	return v.success(ftp_server, ftp_server.sessions().tls.cert.load_extra(&logger_));
}

void administrator::set_ftp_options(fz::ftp::server::options &&opts)
{
	auto server_settings = server_settings_.lock();

	acme_.set_certificate_used_status(server_settings->ftp_server.sessions().tls.cert, false);

	server_settings->ftp_server = std::move(opts);

	if (server_settings->ftp_server.sessions().tls.cert) {
		server_settings->ftp_server.sessions().tls.cert.set_root_path(config_paths_.certificates());
		acme_.set_certificate_used_status(server_settings->ftp_server.sessions().tls.cert, true);
	}

	ftp_server_.set_options(server_settings->ftp_server);
}

FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::get_ftp_options);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::set_ftp_options);
