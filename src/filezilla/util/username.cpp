#include <vector>

#include <libfilezilla/libfilezilla.hpp>

#if defined(FZ_WINDOWS)
#	include <windows.h>
#else
#	include <unistd.h>
#	include <fcntl.h>
#	include <pwd.h>
#endif

#include "username.hpp"

namespace fz::util {

std::wstring get_current_user_name()
{
#if defined(FZ_WINDOWS)
	std::wstring username;

	username.resize(128);
	DWORD size = DWORD(username.size());

	while (!GetUserNameW(username.data(), &size)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return {};

		username.resize(username.size() * 2);
		size = DWORD(username.size());
	}

	username.resize((size-1));
	return username;
#else
	std::vector<char> buf;
	buf.resize(256);
	struct passwd pwd;
	struct passwd *result{};

	int err{};

	while ((err = getpwuid_r(getuid(), &pwd, buf.data(), buf.size(), &result))) {
		if (err != ERANGE)
			break;

		buf.resize(buf.size()*2);
	}

	if (result != nullptr)
		return fz::to_wstring(result->pw_name);

	return {};
#endif
}


}
