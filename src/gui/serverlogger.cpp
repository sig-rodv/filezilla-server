#include <libfilezilla/glue/wx.hpp>

#include <glue.hpp>

#include <wx/menu.h>
#include <wx/listctrl.h>
#include <wx/timer.h>
#include <wx/artprov.h>

#include "serverlogger.hpp"
#include "locale.hpp"
#include "helpers.hpp"
#include "listctrlex.hpp"

#include "../filezilla/util/filesystem.hpp"
#include "../filezilla/logger/type.hpp"
#include "../filezilla/util/bits.hpp"
#include "../filezilla/string.hpp"
#include "../filezilla/transformed_view.hpp"

class ServerLogger::List: public wxListCtrlEx {
	enum ListCol {
		Date,
		Info,
		Type,
		Message
	};

public:
	List(ServerLogger &parent)
		: wxListCtrlEx(&parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxLC_VIRTUAL | wxLC_REPORT)
		, parent_(parent)
	{
		error_attr_.SetTextColour(wxColour(255, 0, 0));
		command_attr_.SetTextColour(wxColour(0, 0, 128));
		reply_attr_.SetTextColour(wxColour(0, 128, 0));
		warning_attr_.SetTextColour(wxColour(0xFF, 0x77, 0x22));
		trace_attr_.SetTextColour(wxColour(128, 0, 128));
		private_attr_.SetTextColour(wxColour(0, 128, 128));

		SetName(wxT("ServerLogger::List"));

		auto cl = new wxFluidColumnLayoutManager(this, true);

		InsertColumn(Date,    _S("Date/Time"));
		InsertColumn(Info,    _S("Info"));
		InsertColumn(Type,    _S("Type"));
		InsertColumn(Message, _S("Message"));

		// Hardcode default sizes that work well with the default Windows font.
		SetColumnWidth(Date, wxDlg2Px(this, 64));
		SetColumnWidth(Info, wxDlg2Px(this, 75));

		cl->SetColumnWeight(Message, 1);
	}

	auto GetItems(bool only_selected)
	{
		fz::scoped_lock lock(parent_.mutex_);

		return wxListCtrlEx::GetItems<Date, Info, Type, Message>(only_selected);
	}

	std::vector<Line::Session> GetSelectedIPs()
	{
		std::vector<Line::Session> ret;

		fz::scoped_lock lock(parent_.mutex_);

		for (long item = -1;;) {
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item < 0)
				break;

			if (std::size_t(item) > parent_.lines_.size())
				continue;

			if (auto &s = parent_.lines_[(std::size_t(item)+parent_.lines_begin_idx_) % parent_.max_number_lines_].session)
				ret.push_back(s);
		}

		ret.erase(std::unique(ret.begin(), ret.end()), ret.end());

		return ret;
	}

	void SelectAll()
	{
		fz::scoped_lock lock(parent_.mutex_);
		wxListCtrlEx::SelectAll();
	}

