#include <libfilezilla/glue/wx.hpp>

#include <wx/dcclient.h>
#include <wx/tokenzr.h>

#include "wrappedtext.hpp"
#include "helpers.hpp"

using evmap_t = std::map<wxEventType, std::string_view>;

namespace {
extern evmap_t evmap;
}

wxWrappedText::wxWrappedText(wxWindow *parent, const wxString &text, int style)
	: wxPanel(parent, wxID_ANY)
	, text_(new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style))
{
	wxPanel::SetLabel(text);

	auto dc = wxWindowDC(this);
	auto extent = dc.GetMultiLineTextExtent(text);
	auto min_width = std::min(extent.GetWidth(), wxDlg2Px(this, 200));

	text_->SetLabel(text);
	text_->Wrap(min_width);

	extent = dc.GetMultiLineTextExtent(text_->GetLabelText());

	text_->SetLabel(wxEmptyString);

	SetMinClientSize({extent.GetWidth(), -1});
	SetMaxClientSize({-1, extent.GetHeight()});


	Bind(wxEVT_SET_FOCUS, [&](auto &ev) {
		wxWindow *sib = nullptr;

		for (wxWindow *cur = this; !sib && cur && !cur->IsTopLevel(); cur = cur->GetParent()) {
			sib = cur->GetNextSibling();
		}

		if (sib)
			sib->SetFocusFromKbd();
		else
			ev.Skip();
	});
}

void wxWrappedText::SetLabel(const wxString &label)
{
	wxPanel::SetLabel(label);
	InvalidateBestSize();
	GetParent()->Layout();
}

bool wxWrappedText::SetForegroundColour(const wxColour &colour)
{
	wxPanel::SetForegroundColour(colour);
	return text_->SetForegroundColour(colour);
}

bool wxWrappedText::SetBackgroundColour(const wxColour &colour)
{
	wxPanel::SetBackgroundColour(colour);
	return text_->SetBackgroundColour(colour);
}

bool wxWrappedText::SetFont(const wxFont &font)
{
	wxPanel::SetFont(font);
	return text_->SetFont(font);
}

bool wxWrappedText::InformFirstDirection(int dir, int size, int other_size)
{
	if (!dir)
		return false;

	if (dir == wxVERTICAL)
		size = other_size;

	if (size != text_->GetSize().GetWidth()) {
		informed_ = true;

		text_->SetLabel(GetLabel());
		text_->Wrap(size);

		return true;
	}

	return false;
}

wxSize wxWrappedText::DoGetBestClientSize() const
{
	if (!informed_) {
		text_->SetLabel(GetLabel());
		text_->Wrap(GetMinClientSize().GetWidth());
	}

	return text_->GetSize();
}

bool wxWrappedText::AcceptsFocus() const
{
	return false;
}

bool wxWrappedText::AcceptsFocusFromKeyboard() const
{
	return false;
}
