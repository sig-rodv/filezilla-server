#include "../administrator.hpp"

auto administrator::operator()(administration::generate_selfsigned_certificate &&v)
{
	auto [id, distinguished_name, hostnames] = std::move(v).tuple();

	auto cert = fz::securable_socket::cert_info::generate_selfsigned(config_paths_.certificates(), &logger_, {}, distinguished_name, hostnames);

	if (cert)
		return v.success(id, std::move(cert), cert.load_extra(&logger_));

	// FIXME: Ideally we should return a proper failure() here, but then there'd be no way to distinguish between ids.
	return v.success(id, fz::securable_socket::cert_info{}, fz::securable_socket::cert_info::extra{});
}

auto administrator::operator()(administration::upload_certificate &&v)
{
	auto [id, cert, key, password] = std::move(v).tuple();

	auto info = fz::securable_socket::cert_info::import_certificate_files(config_paths_.certificates(), cert, key, password, &logger_);

	if (info)
		return v.success(id, std::move(info), info.load_extra(&logger_));

	// FIXME: Ideally we should return a proper failure() here, but then there'd be no way to distinguish between ids.
	return v.success(id, fz::securable_socket::cert_info{}, fz::securable_socket::cert_info::extra{});
}

auto administrator::operator()(administration::get_extra_certs_info &&v)
{
	auto && [id, info] = std::move(v).tuple();

	info.set_root_path(config_paths_.certificates());

	return v.success(id, std::move(info), info.load_extra());
}

FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::generate_selfsigned_certificate);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::upload_certificate);
FZ_RMP_INSTANTIATE_HERE_DISPATCHING_FOR(administration::engine, administrator, administration::get_extra_certs_info);
