#include <libfilezilla/glue/wx.hpp>

#include <wx/textctrl.h>
#include <wx/bookctrl.h>
#include <wx/valtext.h>
#include <wx/valgen.h>
#include <wx/statline.h>
#include <wx/simplebook.h>
#include <wx/log.h>

#include "generatecertificatedialog.hpp"
#include "locale.hpp"

#include "helpers.hpp"
#include "glue.hpp"

#include "../filezilla/util/parser.hpp"

GenerateCertificateDialog::GenerateCertificateDialog()
{
}

bool GenerateCertificateDialog::Create(wxWindow *parent, const wxString &title, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	if (!wxDialogEx::Create(parent, winid, title, pos, size, style, name))
		return false;

	CreateButtons(wxOK | wxCANCEL);

	GetBookCtrl() | [&](auto p) {
		wxPage(p) | [&](auto p){
			wxVBox(p, 0) = {
				{ 0, wxLabel(p, _S("&Distinguished name:")) },
				{ 0, dn_ctrl_ = new wxTextCtrl(p, wxID_ANY) },

				{ 0, wxLabel(p, _S("&Hostnames (separate them with blanks):")) },
				{ 1, hostnames_ctrl_ = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_RICH2) }
			};

			dn_ctrl_->GetPrevSibling()->Show(false);
			dn_ctrl_->Show(false);

			hostnames_ctrl_->GetPrevSibling()->Show(false);
			hostnames_ctrl_->Show(false);
		};
	};

	LayoutDialog();

	return true;
}

void GenerateCertificateDialog::SetDistinguishedName(std::string *dn)
{
	dn_ = dn;
	dn_ctrl_->GetPrevSibling()->Show(dn_ != nullptr);
	dn_ctrl_->Show(dn_ != nullptr);
	dn_ctrl_->GetContainingSizer()->Layout();

	TransferDataToWindow();
}

void GenerateCertificateDialog::SetHostnames(std::vector<std::string> *hostnames, std::size_t minimum_number_of_hostnames, bool at_least_2dn_level)
{
	hostnames_ = hostnames;
	minimum_number_of_hostnames_ = minimum_number_of_hostnames;
	at_least_2nd_level_ = at_least_2dn_level;

	hostnames_ctrl_->GetPrevSibling()->Show(hostnames_ != nullptr);
	hostnames_ctrl_->Show(hostnames_ != nullptr);
	hostnames_ctrl_->GetContainingSizer()->Layout();

	TransferDataToWindow();
}


wxBookCtrlBase *GenerateCertificateDialog::CreateBookCtrl()
{
	return wxCreate<wxSimplebook>(this);
}

bool GenerateCertificateDialog::TransferDataToWindow()
{
	if (!wxDialogEx::TransferDataToWindow())
		return false;

	if (dn_)
		dn_ctrl_->SetValue(fz::to_wxString(*dn_));
	else
		dn_ctrl_->Clear();

	if (hostnames_)
		hostnames_ctrl_->SetValue(fz::join<wxString>(*hostnames_));
	else
		hostnames_ctrl_->Clear();

	return true;
}

bool GenerateCertificateDialog::TransferDataFromWindow()
{
	if (!wxDialogEx::TransferDataFromWindow())
		return false;

	if (dn_)
		*dn_ = fz::to_utf8(dn_ctrl_->GetValue().Trim(true).Trim(false));

	if (hostnames_) {
		auto hostnames = fz::strtok(fz::to_utf8(hostnames_ctrl_->GetValue().Trim(true).Trim(false)), " \n\r\t");
		if (hostnames.size() < minimum_number_of_hostnames_) {
			wxMsg::Error(_F("You must input at least %d hostnames.", minimum_number_of_hostnames_));
			return false;
		}

		for (auto &h: hostnames) {
			if (h.size() > 253) {
				wxMsg::Error(_S("Maximum allowed number of characters is hostnames is 253."));
				return false;
			}

			auto labels = fz::strtok_view(h, ".", false);

			if (at_least_2nd_level_ && labels.size() < 2) {
				wxMsg::Error(_S("You must input at least 2nd level domain names (i.e example.com, example.net, etc.)"));
				return false;
			}

			for (auto &l: labels) {
				if (l.size() == 0 || l.size() > 63) {
					wxMsg::Error(_S("Components of host names cannot be empty and cannot exceed 63 characters"));
					return false;
				}

				auto invalid = l.front() == '-' || l.back() == '-';
				if (!invalid) {
					fz::util::parseable_range r(l);
					while (lit(r, '0', '9') || lit(r, 'a', 'z') || lit(r, 'A', 'Z') || lit(r, '-'));

					invalid = !eol(r);
				}

				if (invalid) {
					wxMsg::Error(_S("Illegal characters in hostname"));
					return false;
				}
			}
		}

		*hostnames_ = std::move(hostnames);
	}

	return true;
}

