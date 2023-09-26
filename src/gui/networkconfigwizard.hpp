#ifndef NETWORKCONFIGWIZARD_HPP
#define NETWORKCONFIGWIZARD_HPP


#include <wx/wizard.h>
#include "dialogex.hpp"

#include "../filezilla/ftp/server.hpp"

class wxCheckBox;
class wxIntegralEditor;
class wxTextCtrl;
class wxRadioButton;
class wxStaticText;

class NetworkConfigWizard: public wxDialogEx<wxWizard>
{
public:
	NetworkConfigWizard(wxWindow *parent, const wxString &title = wxEmptyString);

	void SetFtpOptions(const fz::ftp::server::options &opts);
	const fz::ftp::server::options &GetFtpOptions() const;

	bool Run();

private:
	fz::ftp::server::options ftp_options_;

	wxRadioButton *use_custom_port_range_ctrl_{};
	wxRadioButton *use_ports_from_os_ctrl_{};
	wxIntegralEditor *min_port_range_ctrl_{};
	wxIntegralEditor *max_port_range_ctrl_{};
	wxTextCtrl *host_override_ctrl_{};
	wxCheckBox *disallow_host_override_for_local_peers_ctrl_{};
	wxWindow *ports_from_os_explanation_ctrl_{};
	wxWindow *min_label_{};
	wxWindow *max_label_{};
	wxTextCtrl *summary_ports_ctrl_{};
	wxTextCtrl *summary_host_ctrl_{};
	wxTextCtrl *summary_local_connections_ctrl_{};

	void select_port_range();
};

#endif // NETWORKCONFIGWIZARD_HPP
