#include <libfilezilla/glue/wx.hpp>
#include <libfilezilla/string.hpp>

#include <numeric>

#include <wx/textctrl.h>
#include <wx/log.h>
#include <wx/choicebk.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/simplebook.h>
#include <wx/checkbox.h>
#include <wx/dcclient.h>
#include <wx/settings.h>

#include "certinfoeditor.hpp"
#include "generatecertificatedialog.hpp"

#include "helpers.hpp"
#include "locale.hpp"
#include "glue.hpp"

#include "../filezilla/string.hpp"

bool CertInfoEditor::cert_details::Create(wxWindow *parent)
{
	if (!wxPanel::Create(parent))
		return false;

	wxStaticVBox(this, _S("Information about the certificate")) = wxGBox(this, 2, {1}) = {
		{ 0, wxLabel(this, _S("Fingerprint (SHA-256):")) },
		{ 0, fingerprint_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY) },

		{ 0, wxLabel(this, _S("Activation date:")) },
		{ 0, activation_date_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY) },

		{ 0, wxLabel(this, _S("Expiration date:")) },
		{ 0, expiration_date_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY) },

		{ 0, wxLabel(this, _S("Distinguished name:")) },
		{ 0, distinguished_name_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY) },

		{ 0, wxLabel(this, _S("Applicable hostnames:")) },
		{ 0, hostnames_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY) }
	};

	Disable();

	return true;
}

void CertInfoEditor::cert_details::SetWaiting()
{
	Disable();
	Clear();

	fingerprint_ctrl_->SetValue(_S("Waiting for new fingerprint..."));
	activation_date_ctrl_->Clear();
	expiration_date_ctrl_->Clear();
	distinguished_name_ctrl_->Clear();
	hostnames_ctrl_->Clear();
}

void CertInfoEditor::cert_details::Clear()
{
	fingerprint_ctrl_->Clear();
	activation_date_ctrl_->Clear();
	expiration_date_ctrl_->Clear();
	distinguished_name_ctrl_->Clear();
	hostnames_ctrl_->Clear();
}

