#ifndef SPEEDLIMITSEDITOR_HPP
#define SPEEDLIMITSEDITOR_HPP

#include <wx/panel.h>

#include "../filezilla/authentication/file_based_authenticator.hpp"

class wxIntegralEditor;

class SpeedLimitsEditor: public wxPanel
{
public:
	SpeedLimitsEditor() = default;

	bool Create(wxWindow *parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxTAB_TRAVERSAL | wxNO_BORDER,
				const wxString& name = wxS("speedlimitseditor"));

	//! Sets the limits the editor will have to display and let the user edit.
	//! It *doesn't* take ownership of the objects.
	void SetLimits(fz::authentication::file_based_authenticator::rate_limits *limits);

private:
	wxIntegralEditor *upload_shared_ctrl_{};
	wxIntegralEditor *download_shared_ctrl_{};
	wxIntegralEditor *upload_session_ctrl_{};
	wxIntegralEditor *download_session_ctrl_{};
};

#endif // SPEEDLIMITSEDITOR_HPP