private:
	long GetUpdatedItemCount() const override
	{
		fz::scoped_lock lock(parent_.mutex_);

		return static_cast<long>(parent_.lines_.size());
	}

	wxString OnGetItemText(long row, long column) const override
	{
		static const auto type2str = [](fz::logmsg::type type) {
			using namespace fz;

			switch (type) {
				case logmsg::status:
					return _S("Status");

				case logmsg::error:
					return _S("Error");

				case logmsg::command:
					return _S("Command");

				case logmsg::reply:
					return _S("Response");

				case logmsg::warning:
					return _S("Warning");

				case logmsg::debug_warning:
				case logmsg::debug_info:
				case logmsg::debug_verbose:
				case logmsg::debug_debug:
					return _S("Trace");

				case logmsg::custom32:
					break;
			}

			if (type) {
				static const auto log2 = [](auto i) constexpr {
					return int(sizeof(i) * CHAR_BIT - util::count_leading_zeros(i)) - 1;
				};

				return _F("Private (%d)", log2(type) - log2(logmsg::custom1) + 1);
			}

			return wxString();
		};

		fz::scoped_lock lock(parent_.mutex_);

		if (row >= 0 && std::size_t(row) < parent_.lines_.size()) {
			auto &l = parent_.lines_[(std::size_t(row)+parent_.lines_begin_idx_) % parent_.max_number_lines_];

			switch (column) {
				case Date:    return fz::to_wxString(l.datetime);
				case Type:    return type2str(l.type);
				case Info:    return l.info;
				case Message: return l.message;
			}
		}

		return wxEmptyString;
	}

	int OnGetItemColumnImage(long /*row*/, long /*column*/) const override
	{
		//FIXME: return an image for the type column?
		return -1;
	}

	wxListItemAttr *OnGetItemAttr(long row) const override
	{
		fz::scoped_lock lock(parent_.mutex_);

		if (row >= 0 && std::size_t(row) < parent_.lines_.size()) {
			auto &l = parent_.lines_[(std::size_t(row)+parent_.lines_begin_idx_) % parent_.max_number_lines_];

			using namespace fz;

			switch (l.type) {
				case logmsg::error:
					return &error_attr_;

				case logmsg::command:
					return &command_attr_;

				case logmsg::reply:
					return &reply_attr_;

				case logmsg::warning:
					return &warning_attr_;

				case logmsg::debug_warning:
				case logmsg::debug_info:
				case logmsg::debug_verbose:
				case logmsg::debug_debug:
					return &trace_attr_;

				case logmsg::status:
					return nullptr;

				case logmsg::custom32:
					break;
			}
		}

		return &private_attr_;
	}

	ServerLogger &parent_;

	mutable wxListItemAttr command_attr_;
	mutable wxListItemAttr error_attr_;
	mutable wxListItemAttr reply_attr_;
	mutable wxListItemAttr warning_attr_;
	mutable wxListItemAttr trace_attr_;
	mutable wxListItemAttr private_attr_;
};


ServerLogger::ServerLogger()
{
}

ServerLogger::ServerLogger(wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	Create(parent, winid, pos, size, style, name);
}

