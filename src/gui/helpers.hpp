#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <variant>
#include <functional>
#include <stack>
#include <queue>

#include <libfilezilla/logger.hpp>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/notebook.h>
#include <wx/treebook.h>
#include <wx/choicebk.h>
#include <wx/panel.h>
#include <wx/msgdlg.h>
#include <wx/wizard.h>
#include <wx/hyperlink.h>
#include <wx/textctrl.h>
#include <wx/simplebook.h>
#include <wx/timer.h>
#include <wx/checkbox.h>
#include <wx/settings.h>
#include <wx/button.h>

#include "dialogex.hpp"
#include "locale.hpp"
#include "wrappedtext.hpp"

class wxTitleCtrl: public wxPanel
{
public:
	wxTitleCtrl(wxWindow *parent, const wxString &title, int style = 0);

	bool AcceptsFocus() const override;
	bool AcceptsFocusFromKeyboard() const override;

private:
	wxWrappedText *label_;
};

wxTitleCtrl *wxTitle(wxWindow *parent, const wxString &title, int style = 0);

template <typename Sizer, typename Object, typename Derived>
struct wxSizerWrapper;

struct wxSizerProxy
{
	wxSizerProxy(wxWindow *parent, wxSizer *sizer);
	wxSizerProxy(wxSizerProxy &&rhs) noexcept;

	wxSizerProxy(const wxSizerProxy &) = delete;
	wxSizerProxy &operator=(const wxSizerProxy &) = delete;
	wxSizerProxy &operator=(wxSizerProxy &&) = delete;

	~wxSizerProxy();

	operator wxSizer *();
	wxSizer *operator->();

private:
	wxWindow *parent_;
	wxSizer *sizer_;
};

template <typename T>
struct wxTypedSizerProxy final: wxSizerProxy
{
	static_assert(std::is_base_of_v<wxSizer, T>);

	wxTypedSizerProxy(wxWindow *parent, T *sizer)
		: wxSizerProxy(parent, sizer)
	{}

	wxTypedSizerProxy(wxTypedSizerProxy &&rhs) noexcept
		: wxSizerProxy(std::move(rhs))
	{}

	operator T*()
	{
		return static_cast<T *>(wxSizerProxy::operator wxSizer *());
	}

	T *operator->()
	{
		return static_cast<T*>(wxSizerProxy::operator ->());
	}
};

int wxDlg2Px(wxWindow *w, int dlg);

wxWindow *wxValidate(wxWindow *w, std::function<bool()> func);
wxWindow *wxTransferDataToWindow(wxWindow *w, std::function<bool()> func);
wxWindow *wxTransferDataFromWindow(wxWindow *w, std::function<bool()> func);

bool wxCopyToClipboard(const wxString &str, bool is_html = false);

struct wxWrappedTextProxy
{
	wxWrappedTextProxy(wxWrappedText *w)
		: w_(w)
	{}

	operator wxWrappedText *()
	{
		return w_;
	}

	wxWrappedText* operator ->()
	{
		return w_;
	}

	wxWrappedText& operator *()
	{
		return *w_;
	}

	wxWrappedTextProxy &&Weight(wxFontWeight weight) &&
	{
		auto f = w_->GetFont();
		f.SetWeight(weight);
		w_->SetFont(f);

		return std::move(*this);
	}

	wxWrappedTextProxy &&Style(wxFontStyle style) &&
	{
		auto f = w_->GetFont();
		f.SetStyle(style);
		w_->SetFont(f);

		return std::move(*this);
	}

	wxWrappedTextProxy &&Smaller() &&
	{
		w_->SetFont(w_->GetFont().MakeSmaller());

		return std::move(*this);
	}

	wxWrappedTextProxy &&Larger() &&
	{
		w_->SetFont(w_->GetFont().MakeLarger());

		return std::move(*this);
	}

	wxWrappedTextProxy &&Italic() &&
	{
		w_->SetFont(w_->GetFont().MakeItalic());

		return std::move(*this);
	}

	wxWrappedTextProxy &&Bold() &&
	{
		w_->SetFont(w_->GetFont().MakeBold());

		return std::move(*this);
	}

	template <typename Func>
	std::enable_if_t<
		std::is_invocable_v<Func, wxWrappedText*>,
		wxWrappedTextProxy &&
	>
	operator |(Func && func) &&
	{
		std::forward<Func>(func)(w_);

		return std::move(*this);
	}

private:
	wxWrappedText *w_;
};

wxWrappedTextProxy wxLabel(wxWindow *parent, const wxString &text, int style = 0);
wxWrappedTextProxy wxEscapedLabel(wxWindow *parent, const wxString &text, int style = 0);
wxWrappedTextProxy wxWText(wxWindow *parent, const wxString &text, int style = 0);

