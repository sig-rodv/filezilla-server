#include <libfilezilla/glue/wx.hpp>

#include "passwordeditor.hpp"
#include "locale.hpp"
#include "glue.hpp"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/valtext.h>

#include "helpers.hpp"
#include "textvalidatorex.hpp"


bool PasswordEditor::Create(wxWindow *parent, bool allow_no_password, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	if (!wxPanel::Create(parent, winid, pos, size, style, name))
		return false;

	if (allow_no_password) {
		wxHBox(this, 0) = {
			{ wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT), password_enabler_ = new wxCheckBox(this, wxID_ANY, wxS("")) },
			{ 1, password_text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD) }
		};

		password_enabler_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent &ev) {
			if (!password_)
				return;

			bool enabled = ev.GetInt();

			if (!enabled) {
				*password_ = {};
				SetPassword(password_);
			}
			else {
				password_text_->Enable();
				password_text_->SetValidator(TextValidatorEx(wxFILTER_NONE, nullptr, field_must_not_be_empty(_S("Password"))));
			}

			Event::Changed.Process(this, this);
		});
	}
	else {
		wxHBox(this, 0) = {
			{ 1, password_text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD) }
		};
	}

	return true;
}

void PasswordEditor::SetPassword(fz::authentication::any_password *password)
{
	password_ = password;

	if (!password || !password->has_password()) {
		password_text_->Clear();
		password_text_->SetHint(wxEmptyString);

		if (password_enabler_) {
			password_text_->SetValidator(wxValidator());
			password_enabler_->SetValue(false);
			password_text_->Disable();
		}
		else {
			password_text_->Enable();
			password_text_->SetValidator(TextValidatorEx(wxFILTER_NONE, nullptr, field_must_not_be_empty(_S("Password"))));
		}
	}
	else {
		password_text_->Clear();
		password_text_->SetHint("Leave empty to keep existing password");
		password_text_->Enable();
		password_text_->SetValidator({});
		if (password_enabler_)
			password_enabler_->SetValue(true);
	}
}

bool PasswordEditor::HasPassword() const
{
	return password_text_->IsThisEnabled();
}

bool PasswordEditor::TransferDataToWindow()
{
	if (!wxPanel::TransferDataToWindow())
		return false;

	SetPassword(password_);
	return true;
}

bool PasswordEditor::TransferDataFromWindow()
{
	if (!wxPanel::TransferDataFromWindow())
		return false;

	if (!password_)
		return true;

	bool old_pass_none = !password_->has_password();
	bool new_pass_none = !password_text_->IsThisEnabled();

	if ((!old_pass_none && !password_text_->GetValue().IsEmpty()) || old_pass_none != new_pass_none) {
		if (password_text_->IsThisEnabled()) {
			*password_ = fz::authentication::default_password(fz::to_utf8(password_text_->GetValue()));
			SetPassword(password_);
		}
		else {
			*password_= fz::authentication::password::none();
		}
	}

	return true;
}

// BUG: Clang requires these: clang's bug or something I don't understand about C++?
template struct wxValidateOnlyIfCurrentPage<PasswordEditor>;
template struct wxCreator::CtrlFix<wxValidateOnlyIfCurrentPage<PasswordEditor>>;

