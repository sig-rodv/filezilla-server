#ifndef FZ_ACME_CERT_INFO_HPP
#define FZ_ACME_CERT_INFO_HPP

#include <string>
#include <vector>
#include <libfilezilla/json.hpp>

#include "../util/filesystem.hpp"

namespace fz::acme
{

struct extra_account_info
{
	std::string directory;
	std::vector<std::string> contacts;
	std::string created_at;
	std::pair<json, json> jwk;

	explicit operator bool() const
	{
		return !directory.empty() && jwk.first && jwk.second;
	}

	static extra_account_info load(const util::fs::native_path &root, const std::string &account_id);
	bool save(const util::fs::native_path &root, const std::string &account_id) const;
};

struct cert_info
{
	std::string account_id;
	std::vector<std::string> hostnames;

	explicit operator bool() const
	{
		return !account_id.empty() && !hostnames.empty();
	}

	struct extra
	{
		std::string fingerprint{};
		std::string distinguished_name{};
		extra_account_info account{};
		fz::datetime activation_time{};
		fz::datetime expiration_time{};
	};
};

}

#endif // FZ_ACME_CERT_INFO_HPP