bool CertInfoEditor::Create(wxWindow *parent, cert_id genid, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	genid_ = genid;

	if (!wxPanel::Create(parent, winid, pos, size, style, name))
		return false;

	wxVBox(this, 0) = {
		{ 0, wxLabel(this, _S("&TLS certificate:")) },
		{ 0, book_ = new wxChoicebook(this, wxID_ANY) | [&](auto p) {
			wxPage(p, _S("Provide a X.509 certificate"), true) | [&](auto p) {
				wxVBox(p, 0) = {
					{ 0, wxLabel(p, _S("Private &key file:")) },
					{ 0, key_file_ctrl_ = new wxTextCtrl(p, wxID_ANY) },

					{ 0, wxLabel(p, _S("&Certificate file:")) },
					{ 0, certs_file_ctrl_ = new wxTextCtrl(p, wxID_ANY) },

					{ 0, wxLabel(p, _S("Key &password (stored in plaintext):")) },
					{ 0, key_file_pass_ctrl_ = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD) },
				};
			};

			wxPage(p, _S("Use a self-signed X.509 certificate")) | [&](auto p) {
				wxVBox(p, 0) = {
					{ 0, generate_selfsigned_ctrl_ = new wxButton(p, wxID_ANY, _S("&Generate new")) },
				};

				generate_selfsigned_ctrl_->Bind(wxEVT_BUTTON, [this, genid](auto) {
					if (!selfsigned_func_)
						return;

					generate_selfsigned_ctrl_->SetLabel(_S("Generating..."));
					generate_selfsigned_ctrl_->Disable();

					wxPushDialog<GenerateCertificateDialog>(this, _S("Certificate data")) | [this, genid](GenerateCertificateDialog &diag) {
						std::string dn;
						std::vector<std::string> hostnames;

						diag.SetDistinguishedName(&dn);
						diag.SetHostnames(&hostnames, 0, false);

						if (diag.ShowModal() == wxID_OK) {
							generated_selfsigned_details_ctrl_->SetWaiting();

							selfsigned_func_(genid, dn, hostnames);
						}
						else
							SetValue(cert_info_, extra_info_);
					};
				});
			};

			wxPage(p, _S("Use an uploaded X.509 certificate"), true) | [&](auto p) {
				wxVBox(p, 0) = {
					upload_ctrl_ = new wxButton(p, wxID_ANY, _S("&Upload certificate...")),
				};

				upload_ctrl_->Bind(wxEVT_BUTTON, [this, p](auto) {
					upload_ctrl_->Disable();

					wxPushDialog(p, wxID_ANY, _S("Upload certificate"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) | [this](wxDialog *p) {
						wxTextCtrl *cert;
						wxTextCtrl *key;
						wxTextCtrl *password;

						wxWindow *key_panel;

						auto maybe_split_keys_and_certs = [&key, &cert](const wxString &blob) {
							const auto &data = fz::to_wstring(blob);

							std::size_t begin_pos = data.npos;
							bool parsing_certificate = false;
							bool key_changed = false;
							bool cert_changed = false;

							for (const auto t: fz::strtokenizer(data, L"\n\r", true))
							{
								if (begin_pos == data.npos) {
									if (fz::starts_with(t, std::wstring_view(L"-----BEGIN "))) {
										if (fz::ends_with(t, std::wstring_view(L"PRIVATE KEY-----"))) {
											begin_pos = std::size_t(t.data() - data.data());
											parsing_certificate = false;
										}
										else
										if (fz::ends_with(t, std::wstring_view(L"CERTIFICATE-----"))) {
											begin_pos = std::size_t(t.data() - data.data());
											parsing_certificate = true;
										}
									}
								}

								if (begin_pos != data.npos) {
									if (fz::starts_with(t, std::wstring_view(L"-----END "))) {
										if (!parsing_certificate && fz::ends_with(t, std::wstring_view(L"PRIVATE KEY-----"))) {
											auto end_pos = std::size_t(t.data() - data.data()) + t.size();
											key->ChangeValue(data.substr(begin_pos, end_pos-begin_pos));
											key_changed = true;
											begin_pos = data.npos;
										}
										else
										if (parsing_certificate && fz::ends_with(t, std::wstring_view(L"CERTIFICATE-----"))) {
											auto end_pos = std::size_t(t.data() - data.data()) + t.size();
											cert->ChangeValue(data.substr(begin_pos, end_pos-begin_pos));
											cert_changed = true;
											begin_pos = data.npos;
										}
									}
								}
							}

							if (!key_changed && cert_changed) {
								const auto &to_remove = cert->GetValue();
								auto to_edit = key->GetValue();
								if (auto pos = to_edit.find(to_remove); pos != to_edit.npos) {
									to_edit.Remove(pos, to_remove.size());
									key->ChangeValue(to_edit);
								}
							}

							if (key_changed && !cert_changed) {
								const auto &to_remove = key->GetValue();
								auto to_edit = cert->GetValue();
								if (auto pos = to_edit.find(to_remove); pos != to_edit.npos) {
									to_edit.Remove(pos, to_remove.size());
									cert->ChangeValue(to_edit);
								}
							}
						};

						wxSize extent = wxMonospaceTextExtent(66, 12, p, {wxSYS_VSCROLL_X});

						wxVBox(p) = {
							wxSizerFlags(1).Expand() >>= new wxPanel(p) | [&](auto p) {
								wxVBox(p, 0) = {
									wxLabel(p, _S("&Certificate in PEM format:")),
									wxSizerFlags(1).Expand() >>= cert = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, extent, wxTE_MULTILINE),
									wxLoadTextFromFile(p, maybe_split_keys_and_certs, _S("&Load certificate from file..."), _S("Load certificate file"), wxEmptyString, wxEmptyString)
								};
							},

							wxSizerFlags(1).Expand() >>= key_panel = new wxPanel(p) | [&](auto p) {
								wxVBox(p, 0) = {
									wxLabel(p, _S("&Key in PEM format:")),
									wxSizerFlags(1).Expand() >>= key = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, extent, wxTE_MULTILINE),
									wxLoadTextFromFile(p, maybe_split_keys_and_certs, _S("L&oad key from file..."), _S("Load key file"), wxEmptyString, wxEmptyString)
								};
							},

							wxLabel(p, _S("Key &password (stored in plaintext):")),
							password = new wxTextCtrl(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD),

							new wxStaticLine(p),
							p->CreateButtonSizer(wxOK | wxCANCEL)
						};

						auto font = p->GetFont();
						font.SetFamily(wxFONTFAMILY_TELETYPE);

						key->SetFont(font);
						cert->SetFont(font);

						wxValidate(p, [&] {
							maybe_split_keys_and_certs(key->GetValue());
							maybe_split_keys_and_certs(cert->GetValue());

							if (cert->GetValue().empty()) {
								wxMsg::Error(_S("A certificate must be provided."));
								return false;
							}

							if (key->GetValue().empty()) {
								wxMsg::Error(_S("A private key must be provided."));
								return false;
							}

							auto error = fz::check_certificate_status(fz::to_string(key->GetValue()), fz::to_string(cert->GetValue()), fz::to_native(password->GetValue()), true);
							if (!error.empty()) {
								wxMsg::Error(_F("%s", error));
								return false;
							}

							return true;
						});

						if (auto ok = FindWindowById(wxID_OK, p))
							ok->SetLabel(_S("Upload"));

						if (p->ShowModal() != wxID_OK || !upload_func_) {
							upload_ctrl_->Enable();
							return;
						}

						upload_ctrl_->SetLabel(_S("Uploading..."));
						upload_func_(genid_, fz::to_utf8(cert->GetValue()), fz::to_utf8(key->GetValue()), fz::to_native(password->GetValue()));
					};
				});
			};
		}},

		{ 0, details_book_ = new wxSimplebook(this) |[&](auto p) {
			wxPage(p) |[&](auto p) {
				wxVBox(p, 0) = provided_details_ctrl_ = wxCreate<cert_details>(p);
			};

			wxPage(p) |[&](auto p) {
				wxVBox(p, 0) = generated_selfsigned_details_ctrl_ = wxCreate<cert_details>(p);
			};

			wxPage(p) |[&](auto p) {
				wxVBox(p, 0) = uploaded_details_ctrl_ = wxCreate<cert_details>(p);
			};
		}}
	};

	book_->Bind(wxEVT_CHOICEBOOK_PAGE_CHANGED, [&](auto &) {
		auto sel = book_->GetSelection();
		if (sel < 0)
			return;

		details_book_->ChangeSelection(std::size_t(sel));
	});

	key_file_ctrl_->Bind(wxEVT_TEXT, [this](auto &ev) {
		ev.Skip();
		provided_details_ctrl_->Enable(extra_info_.user_provided() && user_provided_cert_info_ && fz::to_wxString(user_provided_cert_info_->key_path) == key_file_ctrl_->GetValue());
	});

	certs_file_ctrl_->Bind(wxEVT_TEXT, [this](auto &ev) {
		ev.Skip();
		provided_details_ctrl_->Enable(extra_info_.user_provided() && user_provided_cert_info_ && fz::to_wxString(user_provided_cert_info_->certs_path) == certs_file_ctrl_->GetValue());
	});

	key_file_pass_ctrl_->Bind(wxEVT_TEXT, [this](auto &ev) {
		ev.Skip();
		provided_details_ctrl_->Enable(extra_info_.user_provided() && user_provided_cert_info_ && fz::to_wxString(user_provided_cert_info_->password) == key_file_pass_ctrl_->GetValue());
	});

	SetValue(nullptr, {});

	return true;
}

