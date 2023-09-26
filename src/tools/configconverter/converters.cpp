#include "converters.hpp"
#include "util.hpp"

namespace {

	template <typename T>
	std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, T>
	as(const std::string &s)
	{
		return fz::to_integral<T>(s);
	}

	template <typename T>
	std::enable_if_t<std::is_same_v<std::string, T>, const std::string &>
	as(const std::string &s)
	{
		return s;
	}

	template <typename T>
	[[maybe_unused]] std::enable_if_t<std::is_same_v<std::wstring, T>, T>
	as(const std::string &s)
	{
		return fz::to_wstring(s);
	}

	template <typename T>
	[[maybe_unused]] std::enable_if_t<std::is_same_v<std::string_view, T>, T>
	as(const std::string &s)
	{
		return s;
	}

	template <typename T>
	[[maybe_unused]] std::enable_if_t<std::is_same_v<fz::duration, T>, T>
	as(const std::string &s)
	{
		return fz::duration::from_seconds(as<std::int64_t>(s));
	}

	struct any: std::string
	{
		using std::string::string;

		any(const std::string &s)
			: std::string(s)
		{}

		template <typename T>
		operator T() const { return as<T>(*this); }
	};

	template <typename T>
	std::enable_if_t<std::is_same_v<any, T>, any>
	as(const std::string &s)
	{
		return s;
	}

	template <typename T = any, typename C, typename K>
	T get(C && c, K && k, T && d = {})
	{
		auto it = std::forward<C>(c).find(std::forward<K>(k));
		if (it != std::forward<C>(c).end())
			return as<T>(it->second);

		return std::forward<T>(d);
	}

	void convert(const old_config::UserOrGroup::Permissions &old, fz::tvfs::mount_table &mt)
	{
		auto get_access = [](const old_config::UserOrGroup::Permission &p) {
			bool can_modify =
				get<bool>(p.options, old_config::key::FileWrite)  ||
				get<bool>(p.options, old_config::key::FileDelete) ||
				get<bool>(p.options, old_config::key::FileAppend) ||
				get<bool>(p.options, old_config::key::DirCreate)  ||
				get<bool>(p.options, old_config::key::DirDelete);

			bool can_read =
					get<bool>(p.options, old_config::key::FileRead)  ||
					get<bool>(p.options, old_config::key::DirList);

			return can_modify
				? fz::tvfs::mount_point::read_write
				: can_read
					? fz::tvfs::mount_point::read_only
					: fz::tvfs::mount_point::disabled;
		};

		auto get_recursive = [](const old_config::UserOrGroup::Permission &p) {
			bool can_modify_structure =
				get<bool>(p.options, old_config::key::DirCreate) ||
				get<bool>(p.options, old_config::key::DirDelete);

			bool is_recursive =
				get<bool>(p.options, old_config::key::DirSubdirs);

			return is_recursive
				? can_modify_structure
					? fz::tvfs::mount_point::apply_permissions_recursively_and_allow_structure_modification
					: fz::tvfs::mount_point::apply_permissions_recursively
				: fz::tvfs::mount_point::do_not_apply_permissions_recursively;
		};

		auto get_flags = [](const old_config::UserOrGroup::Permission &p) {
			fz::tvfs::mount_point::flags_t flags{};

			if (get<bool>(p.options, old_config::key::AutoCreate))
				flags |= fz::tvfs::mount_point::autocreate;

			return flags;
		};

		for (const auto &[native_path, p]: old) {
			fz::util::fs::windows_native_path wnp(native_path);

			if (!wnp.is_absolute()) {
				if (wnp.str().back() == ':')
					wnp = wnp.str() + fzT("\\");

				if (!wnp.is_absolute()) {
					errout("WARNING: Permission path [%s] is not absolute. Ignoring it.\n", native_path);
					continue;
				}
			}

			static const std::vector<std::string> root_alias = { "/" };

			for (const auto &tvfs_path: p.aliases.empty() ? root_alias : p.aliases) {
				if (!fz::util::fs::unix_path(tvfs_path).is_absolute()) {
					errout("WARNING: Alias path [%s] is not absolute. Ignoring it.\n", tvfs_path);
					continue;
				}

				mt.push_back({tvfs_path, wnp, get_access(p), get_recursive(p), get_flags(p)});
			}
		}
	}

