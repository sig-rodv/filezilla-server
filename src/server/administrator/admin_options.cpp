#include "../administrator.hpp"

auto administrator::operator()(administration::set_admin_options &&v)
{
	auto &&[opts] = std::move(v).tuple();

	set_admin_options(std::move(opts));

	server_settings_.save_later();

	return v.success();
}

auto administrator::operator()(administration::get_admin_options &&v)
{
	auto && [export_cert] = std::move(v).tuple();

	auto s = server_settings_.lock();

	auto &&admin = export_cert ? server_settings::admin_options(s->admin) : s->admin;

	if (export_cert)
		admin.tls.cert = admin.tls.cert.generate_exported();

	return v.success(admin, admin.tls.cert.load_extra(&logger_));
}

void administrator::set_admin_options(server_settings::admin_options &&opts)
{
	{
		auto server_settings = server_settings_.lock();

		acme_.set_certificate_used_status(server_settings->admin.tls.cert, false);
		server_settings->admin = std::move(opts);
		acme_.set_certificate_used_status(server_settings->admin.tls.cert, true);
	}

	handle_new_admin_settings();
}

FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::get_admin_options);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::set_admin_options);