void CertInfoEditor::create_acme_editor()
{
	if (acme_ctrl_)
		return;

	if (!book_)
		return;

	acme_ctrl_ = wxPage(book_, _S("Use a Let's Encrypt® certificate")) | [&](auto p) {
		wxVBox(p, 0) = {
			{ 0, generate_acme_ctrl_ = new wxButton(p, wxID_ANY, _S("Set up Let's &Encrypt® options first")) },
		};

		generate_acme_ctrl_->Bind(wxEVT_BUTTON, [this](auto) {
			if (!acme_opts_)
				return;

			if (!*acme_opts_) {
				if (switch_to_acme_opts_) {
					SetValue(cert_info_, extra_info_);
					switch_to_acme_opts_();
				}

				return;
			}

			if (!acme_func_)
				return;

			generate_acme_ctrl_->SetLabel(_S("Generating..."));
			generate_acme_ctrl_->Disable();

			wxPushDialog<GenerateCertificateDialog>(this, _S("Certificate data")) |[this](GenerateCertificateDialog &diag) {
				fz::securable_socket::acme_cert_info info;

				if (auto a = std::get_if<fz::securable_socket::acme_cert_info>(cert_info_); a)
					info.hostnames = a->hostnames;

				info.account_id = acme_opts_->account_id;

				diag.SetHostnames(&info.hostnames, 1, true);

				if (diag.ShowModal() == wxID_OK) {
					generated_acme_details_ctrl_->SetWaiting();

					acme_func_(genid_, info);
				}
				else {
					SetValue(cert_info_, extra_info_);
				}
			};
		});
	};

	wxPage(details_book_) |[&](auto p) {
		wxVBox(p, 0) = generated_acme_details_ctrl_ = wxCreate<cert_details>(p);
	};

	book_->GetParent()->Layout();
}