	template <old_config::UserOrGroup::SpeedLimits::type SpeedLimitsType>
	bool match_speed_limit(old_config::UserOrGroup::SpeedLimits::type v, const std::string_view &what) {
		switch (v) {
			case old_config::UserOrGroup::SpeedLimits::rules_limits:
				errout("WARNING: parsing speed limits for [%s]: rule based speed limits aren't currently supported. Ignoring.\n", what);
				break;

			case old_config::UserOrGroup::SpeedLimits::constant_limits:
				errout("WARNING: parsing speed limits for [%s]: constant speed limits don't override the Server or the parent Group ones, they work together with them. This is different than how the old server worked.\n", what);
				break;

			case old_config::UserOrGroup::SpeedLimits::no_limits:
				errout("WARNING: parsing speed limits for [%s]: even if this entry has no limits, the parent Group or Server limits still apply. This is different than how the old server worked.\n", what);
				break;

			case old_config::UserOrGroup::SpeedLimits::default_limits:
				break;
		}

		return v == SpeedLimitsType;
	}

	template <old_config::ServerSpeedLimits SpeedLimitsType>
	bool match_speed_limit(old_config::ServerSpeedLimits v, const std::string_view &what) {
		switch (v) {
			case old_config::ServerSpeedLimits::rules_limits:
				errout("WARNING: parsing speed limits for [%s]: rule based speed limits aren't currently supported. Ignoring.\n", what);
				break;

			case old_config::ServerSpeedLimits::constant_limits:
			case old_config::ServerSpeedLimits::no_limits:
				break;
		}

		return v == SpeedLimitsType;
	}

	void convert(const old_config::UserOrGroup::SpeedLimits &old, fz::authentication::file_based_authenticator::rate_limits &rl, const std::string_view &what)
	{
		if (match_speed_limit<old_config::UserOrGroup::SpeedLimits::constant_limits>(old.dl_type, fz::sprintf("%s/download", what)))
			rl.session_inbound = old.dl_limit * 1024;

		if (match_speed_limit<old_config::UserOrGroup::SpeedLimits::constant_limits>(old.ul_type, fz::sprintf("%s/upload", what)))
			rl.session_outbound = old.ul_limit * 1024;
	}

	auto on_ip_convert_error(std::string what) {
		return [what = std::move(what)] (std::size_t, const std::string_view &ip) {
			errout("WARNING: Ignoring bad IP/range [%s] while converting [%s].\n", ip, what);
			return true;
		};
	}

	void convert(const old_config::UserOrGroup &old, fz::authentication::any_password &any_password)
	{
		const std::string &hash = get(old.options, old_config::key::Pass);
		const std::string &salt = get(old.options, old_config::key::Salt);

		if (!hash.empty()) {
			if (salt.empty())
				any_password = fz::authentication::password::md5::from_hash(fz::hex_decode<std::string>(hash));
			else
				any_password = fz::authentication::password::sha512::from_hash_and_salt(fz::hex_decode<std::string>(hash), salt);
		}
	}
}

bool convert(const old_config &old, fz::authentication::file_based_authenticator::groups &groups, fz::authentication::file_based_authenticator::groups::value_type **speed_limited_group)
{
	for (const auto &[name, o]: old.groups) {
		auto &n = groups[name];

		n.description = get<std::string>(o.options, old_config::key::Comments);

		convert(o.permissions, n.mount_table);
		convert(o.speed_limits, n.rate_limits, fz::sprintf("Group %s", name));

		if (!convert(o.ip_filter.disallowed, n.disallowed_ips, on_ip_convert_error(fz::sprintf("Group %s, IpFilter.Disallowed", name))))
			return false;

		if (!convert(o.ip_filter.allowed, n.allowed_ips, on_ip_convert_error(fz::sprintf("Group %s, IpFilter.Allowed", name))))
			return false;
	}

	if (speed_limited_group) {
		old_config::ServerSpeedLimits server_dl_type = get(old.settings, old_config::key::Download_Speedlimit_Type);
		old_config::ServerSpeedLimits server_ul_type = get(old.settings, old_config::key::Upload_Speedlimit_Type);

		bool server_has_download_limits =  match_speed_limit<old_config::ServerSpeedLimits::constant_limits>(server_dl_type, fz::sprintf("Settings/%s", old_config::key::Download_Speedlimit_Type));
		bool server_has_upload_limits = match_speed_limit<old_config::ServerSpeedLimits::constant_limits>(server_ul_type, fz::sprintf("Settings/%s", old_config::key::Upload_Speedlimit_Type));

		if (server_has_download_limits || server_has_upload_limits) {
			std::string name = ":SpeedLimitedGroup:";

			while (old.groups.count(name) != 0)
				name.append(1, ':');

			auto &&g = groups.insert(fz::authentication::file_based_authenticator::groups::value_type(name, {}));
			*speed_limited_group = &*g.first;

			if (server_has_download_limits) {
				g.first->second.rate_limits.session_inbound = get(old.settings, old_config::key::Download_Speedlimit);
				g.first->second.rate_limits.session_inbound *= 1024;
			}

			if (server_has_upload_limits) {
				g.first->second.rate_limits.session_outbound = get(old.settings, old_config::key::Upload_Speedlimit);
				g.first->second.rate_limits.session_outbound *= 1024;
			}
		}
	}

	return true;
}

