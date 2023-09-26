#ifndef FZ_UTIL_FILESYSTEM_HPP
#define FZ_UTIL_FILESYSTEM_HPP

#include <variant>

#include <libfilezilla/local_filesys.hpp>
#include <libfilezilla/file.hpp>

namespace fz::util::fs {

enum path_format: std::uint8_t {
	unix_format,
	windows_format,

#if defined(FZ_WINDOWS) && FZ_WINDOWS
	native_format = windows_format
#elif (defined(FZ_UNIX) && FZ_UNIX) || (defined(FZ_MAC) && FZ_MAC)
	native_format = unix_format
#else
#   error "Your platform is not supported"
#endif
};

//! Encapsulates a filesystem path, with very limited functionalities and no safety checks.
//! Automatically converts to the underlying string type.
template <typename String, path_format>
class basic_path;

template <typename String, path_format>
class basic_path_list;

namespace detail {

	template <typename String>
	struct to_view;

	template <typename CharT, typename Traits, typename Allocator>
	struct to_view<std::basic_string<CharT, Traits, Allocator>>
	{
		using type = std::basic_string_view<CharT, Traits>;
	};

	template <typename CharT, typename Traits>
	struct to_view<std::basic_string_view<CharT, Traits>>
	{
		using type = std::basic_string_view<CharT, Traits>;
	};

	template <typename String>
	using to_view_t = typename to_view<std::decay_t<String>>::type;

	template <typename T>
	struct to_string;

	template <typename CharT, typename... Ts>
	struct to_string<std::basic_string<CharT, Ts...>>
	{
		using type = std::basic_string<CharT, Ts...>;
	};

	template <typename CharT, typename... Ts>
	struct to_string<std::basic_string_view<CharT, Ts...>>
	{
		using type = std::basic_string<CharT, Ts...>;
	};

	template <typename CharT>
	struct to_string<CharT *>
	{
		using type = std::basic_string<std::remove_cv_t<CharT>>;
	};

	template <typename String, path_format Format>
	struct to_string<basic_path<String, Format>>
	{
		using type = String;
	};

	template <typename T>
	using to_string_t = typename to_string<std::decay_t<T>>::type;

	template <typename Path>
	struct format;

	template <typename String, path_format Format>
	struct format<basic_path<String, Format>>
	{
		inline static constexpr path_format value = Format;
	};

	template <typename Path>
	inline constexpr path_format format_v = format<Path>::value;

	template <typename T>
	struct is_basic_path: std::false_type{};

	template <typename String, path_format Format>
	struct is_basic_path<basic_path<String, Format>>: std::true_type{};

	template <typename T>
	inline constexpr bool is_basic_path_v = is_basic_path<T>::value;

	template <typename T>
	struct is_basic_path_list: std::false_type{};

	template <typename String, path_format Format>
	struct is_basic_path_list<basic_path_list<String, Format>>: std::true_type{};

	template <typename T>
	inline constexpr bool is_basic_path_list_v = is_basic_path_list<T>::value;
}

template <typename String, path_format Format>
class basic_path {
	using View = detail::to_view_t<String>;
	using Char = typename View::value_type;

	static_assert(!std::is_reference_v<String> || !std::is_const_v<std::remove_reference_t<String>>, "References to const are not allowed. Use a view instead.");
	static_assert(Format == unix_format || Format == windows_format, "path_format not recognized");

public:
	static inline constexpr path_format format = Format;
	using value_type = Char;

	template <typename S, std::enable_if_t<!detail::is_basic_path_v<std::decay_t<S>> && std::is_constructible_v<String, S>>* = nullptr>
	basic_path(S && s)
		: string_(std::forward<S>(s))
	{}

	template <typename S = String, std::enable_if_t<!std::is_reference_v<S>>* = nullptr>
	basic_path()
	{}

	template <typename S, std::enable_if_t<std::is_constructible_v<String, S>>* = nullptr>
	basic_path(const basic_path<S, Format> &rhs)
		: string_(rhs.string_)
	{}

	basic_path(const basic_path &) = default;
	basic_path(basic_path &&) = default;
	basic_path &operator=(const basic_path &) = default;
	basic_path &operator=(basic_path &&) = default;

	template <typename S = String, std::enable_if_t<std::is_same_v<S, detail::to_view_t<S>>>* = nullptr>
	operator detail::to_string_t<View> () const { return detail::to_string_t<View>(string_); }

	template <typename S = String, std::enable_if_t<!std::is_same_v<S, detail::to_view_t<S>>>* = nullptr>
	operator const String &() const { return string_; }

	operator View () const { return string_; }

