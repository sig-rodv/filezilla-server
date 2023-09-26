#ifndef CONVERTERS_HPP
#define CONVERTERS_HPP

#include "../../server/server_settings.hpp"
#include "../../filezilla/authentication/file_based_authenticator.hpp"

#include "old_config.hpp"

bool convert(const old_config &old, fz::authentication::file_based_authenticator::groups &groups, fz::authentication::file_based_authenticator::groups::value_type **speed_limited_group = nullptr);
bool convert(const old_config &old, fz::authentication::file_based_authenticator::users &users, fz::authentication::file_based_authenticator::groups::value_type *speed_limited_group = nullptr);
bool convert(const old_config &old, server_settings& settings);
bool convert(const old_config &old, fz::tcp::binary_address_list &disallowed_ips, fz::tcp::binary_address_list &allowed_ips);

#endif // CONVERTERS_HPP
