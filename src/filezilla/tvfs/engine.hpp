#ifndef FZ_TVFS_ENGINE_HPP
#define FZ_TVFS_ENGINE_HPP

#include <libfilezilla/local_filesys.hpp>
#include <libfilezilla/file.hpp>

#include "../util/filesystem.hpp"
#include "../receiver.hpp"
#include "../logger/modularized.hpp"

#include "entry.hpp"
#include "backend.hpp"
#include "events.hpp"

namespace fz::tvfs {

class engine
{
public:
	engine(logger_interface &logger, duration sync_timeout = {});

	void set_mount_tree(std::shared_ptr<mount_tree> mt) noexcept;
	void set_backend(std::shared_ptr<backend> backend) noexcept;

	[[nodiscard]] result open_file(file &out_file, std::string_view tvfs_path, file::mode mode, std::int64_t rest);
	[[nodiscard]] result get_entries(entries_iterator &out_iterator, std::string_view tvfs_path, traversal_mode mode);
	[[nodiscard]] std::pair<result, entry> get_entry(std::string_view tvfs_path);
	[[nodiscard]] std::pair<result, std::string /*canonical path*/> make_directory(std::string tvfs_path);
	[[nodiscard]] std::pair<result, entry> set_mtime(std::string_view tvfs_path, entry_time mtime);
	[[nodiscard]] result remove_file(std::string_view tvfs_path);
	[[nodiscard]] result remove_directory(std::string_view tvfs_path);
	[[nodiscard]] result remove_entry(const entry &e);
	[[nodiscard]] result rename(std::string_view from, std::string_view to);
	[[nodiscard]] result set_current_directory(std::string_view tvfs_path);

	void async_open_file(file &out_file, std::string_view tvfs_path, file::mode mode, std::int64_t rest, receiver_handle<completion_event> r);
	void async_get_entries(entries_iterator &out_iterator, std::string_view tvfs_path, traversal_mode mode, receiver_handle<completion_event> r);
	void async_get_entry(std::string_view tvfs_path, receiver_handle<entry_result> r);
	void async_make_directory(std::string tvfs_path, receiver_handle<completion_event> r);
	void async_set_mtime(std::string_view tvfs_path, entry_time mtime, receiver_handle<entry_result> r);
	void async_remove_file(std::string_view tvfs_path, receiver_handle<completion_event> r);
	void async_remove_directory(std::string_view tvfs_path, receiver_handle<completion_event> r);
	void async_remove_entry(entry e, receiver_handle<completion_event> r);
	void async_rename(std::string_view from, std::string_view to, receiver_handle<completion_event> r);
	void async_set_current_directory(std::string_view tvfs_path, receiver_handle<simple_completion_event> r);

	[[nodiscard]] const util::fs::unix_path &get_current_directory() const;

private:
	resolved_path resolve_path(std::string_view path);

	logger::modularized logger_;
	sync_timeout_receive timeout_receive_;

	std::shared_ptr<mount_tree> mount_tree_;
	std::shared_ptr<backend> backend_;
	util::fs::unix_path current_directory_;
};

}

#endif // FZ_TVFS_ENGINE_HPP
