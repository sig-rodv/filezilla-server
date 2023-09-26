#include <libfilezilla/glue/wx.hpp>

#include "listctrlex.hpp"
#include "locale.hpp"

wxListCtrlEx::wxListCtrlEx(wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxValidator &validator, const wxString &name)
{
	Create(parent, winid, pos, size, style, validator, name);
}

bool wxListCtrlEx::Create(wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxValidator &validator, const wxString &name)
{
	if (!wxListCtrl::Create(parent, winid, pos, size, style, validator, name))
		return false;

	timer_.Bind(wxEVT_TIMER, [&](wxTimerEvent &) {
		OnTimer(false);
	});

	Bind(Event::StartRefreshingTimer, [&](Event &) {
		OnTimer(true);
	});


	auto on_left_click = [this](wxMouseEvent &ev) {
		int flags = 0;
		auto row = HitTest({ev.GetX(), ev.GetY()}, flags);

		if (row != -1 && Event::ItemSelecting.Process(this, this, row).IsAllowed())
			ev.Skip();
	};

	Bind(wxEVT_LEFT_DOWN, on_left_click);
	Bind(wxEVT_LEFT_DCLICK, on_left_click);

	return true;
}

wxWindow *wxListCtrlEx::GetMainWindow()
{
	#ifdef __WXMSW__
		return this;
	#else
		return reinterpret_cast<wxWindow*>(m_mainWin);
	#endif
}

const wxWindow *wxListCtrlEx::GetMainWindow() const
{
	#ifdef __WXMSW__
		return this;
	#else
		return reinterpret_cast<const wxWindow*>(m_mainWin);
	#endif
}

long wxListCtrlEx::GetItemFromScreenPosition(const wxPoint &point) const
{
	int flags = 0;
	return HitTest(GetMainWindow()->ScreenToClient(point), flags);
}

void wxListCtrlEx::DoUpdate()
{
	auto old_count = GetItemCount();
	auto new_count = GetUpdatedItemCount();

	if (old_count <= new_count) {
		SetItemCount(new_count);
		Refresh();

		bool must_scroll = (new_count > 0) && (old_count-GetCountPerPage() <= GetTopItem());

		if (must_scroll)
			EnsureVisible(new_count-1);
	}
	else {
		EnsureVisible(new_count);
		SetItemCount(new_count);
		Refresh();
	}
}

void wxListCtrlEx::DelayedUpdate()
{
	fz::scoped_lock lock(mutex_);

	refresh_required_ = true;

	if (!timer_running_) {
		Event::StartRefreshingTimer.Queue(this, this);
		timer_running_ = true;
	}
}

long wxListCtrlEx::GetUpdatedItemCount() const
{
	return long(GetItemCount());
}

void wxListCtrlEx::SelectAll()
{
	for (long i = 0, end_i = GetItemCount(); i != end_i; ++i) {
		SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}
}

void wxListCtrlEx::OnTimer(bool start_timer)
{
	fz::scoped_lock lock(mutex_);

	if (refresh_required_) {
		refresh_required_ = false;

		lock.unlock();

		DoUpdate();

		if (start_timer)
			timer_.Start(100);
	}
	else {
		timer_.Stop();
		timer_running_ = false;
	}
}

wxListCtrlEx::Event::Event(wxEventType eventType, long item)
	: wxEventEx(eventType)
	, item_(item)
{}
