#include <cassert>
#include <deque>

#include <libfilezilla/impersonation.hpp>

#include "client.hpp"
#include "archives.hpp"
#include "messages.hpp"
#include "process.hpp"

#include "../rmp/dispatch.hpp"

#include "../serialization/types/containers.hpp"
#include "../serialization/types/variant.hpp"
#include "../serialization/types/local_filesys.hpp"

namespace fz::impersonator {

class client::get_caller
{
public:
	get_caller(client &client)
		: client_(client)
		, caller_(get())
	{}

	callers::iterator operator->()
	{
		return caller_;
	}

	~get_caller()
	{
		fz::scoped_lock lock(client_.mutex_);

		client_.callers_available_.splice(client_.callers_available_.end(), client_.callers_in_use_, caller_);
		client_.condition_.signal(lock);
	}

private:
	client::callers::iterator get()
	{
		fz::scoped_lock lock(client_.mutex_);

		while (client_.callers_available_.empty() && client_.callers_in_use_.size() == client_.pool_size_) {
			client_.logger_.log_u(logmsg::debug_verbose, "call: All callers are busy. Waiting for one to free up.");
			client_.condition_.wait(lock);
			client_.logger_.log_u(logmsg::debug_verbose, "call: a caller just freed up.");
		}

		while (!client_.callers_available_.empty() && !client_.callers_available_.front()) {
			client_.logger_.log_u(logmsg::debug_verbose, "call: the first available caller is dead. Erasing it from the queue.");
			client_.callers_available_.pop_front();
		}

		if (client_.callers_available_.empty()) {
			client_.logger_.log_u(logmsg::debug_verbose, "call: no available callers. Let's create one.");
			client_.callers_available_.emplace_back(client_.event_loop_, client_.logger_, std::make_unique<process>(client_.event_loop_, client_.thread_pool_, client_.logger_, client_.exe_, client_.token_));
		}

		auto caller = client_.callers_available_.begin();

		// Move the caller from the front of the available queue to the back of the in use queue.
		// It will be moved to the back of the available queue in the destructor.
		client_.callers_in_use_.splice(client_.callers_in_use_.end(), client_.callers_available_, caller);

		return caller;
	}

	client &client_;
	callers::iterator caller_;
};

client::client(thread_pool &thread_pool, logger_interface &logger, impersonation_token &&token, native_string_view exe, std::size_t pool_size)
	: thread_pool_(thread_pool)
	, event_loop_(thread_pool_)
	, logger_(logger, "impersonator client", { { "user", token ? fz::to_utf8(token.username()) : "<invalid tocken>"} })
	, token_(std::move(token))
	, exe_(exe)
	, pool_size_(pool_size > 0 ? pool_size : 1)
{
}

const impersonation_token &client::get_token() const
{
	return token_;
}

template <typename T, typename E, typename... Args>
auto client::call(receiver_handle<E> &&r, Args &&... args) -> decltype(std::declval<caller>().call(T(std::forward<Args>(args)...), std::move(r)))
{
	get_caller(*this)->call(T(std::forward<Args>(args)...), std::move(r));
}


/*********************/


void client::open_file(const native_string &native_path, file::mode mode, file::creation_flags flags, receiver_handle<open_response> r)
{
	call<messages::open_file>(std::move(r), native_path, mode, flags);
}


void client::open_directory(const native_string &native_path, receiver_handle<open_response> r)
{
	call<messages::open_directory>(std::move(r), native_path);
}

void client::rename(const native_string &path_from, const native_string &path_to, receiver_handle<rename_response> r)
{
	call<messages::rename>(std::move(r), path_from, path_to);
}

void client::remove_file(const native_string &path, receiver_handle<remove_response> r)
{
	call<messages::remove_file>(std::move(r), path);
}

void client::remove_directory(const native_string &path, receiver_handle<remove_response> r)
{
	call<messages::remove_directory>(std::move(r), path);
}

void client::info(const native_string &path, bool follow_links, receiver_handle<info_response> r)
{
	call<messages::info>(std::move(r), path, follow_links);
}

void client::mkdir(const native_string &path, bool recurse, mkdir_permissions permissions, receiver_handle<mkdir_response> r)
{
	call<messages::mkdir>(std::move(r), path, recurse, permissions);
}

void client::set_mtime(const native_string &path, const datetime &mtime, receiver_handle<set_mtime_response> r)
{
	call<messages::set_mtime>(std::move(r), path, mtime);
}

}
