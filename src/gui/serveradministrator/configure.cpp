#include <libfilezilla/glue/wx.hpp>

#include <wx/splitter.h>
#include <wx/msgdlg.h>
#include <wx/thread.h>
#include <wx/sizer.h>
#include <wx/log.h>
#include <wx/statline.h>
#include <wx/app.h>
#include <wx/hyperlink.h>
#include <wx/statbmp.h>
#include <wx/choicebk.h>
#include <wx/filepicker.h>
#include <wx/evtloop.h>
#include <wx/checkbox.h>

#include "../serveradministrator.hpp"
#include "../settings.hpp"
#include "../certinfoeditor.hpp"
#include "../settingsdialog.hpp"
#include "../networkconfigwizard.hpp"
#include "../textvalidatorex.hpp"

#include "../locale.hpp"
#include "../glue.hpp"
#include "../helpers.hpp"

void ServerAdministrator::set_configure_opts()
{
	if (!settings_dialog_)
		return;

	settings_dialog_->SetServerPathFormat(server_path_format_);
	settings_dialog_->SetHostaddressAnyIsEquivalent(any_is_equivalent_);
	settings_dialog_->SetGroupsAndUsers(groups_, users_, server_can_impersonate_, server_username_);
	settings_dialog_->SetFilters(disallowed_ips_, allowed_ips_);
	settings_dialog_->SetProtocolsOptions(protocols_options_);
	settings_dialog_->SetAdminOptions(admin_options_, admin_tls_extra_info_);
	settings_dialog_->SetFtpOptions(ftp_options_, ftp_tls_extra_info_);
	settings_dialog_->SetLoggingOptions(logger_options_);
	settings_dialog_->SetAcmeOptions(acme_options_, acme_extra_account_info_);

#ifndef WITHOUT_FZ_UPDATE_CHECKER
	settings_dialog_->SetUpdatesOptions(updates_options_);
	settings_dialog_->SetUpdateInfo(server_update_info_, server_update_next_check_);
#endif
}

void ServerAdministrator::get_configure_opts()
{
	if (!settings_dialog_)
		return;

	std::tie(groups_, users_) = settings_dialog_->GetGroupsAndUsers();
	std::tie(disallowed_ips_, allowed_ips_) = settings_dialog_->GetFilters();
	protocols_options_ = settings_dialog_->GetProtocolsOptions();
	ftp_options_ = settings_dialog_->GetFtpOptions();
	admin_options_ = settings_dialog_->GetAdminOptions();
	logger_options_ = settings_dialog_->GetLoggingOptions();
	acme_options_ = settings_dialog_->GetAcmeOptions();

#ifndef WITHOUT_FZ_UPDATE_CHECKER
	updates_options_ = settings_dialog_->GetUpdatesOptions();
#endif
}

void ServerAdministrator::reset_configure_opts_on_disconnect(Event &ev)
{
	ev.Skip();

	if (GetServerStatus().num_of_active_sessions < 0) {
		get_configure_opts();
		set_configure_opts();
	}
}

