#ifndef GENERATECERTIFICATEDIALOG_HPP
#define GENERATECERTIFICATEDIALOG_HPP


#include <string>
#include <vector>

#include <wx/propdlg.h>
#include "dialogex.hpp"

class wxTextCtrl;

class GenerateCertificateDialog: public wxDialogEx<wxPropertySheetDialog>
{
public:
	GenerateCertificateDialog();

	bool Create(wxWindow *parent,
				const wxString &title,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER,
				const wxString& name = wxS("GenerateCertificateDialog"));

	void SetDistinguishedName(std::string *dn);
	void SetHostnames(std::vector<std::string> *hostnames, std::size_t minimum_number_of_hostnames, bool at_least_2dn_level);

private:
	wxBookCtrlBase* CreateBookCtrl() override;
	bool TransferDataToWindow() override;
	bool TransferDataFromWindow() override;

	std::string *dn_{};
	std::vector<std::string> *hostnames_{};
	std::size_t minimum_number_of_hostnames_{};
	bool at_least_2nd_level_{};

	wxTextCtrl *dn_ctrl_{};
	wxTextCtrl *hostnames_ctrl_{};
};

#endif // GENERATECERTIFICATEDIALOG_HPP
