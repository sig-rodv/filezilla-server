#include <libfilezilla/glue/wx.hpp>
#include <libfilezilla/util.hpp>

#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>
#include <wx/valtext.h>
#include <wx/checkbox.h>
#include <wx/msgdlg.h>
#include <wx/combo.h>
#include <wx/checklst.h>
#include <wx/rearrangectrl.h>
#include <wx/simplebook.h>

#include "rearrangeditemspickerctrl.hpp"
#include "userseditor.hpp"
#include "mounttableeditor.hpp"
#include "userslist.hpp"
#include "filterseditor.hpp"
#include "credentialseditor.hpp"
#include "speedlimitseditor.hpp"
#include "locale.hpp"
#include "glue.hpp"

#include "helpers.hpp"

namespace {

enum page: size_t {
	enabled  = 0,
	disabled = 1
};

}

bool UsersEditor::Create(wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	if (!wxPanel::Create(parent, winid, pos, size, style, name))
		return false;

	wxHBox(this, 0) = {
		{ wxSizerFlags(0).Expand(), users_list_ = new UsersList(this) },
		{ wxSizerFlags(1).Expand(), book_ = wxCreate<wxNavigationEnabled<wxSimplebook>>(this) | [&](auto p) {
			wxPage<wxNotebook>(p, wxS("*Enabled*"), false, wxID_ANY) |[&](wxNotebook *p) {
				wxPage(p,  _S("General"), true) |[&](auto p) {
					wxVBox(p) = {
						{ 0, enabled_check_ = new wxCheckBox(p, wxID_ANY, _S("User is enabled")) },

						{ 0, wxLabel(p, _S("Aut&hentication:")) },
						{ 0, credentials_editor_ = wxCreate<CredentialsEditor>(p) },

						{ 0, wxLabel(p, _S("Member of &groups:")) },
						{ 0, groups_chooser_ = wxCreate<wxRearrangedItemsPickerCtrl>(p) },

						{ 0, wxLabel(p, _S("Mount p&oints:")) },
						{ 3, mount_table_editor_ = wxCreate<MountTableEditor>(p) },

						{ 0, wxLabel(p, _S("Descr&iption:")) },
						{ 1, description_editor_ = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE) }
					};
				};

				wxPage(p, _S("Filters")) |[&](auto p) {
					wxVBox(p) = filters_editor_ = wxCreate<FiltersEditor>(p);
				};

				wxPage(p, _S("Speed Limits")) |[&](auto p) {
					wxVBox(p) = speed_limits_editor_ = wxCreate<SpeedLimitsEditor>(p);
				};
			};

			wxPage(p, wxS("*Disabled*"), true) |[&](auto p) {
				wxVBox(p) = {
					wxEmptySpace,
					{ wxSizerFlags(0).Align(wxALIGN_CENTER), wxLabel(p, _S("Select or add a user in the list to the left")) },
						wxEmptySpace
					};
			};
		}}
	};

	users_list_->Bind(UsersList::Event::AboutToDeselectUser, [this](auto &ev){
		if (!Validate() || !TransferDataFromWindow())
			ev.Veto();
		else
			ev.Skip();
	});

	users_list_->Bind(UsersList::Event::SelectedUser, [this](auto &ev){
		ev.Skip();

		TransferDataToWindow();
	});

	return true;
}

void UsersEditor::SetGroups(const fz::authentication::file_based_authenticator::groups &groups)
{
	wxArrayString items;
	items.resize(groups.size());
	std::generate(items.begin(), items.end(), [it = groups.begin()]() mutable {
		return fz::to_wxString((it++)->first);
	});

	groups_chooser_->SetAvailableItems(std::move(items));
}

void UsersEditor::SetServerPathFormat(fz::util::fs::path_format server_path_format)
{
	mount_table_editor_->SetNativePathFormat(server_path_format);
}

void UsersEditor::SetUsers(fz::authentication::file_based_authenticator::users &users, std::string server_name)
{
	users_ = &users;
	server_name_ = server_name;
	users_list_->SetUsers(users);
}

bool UsersEditor::TransferDataFromWindow()
{
	if (!wxPanel::TransferDataFromWindow())
		return false;

	current_user_ = users_list_->GetSelectedUser();

	if (current_user_) {
		groups_chooser_->GetActiveItems(current_user_->second.groups);

		current_user_->second.description = fz::to_utf8(description_editor_->GetValue());
		current_user_->second.enabled = enabled_check_->GetValue();

		if (no_auth_)
			current_user_->second.methods = { {} };
		else
			current_user_->second.methods = { fz::authentication::method::password() };
	}

	return true;
}

bool UsersEditor::TransferDataToWindow()
{
	if (!wxPanel::TransferDataToWindow())
		return false;

	current_user_ = users_list_->GetSelectedUser();

	if (!current_user_) {
		book_->ChangeSelection(page::disabled);

		credentials_editor_->SetCredentials(false, nullptr, nullptr, {});
		groups_chooser_->SetActiveItems({});
		mount_table_editor_->SetTable(nullptr);
		filters_editor_->SetIps(nullptr, nullptr);
		speed_limits_editor_->SetLimits(nullptr);
		description_editor_->ChangeValue(wxEmptyString);
	}
	else {
		bool is_system_user = current_user_->first == fz::authentication::file_based_authenticator::users::system_user_name;

		book_->ChangeSelection(page::enabled);

		enabled_check_->SetValue(current_user_->second.enabled);

		no_auth_ = !current_user_->second.methods.is_auth_necessary();

		credentials_editor_->SetCredentials(is_system_user, &current_user_->second.credentials, &no_auth_, std::string(current_user_->first).append("@").append(server_name_));
		groups_chooser_->SetActiveItems(current_user_->second.groups);
		mount_table_editor_->SetTable(&current_user_->second.mount_table);
		filters_editor_->SetIps(&current_user_->second.disallowed_ips, &current_user_->second.allowed_ips);
		speed_limits_editor_->SetLimits(&current_user_->second.rate_limits);
		description_editor_->ChangeValue(fz::to_wxString(current_user_->second.description));
	}

	return true;
}
