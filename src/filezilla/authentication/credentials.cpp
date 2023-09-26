#include "credentials.hpp"

namespace fz::authentication {

bool credentials::verify(std::string_view username, const any_method &method, impersonation_token &token) const
{
	if (auto m = method.is<method::password>())
		return password.verify(username, m->data, token);

	token = {};

	return false;
}

}
