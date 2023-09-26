#include <libfilezilla/glue/wx.hpp>

#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/log.h>

#include "binaryaddresslisteditor.hpp"

#include "locale.hpp"
#include "glue.hpp"
#include "helpers.hpp"

bool BinaryAddressListEditor::Create(wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	if (!wxPanel::Create(parent, winid, pos, size, style, name))
		return false;

	if (text_ = new wxTextCtrl; !text_->Create(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE))
		return false;

	auto main = new wxBoxSizer(wxHORIZONTAL);
	main->Add(text_, 1, wxEXPAND, 10);

	SetIps(nullptr);
	SetSizer(main);

	return true;
}

void BinaryAddressListEditor::SetIps(fz::tcp::binary_address_list *list)
{
	list_ = list;

	if (list) {
		text_->Enable();
		text_->SetValue(fz::to_wxString(list->to_string()));
	}
	else {
		text_->Disable();
		text_->Clear();
	}
}

bool BinaryAddressListEditor::TransferDataFromWindow()
{
	if (!list_)
		return true;

	const auto &value = text_->GetValue();
	const auto &wstring = value.ToStdWstring();
	const auto &chunks = fz::strtok_view(wstring, L" \t\r\n;,");

	return convert(chunks, *list_, [&](std::size_t i, const std::wstring_view &s) {
		wxMsg::Error(_F("Invalid IP/Range [%s%s] as element number %zu.", s.substr(0, 10), s.size() > 10 ? wxT("...") : wxT(""), i+1));
		text_->SetSelection(s.data()-wstring.data(), s.data()+s.size()-wstring.data());
		text_->ShowPosition(s.data()-wstring.data());
		return false;
	});
}

