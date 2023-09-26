#include <libfilezilla/glue/wx.hpp>

#include <wx/statline.h>
#include <wx/checkbox.h>

#include "speedlimitseditor.hpp"
#include "integraleditor.hpp"

#include "locale.hpp"

#include "helpers.hpp"

bool SpeedLimitsEditor::Create(wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	if (!wxPanel::Create(parent, winid, pos, size, style, name))
		return false;

	static auto editor_sizer = [](wxWindow *parent, wxIntegralEditor *&editor, const wxString &title) -> wxSizer * {
		wxCheckBox *enabler;

		wxSizer *ret = wxVBox(parent) = {
			{ 0, enabler = new wxCheckBox(parent, wxID_ANY, title) },
			{ 0, editor = wxCreate<wxIntegralEditor>(parent, _S("KiB/s"), 1024) }
		};

		editor->Bind(wxIntegralEditor::Event::Changed, [editor, enabler](wxIntegralEditor::Event &) {
			fz::rate::type cur_limit;

			bool enabled = editor->Get(cur_limit) && cur_limit != fz::rate::unlimited;

			editor->Enable(enabled);
			enabler->SetValue(enabled);
		});

		enabler->Bind(wxEVT_CHECKBOX, [editor](wxCommandEvent &ev) {
			editor->SetValue(ev.GetInt() ? fz::rate::type(1024) : fz::rate::unlimited);
		});

		return ret;
	};

	static auto couple_editor_sizer = [](wxWindow *parent, wxIntegralEditor *&upload_editor, wxIntegralEditor *&download_editor, const wxString &title, const wxString &upload_title, const wxString &download_title) -> wxSizer * {
		return wxStaticHBox(parent, title, 0) = {
			{ 1, editor_sizer(parent, upload_editor, upload_title) },
			{ 1, editor_sizer(parent, download_editor, download_title) }
		};
	};

	wxVBox(this, 0) = {
		{ 0, couple_editor_sizer(this, upload_shared_ctrl_, download_shared_ctrl_, _S("Limits shared by all sessions"), _S("Do&wnload from server:"), _S("&Upload to server:")) },
		{ 0, couple_editor_sizer(this, upload_session_ctrl_, download_session_ctrl_, _S("Limits specific to each session"), _S("D&ownload from server:"), _S("U&pload to server:")) }
	};

	SetLimits(nullptr);

	return true;
}

void SpeedLimitsEditor::SetLimits(fz::authentication::file_based_authenticator::rate_limits *limits)
{
	if (!limits) {
		download_shared_ctrl_->SetRef(nullptr);
		upload_shared_ctrl_->SetRef(nullptr);
		download_session_ctrl_->SetRef(nullptr);
		upload_session_ctrl_->SetRef(nullptr);
	}
	else {
		const auto mapping = std::pair{fz::rate::unlimited, _S("Unlimited")};

		download_shared_ctrl_->SetRef(limits->inbound, 1)->set_mapping({mapping});
		upload_shared_ctrl_->SetRef(limits->outbound, 1)->set_mapping({mapping});
		download_session_ctrl_->SetRef(limits->session_inbound, 1)->set_mapping({mapping});
		upload_session_ctrl_->SetRef(limits->session_outbound, 1)->set_mapping({mapping});
	}
}

