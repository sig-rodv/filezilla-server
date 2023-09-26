#ifndef FZ_HTTP_HEADERS_HPP
#define FZ_HTTP_HEADERS_HPP

#include <string>
#include <map>
#include <libfilezilla/string.hpp>
#include <libfilezilla/util.hpp>

namespace fz::http
{

class headers: public std::map<std::string, std::string, fz::less_insensitive_ascii>
{
	class value_t
	{
	private:
		friend headers;

		value_t(std::string_view rhs)
			: value_(rhs)
		{}

		value_t(std::string &&rhs)
			: value_(std::move(rhs))
		{}

	public:
		operator std::string_view () const &
		{
			return value_;
		}

		operator std::string &&() &&
		{
			return std::move(value_);
		}

		bool operator==(std::string_view rhs) const
		{
			return fz::equal_insensitive_ascii(*this, rhs);
		}

		bool operator!=(std::string_view rhs) const
		{
			return !fz::equal_insensitive_ascii(*this, rhs);
		}

		operator bool() const
		{
			return !value_.empty();
		}

		bool empty() const
		{
			return value_.empty();
		}

	private:
		std::string value_;
	};

public:
	using map::map;

	template <typename Def = std::string>
	std::enable_if_t<std::is_same_v<Def, std::string>, value_t>
	get(std::string_view key, Def && def = {}) const
	{
		return get(std::string(key), std::forward<Def>(def));
	}

	template <typename Key = std::string, typename Def = std::string>
	std::enable_if_t<std::is_same_v<Key, std::string> && std::is_same_v<Def, std::string>, value_t>
	get(const Key &key, Def && def = {}) const
	{
		if (auto it = find(key); it != end())
			return value_t(it->second);

		return std::forward<Def>(def);
	}

	fz::datetime parse_retry_after(unsigned int min_seconds_later = 0) const;
};

}

#endif // FZ_HTTP_HEADERS_HPP
