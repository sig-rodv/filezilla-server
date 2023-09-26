#if !defined(FZ_WINDOWS)
#	include "fcntl.h"
#endif

#include "../../util/filesystem.hpp"

#include "local_filesys.hpp"

namespace fz::tvfs::backends {

local_filesys::local_filesys(logger_interface &logger)
	: logger_(logger, "local_filesys")
{
}

void local_filesys::open_file(const native_string &native_path, file::mode mode, file::creation_flags flags, receiver_handle<open_response> r)
{
	fz::file f;

	auto res = !util::fs::native_path_view(native_path).is_valid()
		? result { result::invalid }
		: f.open(native_path, mode, flags);

	logger_.log_u(logmsg::debug_debug, L"open_file(%s): fd = %d, res = %d", native_path, std::intptr_t(f.fd()), res.error_);

	return r(res, std::move(f));
}

void local_filesys::open_directory(const native_string &native_path, receiver_handle<open_response> r)
{
	fd_owner fd;
	result res;

	if (!util::fs::native_path_view(native_path).is_valid())
		res = {result::invalid};
	else {
		res = { result::other };

		#if defined(FZ_WINDOWS)
			fd = fd_owner(CreateFileW(native_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr));

			if (!fd) {
				switch (GetLastError()) {
					case ERROR_ACCESS_DENIED:
						res = { result::noperm };
						break;

					case ERROR_PATH_NOT_FOUND:
						res = { result::nodir };
						break;
				}
			}
		#else
			fd = fd_owner(::open(native_path.c_str(), O_DIRECTORY|O_RDONLY|O_CLOEXEC));

			if (!fd) {
				switch (errno) {
					case EACCES:
					case EPERM:
						res = { result::noperm };
						break;

					case ENOENT:
					case ENOTDIR:
						res = { result::nodir };
						break;
				}
			}
		#endif

		if (fd)
			res = { result::ok };
	}

	logger_.log_u(logmsg::debug_debug, L"open_directory(%s): fd = %d, res = %d", native_path, std::intptr_t(fd.get()), res.error_);

	return r(res, std::move(fd));
}

void local_filesys::rename(const native_string &path_from, const native_string &path_to, receiver_handle<rename_response> r)
{
	result res;

	if (!util::fs::native_path_view(path_from).is_valid() || !util::fs::native_path_view(path_to).is_valid())
		res = {result::invalid};
	else
		res = fz::rename_file(path_from, path_to);

	logger_.log_u(logmsg::debug_debug, L"rename(%s, %s): result: %d", path_from, path_to, res.error_);

	return r(res);
}

void local_filesys::remove_file(const native_string &path, receiver_handle<remove_response> r)
{
	result res;

	if (!util::fs::native_path_view(path).is_valid())
		res = { result::invalid };
	else
		res = fz::remove_file(path) ? result{ result::ok } : result{ result::nofile };

	logger_.log_u(logmsg::debug_debug, L"remove_file(%s): result: %d", path, res.error_);

	return r(res);
}

void local_filesys::remove_directory(const native_string &path, receiver_handle<remove_response> r)
{
	result res;

	if (!util::fs::native_path_view(path).is_valid())
		res = { result::invalid };
	else
		res = fz::remove_dir(path);

	logger_.log_u(logmsg::debug_debug, L"remove_directory(%s): result: %d", path, res.error_);

	return r(res);
}

void local_filesys::info(const native_string &path, bool follow_links, receiver_handle<info_response> r)
{
	result res;

	bool is_link;
	fz::local_filesys::type type = fz::local_filesys::unknown;
	int64_t size;
	datetime modification_time;
	int mode;

	if (!util::fs::native_path_view(path).is_valid())
		res = { result::invalid };
	else {
		type = fz::local_filesys::get_file_info(path, is_link, &size, &modification_time, &mode, follow_links);

		res = result{ type == fz::local_filesys::unknown ? result::other : result::ok };
	}

	logger_.log_u(logmsg::debug_debug, L"info(%s): result: %d", path, res.error_);

	return r(res, is_link, type, size, modification_time, mode);
}

void local_filesys::mkdir(const native_string &path, bool recurse, mkdir_permissions permissions, receiver_handle<mkdir_response> r)
{
	result res;

	if (!util::fs::native_path_view(path).is_valid())
		res = { result::invalid };
	else
		res = fz::mkdir(path, recurse, permissions);

	logger_.log_u(logmsg::debug_debug, L"mkdir(%s): result: %d", path, res.error_);

	return r(res);
}

void local_filesys::set_mtime(const native_string &path, const datetime &mtime, receiver_handle<set_mtime_response> r)
{
	result res;

	if (!util::fs::native_path_view(path).is_valid())
		res = { result::invalid };
	else
	if (fz::local_filesys::set_modification_time(path, mtime))
		res = { result::ok };
	else
		res = { result::other };

	logger_.log_u(logmsg::debug_debug, L"set_mtime(%s): result: %d", path, res.error_);

	return r(res);
}

}
