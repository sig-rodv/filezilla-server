#include <libfilezilla/glue/wx.hpp>

#include <wx/checkbox.h>
#include <wx/valgen.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/hyperlink.h>
#include <wx/textctrl.h>

#include "networkconfigwizard.hpp"
#include "helpers.hpp"

#include "integraleditor.hpp"
#include "textvalidatorex.hpp"
#include "../filezilla/port_randomizer.hpp"

#include "config.hpp"

NetworkConfigWizard::NetworkConfigWizard(wxWindow *parent, const wxString &title)
	: wxDialogEx(parent, wxID_ANY, title, wxNullBitmap, wxDefaultPosition, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	wxPage(this, _S("FTP Data connection modes")) |[&] (auto p) {
		wxVBox(p, 0) = {
			wxWText(p, _S("FTP supports two ways to establish data connections for transfers: active and passive mode.")),
			wxWText(p, _S("Passive mode is the recommended mode most clients default to.")),
			wxWText(p, _F("In Passive mode clients ask %s which server port to connect to.", PACKAGE_NAME)),
			wxWText(p, _F("The wizard helps you configure %s and set up your router or firewall to support passive mode on your server.", PACKAGE_NAME)),
			wxWText(p, _S("At the end of the configuration process the wizard suggests you test your configurations, providing a link to our online server tester.")),
			wxWText(p, _S("Note: Active mode does not require server-side configuration."))
		};
	};

	wxPage(this, _S("Setting up Passive mode port range")) | [&](auto p) {
		wxVBox(p, 0) = {
			wxWText(p, _F("You need to set the range of ports %s uses for passive mode data connections.", PACKAGE_NAME)),
			wxWText(p, _S("Set the range greater than the number of transfers you want to serve in a 4-minutes time period.")),

			use_custom_port_range_ctrl_ = new wxRadioButton(p, wxID_ANY, _S("&Use custom port range:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP),
			{ wxSizerFlags(0).Border(wxUP, 0), wxVBox(p, 4) = {
				min_label_ = wxLabel(p, _F("&From: (suggested is %d)", fz::port_randomizer::min_ephemeral_value)),
				min_port_range_ctrl_ = wxCreate<wxIntegralEditor>(p),

				max_label_ = wxLabel(p, _F("&To: (suggested is %d)", fz::port_randomizer::max_ephemeral_value)),
				max_port_range_ctrl_ = wxCreate<wxIntegralEditor>(p)
			}},

			use_ports_from_os_ctrl_ = new wxRadioButton(p, wxID_ANY, _S("&Any")),

			{ 0, ports_from_os_explanation_ctrl_ = new wxPanel(p) | [&] (auto p) {
				wxVBox(p, 0) = {
					wxWText(p, _S("Configure your NAT routers to forward respectively the specified range or all TCP ports.")),
					wxLabel(p, _S("Open the same TCP ports on your firewalls as well."))
				};
			}}
		};

		use_custom_port_range_ctrl_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent &) {
			ftp_options_.sessions().pasv.port_range.emplace();
			select_port_range();
		});

		use_ports_from_os_ctrl_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent &) {
			ftp_options_.sessions().pasv.port_range.reset();
			select_port_range();
		});
	};

	wxPage(this, _S("Passive mode: setting public IP or hostname")) |[&](auto p) {
		wxVBox(p, 0) = {
			wxWText(p, _F("To properly support the passive mode, if %1$s is connected to the external network via a NAT device, "
						  "it's necessary to specify which is the external IP address or hostname %1$s can be reached at.", PACKAGE_NAME)),
			wxWText(p, _S("&Enter the public IP or hostname (if you leave it empty FileZilla Server uses the local IP):")),

			host_override_ctrl_ = new wxTextCtrl(p, wxID_ANY),

			disallow_host_override_for_local_peers_ctrl_ = new wxCheckBox(p, wxID_ANY, _S("&Use local IP for local connections (recommended).")),
		};
	};

	wxPage(this, _S("Network Configuration settings")) |[&] (auto p) {
		wxVBox(p, 0) = {
			wxLabel(p, _S("These are the choices you made:")),
			{ wxSizerFlags(0).Border(wxUP, 0), wxVBox(p) = {1, wxGBox(p, 2, {1}, {}, wxGBoxDefaultGap, wxALIGN_TOP) = {
			   wxLabel(p, _S("Port range:")),
			   summary_ports_ctrl_ =  new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_WORDWRAP | wxTE_MULTILINE),

			   wxLabel(p, _S("External IP or hostname:")),
			   summary_host_ctrl_ = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_WORDWRAP | wxTE_MULTILINE),

			   wxLabel(p, _S("Use internal IP for local connections:")),
			   summary_local_connections_ctrl_ = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_WORDWRAP | wxTE_MULTILINE),
			}}},

			wxWText(p, _F("Remember:\n"
						  "1) if %1$s is connected to the external network via a NAT device, the chosen ports must be all forwarded;\n"
						  "2) if %1$s is protected by a firewall, the choosen ports must be all open.", PACKAGE_NAME)),

			new wxStaticLine(p),

			wxWText(p, _S("Click on the Finish button to save network configurations. To change anything click on the Back button.")),
			wxWText(p, _S("To test network configurations from the internet, you can use our online FTP client tester, you find the &link below:")),
			new wxHyperlinkCtrl(p, wxID_ANY, _S("https://ftptest.net"), _S("https://ftptest.net"))
		};

		wxTransferDataToWindow(p, [this] {
			auto &range = ftp_options_.sessions().pasv.port_range;
			wxString summary_ports_value;
			wxString summary_host_value;
			wxString summary_local_connections_value;

			if (range.has_value())
				summary_ports_value = _F("custom, from %d to %d.", range->min, range->max);
			else
				summary_ports_value = _S("the Operating System will choose the first available port from the full set.");

			if (!host_override_ctrl_->IsEmpty())
				summary_host_value = host_override_ctrl_->GetValue();
			else
				summary_host_value = _S("No public IP, only local IP.");

			if (disallow_host_override_for_local_peers_ctrl_->GetValue())
				summary_local_connections_value = _S("Yes.");
			else
				summary_local_connections_value = _S("No.");

			summary_ports_ctrl_->ChangeValue(summary_ports_value);
			summary_host_ctrl_->ChangeValue(summary_host_value);
			summary_local_connections_ctrl_->ChangeValue(summary_local_connections_value);

			return true;
		});
	};
}

