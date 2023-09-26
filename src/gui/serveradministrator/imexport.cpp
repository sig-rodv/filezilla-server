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

#include "../locale.hpp"
#include "../glue.hpp"
#include "../helpers.hpp"

#ifdef HAVE_CONFIG_H
#	include "config.hpp"
#endif

namespace {

struct Checks: public wxCheckBoxGroup
{
	using wxCheckBoxGroup::wxCheckBoxGroup;

	CB listeners_and_protocols = c(_S("Server listeners and protocols"));
	CB rights_management       = c(_S("Rights management"));
	CB administration          = c(_S("Administration"));
	CB logging                 = c(_S("Logging"));
	CB acme                    = c(_S("Let's Encrypt®"));

	#ifndef WITHOUT_FZ_UPDATE_CHECKER
		CB updates = c(_S("Updates"));
	#endif
};

}

void ServerAdministrator::ExportConfig()
{
	if (!IsConnected())
		return;

	if (responses_to_wait_for_ > 0) {
		logger_.log_u(fz::logmsg::debug_warning, L"Still retrieving server's configuration");
		return;
	}

	wxPushDialog(this, wxID_ANY, _S("Export configuration"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER) | [this](wxDialog *p) {
		Checks *checks;

		wxVBox(p) = {
			wxWText(p, _F("Exporting configuration of server %s.", fz::to_wxString(server_info_.name))),

			wxStaticVBox(p, _S("Select parts to export:")) = checks = new Checks(p),
			wxEmptySpace,

			p->CreateButtonSizer(wxOK | wxCANCEL)
		};

		checks->SetValue(true);

		auto *ok = wxWindow::FindWindowById(wxID_OK, p);

		if (ok) {
			checks->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent &ev) {
				ev.Skip();

				ok->Enable(checks->IsAnyChecked());
			});
		}

		wxGUIEventLoop loop;
		wxValidate(p, [&]() mutable -> bool {
			if (loop.IsRunning())
				return false;

			if (!checks->IsAnyChecked()) {
				wxMsg::Error(_S("You must choose something to export."));
				return false;
			}

			if (ok) {
				ok->Disable();
				checks->Disable();
			}

			wxPushDialog<wxFileDialog>(p, _S("Choose a file where to export the configuration to."), wxEmptyString, wxEmptyString, wxT("*.xml"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT) | [&](wxFileDialog &d) {
				if (Settings()->working_dir().type() == fz::local_filesys::dir)
					d.SetDirectory(fz::to_wxString(Settings()->working_dir().str()));

				auto ret = d.ShowModal();

				Settings()->working_dir() = fz::to_native(d.GetDirectory());

				if (ok) {
					ok->Enable();
					checks->Enable();
				}

				if (ret == wxID_CANCEL)
					return loop.Exit(false);

				auto dest = d.GetPath();
				if (!dest.EndsWith(wxT(".xml")))
					dest += wxT(".xml");

				on_settings_received_func_ = [this
					, dest = fz::to_native(dest)
					, checks
					, &loop
				] {
					on_settings_received_func_ = nullptr;

					logger_.log_u(fz::logmsg::status, _S("Server's configuration retrieved. Exporting now..."));

					using namespace fz::serialization;

					xml_output_archive::file_saver saver(dest);	{
						xml_output_archive ar(saver, xml_output_archive::options().root_node_name("filezilla-server-exported"));

						if (checks->listeners_and_protocols) {
							ar(
								nvp(protocols_options_, "protocols_options"),
								nvp(ftp_options_, "ftp_options"),
								nvp(disallowed_ips_, "disallowed_ips"),
								nvp(allowed_ips_, "allowed_ips")
							);
						}

						if (checks->rights_management) {
							ar(
								nvp(groups_, "groups"),
								nvp(users_, "users")
							);
						}

						if (checks->administration) {
							ar(nvp(admin_options_, "admin_options"));
						}

						if (checks->logging) {
							ar(nvp(logger_options_, "logger_options"));
						}

						if (checks->acme) {
							ar(
								nvp(acme_options_, "acme_options"),
								nvp(acme_extra_account_info_, "acme_extra_account_info")
							);
						}

						#ifndef WITHOUT_FZ_UPDATE_CHECKER
							if (checks->updates)
								ar(nvp(updates_options_, "updates_options"));
						#endif
					}

					if (!saver) {
						wxMsg::Error(_S("Failed to export configuration."));
						return loop.Exit(false);
					}

					logger_.log_u(fz::logmsg::status, _S("Server's configuration exported to %s."), dest);
					return loop.Exit(true);
				};

				logger_.log_u(fz::logmsg::status, _S("Retrieving configuration from the server..."));

				responses_to_wait_for_ = 0;

				if (checks->listeners_and_protocols) {
					responses_to_wait_for_ += 3;
					client_.send<administration::get_ip_filters>();
					client_.send<administration::get_protocols_options>();
					client_.send<administration::get_ftp_options>(true);
				}

				if (checks->rights_management) {
					responses_to_wait_for_ += 1;
					client_.send<administration::get_groups_and_users>();
				}

				if (checks->administration) {
					responses_to_wait_for_ += 1;
					client_.send<administration::get_admin_options>(true);
				}

				if (checks->logging) {
					responses_to_wait_for_ += 1;
					client_.send<administration::get_logger_options>();
				}

				if (checks->acme) {
					responses_to_wait_for_ += 1;
					client_.send<administration::get_acme_options>();
				}

				#ifndef WITHOUT_FZ_UPDATE_CHECKER
					if (checks->updates) {
						responses_to_wait_for_ += 1;
						client_.send<administration::get_updates_options>();
					}
				#endif
			};

			return loop.Run();
		});

		p->ShowModal();
	};
}

