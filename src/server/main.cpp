#include <clocale>

#include <libfilezilla/tls_info.hpp>
#include <libfilezilla/recursive_remove.hpp>

#include "../filezilla/ftp/server.hpp"
#include "../filezilla/logger/file.hpp"
#include "../filezilla/logger/splitter.hpp"
#include "../filezilla/logger/modularized.hpp"
#include "../filezilla/authentication/file_based_authenticator.hpp"
#include "../filezilla/authentication/throttled_authenticator.hpp"

#include "../filezilla/serialization/archives/xml.hpp"
#include "../filezilla/serialization/archives/argv.hpp"
#include "../filezilla/known_paths.hpp"
#include "../filezilla/service.hpp"
#include "../filezilla/util/io.hpp"
#include "../filezilla/util/dispatcher.hpp"
#include "../filezilla/util/tools.hpp"
#include "../filezilla/build_info.hpp"
#include "../filezilla/util/username.hpp"
#include "../filezilla/tls_exit.hpp"

#include "server_settings.hpp"
#include "administrator.hpp"
#include "server_config_paths.hpp"

namespace {

	int config_checks_result(std::string_view config_check, const fz::native_string &result_file, bool success, const std::vector<fz::native_string> &backups_made, fz::logger_interface &logger)
	{
		if (config_check.empty() || (config_check == "ignore" && result_file.empty()))
			return -1;

		std::string msg;
		std::string result;

		if (!success) {
			msg = "There were errors";
			result = "error";
		}
		else
		if (!backups_made.empty()) {
			for (const auto &b: backups_made)
				logger.log_u(fz::logmsg::status, L"Backup made: %s.", b);

			msg = "Backups were made. Now going to update the working copies of the configuration files to reflect the current product flavour and version";
			result = "backup";
		}
		else
		if (config_check == "backup") {
			msg = "No need to make any backups";
			result = "ok";
		}
		else {
			msg = "Everything was alright";
			result = "ok";
		}

		auto logmsg = result == "error" ? fz::logmsg::error : fz::logmsg::status;

		logger.log_u(logmsg, "Configuration files have been checked. %s.", msg);

		if (!result_file.empty()) {
			logger.log_u(fz::logmsg::status, L"Writing config-check results to file '%s'", result_file);

			fz::file file(result_file, fz::file::writing, fz::file::empty | fz::file::current_user_and_admins_only);
			if (!file) {
				logger.log_u(fz::logmsg::error, L"Could not open file '%s' for writing.", result_file);
				return EXIT_FAILURE;
			}

			bool success = fz::util::io::write(file, result);
			success &= fz::util::io::write(file, '\n');

			if (result == "backup") {
				for (const auto &b: backups_made)
					success &= fz::util::io::write(file, fz::to_utf8(b).append("\n"));
			}

			if (!success) {
				logger.log_u(fz::logmsg::error, L"Could not write config-check results to file '%s'.", result_file);
				return EXIT_FAILURE;
			}
		}

		return logmsg == fz::logmsg::error ? EXIT_FAILURE : EXIT_SUCCESS;
	}

	std::vector<fz::rmp::address_info> sorted_admin_listeners(const server_settings::admin_options &admin)
	{
		auto admin_listeners = admin.additional_address_info_list;
		admin_listeners.push_back({ {"127.0.0.1", admin.local_port} });
		admin_listeners.push_back({ {"::1", admin.local_port} });
		std::sort(admin_listeners.begin(), admin_listeners.end());
		return admin_listeners;
	}

	template <typename AddressInfo>
	void remove_admin_listeners(std::vector<AddressInfo> &listeners, const std::vector<fz::rmp::address_info> &admin_listeners, fz::logger_interface &logger, const char *name)
	{
		std::sort(listeners.begin(), listeners.end());

		std::vector<AddressInfo> res;
		std::set_difference(listeners.begin(), listeners.end(), admin_listeners.begin(), admin_listeners.end(), std::back_inserter(res));

		if (res.size() < listeners.size())
			logger.log_u(fz::logmsg::debug_warning, L"Some listeners for the %s server were conflicting with the listeners for the Administration Server and have thus been disabled. Check your configuration.", name);

		listeners = std::move(res);
	}

	template <typename Listeners>
	void disable_pasv_range_if_conflicts(fz::ftp::server::options &opts, const Listeners &listeners, fz::logger_interface &logger, const char *name)
	{
		auto &range = opts.sessions().pasv.port_range;

		if (!range)
			return;

		for (auto &l: listeners) {
			if (range->min <= l.port && l.port <= range->max) {
				range.reset();
				logger.log_u(fz::logmsg::debug_warning, L"Port %d used by one of the listeners for the %s server conflicts with the Passive Mode custom port range (%d, %d). The custom port range has thus been disabled. Check your configuration.",
							 l.port, name, range->min, range->max);
				break;
			}
		}
	}
}


