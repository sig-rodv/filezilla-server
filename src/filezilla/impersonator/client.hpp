#ifndef FZ_IMPERSONATOR_CLIENT_HPP
#define FZ_IMPERSONATOR_CLIENT_HPP

#include <list>

#include <libfilezilla/impersonation.hpp>
#include <libfilezilla/thread_pool.hpp>

#include "../logger/modularized.hpp"
#include "../tvfs/backend.hpp"

#include "channel.hpp"

namespace fz::impersonator {

class client: public tvfs::backend
{
public:
	client(thread_pool &thread_pool, logger_interface &logger, impersonation_token &&token = {}, native_string_view exe = {}, std::size_t pool_size = 1);

	const impersonation_token &get_token() const;

	void open_file(const native_string &native_path, file::mode mode, file::creation_flags flags, receiver_handle<open_response> r) override;
	void open_directory(const native_string &native_path, receiver_handle<open_response> r) override;
	void rename(const native_string &path_from, const native_string &path_to, receiver_handle<rename_response> r) override;
	void remove_file(const native_string &path, receiver_handle<remove_response> r) override;
	void remove_directory(const native_string &path, receiver_handle<remove_response> r) override;
	void info(const native_string &path, bool follow_links, receiver_handle<info_response> r) override;
	void mkdir(const native_string &path, bool recurse, mkdir_permissions permissions, receiver_handle<mkdir_response> r) override;
	void set_mtime(const native_string &path, const datetime &mtime, receiver_handle<set_mtime_response> r) override;

private:
	class get_caller;
	using callers = std::list<caller>;

	template <typename T, typename E, typename... Args>
	auto call(receiver_handle<E> &&r, Args &&... args) -> decltype(std::declval<caller>().call(T(std::forward<Args>(args)...), std::move(r)));

	fz::mutex mutex_;
	fz::condition condition_;

	thread_pool &thread_pool_;
	event_loop event_loop_;

	logger::modularized logger_;
	impersonation_token token_{};
	native_string exe_;
	std::size_t pool_size_{};

	callers callers_in_use_;
	callers callers_available_;
};

}

#endif // FZ_IMPERSONATOR_CLIENT_HPP
