#include "authenticator.hpp"

namespace fz::authentication {

void none_authenticator::authenticate(std::string_view, const methods_list &, address_type, std::string_view, event_handler &target, logger::modularized::meta_map)
{
	struct none_op: operation
	{
		shared_user get_user() override
		{
			return {};
		}

		available_methods get_methods() override
		{
			return {};
		}

		error get_error() override
		{
			return error::auth_method_not_supported;
		}

		bool next(const methods_list &) override
		{
			return false;
		}

		void stop() override
		{

		}
	};

	target.send_event<operation::result_event>(*this, std::unique_ptr<none_op>());
}

void none_authenticator::stop_ongoing_authentications(event_handler &)
{}

authenticator::operation::~operation()
{}

authenticator::~authenticator()
{
}

}
