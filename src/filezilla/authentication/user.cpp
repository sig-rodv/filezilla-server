#include "user.hpp"

#include "impersonator/client.hpp"

namespace fz::authentication {

bool subscribe(shared_user &su, event_handler &eh)
{
	if (su) {
		if (auto d = std::get_deleter<shared_user_deleter>(su)) {
			auto lock = su->lock();

			return d->handlers_.emplace(&eh).second;
		}
	}

	return false;
}

bool unsubscribe(shared_user &su, event_handler &eh)
{
	if (su) {
		if (auto d = std::get_deleter<shared_user_deleter>(su)) {
			auto lock = su->lock();

			return d->handlers_.erase(&eh);
		}
	}

	return false;
}

bool notify(shared_user &su)
{
	if (su) {
		if (auto d = std::get_deleter<shared_user_deleter>(su)) {
			auto lock = su->lock();

			for (auto eh: d->handlers_)
				eh->send_event<shared_user_changed_event>(su);


			return true;
		}
	}

	return false;
}

std::string user::real_name() const
{
	if (!name.empty()) {
		if (const auto &iname = impersonator ? impersonator->get_token().username() : fz::native_string(); !iname.empty())
			return fz::to_utf8(iname);
	}

	return name;
}

const impersonation_token &user::get_impersonation_token() const
{
	if (!impersonator) {
		static const impersonation_token empty_token;
		return empty_token;
	}

	return impersonator->get_token();
}

}
