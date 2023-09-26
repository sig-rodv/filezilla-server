#ifndef FZ_TVFS_BACKENDS_LOCAL_FILESYS_HPP
#define FZ_TVFS_BACKENDS_LOCAL_FILESYS_HPP

#include "../../logger/modularized.hpp"
#include "../backend.hpp"

namespace fz::tvfs::backends {

class local_filesys final: public backend
{
public:
	local_filesys(logger_interface &logger);

	void open_file(const native_string &native_path, file::mode mode, file::creation_flags flags, receiver_handle<open_response> r) override;
	void open_directory(const native_string &native_path, receiver_handle<open_response> r) override;
	void rename(const native_string &path_from, const native_string &path_to, receiver_handle<rename_response> r) override;
	void remove_file(const native_string &path, receiver_handle<remove_response> r) override;
	void remove_directory(const native_string &path, receiver_handle<remove_response> r) override;
	void info(const native_string &path, bool follow_links, receiver_handle<info_response> r) override;
	void mkdir(const native_string &path, bool recurse, mkdir_permissions permissions, receiver_handle<mkdir_response> r) override;
	void set_mtime(const native_string &path, const datetime &mtime, receiver_handle<set_mtime_response> r) override;

private:
	logger::modularized logger_;
};

}
#endif // FZ_TVFS_BACKENDS_LOCAL_FILESYS_HPP