void NetworkConfigWizard::SetFtpOptions(const fz::ftp::server::options &opts)
{
	ftp_options_ = opts;

	select_port_range();

	host_override_ctrl_->SetValidator(TextValidatorEx(wxFILTER_NONE, &ftp_options_.sessions().pasv.host_override));
	disallow_host_override_for_local_peers_ctrl_->SetValidator(wxGenericValidator(&ftp_options_.sessions().pasv.do_not_override_host_if_peer_is_local));
}

const fz::ftp::server::options &NetworkConfigWizard::GetFtpOptions() const
{
	return ftp_options_;
}

bool NetworkConfigWizard::Run()
{
	return RunWizard(GetFirstPage(this));
}

void NetworkConfigWizard::select_port_range()
{
	auto &range = ftp_options_.sessions().pasv.port_range;

	if (range.has_value()) {
		use_custom_port_range_ctrl_->SetValue(true);

		min_label_->Enable();
		min_port_range_ctrl_->Enable();
		min_port_range_ctrl_->SetRef(range->min, 1, 65535);

		max_label_->Enable();
		max_port_range_ctrl_->Enable();
		max_port_range_ctrl_->SetRef(range->max, 1, 65535);

		ports_from_os_explanation_ctrl_->Disable();
	}
	else {
		use_ports_from_os_ctrl_->SetValue(true);

		min_label_->Disable();
		min_port_range_ctrl_->Disable();
		min_port_range_ctrl_->SetRef(nullptr);

		max_label_->Disable();
		max_port_range_ctrl_->Disable();
		max_port_range_ctrl_->SetRef(nullptr);

		ports_from_os_explanation_ctrl_->Enable();
	}
}