void ServerAdministrator::ConfigureServer()
{
	if (!IsConnected())
		return;

	if (responses_to_wait_for_ > 0) {
		logger_.log_u(fz::logmsg::debug_warning, L"Still retrieving server's configuration");
		return;
	}

	on_settings_received_func_ = [this] {
		logger_.log_u(fz::logmsg::status, _S("Server's configuration retrieved."));

		wxPushDialog<SettingsDialog>(this, fz::to_wxString(server_info_.name)) | [this](SettingsDialog &d) {
			on_settings_received_func_ = nullptr;

			settings_dialog_ = &d;

			set_configure_opts();

			Bind(Event::ServerStatusChanged, &ServerAdministrator::reset_configure_opts_on_disconnect, this);

			settings_dialog_->SetApplyFunction([this] {
				if (!client_.is_connected()) {
					wxMsg::Error(_S("Got disconnected from the server, cannot apply the changes.\n\nPlease, try again later."));
					return false;
				}

				get_configure_opts();

				client_.send<administration::set_groups_and_users>(groups_, users_, true);
				client_.send<administration::set_ip_filters>(disallowed_ips_, allowed_ips_);
				client_.send<administration::set_protocols_options>(protocols_options_);
				client_.send<administration::set_admin_options>(admin_options_);
				client_.send<administration::set_ftp_options>(ftp_options_);
				client_.send<administration::set_logger_options>(logger_options_);
				client_.send<administration::set_acme_options>(acme_options_);

#ifndef WITHOUT_FZ_UPDATE_CHECKER
				client_.send<administration::set_updates_options>(updates_options_);
#endif
				client_.send<administration::get_extra_certs_info>(CertInfoEditor::ftp_cert_id, ftp_options_.sessions().tls.cert);
				client_.send<administration::get_extra_certs_info>(CertInfoEditor::admin_cert_id, admin_options_.tls.cert);

				return true;
			});

			settings_dialog_->SetGenerateSelfsignedCertificateFunction([this](CertInfoEditor::cert_id id, const std::string &dn, const std::vector<std::string> &hostnames) {
				client_.send<administration::generate_selfsigned_certificate>(id, dn, hostnames);
			});

			settings_dialog_->SetGenerateAcmeCertificateFunction([this](CertInfoEditor::cert_id id, const fz::securable_socket::acme_cert_info &ci) {
				auto opts = settings_dialog_->GetAcmeOptions();
				logger_.log_u(fz::logmsg::status, _S("Generating ACME certificate"));
				client_.send<administration::generate_acme_certificate>(id, opts.how_to_serve_challenges, ci);
			});

			settings_dialog_->SetUploadCertificateFunction([this](CertInfoEditor::cert_id id, const std::string &cert, const std::string &key, const fz::native_string &password) {
				client_.send<administration::upload_certificate>(id, cert, key, password);
			});

			settings_dialog_->SetGenerateAcmeAccountFunction([this] { generate_acme_account(); });

#ifndef WITHOUT_FZ_UPDATE_CHECKER
			settings_dialog_->SetUpdateCheckFunc([this] {
				return CheckForUpdates();
			});
#endif
			settings_dialog_->ShowModal();
			settings_dialog_ = nullptr;

			Unbind(Event::ServerStatusChanged, &ServerAdministrator::reset_configure_opts_on_disconnect, this);
		};
	};

	responses_to_wait_for_ = 7;

	logger_.log_u(fz::logmsg::status, _S("Retrieving configuration from the server..."));

	client_.send<administration::get_groups_and_users>();
	client_.send<administration::get_ip_filters>();
	client_.send<administration::get_protocols_options>();
	client_.send<administration::get_admin_options>();
	client_.send<administration::get_ftp_options>();
	client_.send<administration::get_logger_options>();
	client_.send<administration::get_acme_options>();

#ifndef WITHOUT_FZ_UPDATE_CHECKER
	responses_to_wait_for_ += 1;
	client_.send<administration::get_updates_options>();
#endif

}

void ServerAdministrator::ConfigureNetwork()
{
	if (!IsConnected())
		return;

	if (responses_to_wait_for_ > 0) {
		logger_.log_u(fz::logmsg::debug_warning, L"Still retrieving server's configuration");
		return;
	}

	on_settings_received_func_ = [this] {
		wxPushDialog<NetworkConfigWizard>(this, _F("Network configuration wizard for server %s", fz::to_wxString(server_info_.name))) | [this](NetworkConfigWizard &wiz) {
			on_settings_received_func_ = nullptr;

			wiz.SetFtpOptions(ftp_options_);

			if (wiz.Run()) {
				if (!client_.is_connected()) {
					wxMsg::Error(_S("Got disconnected from the server, cannot apply the changes.\n\nPlease, try again later."));
					return;
				}

				client_.send<administration::set_ftp_options>(wiz.GetFtpOptions());
			}
		};
	};

	responses_to_wait_for_ = 1;
	client_.send<administration::get_ftp_options>();
}
