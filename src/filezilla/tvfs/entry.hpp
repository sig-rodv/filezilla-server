#ifndef FZ_TVFS_ENTRY_HPP
#define FZ_TVFS_ENTRY_HPP

#include <string>
#include <memory>
#include <optional>

#include <libfilezilla/time.hpp>
#include <libfilezilla/local_filesys.hpp>
#include <libfilezilla/logger.hpp>

#include "../util/buffer_streamer.hpp"
#include "../util/filesystem.hpp"
#include "../receiver.hpp"

#include "backend.hpp"
#include "permissions.hpp"
#include "canonicalized_path_elements.hpp"
#include "mount.hpp"
#include "events.hpp"


namespace fz::tvfs
{

using entry_type = local_filesys::type;
using entry_size = std::int64_t;
using entry_time = datetime;

enum class traversal_mode { no_children, only_children, autodetect };

enum rest_mode : std::int64_t { append = -1 };

class entry {
public:
	explicit operator bool() const {
		return type_ != entry_type::unknown;
	}

	const std::string &name() const {
		return name_;
	}

	entry_type type() const {
		return type_;
	}

	entry_size size() const {
		return size_;
	}

	const entry_time &mtime() const {
		return mtime_;
	}

	permissions perms() const {
		return perms_;
	}

	bool is_directory() const {
		return type_ == entry_type::dir;
	}

	bool is_file() const {
		return type_ == entry_type::file;
	}

	bool is_symlink() const {
		return type_ == entry_type::link;
	}

	bool can_rename() const;

	static auto timeval(const entry_time &t) {
		return [&t](util::buffer_streamer &bs) {
			auto tm = t.get_tm(entry_time::zone::utc);

			bs << bs.dec(1900+tm.tm_year, 4);
			bs << bs.dec(tm.tm_mon+1, 2, '0');
			bs << bs.dec(tm.tm_mday, 2, '0');
			bs << bs.dec(tm.tm_hour, 2, '0');
			bs << bs.dec(tm.tm_min, 2, '0');
			bs << bs.dec(tm.tm_sec, 2, '0');

			if (t.get_accuracy() == t.milliseconds)
				bs << '.' << bs.dec(t.get_milliseconds(), 3, '0');
		};
	}

	static entry_time parse_timeval(std::string_view str);

	entry();

protected:
	friend class engine;
	friend class resolved_path;
	friend class entries_iterator;

	entry(const mount_tree::nodes::value_type &mnv);

	void fixup_perms(permissions parent_perms);

	std::string name_{};
	native_string native_name_{};
	entry_type type_{entry_type::unknown};
	entry_size size_{-1};
	entry_time mtime_{};
	permissions perms_{};
};

class resolved_path {
public:
	std::string tvfs_path{};
	native_string native_path{};

	struct node_t {
		permissions perms{};
		mount_tree::shared_const_nodes children{};
	} node;

	void async_to_entry(std::shared_ptr<backend> i, receiver_handle<entry_result> r);

	explicit operator bool() const
	{
		return !tvfs_path.empty();
	}
};

class entries_iterator final {
public:
	entries_iterator(){}

	bool has_next() const
	{
		return bool(next_entry_);
	}

	entry next();
	void async_next(receiver_handle<entry_result> r);

	void end_iteration();

	traversal_mode get_effective_traversal_mode() const
	{
		return mode_;
	}

private:
	friend class engine;

	void async_begin_iteration(traversal_mode mode, resolved_path &&resolved_path, std::shared_ptr<backend> backend, logger_interface &logger, receiver_handle<completion_event> r);
	void async_next_to_entry(receiver_handle<entry_result> r);
	void async_load_next_entry(receiver_handle<> r);

	local_filesys lf_;
	resolved_path resolved_;
	std::shared_ptr<backend> backend_;
	std::optional<mount_tree::nodes::const_iterator> mount_nodes_it_{};
	entry next_entry_;
	traversal_mode mode_{traversal_mode::autodetect};
};

class entry_facts {
public:
	enum which: unsigned int {
		type   = 1,
		size   = 2,
		modify = 4,
		perm   = 8,

		all = type | size | modify | perm
	};

	FZ_ENUM_BITOPS_FRIEND_DEFINE_FOR(which)

	entry_facts(const entry &e, which w = all)
		: e_{e}
		, w_{w}
	{}

	void operator()(util::buffer_streamer &bs) const;

	struct feat {
		feat(which w);

		void operator()(util::buffer_streamer &bs) const;

	private:
		which w_;
	};

	struct opts {
		opts(which& w, std::string_view str);

		void operator()(util::buffer_streamer &bs) const;

	private:
		which &w_;
	};

private:
	const entry &e_;
	which w_;
};

class entry_stats {
public:
	entry_stats(const entry &e)
		: e_{e}
	{}

	void operator()(util::buffer_streamer &bs) const;

private:
	const entry &e_;
};

class entry_name {
public:
	entry_name(const entry &e, std::string_view prefix = {})
		: e_(e)
		, prefix_(prefix)
	{}

	void operator()(util::buffer_streamer &bs) const {
		bs << prefix_ << util::fs::unix_path_view(e_.name()).base().str_view();
	}

private:
	const entry &e_;
	std::string_view prefix_;
};

}

#endif // FZ_TVFS_ENTRY_HPP