void CertInfoEditor::SetValue(fz::securable_socket::cert_info *cert_info, const fz::securable_socket::cert_info::extra &extra)
{
	cert_info_ = cert_info;
	extra_info_ = extra;

	if (!cert_info_) {
		Disable();
		book_->SetSelection(0);
		return;
	}

	Enable();

	provided_details_ctrl_->Disable();
	provided_details_ctrl_->Clear();

	generated_selfsigned_details_ctrl_->Disable();
	generated_selfsigned_details_ctrl_->Clear();

	if (generated_acme_details_ctrl_) {
		generated_acme_details_ctrl_->Disable();
		generated_acme_details_ctrl_->Clear();
	}

	generate_selfsigned_ctrl_->Enable();
	generate_selfsigned_ctrl_->SetLabel(_S("&Generate new"));

	upload_ctrl_->Enable();
	upload_ctrl_->SetLabel(_S("&Upload certificate..."));

	if (generate_acme_ctrl_)
		generate_acme_ctrl_->Enable();

	key_file_ctrl_->Clear();
	certs_file_ctrl_->Clear();
	key_file_pass_ctrl_->Clear();

	if (!*cert_info_)
		book_->SetSelection(0);

	if (auto u = cert_info_->user_provided()) {
		book_->SetSelection(0);

		user_provided_cert_info_ = *u;

		key_file_ctrl_->SetValue(fz::to_wxString(u->key_path));
		certs_file_ctrl_->SetValue(fz::to_wxString(u->certs_path));
		key_file_pass_ctrl_->SetValue(fz::to_wxString(u->password));

		if (auto e = extra.user_provided()) {
			provided_details_ctrl_->Enable();

			provided_details_ctrl_->fingerprint_ctrl_->SetValue(fz::to_wxString(e->fingerprint));
			provided_details_ctrl_->activation_date_ctrl_->SetValue(fz::to_wxString(e->activation_time));
			provided_details_ctrl_->expiration_date_ctrl_->SetValue(fz::to_wxString(e->expiration_time));
			provided_details_ctrl_->distinguished_name_ctrl_->SetValue(fz::to_wxString(e->distinguished_name));
			provided_details_ctrl_->hostnames_ctrl_->SetValue(fz::join<wxString>(e->hostnames));
		}
	}

	if (auto g = cert_info_->autogenerated()) {
		book_->SetSelection(1);

		autogenerated_cert_info_ = *g;

		if (auto e = extra.autogenerated()) {
			generated_selfsigned_details_ctrl_->Enable();

			generated_selfsigned_details_ctrl_->fingerprint_ctrl_->SetValue(fz::to_wxString(g->fingerprint));
			generated_selfsigned_details_ctrl_->activation_date_ctrl_->SetValue(fz::to_wxString(e->activation_time));
			generated_selfsigned_details_ctrl_->expiration_date_ctrl_->SetValue(fz::to_wxString(e->expiration_time));
			generated_selfsigned_details_ctrl_->distinguished_name_ctrl_->SetValue(fz::to_wxString(e->distinguished_name));
			generated_selfsigned_details_ctrl_->hostnames_ctrl_->SetValue(fz::join<wxString>(e->hostnames));
		}
	}

	if (auto u = cert_info_->uploaded()) {
		book_->SetSelection(2);

		uploaded_cert_info_ = *u;

		if (auto e = extra.uploaded()) {
			uploaded_details_ctrl_->Enable();

			uploaded_details_ctrl_->fingerprint_ctrl_->SetValue(fz::to_wxString(e->fingerprint));
			uploaded_details_ctrl_->activation_date_ctrl_->SetValue(fz::to_wxString(e->activation_time));
			uploaded_details_ctrl_->expiration_date_ctrl_->SetValue(fz::to_wxString(e->expiration_time));
			uploaded_details_ctrl_->distinguished_name_ctrl_->SetValue(fz::to_wxString(e->distinguished_name));
			uploaded_details_ctrl_->hostnames_ctrl_->SetValue(fz::join<wxString>(e->hostnames));
		}
	}

	update_acme();
}