bool convert(const old_config &old, fz::authentication::file_based_authenticator::users &users, fz::authentication::file_based_authenticator::groups::value_type *speed_limited_group)
{
	for (const auto &[name, o]: old.users) {
		auto &n = users[name];

		n.description = get(o.options, old_config::key::Comments);
		n.enabled = get(o.options, old_config::key::Enabled, true);

		convert(o.permissions, n.mount_table);
		convert(o.speed_limits, n.rate_limits, fz::sprintf("User %s", name));
		fz::authentication::any_password pw;
		convert(o, pw);
		n.credentials.password = std::move(pw);

		if (!convert(o.ip_filter.disallowed, n.disallowed_ips, on_ip_convert_error(fz::sprintf("User %s, IpFilter.Disallowed", name))))
			return false;

		if (!convert(o.ip_filter.allowed, n.allowed_ips, on_ip_convert_error(fz::sprintf("User %s, IpFilter.Allowed", name))))
			return false;

		std::string group = get(o.options, old_config::key::Group);
		if (!group.empty())
			n.groups.push_back(group);

		if (speed_limited_group)
			n.groups.push_back(speed_limited_group->first);
	}

	return true;
}

bool convert(const old_config &old, server_settings &s)
{
	auto &o = old.settings;

	static const std::vector<std::string> any_ip = { "*" };

	{   /*** Control addresses and ports ***/
		auto control_ips = fz::strtok(get(o, old_config::key::IP_Bindings), ' ');
		auto control_ports = fz::strtok(get(o, old_config::key::Serverports), ' ');
		auto implicit_tls_control_ports = fz::strtok(get(o, old_config::key::Implicit_SSL_ports), ' ');

		auto push_ip = [&control_ports, &implicit_tls_control_ports, &s](const std::string &ip) {
			for (const auto &p: control_ports)
				s.ftp_server.listeners_info().push_back({{ip, as<unsigned>(p)}, fz::ftp::session::allow_tls});

			for (const auto &p: implicit_tls_control_ports)
				s.ftp_server.listeners_info().push_back({{ip, as<unsigned>(p)}, fz::ftp::session::implicit_tls});
		};

		for (const auto &i: control_ips.empty() ? any_ip : control_ips) {
			if (i == "*") {
				push_ip("0.0.0.0");
				push_ip("::");
			}
			else
				push_ip(i);
		}
	}

	{   /*** Admin addresses and ports ***/
		auto admin_ips = fz::strtok(get(o, old_config::key::Admin_IP_Bindings), ' ');
		auto admin_port = get<unsigned>(o, old_config::key::Admin_port);

		s.admin.local_port = admin_port;

		for (const auto &i: admin_ips)
			s.admin.additional_address_info_list.push_back({{i, admin_port}, fz::ftp::session::implicit_tls});
	}

	/*** TLS info ***/
	s.ftp_server.sessions().tls.cert = fz::securable_socket::user_provided_cert_info {
		get(o, old_config::key::SSL_Key_file),
		get(o, old_config::key::SSL_Certificate_file),
		get(o, old_config::key::SSL_Key_Password)
	};

	s.ftp_server.sessions().tls.min_tls_ver = get(o, old_config::key::Minimum_TLS_version);
	s.admin.tls.min_tls_ver = get(o, old_config::key::Minimum_TLS_version);

	/*** Misc ***/
	s.protocols.performance.number_of_session_threads = get(o, old_config::key::Number_of_Threads);
	s.protocols.performance.receive_buffer_size = get(o, old_config::key::Network_Buffer_Size);
	s.protocols.performance.send_buffer_size = get(o, old_config::key::Network_Buffer_Size);
	s.protocols.timeouts.login_timeout = get(o, old_config::key::Login_Timeout);
	s.protocols.timeouts.activity_timeout = get(o, old_config::key::No_Transfer_Timeout);

	return true;
}

bool convert(const old_config &old, fz::tcp::binary_address_list &disallowed_ips, fz::tcp::binary_address_list &allowed_ips)
{
	return
		convert(get<std::string>(old.settings, old_config::key::IP_Filter_Disallowed), disallowed_ips, on_ip_convert_error(fz::sprintf("Settings/%s", old_config::key::IP_Filter_Disallowed))) &&
		convert(get<std::string>(old.settings, old_config::key::IP_Filter_Allowed), allowed_ips, on_ip_convert_error(fz::sprintf("Settings/%s", old_config::key::IP_Filter_Allowed)));
}