bool ServerLogger::Create(wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxString &name)
{
	if (!wxPanel::Create(parent, winid, pos, size, style, name))
		return false;

	wxVBox(this, 0) = list_ = wxCreate<List>(*this);

	SetMaxNumberLines(10'000);

	Bind(wxEVT_CONTEXT_MENU, [&](const wxContextMenuEvent &){
		std::vector<std::array<wxString, 4>> selected_lines;

		bool only_selected = list_->GetSelectedItemCount() > 0;

		wxMenu menu;

		enum { clear = wxID_CLEAR, select_all = wxID_SELECTALL, copy_as_csv = wxID_HIGHEST, copy_as_html, copy_as_log, ban_ip };

		static const auto append = [](wxMenu &menu, auto id, auto text, const char *bmid = nullptr) {
			auto item = new wxMenuItem(&menu, id, text);
			if (bmid)
				item->SetBitmap(wxArtProvider::GetBitmap(bmid, wxART_MENU));
			menu.Append(item);
		};

		if (only_selected) {
			append(menu, copy_as_csv, _S("Copy selected lines as CSV"));
			append(menu, copy_as_html, _S("Copy selected lines as HTML"));
			append(menu, copy_as_log, _S("Copy selected lines as plaintext"));
		}
		else {
			append(menu, copy_as_csv, _S("Copy all lines as CSV"));
			append(menu, copy_as_html, _S("Copy all lines as HTML"));
			append(menu, copy_as_log, _S("Copy all lines as plaintext"));
		}

		menu.AppendSeparator();

		append(menu, clear, _S("Clear log"));
		append(menu, select_all, _S("Select all"));

		if (only_selected) {
			menu.AppendSeparator();
			append(menu, ban_ip, _S("Ban IPs"));
		}

		menu.Bind(wxEVT_MENU, [&](auto) {
			CopyToClipboardAsCsv(only_selected);
		}, copy_as_csv);

		menu.Bind(wxEVT_MENU, [&](auto) {
			CopyToClipboardAsHtml(only_selected);
		}, copy_as_html);

		menu.Bind(wxEVT_MENU, [&](auto) {
			CopyToClipboardAsLog(only_selected);
		}, copy_as_log);

		menu.Bind(wxEVT_MENU, [&](auto) {
			Clear();
		}, clear);

		menu.Bind(wxEVT_MENU, [&](auto) {
			list_->SelectAll();
		}, select_all);

		menu.Bind(wxEVT_MENU, [&](auto) {
			Event::IPsNeedToBeBanned.Process(this, this, list_->GetSelectedIPs());
		}, ban_ip);

		PopupMenu(&menu);
	});

	Bind(wxEVT_CHAR_HOOK, [&](wxKeyEvent &ev){
		ev.Skip();

		if (ev.ControlDown()) {
			if (ev.GetKeyCode() == 'A')
				list_->SelectAll();
			else
			if (ev.GetKeyCode() == 'C')
				CopyToClipboardAsLog(list_->GetSelectedItemCount() > 0);
		}
	});

	return true;
}

void ServerLogger::SetMaxNumberLines(std::size_t max)
{
	fz::scoped_lock lock(mutex_);

	max_number_lines_ = max;

	if (lines_.size() > max_number_lines_) {
		lines_.resize(max_number_lines_);
		list_->DelayedUpdate();
	}
	else {
		lines_.reserve(max_number_lines_);
	}
}

void ServerLogger::Log(Line &&line, bool remove_cntrl) {
	fz::scoped_lock lock(mutex_);

	if (remove_cntrl)
		line.message.erase(std::remove_if(line.message.begin(), line.message.end(), [](auto c) { return c <= 31 || c == 127;}), line.message.end());

	if (lines_.size() == max_number_lines_) {
		if (max_number_lines_ > 0) {
			lines_[lines_begin_idx_++] = std::move(line);
			lines_begin_idx_ %= max_number_lines_;
		}
	}
	else {
		lines_.emplace_back(std::move(line));
	}

	list_->DelayedUpdate();
}

void ServerLogger::Clear()
{
	fz::scoped_lock lock(mutex_);
	lines_.clear();
	lines_begin_idx_ = 0;
	list_->DelayedUpdate();
}

void ServerLogger::CopyToClipboardAsCsv(bool only_selected)
{
	auto lines = list_->GetItems(only_selected);

	auto to_copy = fz::join(fz::transformed_view(lines, [](const auto &l) {
		return fz::join(fz::transformed_view(l, [](const auto &v) {
			return fz::quote(fz::escaped(fz::to_wstring(v), L"\""));
		}), L",");
	}), L"\n");

	wxCopyToClipboard(fz::to_wxString(to_copy));
}

void ServerLogger::CopyToClipboardAsHtml(bool only_selected)
{
	wxString ret = wxT("<!doctype html>\n");

	ret += wxT("<html><body><table>\n");

	const auto &rows = list_->GetItems(only_selected);
	if (auto row = rows.cbegin(); row != rows.cend()) {
		ret += wxT("\t<tr>");

		for (const auto &e: *row)
			ret.append(wxT("<th style=\"text-align:left\">")).append(e).append("</th>");

		ret += wxT("</tr>\n");

		for (++row; row != rows.cend(); ++row) {
			ret += wxT("\t<tr>");

			for (const auto &e: *row)
				ret.append(wxT("<td>")).append(e).append("</td>");

			ret += wxT("</tr>\n");
		}
	}

	ret += wxT("</table><body></html>\n");

	wxCopyToClipboard(ret, true);
}

void ServerLogger::CopyToClipboardAsLog(bool only_selected)
{
	auto lines = list_->GetItems(only_selected);

	auto to_copy = fz::join(fz::transformed_view(lines, [](const auto &l) {
		return fz::sprintf(L"<%s> %s [%s] %s", l[0], l[1], l[2], l[3]);
	}), L"\n");

	wxCopyToClipboard(fz::to_wxString(to_copy));
}

void ServerLogger::do_log(fz::logmsg::type t, const fz::logger::modularized::info_list &info_list, std::wstring &&msg)
{
	Log({fz::datetime::now(), t, fz::to_wxString(info_list.as_string), fz::to_wxString(msg), {}});
}

ServerLogger::Line::Session::operator bool() const
{
	return id && !host.empty();
}

bool ServerLogger::Line::Session::operator ==(const Session &rhs) const
{
	return std::tie(id, host, address_family) == std::tie(rhs.id, rhs.host, rhs.address_family);
}