void CertInfoEditor::update_acme()
{
	if (!acme_ctrl_ || !cert_info_)
		return;

	if (auto a = cert_info_->acme()) {
		book_->SetSelection(3);

		acme_cert_info_ = *a;
		generate_acme_ctrl_->Enable();

		if (auto e = extra_info_.acme()) {
			generated_acme_details_ctrl_->Enable();

			generated_acme_details_ctrl_->fingerprint_ctrl_->SetValue(fz::to_wxString(e->fingerprint));
			generated_acme_details_ctrl_->activation_date_ctrl_->SetValue(fz::to_wxString(e->activation_time));
			generated_acme_details_ctrl_->expiration_date_ctrl_->SetValue(fz::to_wxString(e->expiration_time));
			generated_acme_details_ctrl_->distinguished_name_ctrl_->SetValue(fz::to_wxString(e->distinguished_name));
			generated_acme_details_ctrl_->hostnames_ctrl_->SetValue(fz::join<wxString>(a->hostnames));
		}
	}

	if (generate_acme_ctrl_) {
		if (!acme_opts_ || !*acme_opts_)
			generate_acme_ctrl_->SetLabel(_S("Set up Let's &Encrypt® options first"));
		else
			generate_acme_ctrl_->SetLabel(_S("&Generate new"));
	}
}

void CertInfoEditor::SetGenerateSelfsignedCertificateFunction(GenerateSelfsignedFunc func)
{
	selfsigned_func_ = std::move(func);
}

void CertInfoEditor::SetGenerateAcmeCertificateFunction(GenerateAcmeFunc func)
{
	acme_func_ = std::move(func);
}

void CertInfoEditor::SetUploadCertificateFunction(UploadCertificateFunc func)
{
	upload_func_ = std::move(func);
}

void CertInfoEditor::SetAcmeOptions(const server_settings::acme_options &acme_opts)
{
	acme_opts_ = &acme_opts;
	create_acme_editor();
	update_acme();
}

void CertInfoEditor::SetSwitchToAcmeOptsFunc(CertInfoEditor::SwitchToAcmeOptsFunc func)
{
	switch_to_acme_opts_ = std::move(func);
}

bool CertInfoEditor::TransferDataFromWindow()
{
	if (!wxPanel::TransferDataFromWindow())
		return false;

	if (!cert_info_)
		return true;

	if (book_->GetSelection() == 0) {
		if (upload_ctrl_->GetLabel() == _S("Uploading...")) {
			wxMsg::Error(_S("Still waiting for the new fingerprint.\nIn case of troubles, hit the CANCEL button and enter the settings dialog again."));
			return false;
		}

		if (key_file_ctrl_->IsEmpty() || certs_file_ctrl_->IsEmpty()) {
			wxMsg::Error(_S("The key and certificate fields must not be empty"));
			return false;
		}

		*cert_info_ = fz::securable_socket::user_provided_cert_info {
			fz::to_native(key_file_ctrl_->GetValue()),
			fz::to_native(certs_file_ctrl_->GetValue()),
			fz::to_native(key_file_pass_ctrl_->GetValue())
		};

		return true;
	}

	if (book_->GetSelection() == 1) {
		if (!autogenerated_cert_info_) {
			wxMsg::Error(_S("You must either generate a certificate or provide your own."));
			return false;
		}

		if (generate_selfsigned_ctrl_->GetLabel() == _S("Generating...")) {
			wxMsg::Error(_S("Still waiting for the new fingerprint.\nIn case of troubles, hit the CANCEL button and enter the settings dialog again."));
			return false;
		}

		*cert_info_ = *autogenerated_cert_info_;

		return true;
	}

	if (book_->GetSelection() == 2) {
		if (!uploaded_cert_info_) {
			wxMsg::Error(_S("You must either generate a certificate or provide your own."));
			return false;
		}

		if (upload_ctrl_->GetLabel() == _S("Uploading...")) {
			wxMsg::Error(_S("Still waiting for the new fingerprint.\nIn case of troubles, hit the CANCEL button and enter the settings dialog again."));
			return false;
		}

		*cert_info_ = *uploaded_cert_info_;

		return true;
	}

	if (book_->GetSelection() == 3) {
		if (!acme_cert_info_) {
			wxMsg::Error(_S("You must either generate a certificate or provide your own."));
			return false;
		}

		if (generate_acme_ctrl_->GetLabel() == _S("Generating...")) {
			wxMsg::Error(_S("Still waiting for the new fingerprint.\nIn case of troubles, hit the CANCEL button and enter the settings dialog again."));
			return false;
		}

		*cert_info_ = *acme_cert_info_;

		return true;
	}

	return false;
}

