#ifndef WXLISTCTRLEX_HPP
#define WXLISTCTRLEX_HPP

#include <array>

#include <libfilezilla/mutex.hpp>

#include <wx/listctrl.h>
#include <wx/timer.h>

#include "eventex.hpp"

class wxListCtrlEx: public wxListCtrl
{
public:
	struct Event: wxEventEx<Event>
	{
		using wxEventEx::wxEventEx;
		Event(wxEventType eventType, long item = -1);

		inline const static Tag ItemSelecting;

		bool IsAllowed() const {
			return allowed_;
		}

		void Veto() {
			allowed_ = false;
		}

		long GetItem() const {
			return item_;
		}

	private:
		long item_{-1};
		bool allowed_{true};

		friend wxListCtrlEx;

		inline const static Tag StartRefreshingTimer;
	};


	using wxListCtrl::wxListCtrl;

	wxListCtrlEx(wxWindow *parent,
				 wxWindowID winid = wxID_ANY,
				 const wxPoint &pos = wxDefaultPosition,
				 const wxSize &size = wxDefaultSize,
				 long style = wxLC_ICON,
				 const wxValidator& validator = wxDefaultValidator,
				 const wxString &name = wxS("wxListCtrlEx"));

	bool Create(wxWindow *parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint &pos = wxDefaultPosition,
				const wxSize &size = wxDefaultSize,
				long style = wxLC_ICON,
				const wxValidator& validator = wxDefaultValidator,
				const wxString &name = wxS("wxListCtrlEx"));

	wxWindow *GetMainWindow();
	const wxWindow *GetMainWindow() const;

	long GetItemFromScreenPosition(const wxPoint &point) const;

	void DelayedUpdate();

	virtual long GetUpdatedItemCount() const;

	template <long I, long... Is>
	std::vector<std::array<wxString, 1+sizeof...(Is)>> GetItems(bool only_selected) const
	{
		fz::scoped_lock lock(mutex_);

		std::vector<std::array<wxString, 1+sizeof...(Is)>> lines;

		auto get_col_name = [&](int i) {
			wxListItem li;
			li.SetMask(wxLIST_MASK_TEXT);
			GetColumn(i, li);
			return li.GetText();
		};

		lines.push_back({get_col_name(I), get_col_name(Is)...});

		if (only_selected > 0) {
			for (long item = -1;;) {
				item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
				if (item == -1)
					break;

				lines.push_back({OnGetItemText(item, I), OnGetItemText(item, Is)...});
			}
		}
		else {
			for (long item = 0, end_item = GetItemCount(); item != end_item; ++item) {
				lines.push_back({OnGetItemText(item, I), OnGetItemText(item, Is)...});
			}
		}

		return lines;
	}

	void SelectAll();

private:
	void DoUpdate();
	void OnTimer(bool start_timer);

	wxTimer timer_;
	bool refresh_required_{};
	bool timer_running_{};

	mutable fz::mutex mutex_;
};

#endif // WXLISTCTRLEX_HPP
