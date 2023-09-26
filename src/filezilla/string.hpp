#ifndef FZ_STRING_HPP
#define FZ_STRING_HPP

#include <libfilezilla/string.hpp>

namespace fz {

namespace impl {

	template <typename String, typename View, typename EscapeMap>
	String escaped(View str, View escape_string, const EscapeMap &escape_map)
	{
		String ret(str);

		fz::replace_substrings(ret, escape_string, String(escape_string).append(escape_string));
		for (auto &p: escape_map)
			fz::replace_substrings(ret, p.first, String(escape_string).append(p.second));

		return ret;
	}

	template <typename String, typename View, typename EscapeMap>
	String unescaped(View str, View escape_string, const EscapeMap &escape_map = {})
	{
		String ret;

		while (true) {
			auto esc_pos = str.find(escape_string);
			ret.append(str.substr(0, esc_pos));

			if (esc_pos == View::npos)
				break;

			str.remove_prefix(esc_pos+1);

			if (fz::starts_with(str, escape_string))
				str.remove_prefix(escape_string.size());
			else
			for (auto &p: escape_map) {
				if (fz::starts_with(str, p.second)) {
					ret.append(p.first);
					str.remove_prefix(p.second.size());
					break;
				}
			}
		}

		return ret;
	}

}

template <typename EscapeMap = std::initializer_list<std::pair<std::string_view, std::string_view>>, typename From = typename EscapeMap::value_type::first_type, typename To = typename EscapeMap::value_type::second_type, std::enable_if_t<
	std::is_constructible_v<std::string_view, From> &&
	std::is_constructible_v<std::string_view, To>
>* = nullptr>
std::string escaped(std::string_view str, std::string_view escape_string, const EscapeMap &escape_map = {})
{
	return impl::escaped<std::string>(str, escape_string, escape_map);
}

template <typename EscapeMap = std::initializer_list<std::pair<std::wstring_view, std::wstring_view>>, typename From = typename EscapeMap::value_type::first_type, typename To = typename EscapeMap::value_type::second_type, std::enable_if_t<
	std::is_constructible_v<std::wstring_view, From> &&
	std::is_constructible_v<std::wstring_view, To>
>* = nullptr>
std::wstring escaped(std::wstring_view str, std::wstring_view escape_string, const EscapeMap &escape_map = {})
{
	return impl::escaped<std::wstring>(str, escape_string, escape_map);
}

template <typename EscapeMap = std::initializer_list<std::pair<std::string_view, std::string_view>>, typename From = typename EscapeMap::value_type::first_type, typename To = typename EscapeMap::value_type::second_type, std::enable_if_t<
	std::is_constructible_v<std::string_view, From> &&
	std::is_constructible_v<std::string_view, To>
>* = nullptr>
std::string unescaped(std::string_view str, std::string_view escape_string, const EscapeMap &escape_map = {})
{
	return impl::unescaped<std::string>(str, escape_string, escape_map);
}

template <typename EscapeMap = std::initializer_list<std::pair<std::wstring_view, std::wstring_view>>, typename From = typename EscapeMap::value_type::first_type, typename To = typename EscapeMap::value_type::second_type, std::enable_if_t<
	std::is_constructible_v<std::wstring_view, From> &&
	std::is_constructible_v<std::wstring_view, To>
>* = nullptr>
std::wstring unescaped(std::wstring_view str, std::wstring_view escape_string, const EscapeMap &escape_map = {})
{
	return impl::unescaped<std::wstring>(str, escape_string, escape_map);
}

template <typename String>
struct join_traits
{
	static inline const String default_separator = fzS(typename String::value_type, " ");
	static inline const String default_prefix = fzS(typename String::value_type, "");

	template <typename Value>
	static inline auto to_string(const Value &v) -> decltype(toString<String>(v))
	{
		return toString<String>(v);
	}
};

template <typename String, typename Container>
auto join(
	const Container &c,
	const decltype(join_traits<String>::default_separator) &sep = join_traits<String>::default_separator,
	const decltype(join_traits<String>::default_prefix) &pre = join_traits<String>::default_prefix
) -> decltype(join_traits<String>::to_string(*c.begin()))
{
	String ret;

	for (const auto &s: c)
		ret.append(pre).append(join_traits<String>::to_string(s)).append(sep);

	if (!ret.empty())
		ret.resize(ret.size() - sep.size());

	return ret;
}

template <typename Container, typename String = std::decay_t<decltype(*std::cbegin(std::declval<Container>()))>>
auto join(
	const Container &c,
	const decltype(join_traits<String>::default_separator) &sep = join_traits<String>::default_separator,
	const decltype(join_traits<String>::default_prefix) &pre = join_traits<String>::default_prefix
) -> decltype(join_traits<String>::to_string(*std::cbegin(c)))
{
	String ret;

	for (const auto &s: c)
		ret.append(pre).append(join_traits<String>::to_string(s)).append(sep);

	if (!ret.empty())
		ret.resize(ret.size() - sep.size());

	return ret;
}

template <typename String>
struct quote_traits: join_traits<String>
{
	static inline const String default_opening = fzS(typename String::value_type, "\"");
	static inline const String default_closing = fzS(typename String::value_type, "\"");
};

template <typename String>
String quote(const String &v, const String &opening = quote_traits<String>::default_opening, const String &closing = quote_traits<String>::default_closing)
{
	String ret;

	ret.append(opening).append(v).append(closing);

	return ret;
}

template <typename String, typename Value, std::enable_if_t<!std::is_same_v<String, Value>>* = nullptr>
auto quote(const Value &v, const decltype(quote_traits<String>::default_opening) &opening = quote_traits<String>::default_opening, const decltype(quote_traits<String>::default_closing) &closing = quote_traits<String>::default_closing) -> decltype(quote(join_traits<String>::to_string(v), opening, closing))
{
	return quote(join_traits<String>::to_string(v), opening, closing);
}


template <typename String>
String &&remove_ctrl_chars(String &&s) {
	s.erase(std::remove_if(s.begin(), s.end(), [](auto c) {
		return c <= 31 || c == 127;
	}), s.end());

	return std::forward<String>(s);
}

}

#endif // FZ_STRING_HPP
