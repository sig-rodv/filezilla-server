#ifndef FZ_AUTHENTICATION_AUTHENTICATOR_HPP
#define FZ_AUTHENTICATION_AUTHENTICATOR_HPP

#include <vector>

#include <libfilezilla/event_handler.hpp>
#include <libfilezilla/iputils.hpp>

#include "user.hpp"
#include "error.hpp"
#include "method.hpp"

#include "../util/parser.hpp"

#ifndef FZ_AUTHENTICATION_AUTHENTICATOR_USERS_CASE_INSENSITIVE
#	ifdef FZ_WINDOWS
#		define FZ_AUTHENTICATION_AUTHENTICATOR_USERS_CASE_INSENSITIVE 1
#	else
#		define FZ_AUTHENTICATION_AUTHENTICATOR_USERS_CASE_INSENSITIVE 0
#	endif
#endif

#if FZ_AUTHENTICATION_AUTHENTICATOR_USERS_CASE_INSENSITIVE
#	include <map>
#endif

namespace fz::authentication {

class authenticator {
public:
	struct operation {
		using result_event = simple_event<operation, authenticator &, std::unique_ptr<operation>>;

		virtual ~operation();

		virtual shared_user get_user() = 0;
		virtual available_methods get_methods() = 0;
		virtual error get_error() = 0;

		friend void stop(std::unique_ptr<operation> op)
		{
			if (op)
				op->stop();
		}

		friend bool next(std::unique_ptr<operation> op, const methods_list &methods)
		{
			if (op)
				return op->next(methods);

			return false;
		}

	private:
		/// \brief Stops the authetication process.
		/// Can be invoked only once.
		virtual void stop() = 0;

		/// \brief Invokes the next step of the authentication.
		/// Can be invoked only once.
		/// \returns false if invoking failed.
		virtual bool next(const methods_list &methods) = 0;
	};

	virtual ~authenticator();

	/// \brief Starts the authentication process.
	/// The \param methods contains a sequence of methods to be evaluated at once. Authentication succeeds IFF all of the methods succeed.
	virtual void authenticate(std::string_view user_name, const methods_list &methods, address_type family, std::string_view ip, event_handler &target, logger::modularized::meta_map meta_for_logging = {}) = 0;
	virtual void stop_ongoing_authentications(event_handler &target) = 0;

protected:
#if FZ_AUTHENTICATION_AUTHENTICATOR_USERS_CASE_INSENSITIVE
	struct case_insensitive_utf8_less
	{
		bool operator()(const std::string &lhs, const std::string &rhs) const
		{
			return fz::stricmp(fz::to_wstring_from_utf8(lhs), fz::to_wstring_from_utf8(rhs)) < 0;
		}
	};

	template <typename T>
	using users_map = std::map<std::string /*user name*/, T, case_insensitive_utf8_less>;
#else
	template <typename T>
	using users_map = std::unordered_map<std::string /*user name*/, T>;
#endif
};

void stop(std::unique_ptr<authenticator::operation> op);
bool next(std::unique_ptr<authenticator::operation> op, const methods_list &methods);

struct none_authenticator: authenticator
{

	void authenticate(std::string_view, const methods_list &, address_type, std::string_view, event_handler &, logger::modularized::meta_map meta_for_logging = {}) override;

	void stop_ongoing_authentications(event_handler &) override;
};

}

namespace fz::serialization {

template <typename Archive, trait::enable_if<trait::is_textual_v<Archive>> = trait::sfinae>
void load_minimal(const Archive &, fz::authentication::available_methods &am, const std::string &s)
{
	am = {};

	for (auto x: fz::strtokenizer(s, ", ", true)) {
		util::parseable_range r(x);

		if (unsigned long long v = 0; parse_int(r, v) && eol(r))
			am.push_back(fz::authentication::methods_set(v));
	}
}

template <typename Archive, trait::enable_if<trait::is_textual_v<Archive>> = trait::sfinae>
std::string save_minimal(const Archive &, const fz::authentication::available_methods &am)
{
	std::string ret;

	for (auto &x: am) {
		ret += to_string(x.to_ullong());
		ret += ",";
	}

	if (ret.size() > 0)
		ret.resize(ret.size() - 1);

	return ret;
}

template <typename Archive, trait::enable_if<!trait::is_textual_v<Archive>> = trait::sfinae>
void serialize(Archive &ar, fz::authentication::available_methods &am)
{
	ar(static_cast<fz::authentication::available_methods::vector&>(am));
}

template <typename Archive>
struct specialize<
	Archive,
	fz::authentication::available_methods,
	specialization::unversioned_nonmember_load_save_minimal,
	std::enable_if_t<trait::is_textual_v<Archive>>
>{};

template <typename Archive>
struct specialize<
	Archive,
	fz::authentication::available_methods,
	specialization::unversioned_nonmember_serialize,
	std::enable_if_t<!trait::is_textual_v<Archive>>
>{};

}

#endif // FZ_AUTHENTICATION_AUTHENTICATOR_HPP