	template <typename S = String, std::enable_if_t<std::is_same_v<S, detail::to_view_t<S>>>* = nullptr>
	detail::to_string_t<View> str() const { return detail::to_string_t<View>(string_); }

	template <typename S = String, std::enable_if_t<!std::is_same_v<S, detail::to_view_t<S>>>* = nullptr>
	const String &str() const { return string_; }

	View str_view() const { return string_; }

	//! Resolves a relative path into an absolute one.
	//! If the \ref other path is absolute, then it is returned directly.
	basic_path<detail::to_string_t<String>, Format> resolve(const basic_path<View, Format> &other) const;

	//! Treats the path as referring to a file and tries to open it.
	fz::file open(fz::file::mode mode, file::creation_flags flags = file::existing) const;

	//! Treats the path as referring to a directory and tries to create it.
	result mkdir(bool recurse, mkdir_permissions permissions = mkdir_permissions::normal, native_string * last_created = nullptr) const;

	local_filesys::type type() const;

	//! Resolves a relative path into an absolute one, using the specified mode.
	//! If the \ref other path is absolute, then it is returned directly.
	//! If mode == writing, then the file gets created in the first usable directory path, if not existing already in that path.
	basic_path<detail::to_string_t<String>, Format> resolve(const basic_path<View, Format> &other, fz::file::mode mode, file::creation_flags flags = {}) const;

	bool is_absolute() const;
	bool is_valid(path_format as_if_native_format_were = native_format) const;

	template <typename S = String, std::enable_if_t<!std::is_same_v<S, detail::to_view_t<S>>>* = nullptr>
	basic_path &normalize();

	basic_path<detail::to_string_t<String>, Format> normalized() const
	{
		basic_path<detail::to_string_t<String>, Format> ret{*this};

		ret.normalize();

		return ret;
	}

	basic_path parent() const;
	basic_path<std::decay_t<String>, Format> base(bool remove_suffixes = false) const;
	bool is_base() const;
	basic_path &make_parent();

	template <typename S = String, std::enable_if_t<!std::is_same_v<S, detail::to_view_t<S>>>* = nullptr>
	basic_path &operator /= (const basic_path<View, Format> &rhs)
	{
		if (string_.empty() || rhs.is_absolute())
			*this = rhs;
		else
		if (!rhs.string_.empty()) {
			if (!is_separator(string_.back()) && !is_separator(rhs.string_.front()))
				string_.append(1, separator());

			string_.append(rhs.string_);
		}

		return *this;
	}

	template <typename S = String, std::enable_if_t<!std::is_same_v<S, detail::to_view_t<S>>>* = nullptr>
	basic_path &&operator / (const basic_path<View, Format> &rhs) &&
	{
		return std::move(*this /= rhs);
	}

	basic_path<detail::to_string_t<String>, Format> operator /(const basic_path<View, Format> &rhs) const &
	{
		basic_path<detail::to_string_t<String>, Format> ret{*this};

		ret /= rhs;
		return ret;
	}

	basic_path_list<String, Format> operator +(const basic_path<View, Format> &rhs) const &
	{
		return { string_, String(rhs) };
	}

private:
	template <typename, path_format>
	friend class basic_path;

	String string_;

	template <auto F = Format, std::enable_if_t<F == unix_format>* = nullptr>
	std::size_t first_after_root() const;

	template <auto F = Format, std::enable_if_t<F == windows_format>* = nullptr>
	std::size_t first_after_root(View *pspecifier = nullptr, bool *pis_unc = nullptr) const;