wxSize wxTextExtent(std::size_t num_characters_per_line, std::size_t num_lines, wxWindow *win, wxFontFamily = wxFONTFAMILY_UNKNOWN, std::initializer_list<wxSystemMetric> x_metrics = {}, std::initializer_list<wxSystemMetric> y_metrics = {});
wxSize wxTextExtent(const wxString &text, wxWindow *win, wxFontFamily = wxFONTFAMILY_UNKNOWN, std::initializer_list<wxSystemMetric> x_metrics = {}, std::initializer_list<wxSystemMetric> y_metrics = {});

wxSize wxMonospaceTextExtent(std::size_t num_characters_per_line, std::size_t num_lines, wxWindow *win, std::initializer_list<wxSystemMetric> x_metrics = {}, std::initializer_list<wxSystemMetric> y_metrics = {});
wxSize wxMonospaceTextExtent(const wxString &text, wxWindow *win, std::initializer_list<wxSystemMetric> x_metrics = {}, std::initializer_list<wxSystemMetric> y_metrics = {});

void wxSetWindowLogger(wxWindow *win, fz::logger_interface *logger);
fz::logger_interface *wxGetWindowLogger(wxWindow *win);

struct wxMsg
{
	struct Builder;
	struct Opener;

	static Opener Error(const wxString &msg);
	static Opener Warning(const wxString &msg);
	static Opener Success(const wxString &msg);
	static Opener Confirm(const wxString &msg);

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	static Error(const wxString &format, const Args &... args)
	{
		return Error(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	static Warning(const wxString &format, const Args &... args)
	{
		return Warning(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	static Success(const wxString &format, const Args &... args)
	{
		return Success(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	static Confirm(const wxString &format, const Args &... args)
	{
		return Confirm(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}
};

struct wxMsg::Builder
{
	Builder() = default;
	Builder(Builder &&rhs) = default;
	Builder(const Builder &) = default;
	Builder &operator=(const Builder &) = default;
	Builder &operator=(Builder &&) = default;

	Builder &Flags(int flags);
	Builder &Message(const wxString &msg);
	Builder &Title(const wxString &title);
	Builder &Ext(const wxString &ext);
	Builder &JustLog(bool just_log = true);
	Builder &Wait(int *result = nullptr);

	void Open();

	Opener Error(const wxString &);
	Opener Warning(const wxString &);
	Opener Success(const wxString &);
	Opener Confirm(const wxString &);

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Builder&>
	Message(const wxString &format, const Args &... args)
	{
		return Message(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Builder&>
	Ext(const wxString &format, const Args &... args)
	{
		return Ext(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	Error(const wxString &format, const Args &... args)
	{
		return Error(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	Warning(const wxString &format, const Args &... args)
	{
		return Warning(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	Success(const wxString &format, const Args &... args)
	{
		return Success(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener>
	Confirm(const wxString &format, const Args &... args)
	{
		return Confirm(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

private:
	friend Opener;

	int flags_{};
	wxString msg_;
	wxString title_;
	wxString ext_;
	int *result_{};
};

struct wxMsg::Opener
{
	Opener(Opener &&) = delete;
	Opener(const Opener &) = delete;
	Opener &operator=(Opener &&) = delete;
	Opener &operator=(const Opener &) = delete;

	~Opener();

	operator int();

	Opener &&Ext(const wxString &ext) &&;
	Opener &&Title(const wxString &title) &&;
	Opener &&JustLog(bool just_log = true) &&;
	Opener &&Wait(int *result = nullptr) &&;

	template <typename... Args>
	std::enable_if_t<(sizeof...(Args) > 0), Opener&&>
	Ext(const wxString &format, const Args &... args) &&
	{
		return std::move(*this).Ext(fz::to_wxString(fz::sprintf(format.ToStdWstring(), args...)));
	}

private:
	friend Builder;

	Opener(const Builder &bld, int flags, const wxString &msg);

	Builder bld_;
	bool already_open_{};
};

template <typename SizerObject>
class wxSizerPair: public std::pair<wxSizerFlags, SizerObject>
{
public:
	using std::pair<wxSizerFlags, SizerObject>::pair;
	using std::pair<wxSizerFlags, SizerObject>::operator=;

	template <typename T, std::enable_if_t<std::is_convertible_v<T, SizerObject>>* = nullptr>
	wxSizerPair(T obj)
		: wxSizerPair{wxSizerFlags(0), std::move(obj)}
	{}

	static const wxSizerFlags default_first;
};

template <typename SizerObject>
const wxSizerFlags wxSizerPair<SizerObject>::default_first = wxSizerFlags(1);

template <typename T>
std::pair<wxSizerFlags, T> operator >>=(wxSizerFlags f, T t)
{
	return { std::move(f), std::move(t) };
}

using wxBoxSizerObject = std::variant<wxWindow *, wxSizer *, int>;
void wxAddToSizer(wxBoxSizer *sizer, wxWindow *parent, std::initializer_list<wxSizerPair<wxBoxSizerObject>> elems, int gap);

using wxGridSizerObject = wxBoxSizerObject; //std::variant<wxWindow *, wxSizer *, wxString>;
void wxAddToSizer(wxFlexGridSizer *sizer, wxWindow *parent, std::initializer_list<wxSizerPair<wxGridSizerObject>> elems, wxAlignment default_alignment = wxALIGN_CENTER_VERTICAL);

using wxSDBSizerObject = wxButton*;
enum wxSDBType {
	wxAffirmative = 1,
	wxNegative,
	wxCancel
};

template <>
class wxSizerPair<wxSDBSizerObject>: public std::pair<wxSDBType, wxSDBSizerObject>
{
public:
	using std::pair<wxSDBType, wxSDBSizerObject>::pair;
	using std::pair<wxSDBType, wxSDBSizerObject>::operator=;

	template <typename T, std::enable_if_t<std::is_convertible_v<T, wxSDBSizerObject>>* = nullptr>
	wxSizerPair(T obj)
		: wxSizerPair{wxSDBType{}, std::move(obj)}
	{}

	template <typename T, std::enable_if_t<std::is_convertible_v<T, wxSDBSizerObject>>* = nullptr>
	wxSizerPair(const std::pair<wxSDBType, T> &p)
		: wxSizerPair{p.first, p.second}
	{}

	static const wxSDBType default_first;
};

inline const wxSDBType wxSizerPair<wxSDBSizerObject>::default_first = wxSDBType{};

template <typename T>
std::pair<wxSDBType, T> operator >>=(wxSDBType f, T t)
{
	return { std::move(f), std::move(t) };
}

struct wxSDBSizer: wxBoxSizer
{
	wxSDBSizer()
		: wxBoxSizer(wxVERTICAL)
	{}

private:
	friend void wxAddToSizer(wxSDBSizer *sizer, wxWindow *w, std::initializer_list<wxSizerPair<wxSDBSizerObject>> elems);
	wxStdDialogButtonSizer *sdb_{};
};

void wxAddToSizer(wxSDBSizer *sizer, wxWindow *w, std::initializer_list<wxSizerPair<wxSDBSizerObject>> elems);

template <typename Sizer, typename Object, typename Derived>
struct wxSizerWrapper
{
	wxSizerWrapper(wxWindow *parent, Sizer *sizer)
		: parent_(parent)
		, sizer_(sizer)
	{}

	wxTypedSizerProxy<Sizer> operator =(std::initializer_list<wxSizerPair<Object>> elems) &&
	{
		static_cast<Derived &>(*this).wxAddToSizer(sizer_, parent_, std::move(elems));

		return {parent_, sizer_};
	}

	wxTypedSizerProxy<Sizer> operator =(std::pair<typename wxSizerPair<Object>::first_type, Object> elem) &&
	{
		return (std::move(static_cast<Derived &>(*this))) = { wxSizerPair<Object>(std::move(elem.first), std::move(elem.second)) };
	}

	wxTypedSizerProxy<Sizer> operator =(Object elem) &&
	{
		return (std::move(static_cast<Derived &>(*this))) = { wxSizerPair<Object>(wxSizerPair<Object>::default_first, std::move(elem)) };
	}

protected:
	void wxAddToSizer(Sizer *sizer, wxWindow *parent, std::initializer_list<wxSizerPair<Object>> elems)
	{
		::wxAddToSizer(sizer, parent, elems);
	}

	wxWindow *const parent_;
	Sizer *sizer_;
};

struct wxGridSizerWrapper: wxSizerWrapper<wxFlexGridSizer, wxGridSizerObject, wxGridSizerWrapper>
{
	using wxSizerWrapper::wxSizerWrapper;
	using wxSizerWrapper::operator=;

	wxGridSizerWrapper(wxWindow *parent, wxFlexGridSizer *sizer, wxAlignment default_alignment = wxALIGN_CENTER_VERTICAL);

	void wxAddToSizer(wxFlexGridSizer *sizer, wxWindow *parent, std::initializer_list<wxSizerPair<wxGridSizerObject>> elems);

private:
	wxAlignment default_alignment_;
};


struct wxSBDSizerWrapper: wxSizerWrapper<wxSDBSizer, wxSDBSizerObject, wxSBDSizerWrapper>
{
	using wxSizerWrapper::wxSizerWrapper;
	using wxSizerWrapper::operator=;

	wxSBDSizerWrapper(wxWindow *parent, wxSDBSizer *sizer);
};

enum: int { wxDefaultPadding = 2 };
enum: int { wxDefaultGap = 2 };

struct wxPadding
{
	constexpr wxPadding(wxDirection dir, int pad = wxDefaultPadding)
		: dir(dir)
		, pad(pad)
	{}

	constexpr wxPadding(int pad = wxDefaultPadding)
		: wxPadding(wxALL, pad)
	{}

	wxDirection dir;
	int pad;
};

struct wxBoxSizerWrapper: wxSizerWrapper<wxBoxSizer, wxBoxSizerObject, wxBoxSizerWrapper>
{
	wxBoxSizerWrapper(wxWindow *parent, wxBoxSizer *sizer, wxPadding padding = wxDefaultPadding, int gap = wxDefaultGap);

	wxTypedSizerProxy<wxBoxSizer> operator =(std::initializer_list<wxSizerPair<wxBoxSizerObject>> elems) &&;

	using wxSizerWrapper::operator=;

	void wxAddToSizer(wxBoxSizer *sizer, wxWindow *parent, std::initializer_list<wxSizerPair<wxBoxSizerObject>> elems);

protected:
	int padding_;
	wxDirection padding_direction_;
	int gap_;
};

wxBoxSizerWrapper wxHBox(wxWindow *parent, wxPadding padding = wxDefaultPadding, int gap = wxDefaultGap);
wxBoxSizerWrapper wxVBox(wxWindow *parent, wxPadding padding = wxDefaultPadding, int gap = wxDefaultGap);
wxBoxSizerWrapper wxStaticHBox(wxWindow *p, const wxString &label = wxEmptyString, wxPadding padding = wxDefaultPadding, int gap = wxDefaultGap);
wxBoxSizerWrapper wxStaticVBox(wxWindow *p, const wxString &label = wxEmptyString, wxPadding padding = wxDefaultPadding, int gap = wxDefaultGap);
wxSBDSizerWrapper wxSBDBox(wxWindow *parent);

constexpr inline auto wxEmptySpace = std::pair{1, -1};

struct wxGBoxGrowableParams
{
	wxGBoxGrowableParams(std::size_t idx, int proportion = 0)
		: idx(idx)
		, proportion(proportion)
	{}

	const std::size_t idx;
	const int proportion;
};

inline static const wxSize wxGBoxDefaultGap = {wxDefaultGap, wxDefaultGap};
wxGridSizerWrapper wxGBox(wxWindow *parent, int cols, std::initializer_list<wxGBoxGrowableParams> growable_cols = {}, std::initializer_list<wxGBoxGrowableParams> growable_rows = {}, wxSize gap = wxGBoxDefaultGap, wxAlignment default_alignment = wxALIGN_CENTER_VERTICAL);

struct wxValidateOnlyIfCurrentPageBase
{
	virtual ~wxValidateOnlyIfCurrentPageBase() = default;

	bool changing_ = false;
};

template <typename W = wxPanel, std::enable_if_t<std::is_base_of_v<wxWindow,W>>* = nullptr>
struct wxValidateOnlyIfCurrentPage: W, virtual wxValidateOnlyIfCurrentPageBase
{
	using W::W;

	bool Validate() override
	{
		if (auto b = dynamic_cast<wxBookCtrlBase *>(this->GetParent())) {
			return b->GetCurrentPage() != this || W::Validate();
		}

		return W::Validate();
	}

	bool TransferDataToWindow() override
	{
		if (auto b = dynamic_cast<wxBookCtrlBase *>(this->GetParent())) {
			return (!changing_ && b->GetCurrentPage() != this) || W::TransferDataToWindow();
		}

		return W::TransferDataToWindow();
	}

	bool TransferDataFromWindow() override
	{
		if (auto b = dynamic_cast<wxBookCtrlBase *>(this->GetParent())) {
			return b->GetCurrentPage() != this || W::TransferDataFromWindow();
		}

		return W::TransferDataFromWindow();
	}
};

struct wxCreator
{
	template <typename T, typename SFINAE = void>
	struct CtrlFix: T
	{
		using T::T;
	};

	template <typename T>
	struct CtrlFix<T, std::enable_if_t<std::is_base_of_v<wxBookCtrlBase, T>>>: T
	{
		using T::T;

		// BUG: see https://trac.wxwidgets.org/ticket/16878
		bool HasTransparentBackground() override
		{
			return true;
		}

		bool Validate() override
		{
			if constexpr (!std::is_base_of_v<wxValidateOnlyIfCurrentPageBase, T>) {
				bool not_overridden = typeid(&wxWindow::Validate) == typeid(&T::Validate);
				wxASSERT_MSG(not_overridden, "The book control you're using has overridden the Validate method: this is not supported");

				if (not_overridden)
					return apply_to_all_pages(this, &wxWindow::Validate, "Validate");
			}

			return T::Validate();
		}

		bool TransferDataFromWindow() override
		{
			if constexpr (!std::is_base_of_v<wxValidateOnlyIfCurrentPageBase, T>) {
				bool not_overridden = typeid(&wxWindow::TransferDataFromWindow) == typeid(&T::TransferDataFromWindow);
				wxASSERT_MSG(not_overridden, "The book control you're using has overridden the TransferDataFromWindow method: this is not supported");

				if (not_overridden)
					return apply_to_all_pages(this, &wxWindow::TransferDataFromWindow, "TransferDataFromWindow");
			}

			return T::TransferDataFromWindow();
		}

		bool TransferDataToWindow() override
		{
			if constexpr (!std::is_base_of_v<wxValidateOnlyIfCurrentPageBase, T>) {
				bool not_overridden = typeid(&wxWindow::TransferDataToWindow) == typeid(&T::TransferDataToWindow);
				wxASSERT_MSG(not_overridden, "The book control you're using has overridden the TransferDataToWindow method: this is not supported");

				if (not_overridden)
					return apply_to_all_pages(this, &wxWindow::TransferDataToWindow, "TransferDataToWindow");
			}

			return T::TransferDataToWindow();
		}
	};

	template <typename W, typename Args, typename = void>
	struct supports_two_phases_creation: std::false_type {};

	template <typename W, typename... Args>
	struct supports_two_phases_creation<W, std::tuple<Args...>, std::void_t<decltype(W(), std::declval<W>().Create(std::declval<Args>()...) == true)>>: std::true_type{};

	template <typename W, typename... Args>
	std::enable_if_t<
		supports_two_phases_creation<W, std::tuple<Args...>>::value,
	std::add_pointer_t<W>>
	static create(Args &&... args)
	{
		auto w = new CtrlFix<W>();

		if (!w->Create(std::forward<Args>(args)...))
			abort();

		return w;
	}

	template <typename W, typename... Args>
	std::enable_if_t<
		sizeof...(Args) >= 1 &&
		std::is_constructible_v<W, Args...> &&
		!wxCreator::supports_two_phases_creation<W, std::tuple<Args...>>::value,
	std::add_pointer_t<W>>
	static create(Args &&... args)
	{
		return new CtrlFix<W>(std::forward<Args>(args)...);
	}

private:
	static bool apply_to_all_pages(wxBookCtrlBase *b, bool (wxWindow::*m)(), const char *name);
};


template <typename W, typename... Args>
auto wxCreate(Args &&... args) -> decltype(wxCreator::create<W>(std::forward<Args>(args)...))
{
	return wxCreator::create<W>(std::forward<Args>(args)...);
}


class wxDialogQueue
{
	using creator_t = std::function<wxDialog *(wxWindow *parent)>;
	using opener_t = std::function<void (wxDialog *)>;

	struct entry_t
	{
		wxWeakRef<wxWindow> parent;
		creator_t creator;
		opener_t opener;
	};

	using stack = std::stack<wxWeakRef<wxDialog>>;
	using queue = std::queue<entry_t>;

	template <typename D, typename ...Args>
	struct Pusher
	{
		Pusher(wxDialogQueue &owner, wxWindow *parent, creator_t && creator)
			: owner_(owner)
			, parent_(parent)
			, creator_(std::move(creator))
		{
			wxASSERT_MSG(parent != nullptr, "parent must not be NULL");
		}

		~Pusher()
		{
			if (!opener_)
				opener_ = [](wxDialog *d) {	d->ShowModal();	};

			owner_.queue_.push({parent_, std::move(creator_), opener_});

			// Asynchronously invoke try_dequeue. The calling code must make no assumptions about whether pushing a dialog immediately opens it or not.
			owner_.timer_.CallAfter([owner = &this->owner_] {
				owner->try_dequeue();
			});
		}

		template <typename O>
		std::enable_if_t<std::is_invocable_v<O, D*>>
		operator|(O opener) &&
		{
			opener_ = [opener = std::move(opener)](wxDialog *d) mutable {
				opener(static_cast<D*>(d));
			};
		}

		template <typename O>
		std::enable_if_t<std::is_invocable_v<O, D&>>
		operator|(O opener) &&
		{
			opener_ = [opener = std::move(opener)](wxDialog *d) mutable {
				opener(*static_cast<D*>(d));
			};
		}

	private:
		wxDialogQueue &owner_;
		wxWindow *parent_;
		creator_t creator_;
		opener_t opener_;
	};

public:
	template <typename D, typename... Args>
		std::enable_if_t<std::is_base_of_v<wxDialog, D> && std::is_invocable_v<decltype(wxCreate<D, wxWindow *, Args...>), wxWindow *, Args...>,
	Pusher<D>> static push(wxWindow *parent, Args &&... args)
	{
		return {instance(), parent, [t = std::make_tuple(std::forward<Args>(args)...)](wxWindow *parent) mutable {
			return std::apply([parent](auto &&... args) mutable {
				return wxCreate<D>(parent, std::forward<decltype(args)>(args)...);
			}, std::move(t));
		}};
	}

private:
	wxDialogQueue();

	static wxDialogQueue &instance();
	void try_dequeue();
	bool must_be_delayed();

	stack stack_;
	queue queue_;

	wxTimer timer_;

#ifdef __WXMAC__
	std::stack<void *> shown_dialogs_creation_events_;
#endif
};

template <typename D = wxDialog, typename... Args, std::enable_if_t<!fx::trait::is_a_wxDialogEx_v<D>>* = nullptr>
auto wxPushDialog(Args &&... args) -> decltype(wxDialogQueue::push<wxDialogEx<D>>(std::forward<Args>(args)...))
{
	return wxDialogQueue::push<wxDialogEx<D>>(std::forward<Args>(args)...);
}

template <typename D = wxDialog, typename... Args, std::enable_if_t<fx::trait::is_a_wxDialogEx_v<D>>* = nullptr>
auto wxPushDialog(Args &&... args) -> decltype(wxDialogQueue::push<D>(std::forward<Args>(args)...))
{
	return wxDialogQueue::push<D>(std::forward<Args>(args)...);
}

template <typename W = wxPanel, typename... Args>
auto wxPage(wxBookCtrlBase *b, const wxString &title, bool select, Args &&... args) -> decltype(wxCreate<W>(b, std::forward<Args>(args)...))
{
	auto page = wxCreate<W>(b, std::forward<Args>(args)...);

	if (!dynamic_cast<wxSimplebook *>(b))
		b->SetInternalBorder(unsigned(wxDlg2Px(b, wxDefaultGap)));

	b->AddPage(page, title, select);
	return page;
}

template <typename W = wxPanel>
auto wxPage(wxBookCtrlBase *b, const wxString &title = wxEmptyString) -> decltype(wxCreate<W>(b))
{
	auto page = wxCreate<W>(b);

	if (!dynamic_cast<wxSimplebook *>(b))
		b->SetInternalBorder(unsigned(wxDlg2Px(b, wxDefaultGap)));

	b->AddPage(page, title);
	return page;
}

template <typename W>
struct wxTreebookPageWrapper
{
	wxTreebookPageWrapper(W *w, wxTreebook *b, wxPanel *p)
		: ctrl_(w)
		, book_(b)
		, page_(p)
	{}

	operator W*() const
	{
		return ctrl_;
	}

	W* operator->() const
	{
		return ctrl_;
	}

	W &operator*() const
	{
		return *ctrl_;
	}

	wxTreebook *GetBook() const
	{
		return book_;
	}

	int GetPagePos() const
	{
		return book_->FindPage(page_);
	}

	template <typename Func>
	std::enable_if_t<
		std::is_invocable_v<Func, wxTreebookPageWrapper &>,
		wxTreebookPageWrapper &&
	>
	operator |(Func && func) &&
	{
		std::forward<Func>(func)(*this);

		return std::move(*this);
	}

private:
	W *ctrl_;
	wxTreebook *book_;
	wxPanel *page_;
};

struct wxAutoRefocuser final
{
	wxAutoRefocuser();
	~wxAutoRefocuser();

private:
	int previously_focused_id_{wxNOT_FOUND};
};

template <typename P>
wxString wxTreebookSubPageTitle(wxTreebookPageWrapper<P> &parent, wxString title)
{
	auto book = parent.GetBook();
	auto page_pos = parent.GetPagePos();

	do {
		title.Prepend(wxS(" / ")).Prepend(book->GetPageText(page_pos));

		page_pos = book->GetPageParent(page_pos);
	} while (page_pos != wxNOT_FOUND);

	return title;
}

wxString wxGetContainingPageTitle(wxWindow *w, bool full_path = true);

template <typename W = wxPanel, typename... Args>
auto wxPage(wxTreebook *b, const wxString &title, bool select, Args &&... args) -> decltype(wxTreebookPageWrapper<W>(wxCreate<W>(std::declval<wxPanel*>(), std::forward<Args>(args)...), b, std::declval<wxPanel*>()))
{
	W *ctrl;

	auto page = new wxPanel(b) | [&] (auto p){
		wxVBox(p, 0) = {
			{ 0, wxTitle(p, title) },
			{ 1, ctrl = wxCreate<W>(p, std::forward<Args>(args)...) }
		};
	};

	b->SetInternalBorder(unsigned(wxDlg2Px(b, wxDefaultGap)));
	b->AddPage(page, title, select);
	return { ctrl, b, page };
}

template <typename W = wxPanel>
auto wxPage(wxTreebook *b, const wxString &title = wxEmptyString) -> decltype(wxTreebookPageWrapper<W>(wxCreate<W>(std::declval<wxPanel*>()), b, std::declval<wxPanel*>()))
{
	W *ctrl;

	auto page = new wxPanel(b) | [&] (auto p){
		wxVBox(p, 0) = {
			{ 0, wxTitle(p, title) },
			{ 1, ctrl = wxCreate<W>(p) }
		};
	};

	b->SetInternalBorder(unsigned(wxDlg2Px(b, wxDefaultGap)));
	b->AddPage(page, title);
	return { ctrl, b, page};
}

template <typename W = wxPanel, typename P, typename... Args, std::enable_if_t<
	!std::is_base_of_v<wxBookCtrlBase, P>
>* = nullptr>
auto wxPage(wxTreebookPageWrapper<P> &b, const wxString &title, bool select, Args &&... args) -> decltype(wxTreebookPageWrapper<W>(wxCreate<W>(std::declval<wxPanel*>(), std::forward<Args>(args)...), b.GetBook(), std::declval<wxPanel*>()))
{
	W* ctrl;
	wxTreebook *book = b.GetBook();

	auto page = new wxPanel(book) | [&] (auto p){
		wxVBox(p, 0) = {
			{ 0, wxTitle(p, wxTreebookSubPageTitle(b, title)) },
			{ 1, ctrl = wxCreate<W>(p, std::forward<Args>(args)...) }
		};
	};

	auto parent_page_pos = b.GetPagePos();

	book->InsertSubPage(parent_page_pos, page, title, select);
	book->ExpandNode(parent_page_pos, true);

	return { ctrl, book, page };
}

template <typename W = wxPanel, typename P, std::enable_if_t<
	!std::is_base_of_v<wxBookCtrlBase, P>
>* = nullptr>
auto wxPage(wxTreebookPageWrapper<P> &b, const wxString &title = wxEmptyString) -> decltype(wxTreebookPageWrapper<W>(wxCreate<W>(std::declval<wxPanel*>()), b.GetBook(), std::declval<wxPanel*>()))
{
	W* ctrl;
	wxTreebook *book = b.GetBook();

	auto page = new wxPanel(book) | [&] (auto p){
		wxVBox(p, 0) = {
			{ 0, wxTitle(p, wxTreebookSubPageTitle(b, title)) },
			{ 1, ctrl = wxCreate<W>(p) }
		};
	};

	auto parent_page_pos = b.GetPagePos();

	book->InsertSubPage(parent_page_pos, page, title);
	book->ExpandNode(parent_page_pos, true);

	return { ctrl, book, page };
}


/*************************/

wxWizardPageSimple *GetFirstPage(wxWizard *wiz);
int GetNumberOfFollowingPages(wxWizardPage *page);
int GetIndexOfPage(wxWizardPage *page);

template <typename W>
struct wxWizardPageWrapper
{
	wxWizardPageWrapper(W *w, wxWizard *wiz)
		: ctrl_(w)
		, wiz_(wiz)
	{}

	operator W*()
	{
		return ctrl_;
	}

	W* operator->()
	{
		return ctrl_;
	}

	W &operator*()
	{
		return *ctrl_;
	}

	wxWizard *GetWizard()
	{
		return wiz_;
	}


	template <typename Func>
	std::enable_if_t<
		std::is_invocable_v<Func, W*>,
		wxWizardPageWrapper &&
	>
	operator |(Func && func) &&
	{
		std::forward<Func>(func)(ctrl_);

		return std::move(*this);
	}

private:
	W *ctrl_;
	wxWizard *wiz_;
};

template <typename W = wxPanel, typename... Args>
auto wxPage(wxWizard *wiz, const wxString &title = wxEmptyString, bool = false, Args &&... args) -> W*//decltype(wxWizardPageWrapper<W>(wxCreate<W>(std::declval<wxPanel*>(), std::forward<Args>(args)...), wiz))
{
	W *ctrl;

	auto new_page = new wxWizardPageSimple(wiz) | [&] (auto p) {
		wxVBox(p, 0) = {
			{ 0, wxTitle(p, title) },
			{ wxSizerFlags(1).Expand(), ctrl = wxCreate<W>(p, std::forward<Args>(args)...) }
		};
	};

	if (auto last_page = GetFirstPage(wiz)) {
		while (last_page->GetNext())
			last_page = static_cast<wxWizardPageSimple *>(last_page->GetNext());

		last_page->Chain(new_page);
	}

	wiz->GetPageAreaSizer()->Add(new_page);

	//return { ctrl, wiz };
	return ctrl;
}


/*************************/


enum class wxValidateOnlyIfCurrent_t {};
inline constexpr wxValidateOnlyIfCurrent_t wxValidateOnlyIfCurrent{};

template <typename W = wxPanel>
auto wxPage(wxValidateOnlyIfCurrent_t) {
	return [](auto &&... args) -> decltype(wxPage<wxValidateOnlyIfCurrentPage<W>>(std::forward<decltype(args)>(args)...)) {
		return wxPage<wxValidateOnlyIfCurrentPage<W>>(std::forward<decltype(args)>(args)...);
	};
}


/*************************/

wxWindow *wxPageLink(wxWindow *parent, const wxString &label, wxBookCtrlBase *book, std::size_t pageid, int style = wxHL_DEFAULT_STYLE & ~(wxHL_CONTEXTMENU) );

/*************************/


void wxEnableStrictValidation(wxBookCtrlBase *b);

template <typename W, typename Func, std::enable_if_t<
	std::is_base_of_v<wxWindow, W> &&
	std::is_invocable_v<Func, W*>
>* = nullptr>
W* operator |(W *w, Func && func)
{
	if (w) {
		std::forward<Func>(func)(w);

		if constexpr (std::is_base_of_v<wxBookCtrlBase, W>)
			wxEnableStrictValidation(w);
	}

	return w;
}

template <typename W, typename Func, std::enable_if_t<std::conjunction_v<
	std::negation<std::is_pointer<W>>,
	std::is_base_of<wxWindow, std::decay_t<W>>,
	std::is_invocable<Func, std::decay_t<W>*>
>>* = nullptr>
decltype(auto) operator |(W && w, Func && func)
{
	std::forward<Func>(func)(&w);

	if constexpr (std::is_base_of_v<wxBookCtrlBase, std::decay_t<W>>)
		wxEnableStrictValidation(&w);

	return std::forward<W>(w);
}

/******************************/

wxSizer *wxSaveTextToFile(wxWindow *parent, wxTextCtrl &text, const wxString &label, const wxString &message = wxEmptyString, const wxString &default_name = wxEmptyString, const wxString &wildcards = wxT("*.txt"));
wxSizer *wxLoadTextFromFile(wxWindow *parent, wxTextCtrl &text, const wxString &label, const wxString &message = wxEmptyString, const wxString &default_name = wxEmptyString, const wxString &wildcards = wxT("*.txt"));
wxSizer *wxCopyTextToClipboard(wxWindow *parent, wxTextCtrl &text, const wxString &label);

wxSizer *wxSaveTextToFile(wxWindow *parent, std::function<wxString()> text_func, const wxString &label, const wxString &message = wxEmptyString, const wxString &default_name = wxEmptyString, const wxString &wildcards = wxT("*.txt"));
wxSizer *wxLoadTextFromFile(wxWindow *parent, std::function<void(const wxString &)> text_func, const wxString &label, const wxString &message = wxEmptyString, const wxString &default_name = wxEmptyString, const wxString &wildcards = wxT("*.txt"));

/*****************************/

class wxCheckBoxGroup: public wxPanel
{
public:
	struct CB
	{
		operator bool() const;
		CB &operator=(bool v);
		void Enable(bool enabled);
		void EnableAndSet(bool value);

	private:
		friend wxCheckBoxGroup;
		CB(wxCheckBox *cb);

		wxCheckBox *cb_;
	};

	wxCheckBoxGroup(wxWindow *parent);

	CB c(const wxString &label);

	bool IsAnyChecked() const;
	void SetValue(bool value);
	std::size_t GetCheckedNumber() const;
	std::size_t GetNumberOfEnabledCheckboxes() const;

private:
	wxHyperlinkCtrl *select_all_{};
	wxHyperlinkCtrl *deselect_all_{};
};

/*****************************/

// BUG: Clang requires these: clang's bug or something I don't understand about C++?
extern template struct wxValidateOnlyIfCurrentPage<wxChoicebook>;
extern template struct wxCreator::CtrlFix<wxValidateOnlyIfCurrentPage<wxChoicebook>>;

#endif // HELPERS_HPP
