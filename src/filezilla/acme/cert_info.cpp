#include <libfilezilla/buffer.hpp>
#include <libfilezilla/json.hpp>
#include <libfilezilla/local_filesys.hpp>

#include "cert_info.hpp"
#include "../securable_socket.hpp"
#include "../util/io.hpp"

namespace fz::acme
{

extra_account_info extra_account_info::load(const util::fs::native_path &root, const std::string &account_id)
{
	extra_account_info extra{};

	if (!root.is_absolute())
		return extra;

	securable_socket::cert_info ci = securable_socket::acme_cert_info{ account_id, {} };
	ci.set_root_path(root);

	fz::buffer account_info_s;
	auto read = util::io::read(ci.key_path().parent().parent() / fzT("account.info"), account_info_s);
	if (!read)
		return extra;

	auto account_info = fz::json::parse(account_info_s.to_view());
	if (!account_info)
		return extra;

	extra.directory = account_info["directory"].string_value();
	for (auto &c: account_info["contact"])
		extra.contacts.push_back(c.string_value());
	extra.created_at = account_info["createdAt"].string_value();
	extra.jwk.first = std::move(account_info["jwk"]["priv"]);
	extra.jwk.second = std::move(account_info["jwk"]["pub"]);

	return extra;
}

bool extra_account_info::save(const util::fs::native_path &root, const std::string &account_id) const
{
	if (!root.is_absolute())
		return false;

	json account_info;

	account_info["kid"] = account_id;
	account_info["directory"] = directory;
	account_info["createdAt"] = created_at;
	account_info["jwk"]["priv"] = jwk.first;
	account_info["jwk"]["pub"] = jwk.second;

	auto &c = account_info["contacts"] = json(json_type::array);
	for (std::size_t i = 0; i < contacts.size(); ++i)
		c[i] = contacts[i];


	securable_socket::cert_info ci = securable_socket::acme_cert_info{ account_id, {} };
	ci.set_root_path(root);

	auto account_dir = ci.key_path().parent().parent();
	if (!fz::mkdir(account_dir, true, mkdir_permissions::cur_user_and_admins))
		return false;

	return util::io::write(account_dir / fzT("account.info"), account_info.to_string());
}

}