void ServerAdministrator::ImportConfig()
{
	if (!IsConnected())
		return;

	wxPushDialog<wxFileDialog>(this, _S("Choose a file to import the configuration from."), wxEmptyString, wxEmptyString, wxT("*.xml"), wxFD_OPEN | wxFD_FILE_MUST_EXIST) | [this](wxFileDialog &d) {
		if (Settings()->working_dir().type() == fz::local_filesys::dir)
			d.SetDirectory(fz::to_wxString(Settings()->working_dir().str()));

		auto ret = d.ShowModal();

		Settings()->working_dir() = fz::to_native(d.GetDirectory());

		if (ret == wxID_CANCEL)
			return;

		using namespace fz::serialization;

		auto load = [&, src = fz::to_native(d.GetPath())](xml_input_archive::options::verify_mode verify_mode) -> std::string {
			xml_input_archive::file_loader loader(src);
			xml_input_archive ar(loader, xml_input_archive::options()
				.root_node_name("filezilla-server-exported")
				.verify_version(verify_mode)
			);

			std::optional<decltype(groups_)> groups;
			std::optional<decltype(users_)> users;
			std::optional<decltype(disallowed_ips_)> disallowed_ips;
			std::optional<decltype(allowed_ips_)> allowed_ips;
			std::optional<decltype(protocols_options_)> protocols_options;
			std::optional<decltype(ftp_options_)> ftp_options;
			std::optional<decltype(admin_options_)> admin_options;
			std::optional<decltype(logger_options_)> logger_options;
			std::optional<decltype(acme_options_)> acme_options;
			std::optional<decltype(acme_extra_account_info_)> acme_extra_account_info;

			#ifndef WITHOUT_FZ_UPDATE_CHECKER
				std::optional<decltype(updates_options_)> updates_options;
			#endif

			ar(
				nvp(protocols_options, "protocols_options"),
				nvp(ftp_options, "ftp_options"),
				nvp(disallowed_ips, "disallowed_ips"),
				nvp(allowed_ips, "allowed_ips"),

				nvp(groups, "groups"),
				nvp(users, "users"),

				nvp(admin_options, "admin_options"),

				nvp(logger_options, "logger_options"),

				nvp(acme_options, "acme_options"),
				nvp(acme_extra_account_info, "acme_extra_account_info")

				#ifndef WITHOUT_FZ_UPDATE_CHECKER
					, nvp(updates_options, "updates_options")
				#endif
			);

			if (!ar) {
				if (ar.error().is_flavour_or_version_mismatch())
					return ar.error().description();

				wxMsg::Error(_S("Error while reading the configuration from file."))
					.Ext(fz::to_wxString(ar.error().description()));

				return {};
			}

			auto checks = new Checks(this);
			checks->Hide();

			checks->listeners_and_protocols.EnableAndSet(
				protocols_options &&
				ftp_options &&
				disallowed_ips && allowed_ips
			);

			checks->rights_management.EnableAndSet(
				groups && users
			);

			checks->administration.EnableAndSet(
				admin_options.has_value()
			);

			checks->logging.EnableAndSet(
				logger_options.has_value()
			);

			checks->acme.EnableAndSet(
				acme_options && acme_extra_account_info
			);

			#ifndef WITHOUT_FZ_UPDATE_CHECKER
				checks->updates.EnableAndSet(
					updates_options.has_value()
				);
			#endif

			if (!checks->IsAnyChecked()) {
				wxMsg::Error(_S("Chosen file doesn't contain any useful data."));
				return {};
			}

			wxGUIEventLoop loop;

			wxPushDialog(&d, wxID_ANY, _S("Import configuration"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER) | [&](wxDialog *p) {
				checks->Reparent(p);

				wxVBox(p) = {
					wxWText(p, _F("Importing configuration of server %s from file \"%s\".", fz::to_wxString(server_info_.name), src)),

					wxStaticVBox(p, _S("Select which configuration's parts to import:")) = checks |[&](auto p) { p->Show(); },
					wxEmptySpace,

					p->CreateButtonSizer(wxOK | wxCANCEL)
				};

				if (auto *ok = wxWindow::FindWindowById(wxID_OK, p)) {
					checks->Bind(wxEVT_CHECKBOX, [ok, checks](wxCommandEvent &ev) {
						ev.Skip();

						ok->Enable(checks->IsAnyChecked());
					});
				}

				wxValidate(p, [&] {
					if (!checks->IsAnyChecked()) {
						wxMsg::Error(_S("You must choose something to import."));
						return false;
					}

					return true;
				});

				if (p->ShowModal() == wxID_OK) {
					if (checks->listeners_and_protocols) {
						if (disallowed_ips && allowed_ips)
							client_.send<administration::set_ip_filters>(*disallowed_ips, *allowed_ips);

						if (protocols_options)
							client_.send<administration::set_protocols_options>(*protocols_options);

						if (ftp_options)
							client_.send<administration::set_ftp_options>(*ftp_options);
					}

					if (checks->rights_management) {
						if (groups && users)
							client_.send<administration::set_groups_and_users>(*groups, *users, true);
					}

					if (checks->administration) {
						if (admin_options)
							client_.send<administration::set_admin_options>(*admin_options);
					}

					if (checks->logging) {
						if (logger_options)
							client_.send<administration::set_logger_options>(*logger_options);
					}

					if (checks->acme) {
						if (acme_options && acme_extra_account_info) {
							client_.send<administration::restore_acme_account>(acme_options->account_id, *acme_extra_account_info);
							client_.send<administration::set_acme_options>(*acme_options);
						}
					}

					#ifndef WITHOUT_FZ_UPDATE_CHECKER
						if (checks->updates) {
							if (updates_options)
								client_.send<administration::set_updates_options>(*updates_options);
						}
					#endif
				}

				loop.Exit();
			};

			loop.Run();

			return {};
		};

		if (auto mismatch = load(xml_input_archive::options::verify_mode::error); !mismatch.empty()) {
			wxGUIEventLoop loop;
			wxPushDialog(&d, wxID_ANY, _S("Error."), wxDefaultPosition, wxDefaultSize, wxCAPTION) | [&mismatch, &loop](wxDialog *p) {
				wxVBox(p) = {
					wxWText(p, _F("There was a problem while reading the configuration from file: %s\n\n"
								  "The configuration can still be imported, but some data might be lost.\n"
								  "Do you want to proceed anyway?", fz::to_wxString(mismatch))),

					p->CreateButtonSizer(wxYES_NO)
				};

				p->SetEscapeId(wxID_NO);
				p->SetAffirmativeId(wxID_YES);

				loop.Exit(p->ShowModal());
			};

			if (loop.Run() == wxID_YES)
				load(xml_input_archive::options::verify_mode::ignore);
		}
	};
}
