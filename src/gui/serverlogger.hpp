#ifndef WXFZFTPSESSIONLOGGER_HPP
#define WXFZFTPSESSIONLOGGER_HPP

#include <libfilezilla/time.hpp>
#include <libfilezilla/mutex.hpp>

#include "../filezilla/logger/modularized.hpp"

#include <list>
#include <memory>

#include <wx/panel.h>

#include "fluidcolumnlayoutmanager.hpp"
#include "eventex.hpp"

class ServerLogger: public wxPanel, public fz::logger::modularized
{
public:
	ServerLogger();

	ServerLogger(wxWindow *parent,
						 wxWindowID winid = wxID_ANY,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize,
						 long style = wxTAB_TRAVERSAL | wxNO_BORDER,
						 const wxString& name = wxS("ServerLogger"));

	bool Create(wxWindow *parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxTAB_TRAVERSAL | wxNO_BORDER,
				const wxString& name = wxS("ServerLogger"));

	struct Line
	{
		fz::datetime datetime;
		fz::logmsg::type type;
		wxString info;
		wxString message;

		struct Session
		{
			uint64_t id{};
			wxString host;
			int address_family{};

			operator bool() const;
			bool operator ==(const Session &) const;
		} session;
	};

	struct Event: wxEventEx<Event>
	{
		inline const static Tag IPsNeedToBeBanned;

		std::vector<Line::Session> sessions;

	private:
		friend Tag;

		Event(const Tag &tag, std::vector<Line::Session> &&sessions)
			: wxEventEx(tag)
			, sessions(std::move(sessions))
		{}
	};

	void SetMaxNumberLines(std::size_t max);
	void Log(Line &&line, bool remove_cntrl = true);
	void Clear();

	void CopyToClipboardAsCsv(bool only_selected);
	void CopyToClipboardAsHtml(bool only_selected);
	void CopyToClipboardAsLog(bool only_selected);

private:
	class List;
	List *list_{};

	std::size_t max_number_lines_{};

	std::vector<Line> lines_{};
	std::size_t lines_begin_idx_{};

	fz::mutex mutex_{};

	// modularized interface
private:
	void do_log(fz::logmsg::type t, const info_list &info_list, std::wstring &&msg) override;
};

#endif // WXFZFTPSESSIONLOGGER_HPP