	static bool is_separator(typename View::value_type);
	static Char separator();
};

template <typename S1, typename S2, path_format Format,
	std::enable_if_t<std::is_same_v<typename basic_path<S1, Format>::value_type, typename basic_path<S2, Format>::value_type>>* = nullptr
>
bool operator==(const basic_path<S1, Format> &lhs, const basic_path<S2, Format> &rhs)
{
	return lhs.normalized().str() == rhs.normalized().str();
}

template <typename S1, typename S2, path_format Format,
	std::enable_if_t<std::is_same_v<typename basic_path<S1, Format>::value_type, typename basic_path<S2, Format>::value_type>>* = nullptr
>
bool operator!=(const basic_path<S1, Format> &lhs, const basic_path<S2, Format> &rhs)
{
	return lhs.normalized().str() != rhs.normalized().str();
}

template <typename S1, typename S2, path_format Format,
	std::enable_if_t<std::is_same_v<typename basic_path<S1, Format>::value_type, typename basic_path<S2, Format>::value_type>>* = nullptr
>
bool operator<(const basic_path<S1, Format> &lhs, const basic_path<S2, Format> &rhs)
{
	return lhs.normalized().str() < rhs.normalized().str();
}

template <typename S1, typename S2, path_format Format,
	std::enable_if_t<std::is_same_v<typename basic_path<S1, Format>::value_type, typename basic_path<S2, Format>::value_type>>* = nullptr
>
bool operator>(const basic_path<S1, Format> &lhs, const basic_path<S2, Format> &rhs)
{
	return lhs.normalized().str() > rhs.normalized().str();
}

template <typename S1, typename S2, path_format Format,
	std::enable_if_t<std::is_same_v<typename basic_path<S1, Format>::value_type, typename basic_path<S2, Format>::value_type>>* = nullptr
>
bool operator<=(const basic_path<S1, Format> &lhs, const basic_path<S2, Format> &rhs)
{
	return lhs.normalized().str() <= rhs.normalized().str();
}

template <typename S1, typename S2, path_format Format,
	std::enable_if_t<std::is_same_v<typename basic_path<S1, Format>::value_type, typename basic_path<S2, Format>::value_type>>* = nullptr
>
bool operator>=(const basic_path<S1, Format> &lhs, const basic_path<S2, Format> &rhs)
{
	return lhs.normalized().str() >= rhs.normalized().str();
}

template <typename String, path_format Format>
class basic_path_list: public std::vector<basic_path<String, Format>> {
	using Path = basic_path<String, Format>;

public:
	using std::vector<Path>::vector;

	template <typename... P, std::enable_if_t<!(detail::is_basic_path_list_v<std::decay_t<P>> || ...) && (std::is_constructible_v<Path, P> && ...)>* = nullptr>
	basic_path_list(P &&... p)
		: std::vector<Path>{std::forward<P>(p)...}
	{}

	//! Resolves a relative path into an absolute one.
	//! If the \ref other path is absolute, then it is returned directly.
	Path resolve(const Path &other) const;

	//! Resolves a relative path into an absolute one, using the specified mode.
	//! If the \ref other path is absolute, then it is returned directly.
	//! If mode == writing, then the file gets created in the first usable directory path, if not existing already in that path.
	Path resolve(const Path &other, fz::file::mode mode, file::creation_flags flags = {}) const;

	basic_path_list &operator+=(const basic_path_list &rhs);
	basic_path_list &operator/=(const Path &rhs);

	basic_path_list operator/(const Path &rhs) const &;
	basic_path_list &&operator/(const Path &rhs) &&;

