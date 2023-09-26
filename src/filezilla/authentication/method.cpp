#include "method.hpp"
#include "../string.hpp"
#include "../mpl/with_index.hpp"
#include "../mpl/at.hpp"

namespace fz::authentication {

const available_methods available_methods::none;

available_methods available_methods::filter(const methods_set &set) const
{
	available_methods ret;

	for (const auto &s: *this) {
		if ((s & set) == set)
			ret.push_back(s);
	}

	return ret;
}

bool available_methods::has(const methods_set &set) const
{
	for (const auto &s: *this) {
		if (s == set)
			return true;
	}

	return false;
}


bool available_methods::can_verify(const methods_set &methods) const
{
	for (const auto &s: *this) {
		// Anything can verify none.
		// All the rest can verify only itself
		if (!s || (methods && (s & methods) == methods))
			return true;
	}

	return false;

}

bool available_methods::set_verified(const any_method &method)
{
	for (auto &s: *this) {
		s.erase(method);

		if (!s) {
			assign({ {} });
			return false;
		}
	}

	return !empty();
}

bool available_methods::is_auth_necessary() const
{
	for (const auto &s: *this) {
		if (!s)
			return false;
	}

	return !empty();
}

bool available_methods::is_auth_possible() const
{
	return !empty();
}

std::string to_string(const available_methods &am)
{
	return fz::join<std::string>(am, ",");
}

std::string to_string(const any_method &m)
{
	return std::visit([](auto &m) { return m.template name<char>; }, static_cast<const any_method::variant&>(m));
}

std::string to_string(const methods_list &ml)
{
	return fz::sprintf("(%s)", fz::join<std::string>(ml, ","));
}

std::wstring to_wstring(const available_methods &am)
{
	return fz::join<std::wstring>(am, L",");
}

std::wstring to_wstring(const any_method &m)
{
	return std::visit([](auto &m) { return m.template name<wchar_t>; }, static_cast<const any_method::variant&>(m));
}

std::wstring to_wstring(const methods_list &ml)
{
	return fz::sprintf(L"(%s)", fz::join<std::wstring>(ml, L","));
}

template <typename String, std::size_t Size>
static String to_string(const std::bitset<Size> &bits)
{
	using C = typename String::value_type;
	String ret;

	for (std::size_t i = 0; i < Size; ++i) {
		if (bits.test(i)) {
			ret += mpl::with_index<Size>(i, [](auto i) {
				return mpl::at_c_t<any_method::variant, i+1>::template name<C>;
			});

			ret += fzS(C, "|");
		}
	}

	if (ret.size() > 0)
		ret.resize(ret.size()-1);

	return ret;
}

std::string to_string(const methods_set &set)
{
	return to_string<std::string>(set.bits_);
}

std::wstring to_wstring(const methods_set &set)
{
	return to_string<std::wstring>(set.bits_);
}


}
