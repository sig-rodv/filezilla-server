#ifndef FZ_BUILD_INFO_HPP
#define FZ_BUILD_INFO_HPP

#include <string>
#include <tuple>

#include <libfilezilla/time.hpp>
#include <libfilezilla/string.hpp>
#include <libfilezilla/format.hpp>

#include "util/parser.hpp"

#ifdef HAVE_CONFIG_H
#include "config.hpp"
#endif

#ifndef FZ_BUILD_TYPE
#	define FZ_BUILD_TYPE official
#endif

#ifndef FZ_BUILD_FLAVOUR
#	define FZ_BUILD_FLAVOUR standard
#endif

#ifndef PACKAGE_VERSION
#	define PACKAGE_VERSION "0.0.0"
#endif

namespace fz::build_info {

enum class build_type: std::uint8_t {
	official,
	nightly
};

enum class flavour_type: std::uint8_t {
	standard                = 1,
	professional_enterprise = 2
};

struct version_info
{
	unsigned int major{};
	unsigned int minor{};
	unsigned int micro{};
	unsigned int rc{};
	unsigned int beta{};

	constexpr explicit operator bool() const
	{
		return *this != version_info();
	}

	constexpr bool operator <(const version_info &rhs) const
	{
		return std::forward_as_tuple(major, minor, micro, rc-1, beta-1) < std::forward_as_tuple(rhs.major, rhs.minor, rhs.micro, rhs.rc-1, rhs.beta-1);
	}

	constexpr bool operator ==(const version_info &rhs) const
	{
		return std::tie(major, minor, micro, rc, beta) == std::tie(rhs.major, rhs.minor, rhs.micro, rhs.rc, rhs.beta);
	}

	constexpr bool operator >(const version_info &rhs) const
	{
		return std::forward_as_tuple(major, minor, micro, rc-1, beta-1) > std::forward_as_tuple(rhs.major, rhs.minor, rhs.micro, rhs.rc-1, rhs.beta-1);
	}

	constexpr bool operator !=(const version_info &rhs) const
	{
		return std::tie(major, minor, micro, rc, beta) != std::tie(rhs.major, rhs.minor, rhs.micro, rhs.rc, rhs.beta);
	}

	constexpr version_info() noexcept
	{}

	constexpr version_info(std::string_view s) noexcept
	{
		version_info new_info;

		util::parseable_range r(s);

		if (!parse_int(r, new_info.major))
			return;

		if (!lit(r, '.') || !parse_int(r, new_info.minor))
			return;

		if (!lit(r, '.') || !parse_int(r, new_info.micro))
			return;

		if (!eol(r)) {
			if (match_string(r, "-rc")) {
				if (!parse_int(r, new_info.rc))
					return;
			}

			if (match_string(r, "-beta")) {
				if (!parse_int(r, new_info.beta))
					return;
			}
		}

		if (!eol(r))
			return;

		*this = new_info;
	}

	version_info(const std::string &s) noexcept
		: version_info(std::string_view(s))
	{}

	template <std::size_t N>
	constexpr version_info(const char(&s)[N]) noexcept
		: version_info(std::string_view(s))
	{}

	template <typename String>
	friend String toString(const version_info &vi)
	{
		using C = typename String::value_type;

		auto ret = fz::sprintf(fzS(C, "%d.%d.%d"), vi.major, vi.minor, vi.micro);

		if (vi.rc)
			ret.append(fzS(C, "-rc")).append(toString<String>(vi.rc));

		if (vi.beta)
			ret.append(fzS(C, "-beta")).append(toString<String>(vi.beta));

		return ret;
	}

	operator std::string () const
	{
		return toString<std::string>(*this);
	}

	operator std::wstring () const
	{
		return toString<std::wstring>(*this);
	}
};

inline constexpr build_type type = build_type::FZ_BUILD_TYPE;
inline constexpr flavour_type flavour = flavour_type::FZ_BUILD_FLAVOUR;
inline constexpr version_info version = PACKAGE_VERSION;
extern const std::string_view package_name;
extern const std::string_view host;
extern const std::string_view warning_message;
extern const fz::datetime datetime;

bool convert(std::string_view s, flavour_type &f);

template <typename String>
constexpr String toString(flavour_type f)
{
	using C = typename String::value_type;

	switch (f) {
		case flavour_type::standard:
			return fzS(C, "standard");

		case flavour_type::professional_enterprise:
			return fzS(C, "professional_enterprise");
	}

	return {};
}

static_assert(build_info::version, "PACKAGE_VERSION must be defined to a valid version, but instead it's [" PACKAGE_VERSION "]");

}

#endif // FZ_BUILD_INFO_HPP