	basic_path_list operator+(const basic_path_list &rhs) const &;
	basic_path_list &&operator+(const basic_path_list &rhs) &&;
};

template <typename String, path_format Format>
basic_path_list<String, Format> operator+(basic_path<String, Format> lhs, const basic_path_list<String, Format> &rhs) {
	return basic_path_list<String, Format>(std::move(lhs)) += rhs;
}

using path = basic_path<std::string, native_format>;
using wpath = basic_path<std::wstring, native_format>;
using native_path = basic_path<native_string, native_format>;
using path_list = basic_path_list<std::string, native_format>;
using wpath_list = basic_path_list<std::wstring, native_format>;
using native_path_list = basic_path_list<native_string, native_format>;
using path_view = basic_path<std::string_view, native_format>;
using wpath_view = basic_path<std::wstring_view, native_format>;
using native_path_view = basic_path<native_string_view, native_format>;
using path_ref = basic_path<std::string &, native_format>;
using wpath_ref = basic_path<std::wstring &, native_format>;
using native_path_ref = basic_path<native_string &, native_format>;

using unix_path = basic_path<std::string, unix_format>;
using unix_wpath = basic_path<std::wstring, unix_format>;
using unix_native_path = basic_path<native_string, unix_format>;
using unix_path_list = basic_path_list<std::string, unix_format>;
using unix_wpath_list = basic_path_list<std::wstring, unix_format>;
using unix_native_path_list = basic_path_list<native_string, unix_format>;
using unix_path_view = basic_path<std::string_view, unix_format>;
using unix_wpath_view = basic_path<std::wstring_view, unix_format>;
using unix_native_path_view = basic_path<native_string_view, unix_format>;
using unix_path_ref = basic_path<std::string &, unix_format>;
using unix_wpath_ref = basic_path<std::wstring &, unix_format>;
using unix_native_path_ref = basic_path<native_string &, unix_format>;

using windows_path = basic_path<std::string, windows_format>;
using windows_wpath = basic_path<std::wstring, windows_format>;
using windows_native_path = basic_path<native_string, windows_format>;
using windows_path_list = basic_path_list<std::string, windows_format>;
using windows_wpath_list = basic_path_list<std::wstring, windows_format>;
using windows_native_path_list = basic_path_list<native_path, windows_format>;
using windows_path_view = basic_path<std::string_view, windows_format>;
using windows_wpath_view = basic_path<std::wstring_view, windows_format>;
using windows_native_path_view = basic_path<native_string_view, windows_format>;
using windows_path_ref = basic_path<std::string &, windows_format>;
using windows_wpath_ref = basic_path<std::wstring &, windows_format>;
using windows_native_path_ref = basic_path<native_string &, windows_format>;

extern template class basic_path<std::string, unix_format>;
extern template class basic_path<std::wstring, unix_format>;
extern template class basic_path<std::string_view, unix_format>;
extern template class basic_path<std::wstring_view, unix_format>;
extern template class basic_path<std::string &, unix_format>;
extern template class basic_path<std::wstring &, unix_format>;
extern template class basic_path_list<std::string, unix_format>;
extern template class basic_path_list<std::wstring, unix_format>;

extern template class basic_path<std::string, windows_format>;
extern template class basic_path<std::wstring, windows_format>;
extern template class basic_path<std::string_view, windows_format>;
extern template class basic_path<std::wstring_view, windows_format>;
extern template class basic_path<std::string &, windows_format>;
extern template class basic_path<std::wstring &, windows_format>;
extern template class basic_path_list<std::string, windows_format>;
extern template class basic_path_list<std::wstring, windows_format>;

extern template unix_path &unix_path::normalize();
extern template unix_wpath &unix_wpath::normalize();
extern template unix_path_ref &unix_path_ref::normalize();
extern template unix_wpath_ref &unix_wpath_ref::normalize();

extern template windows_path &windows_path::normalize();
extern template windows_wpath &windows_wpath::normalize();
extern template windows_path_ref &windows_path_ref::normalize();
extern template windows_wpath_ref &windows_wpath_ref::normalize();

template <typename T, std::enable_if_t<std::is_same_v<std::decay_t<T>, native_path>>* = nullptr>
struct only_dirs_t
{
	only_dirs_t(T && path)
		: path_(std::forward<T>(path))
	{}

private:
	template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>*>
	friend class native_directory_iterator;

	T path_;
};

template <typename T>
only_dirs_t(T && t) -> only_dirs_t<T>;

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
class native_directory_iterator
{
public:
	using iterator_category = std::input_iterator_tag;
	using difference_type   = std::ptrdiff_t;
	using value_type        = native_path;
	using pointer           = value_type*;
	using reference         = value_type&;

	struct sentinel{};

	native_directory_iterator(Dir && dir, bool iterate_over_dirs_only = false)
		: dir_(std::forward<Dir>(dir))
	{
		lfs_.begin_find_files(dir_, iterate_over_dirs_only);
		++*this;
	}

	native_directory_iterator(only_dirs_t<Dir> &&od)
		: native_directory_iterator(std::forward<Dir>(od.path_), true)
	{}

	~native_directory_iterator()
	{
		lfs_.end_find_files();
	}

	native_directory_iterator &operator++()
	{
		native_string name;
		is_valid_ = lfs_.get_next_file(name);
		curr_path_ = dir_ / name;

		return *this;
	}

	const value_type &operator*() const
	{
		return curr_path_;
	}

	bool operator!=(const sentinel &) const
	{
		return is_valid_;
	}

	bool operator==(const sentinel &) const
	{
		return !is_valid_;
	}

private:
	local_filesys lfs_;
	bool is_valid_;
	Dir dir_;
	value_type curr_path_{};
};

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
native_directory_iterator(Dir && dir, bool iterate_over_dirs_only = false) -> native_directory_iterator<Dir>;

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
native_directory_iterator(only_dirs_t<Dir> &&od) -> native_directory_iterator<Dir>;

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
native_directory_iterator<Dir> begin(Dir && root_path) { return { std::forward<Dir>(root_path) }; }

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
typename native_directory_iterator<Dir>::sentinel end(Dir &&) { return {}; }

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
only_dirs_t<Dir> only_dirs(Dir &&dir) { return { std::forward<Dir>(dir) }; }

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
native_directory_iterator<Dir> begin(only_dirs_t<Dir> root_path) { return { std::move(root_path) }; }

template <typename Dir, std::enable_if_t<std::is_same_v<std::decay_t<Dir>, native_path>>* = nullptr>
typename native_directory_iterator<Dir>::sentinel end(only_dirs_t<Dir>) { return {}; }

}

#endif // FZ_UTIL_FILESYSTEM_HPP
