#ifndef FZ_AUTHENTICATION_ERROR_HPP
#define FZ_AUTHENTICATION_ERROR_HPP

#include <libfilezilla/string.hpp>

#include <cstdint>

namespace fz::authentication
{

struct error
{
	enum type: std::uint8_t {
		none,
		user_disabled,
		user_nonexisting,
		ip_disallowed,
		auth_method_not_supported,
		invalid_credentials,
		internal
	};

	error(type v = none)
		: v_(v)
	{}

	explicit operator bool() const noexcept
	{
		return v_ != none;
	}

	operator type() const noexcept
	{
		return v_;
	}

	template <typename String>
	friend String toString(const error &e)
	{
		using C = typename String::value_type;

		switch (e.v_) {
			case none: return fzS(C, "No error");
			case user_disabled: return fzS(C, "User is disabled");
			case user_nonexisting: return fzS(C, "User does not exist");
			case ip_disallowed: return fzS(C, "IP is not allowed");
			case auth_method_not_supported: return fzS(C, "Auth method is not supported");
			case invalid_credentials: return fzS(C, "Invalid credentials");
			case internal: return fzS(C, "Internal error");
		}

		return fzS(C, "Unknown error");
	}

private:
	type v_;
};

}


#endif // FZ_AUTHENTICATION_ERROR_HPP
