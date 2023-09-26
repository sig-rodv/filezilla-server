#include <libfilezilla/local_filesys.hpp>
#include "../../filezilla/util/filesystem.hpp"

#if !defined(FZ_WINDOWS) || !FZ_WINDOWS
#   include <unistd.h>
#   include <cerrno>
#else
#   include <windows.h>
#endif

#include "parser.hpp"

namespace fz::util::fs {

template <typename String, path_format Format>
bool basic_path<String, Format>::is_separator(typename View::value_type ch)
{
	if constexpr (Format == windows_format) {
		if (ch == '\\' || ch == ':')
			return true;
	}

	return ch == '/';
}

template <typename String, path_format Format>
typename basic_path<String, Format>::Char basic_path<String, Format>::separator()
{
	if constexpr (Format == windows_format)
		return Char('\\');
	else
		return Char('/');
}

template <typename String, path_format Format>
basic_path<detail::to_string_t<String>, Format> basic_path<String, Format>::resolve(const basic_path<View, Format> &other) const
{
	if (other.is_absolute())
		return other;

	if (!is_absolute())
		return {};

	return *this / other;
}

template<typename String, path_format Format>
fz::file basic_path<String, Format>::open(file::mode mode, file::creation_flags flags) const
{
	if (!is_absolute())
		return {};

	if (mode == file::mode::writing) {
		auto mkd_flag = fz::mkdir_permissions::normal;

		if (flags & file::current_user_only)
			mkd_flag = fz::mkdir_permissions::cur_user;
		else
		if (flags & file::current_user_and_admins_only)
			mkd_flag = fz::mkdir_permissions::cur_user_and_admins;

		parent().mkdir(true, mkd_flag);
	}

	return {fz::to_native(string_), mode, flags};
}

template<typename String, path_format Format>
result basic_path<String, Format>::mkdir(bool recurse, mkdir_permissions permissions, native_string *last_created) const
{
	return fz::mkdir(to_native(string_), recurse, permissions, last_created);
}

template<typename String, path_format Format>
local_filesys::type basic_path<String, Format>::type() const
{
	return local_filesys::get_file_type(fz::to_native(string_));
}

template <typename String, path_format Format>
basic_path<detail::to_string_t<String>, Format> basic_path<String, Format>::resolve(const basic_path<View, Format> &other, file::mode mode, file::creation_flags flags) const
{
	auto absolute = resolve(other);

	if (absolute.open(mode, flags))
		return absolute;

	return {};
}

FZ_UTIL_PARSER( ) drive_letter_followed_by_slash(Range &r)
{
	return (lit(r, 'A', 'Z') || lit(r, 'a', 'z')) && lit(r, ':') && (lit(r, '\\') || lit(r, '/'));
}

FZ_UTIL_PARSER( ) parse_dos_dev_path(Range &r, View &specifier, View &server, View &share, CharT &sep, bool &is_unc) {
	if (seq(r, View(fzS(CharT, "\\\\"))))
		sep = '\\';
	else
	if (seq(r, View(fzS(CharT, "//"))))
		sep = '/';
	else
		return false;

	if (!(parse_until_lit(r, server, {sep}) && lit(r, sep)) || server.empty())
		return false;

	if (!(parse_until_lit(r, share, {sep}, true)) || share.empty())
		return false;

	lit(r, sep);

	is_unc = true;

	if (server == View(fzS(CharT, ".")) || server == View(fzS(CharT, "?"))) {
		is_unc = false;
		specifier = server;
		server = {};

		if (fz::equal_insensitive_ascii(share, fzS(CharT, "UNC"))) {
			share = {};

			if (!(lit(r, sep) && parse_until_lit(r, server, {sep}) && lit(r, sep)) || server.empty())
				return false;

			if (!(parse_until_lit(r, share, {sep}, true)) || share.empty())
				return false;

			is_unc = true;
		}
	}

	return true;
}

template <typename String, path_format Format>
template <auto F, std::enable_if_t<F == unix_format>*>
std::size_t basic_path<String, Format>::first_after_root() const
{
	if (string_.empty())
		return string_.npos;

	return (string_[0] == '/') ? 1 : 0;
}

template <typename String, path_format Format>
template <auto F, std::enable_if_t<F == windows_format>*>
std::size_t basic_path<String, Format>::first_after_root(View *pspecifier, bool *pis_unc) const
{
	if (string_.empty())
		return string_.npos;

	// See: https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats
	if (parseable_range r(string_); drive_letter_followed_by_slash(r))
		return std::size_t(std::addressof(*r.it) - std::addressof(*string_.cbegin()));

	View specifier, server, share;
	Char sep{}; bool is_unc;

	if (parseable_range r(string_); parse_dos_dev_path(r, specifier, server, share, sep, is_unc)) {
		if (pspecifier)
			*pspecifier = specifier;

		if (pis_unc)
			*pis_unc = is_unc;

		return std::size_t(std::addressof(*r.it) - std::addressof(*string_.cbegin()));
	}

	return 0;
}

template <typename String, path_format Format>
bool basic_path<String, Format>::is_absolute() const
{
	auto pos = first_after_root();
	return pos != View::npos && pos > 0;
}

template <typename String, path_format Format>
bool basic_path<String, Format>::is_valid(path_format as_if_native_format_were [[maybe_unused]]) const
{
	static auto check_chars = [](View str, View extra = {}, std::size_t start = 0) {
		for (
			std::size_t beg = start, end;
			beg < str.size();
			beg = end + 1
		) {
			auto it = std::find_if(&str[beg], &str.back()+1, is_separator);
			end = std::size_t(it-&str[0]);

			View e(&str[beg], end-beg);

			if (e.empty())
				continue;

			if (e == fzS(Char, ".") || e == fzS(Char, ".."))
				continue;

			if (e.back() == '.' || e.back() == ' ')
				return false;

			if (e.find_first_of(extra) != View::npos)
				return false;
		}

		return true;
	};

	if constexpr (Format == windows_format) {
		View specifier;	bool is_unc{};

		auto pos_after_root = first_after_root(&specifier, &is_unc);
		if (pos_after_root == string_.npos)
			return false;

		if (!specifier.empty() && specifier != fzS(Char, "."))
			return false;

		if (specifier == fzS(Char, ".") && !is_unc)
			return false;

		// Disallow colons in the whole string, spaces and dots only at the end of path elements.
		return check_chars(string_, fzS(Char, ":"), pos_after_root);
	}
	else
	if (string_.empty())
		return false;
	else
	if (as_if_native_format_were == windows_format)
		// Disallow backslashes and colons in the whole string, spaces and dots only at the end of path segments.
		return check_chars(string_, fzS(Char, "\\:"));

	return true;
}

template <typename String, path_format Format>
template <typename S, std::enable_if_t<!std::is_same_v<S, detail::to_view_t<S>>>*>
basic_path<String, Format> &basic_path<String, Format>::normalize()
{
	if constexpr (Format == windows_format) {
		replace_substrings(string_, '/', '\\');

		const static detail::to_string_t<String> unc = fzS(Char, "\\\\.\\UNC\\");
		const static detail::to_string_t<String> double_slashes = fzS(Char, "\\\\");

		if (fz::starts_with<true>(string_, unc))
			string_.replace(0, unc.size(), double_slashes);
	}

	auto pos_after_root = first_after_root();
	if (pos_after_root == string_.npos)
		return *this;

	// Remove .. and .
	auto beg = pos_after_root, wbeg = beg;
	do {
		auto end = std::min(string_.find(separator(), beg), string_.size());
		View e(&string_[beg], end-beg);

		if (e == fzS(Char, "..")) {
			if (wbeg > pos_after_root+1) {
				wbeg -= 2;
				if (wbeg > 0)
					wbeg = string_.rfind(separator(), wbeg)+1;
			}
		}
		else
		if (!e.empty() && e != fzS(Char, ".")) {
			if (wbeg < beg) {
				std::copy(e.begin(), e.end(), &string_[wbeg]);
				wbeg += e.size();

				if (end < string_.size())
					string_[wbeg++] = string_[end];
			}
			else
				wbeg = end + (end < string_.size());
		}

		beg = end + 1;
	} while (beg < string_.size());

	// Remove any trailing separator
	if (wbeg > pos_after_root && is_separator(string_[wbeg-1]))
		wbeg -= 1;

	string_.resize(wbeg);

	return *this;
}

template <typename String, path_format Format>
basic_path<std::decay_t<String>, Format> basic_path<String, Format>::base(bool remove_suffixes) const
{
	auto begin = std::find_if(string_.rbegin(), string_.rend(), is_separator).base();
	auto end = remove_suffixes ? std::find(begin, string_.end(), '.') : string_.end();

	return {std::decay_t<String>(&*begin, std::size_t(end-begin))};
}

template <typename String, path_format Format>
basic_path<String, Format> &basic_path<String, Format>::make_parent()
{
	auto it = string_.end();
	auto n = first_after_root();
	auto begin = n != string_.npos ? string_.begin() + std::string::difference_type(n) : string_.end();

	while (it != begin && is_separator(*--it));
	while (it != begin && !is_separator(*--it));
	for (;it != begin && is_separator(*std::prev(it)); --it);

	if constexpr (std::is_same_v<String, View>)
		string_.remove_suffix(std::size_t(string_.end()-it));
	else
		string_.erase(it, string_.end());

	if (it == begin && (n == 0 || n == string_.npos))
		string_ = []{static const Char dot = '.'; return View{&dot, 1};}();

	return *this;
}

template <typename String, path_format Format>
basic_path<String, Format> basic_path<String, Format>::parent() const
{
	basic_path ret(*this);
	ret.make_parent();

	return ret;
}


template <typename String, path_format Format>
bool basic_path<String, Format>::is_base() const
{
	static constexpr typename View::value_type dotc[] = { '.', '\0' };
	static constexpr typename View::value_type dotdotc[] = { '.', '.', '\0' };
	static const auto dot = View(dotc);
	static const auto dotdot = View(dotdotc);

	return !is_absolute() && !string_.empty() && string_ != dot && string_ != dotdot;
}

template <typename String, path_format Format>
basic_path<String, Format> basic_path_list<String, Format>::resolve(const basic_path<String, Format> &other) const
{
	for (const auto &p: *this) {
		auto res = p.resolve(other);
		if (!res.str().empty())
			return res;
	}

	return {};
}

template <typename String, path_format Format>
basic_path<String, Format> basic_path_list<String, Format>::resolve(const basic_path<String, Format> &other, file::mode mode, file::creation_flags flags) const
{
	for (const auto &p: *this) {
		auto res = p.resolve(other, mode, flags);
		if (!res.str().empty())
			return res;
	}

	return {};
}

template <typename String, path_format Format>
basic_path_list<String, Format> &basic_path_list<String, Format>::operator+=(const basic_path_list &rhs)
{
	this->insert(this->end(), rhs.begin(), rhs.end());
	return *this;
}

template <typename String, path_format Format>
basic_path_list<String, Format> &basic_path_list<String, Format>::operator/=(const basic_path<String, Format> &rhs)
{
	for (auto &p: *this)
		p /= rhs;

	return *this;
}

template <typename String, path_format Format>
basic_path_list<String, Format> basic_path_list<String, Format>::operator +(const basic_path_list &rhs) const &{
	basic_path_list ret(*this);
	ret += rhs;
	return ret;
}

template <typename String, path_format Format>
basic_path_list<String, Format> basic_path_list<String, Format>::operator /(const basic_path<String, Format> &rhs) const & {
	basic_path_list ret(*this);
	ret /= rhs;
	return ret;
}

template <typename String, path_format Format>
basic_path_list<String, Format> &&basic_path_list<String, Format>::operator +(const basic_path_list &rhs) && {
	return std::move(*this += rhs);
}

template <typename String, path_format Format>
basic_path_list<String, Format> &&basic_path_list<String, Format>::operator /(const basic_path<String, Format> &rhs) && {
	return std::move(*this /= rhs);
}

template class basic_path<std::string, unix_format>;
template class basic_path<std::wstring, unix_format>;
template class basic_path<std::string_view, unix_format>;
template class basic_path<std::wstring_view, unix_format>;
template class basic_path<std::string &, unix_format>;
template class basic_path<std::wstring &, unix_format>;
template class basic_path_list<std::string, unix_format>;
template class basic_path_list<std::wstring, unix_format>;

template class basic_path<std::string, windows_format>;
template class basic_path<std::wstring, windows_format>;
template class basic_path<std::string_view, windows_format>;
template class basic_path<std::wstring_view, windows_format>;
template class basic_path<std::string &, windows_format>;
template class basic_path<std::wstring &, windows_format>;
template class basic_path_list<std::string, windows_format>;
template class basic_path_list<std::wstring, windows_format>;

template unix_path &unix_path::normalize();
template unix_wpath &unix_wpath::normalize();
template unix_path_ref &unix_path_ref::normalize();
template unix_wpath_ref &unix_wpath_ref::normalize();

template windows_path &windows_path::normalize();
template windows_wpath &windows_wpath::normalize();
template windows_path_ref &windows_path_ref::normalize();
template windows_wpath_ref &windows_wpath_ref::normalize();

}