int FZ_SERVICE_PROGRAM_MAIN(argc, argv) {
	fz::event_loop server_loop {fz::event_loop::threadless};

	fz::mutex administrator_ptr_mutex{};
	administrator* administrator_ptr{};

	auto start_server = [&](int argc, char *argv[]) -> int {
		using namespace fz::serialization;

		auto file_logger = fz::logger::file::options()
			.enabled_types(fz::logmsg::type(fz::logmsg::status | fz::logmsg::error | fz::logmsg::command | fz::logmsg::reply | fz::logmsg::warning))
			.construct();

		fz::logger::splitter logger(file_logger);

		if (!fz::build_info::warning_message.empty()) {
			logger.log_u(fz::logmsg::warning, L"%s", fz::build_info::warning_message);
		}

	#if defined(__linux__)
		bool has_unprotected_hardlinks = fz::trimmed(fz::util::io::read(fzT("/proc/sys/fs/protected_hardlinks")).to_view()) == "0";

		if (has_unprotected_hardlinks) {
			logger.log_u(fz::logmsg::error, L"Refusing to run because fs.protected_hardlinks == 0.");
			return EXIT_FAILURE;
		}
	#endif

		fz::util::fs::native_path config_dir;
		std::string config_version_check;
		fz::native_string config_version_check_result_file;

		argv_input_archive{argc, argv}(
			// Allow the user to specify their own config directory, if they so want.
			optional_nvp{config_dir, "config-dir"},
			optional_nvp{config_version_check, "config-version-check"},
			optional_nvp{config_version_check_result_file, "config-version-check-result-file"}
		);

		auto verify_version = xml_input_archive::options::verify_mode::error;
		if (!config_version_check.empty() && !convert(config_version_check, verify_version)) {
			logger.log_u(fz::logmsg::error, L"Invalid value for the --config-version-check option.");
			return EXIT_FAILURE;
		}

		bool config_dir_was_empty = config_dir.str().empty();

		if (config_dir_was_empty) {
			config_dir = fz::util::fs::native_path(fz::to_native(argv[0])).base(true);
			if (!config_dir.is_base())
				config_dir = fzT("filezilla-server");
		}

		server_config_paths config_paths(config_dir);

	#if defined(FZ_WINDOWS)
		// Due to task #356, if the user is SYSTEM we store the configuration under %PROGRAMDATA%, to avoid Windows erasing the configuration upon system updates.
		bool is_system = fz::stricmp(fz::util::get_current_user_name(), L"SYSTEM") == 0;

		if (config_dir_was_empty && is_system && config_paths.for_writing().type() == fz::local_filesys::unknown) {
			auto restore = [&](const fz::util::fs::native_path &old) {
				logger.log_u(fz::logmsg::status, L"Running as SYSTEM and configuration not available at '%s'.", config_paths.for_writing().str());
				logger.log_u(fz::logmsg::status, L"Found old config at '%s'. Copying that one over.", old.str());

				int error = 0;
				if (fz::util::io::copy_dir(old, config_paths.for_writing(), fz::mkdir_permissions::cur_user_and_admins, &error)) {
					logger.log_u(fz::logmsg::status, L"Copy was successful. Removing the old config.");
					if (!fz::recursive_remove().remove(old))
						logger.log_u(fz::logmsg::debug_warning, L"Removing the old config failed.");
				}
				else
					logger.log_u(fz::logmsg::status, L"Something went wrong: not all files were copied. The server might malfunction. Error: %s", fz::socket_error_description(error));
			};

			auto old_config_dir = fz::known_paths::config::home() / config_dir;

			// If the old config dir exists, copy that.
			if (old_config_dir.type() == fz::local_filesys::dir)
				restore(old_config_dir);
			else {
				// If not, look into %SYSDIR%.old as well.
				auto win_dir = fz::known_paths::windows::windir().str();
				if (fz::starts_with(old_config_dir.str(), win_dir)) {
					old_config_dir = fz::util::fs::native_path(win_dir + L".old") / old_config_dir.str().substr(win_dir.size());

					if (old_config_dir.type() == fz::local_filesys::dir)
						restore(old_config_dir);
				}
			}
		}
	#endif

		bool write_config = false;
		fz::native_string admin_fingerprints_file_path;

		server_settings settings;
		using delayed_server_settings = fz::util::xml_archiver<server_settings>;

		auto server_settings_save_result_catcher = fz::util::make_dispatcher<delayed_server_settings::archive_result>(server_loop, [&](fz::util::xml_archiver_base &, const fz::util::xml_archiver_base::archive_info &info, int error){
			if (!error)
				logger.log_u(fz::logmsg::status, L"Settings written to %s.", info.file_name);
			else
				logger.log_u(fz::logmsg::error, L"Failed writing settings to %s.", info.file_name);
		});

		delayed_server_settings delayed_settings(
			server_loop,
			settings, "", config_paths.settings(fz::file::writing),
			fz::duration::from_milliseconds(100),
			nullptr,
			&server_settings_save_result_catcher
		);

		fz::authentication::file_based_authenticator::groups groups;
		fz::authentication::file_based_authenticator::users users;

		fz::tcp::binary_address_list disallowed_ips;
		fz::tcp::binary_address_list allowed_ips;

		fz::native_string impersonator_exe;

		std::vector<fz::native_string> backups_made;
		bool verify_errors_ignored{};

		{
			argv_input_archive ar{argc, argv, {
				{ config_paths.settings(fz::file::reading), true },
				{ config_paths.groups(fz::file::reading), false },
				{ config_paths.users(fz::file::reading), true },
				{ config_paths.disallowed_ips(fz::file::reading), false },
				{ config_paths.allowed_ips(fz::file::reading), false }
			}, verify_version, &backups_made, &verify_errors_ignored};

			ar(
				nvp{settings, ""},
				nvp{groups, ""},
				nvp{users, ""},
				optional_nvp{disallowed_ips, "disallowed_ips"},
				optional_nvp{allowed_ips, "allowed_ips"},
				value_info{optional_nvp{write_config, "write-config"}, "Write the passed in configuration to the proper files, then exit."},
				value_info{optional_nvp{admin_fingerprints_file_path, "write-admin-tls-fingerprints-to-file"}, "Dump the administration TLS fingerprint into the given file."},
				value_info{optional_nvp{impersonator_exe, "impersonator-exe"}, "Path to the impersonator executable."},
				// The following is provided just so that the parser doesn't choke on the other options we recognize that were handled at the top of this function (look back up).
				// We cannot handle them here because it'd be too late, at this point.
				value_info{optional_nvp{config_dir, "config-dir"}, "Specify a configuration directory other than the default one."},
				value_info{optional_nvp{config_version_check, "config-version-check"}, "Action to take if the configuration files version is not the expected one. One of error, ignore, backup. If error or backup, after the action is taken the program exists."},
				value_info{optional_nvp{config_version_check_result_file, "config-version-check-result-file"}, "Dump the result of the config version check into the given file."}
			).check_for_unhandled_options();

			if (!ar) {
				logger.log_u(fz::logmsg::error, "%s", ar.error().description());

				if (ar.error().is_xml_flavour_or_version_mismatch())
					config_checks_result(config_version_check, config_version_check_result_file, false, backups_made, logger);

				return EXIT_FAILURE;
			}
		}

		if (int res = config_checks_result(config_version_check, config_version_check_result_file, true, backups_made, logger); res != -1 && backups_made.empty()) {
			if (config_version_check != "ignore" || res != EXIT_SUCCESS)
				return res;
		}

		std::setlocale(LC_ALL, settings.locale.c_str());
		file_logger.set_options(settings.logger);

		auto setup_tls_for = [&](const char *what, fz::securable_socket::cert_info &tls, const fz::native_string &fingerprints_file_path = {}, bool dump_only_sha256 = false) {
			logger.log_u(fz::logmsg::status, L"Setting up TLS for the %s", what);

			bool generated = false;

			tls.set_root_path(config_paths.certificates());

			if (tls.load_certs(&logger).empty())  {
				tls = tls.generate_selfsigned(config_paths.certificates(), &logger);
				generated = tls.is_valid();
			}

			tls.dump(logger, dump_only_sha256);

			if (!fingerprints_file_path.empty()) {
				fz::remove_file(fingerprints_file_path);

				auto file = fz::logger::file::options().
					name(fingerprints_file_path).
					include_headers(false).
					start_line(false).
				construct();

				tls.dump(file, dump_only_sha256);
			}

			return generated;
		};

		auto setup_default_listeners = [&] {
			bool defaults_set = false;
			if (fz::local_filesys::get_size(config_paths.settings(fz::file::reading)) <= 0) {
				// If no settings were previously available on the filesystem and no control socket info has been provided on the command line,
				// then by default listen on port 21, ANY_ADDR, both IPV4 and IPV6
				if (settings.ftp_server.listeners_info().empty()) {
					settings.ftp_server.listeners_info().push_back({{"0.0.0.0", 21}});
					settings.ftp_server.listeners_info().push_back({{"::", 21}});
					defaults_set = true;
				}
			}
			return defaults_set;
		};

		fz::rate_limit_manager rate_limit_manager(server_loop);

		if (impersonator_exe.empty())
			impersonator_exe = fz::util::find_tool(fzT("filezilla-server-impersonator"), fzT("../tools/impersonator"), "FZ_SRVIMPERSONATOR");

		fz::thread_pool pool;

		fz::authentication::file_based_authenticator file_auth(
			pool,
			server_loop,
			logger,
			rate_limit_manager,
			config_paths.groups(fz::file::writing), std::move(groups),
			config_paths.users(fz::file::writing), std::move(users),
			std::move(impersonator_exe)
		);

		file_auth.set_save_result_event_handler(&server_settings_save_result_catcher);

		fz::tcp::automatically_serializable_binary_address_list automatic_disallowed_ips (
			server_loop, disallowed_ips, "disallowed_ips", config_paths.disallowed_ips(fz::file::writing), fz::duration::from_milliseconds(100), &server_settings_save_result_catcher
		);

		fz::tcp::automatically_serializable_binary_address_list automatic_allowed_ips (
			server_loop, allowed_ips, "allowed_ips", config_paths.allowed_ips(fz::file::writing), fz::duration::from_milliseconds(100), &server_settings_save_result_catcher
		);

		if (!backups_made.empty())
			write_config = true;

		bool legacy_converted = settings.convert_legacy(logger);
		auto generated_server_tls = setup_tls_for("FTP Server", settings.ftp_server.sessions().tls.cert);
		auto generated_admin_tls = setup_tls_for("Administration Server", settings.admin.tls.cert, admin_fingerprints_file_path, true);
		auto generated_default_listeners = setup_default_listeners();

		if (write_config || verify_errors_ignored || generated_admin_tls || generated_server_tls || generated_default_listeners || legacy_converted) {
			// We need to make sure the data is saved right now, because we might have been asked to exit right away
			// and, therefore, no event loop would be started that could execute the delayed saving.
			bool success = delayed_settings.save_now(fz::util::xml_archiver_base::direct_dispatch) == 0;

			if (success && (write_config || verify_errors_ignored)) {
				success = file_auth.save(fz::util::xml_archiver_base::direct_dispatch);

				if (success)
					success = automatic_allowed_ips.save(fz::util::xml_archiver_base::direct_dispatch);

				if (success)
					success = automatic_disallowed_ips.save(fz::util::xml_archiver_base::direct_dispatch);

				if (success && write_config)
					return EXIT_SUCCESS;
			}

			if (!success) {
				logger.log_u(fz::logmsg::error, L"Couldn't write configuration files.");
				return EXIT_FAILURE;
			}
		}

		fz::authentication::autobanner autobanner(server_loop, settings.protocols.autobanner);
		fz::event_loop_pool loop_pool(server_loop, pool, settings.protocols.performance.number_of_session_threads);
		fz::port_manager port_manager;

		fz::authentication::throttled_authenticator authenticator(server_loop, file_auth, file_logger);

		fz::tcp::server_context context{pool, server_loop};

		auto admin_listeners = sorted_admin_listeners(settings.admin);
		auto ftp_server_options = settings.ftp_server;
		disable_pasv_range_if_conflicts(ftp_server_options, ftp_server_options.listeners_info(), logger, "FTP");
		disable_pasv_range_if_conflicts(ftp_server_options, admin_listeners, logger, "Administration");

		remove_admin_listeners(ftp_server_options.listeners_info(), admin_listeners, logger, "FTP");

		fz::ftp::server ftp_server(
			context, loop_pool, logger, file_logger,
			authenticator,
			rate_limit_manager,
			automatic_disallowed_ips, automatic_allowed_ips,
			autobanner,
			port_manager,
			ftp_server_options
		);

		ftp_server.start();

		fz::tls_system_trust_store trust_store(pool);
		fz::acme::daemon acme(pool, server_loop, logger, trust_store);
		acme.set_root_path(config_paths.certificates());
		acme.set_how_to_serve_challenges(settings.acme.how_to_serve_challenges);
		acme.set_certificate_used_status(settings.ftp_server.sessions().tls.cert, true);
		acme.set_certificate_used_status(settings.admin.tls.cert, true);

		administrator admin(
			context, loop_pool, file_logger, logger,
			ftp_server,
			automatic_disallowed_ips, automatic_allowed_ips,
			autobanner,
			file_auth,
			delayed_settings,
			acme,
			config_paths
		);

		{
			fz::scoped_lock lock(administrator_ptr_mutex);
			administrator_ptr = &admin;
		}

		server_loop.run();

		{
			fz::scoped_lock lock(administrator_ptr_mutex);
			administrator_ptr = nullptr;
		}

		return EXIT_SUCCESS;
	};

	fz::tls_exit(fz::service::make(argc, argv,
		start_server,

		[&] {
			server_loop.stop();
		},

		[&] {
			fz::scoped_lock lock(administrator_ptr_mutex);
			if (administrator_ptr)
				administrator_ptr->reload_config();
		}
	));
}

