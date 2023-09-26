#include <cassert>

#include "../util/filesystem.hpp"

#include "entry.hpp"
#include <iostream>
namespace fz::tvfs {

using namespace std::string_view_literals;

void resolved_path::async_to_entry(std::shared_ptr<backend> i, receiver_handle<entry_result> r)
{
	entry e;

	if (!*this)
		return r(result{result::invalid}, std::move(e));

	if (!(node.perms & permissions::access_mask))
		return r(result{result::noperm}, std::move(e));

	if (tvfs_path.empty())
		return r(result{result::invalid}, std::move(e));

	e.name_ = tvfs_path;
	e.native_name_ = native_path;
	e.perms_ = node.perms;

	if (node.children && !node.children->empty()) {
		e.type_ = entry_type::dir;
		e.size_ = -1;
		e.mtime_ = entry_time::now();
		e.perms_ &= ~(permissions::remove | permissions::rename);

		return r(result{result::ok}, std::move(e));
	}

	if (e.native_name_.empty())
		// Entry not found
		return r(result{result::other}, std::move(e));

	auto native_name = e.native_name_;

	return i->info(native_name, true, async_receive(r)
		>> [r = std::move(r), e = std::move(e), is_mountpoint = bool(node.children)]
	(auto res, auto is_link, auto type, auto size, auto mtime, auto &) mutable
	{
		if (is_link)
			e.type_ = local_filesys::type::link;

		e.type_ = type;
		e.size_ = size;
		e.mtime_ = mtime;

		if (is_mountpoint)
			e.perms_ &= ~(permissions::remove | permissions::rename);
		else
			e.fixup_perms(e.perms_);

		return r(res, std::move(e));
	});
}

entry::entry()
{}

void entry::fixup_perms(permissions parent_perms)
{
	if (type_ == local_filesys::dir) {
		if (!(parent_perms & permissions::apply_recursively))
			perms_ = {};
		else
		if (!(parent_perms & permissions::allow_structure_modification))
			perms_ &= ~(permissions::remove | permissions::rename);
	}
}

entry::entry(const mount_tree::nodes::value_type &mnv)
	: name_(mnv.first)
	, native_name_(mnv.second.target)
	, type_(entry_type::dir)
	, size_(-1)
	, mtime_(entry_time::now())
	, perms_(mnv.second.perms & ~(permissions::remove | permissions::rename))
{
}

bool entry::can_rename() const
{
	return bool (perms_ & permissions::rename);
}

entry_time entry::parse_timeval(std::string_view str)
{
	int year, month, day, hour, minute, second;
	int milli = 0;

	static constexpr auto parse_int = [](const char *s, size_t n, int cap = std::numeric_limits<int>::max()) constexpr {
		int res = 0;

		while (n-- > 0) {
			auto &c = *(s++);

			if (c < '0' || c > '9')
				return -1;

			int new_res = res*10 + c-'0';
			if (new_res < cap)
				res = new_res;

		}

		return res;
	};

	if (str.size() < 14)
		return {};

	if (str.size() > 14) {
		if (str[14] != '.')
			return {};

		milli = parse_int(&str[15], str.size()-15, 999);
	}

	year   = parse_int(&str[0], 4);
	month  = parse_int(&str[4], 2);
	day    = parse_int(&str[6], 2);
	hour   = parse_int(&str[8], 2);
	minute = parse_int(&str[10], 2);
	second = parse_int(&str[12], 2);

	if (year < 1900 ||
		month < 1 || month > 12 ||
		day < 1 || day > 31 ||
		hour < 0 || hour > 23 ||
		minute < 0 || minute > 59 ||
		second < 0 || second > 59 ||
		milli < 0)
	{
		return {};
	}

	return {datetime::utc, year, month, day, hour, minute, second, milli};
}

void entries_iterator::async_begin_iteration(traversal_mode mode, resolved_path &&resolved_path, std::shared_ptr<backend> backend, logger_interface &logger, receiver_handle<completion_event> r)
{
	end_iteration();

	resolved_ = std::move(resolved_path);
	backend_ = std::move(backend);
	mode_ = mode;

	resolved_.async_to_entry(backend_, async_receive(r) >> [this, r = std::move(r), &logger](result result, entry &e) mutable {
		if (!result)
			return r(result, std::move(resolved_.tvfs_path));

		if (mode_ == traversal_mode::autodetect) {
			mode_ = e.type_ == entry_type::dir
				? traversal_mode::only_children
				: traversal_mode::no_children;
		}

		if (!(e.perms_ & permissions::read)) {
			if (mode_ == traversal_mode::no_children || !(e.perms_ & permissions::list_mounts))
				return r(fz::result{result::noperm}, std::move(resolved_.tvfs_path));
		}

		if (mode_ == traversal_mode::no_children)  {
			resolved_.node.children.reset();
			next_entry_ = std::move(e);

			return r(fz::result{result::ok}, std::move(resolved_.tvfs_path));
		}

		if (mode_ == traversal_mode::only_children) {
			if (e.type_ != entry_type::dir)
				return r(fz::result{result::nodir}, std::move(resolved_.tvfs_path));

			auto list_mounts = [this](auto &logger, auto &r) {
				if (!resolved_.node.children || resolved_.node.children->empty()) {
					logger.log_u(logmsg::error, L"Entry '%s' has empty native path and no mount nodes: this should never happen. Please report to the maintainer.", resolved_.tvfs_path);
					return r(fz::result{result::other}, std::move(resolved_.tvfs_path));
				}

				mount_nodes_it_ = resolved_.node.children->cbegin();

				return async_load_next_entry(async_receive(r) >> [this, r = std::move(r)] {
					return r(fz::result{result::ok}, std::move(resolved_.tvfs_path));
				});
			};

			bool must_attempt_to_open_directory = (e.perms_ & permissions::read) && !resolved_.native_path.empty();
			bool can_list_mounts = e.perms_ & permissions::list_mounts && resolved_.node.children && !resolved_.node.children->empty();

			if (must_attempt_to_open_directory) {
				return backend_->open_directory(resolved_.native_path, async_receive(r)
					>> [r = std::move(r), list_mounts, &logger, can_list_mounts, this]
				(auto result, auto &fd) mutable
				{
					if (result)
						result = lf_.begin_find_files(fd.release(), false, false);

					if (!result) {
						if (can_list_mounts)
							 return list_mounts(logger, r);

						return r(result, std::move(resolved_.tvfs_path));
					}

					return async_load_next_entry(async_receive(r) >> [this, r = std::move(r)] {
						return r(fz::result{result::ok}, std::move(resolved_.tvfs_path));
					});
				});
			}

			return list_mounts(logger, r);
		}

		assert(false && "Unknown traversal mode");
		return r(fz::result{result::other}, std::move(resolved_.tvfs_path));
	});
}

void entries_iterator::async_next_to_entry(receiver_handle<entry_result> r)
{
	entry e;
	bool is_link;

	e.perms_ = resolved_.node.perms;
	e.type_ = local_filesys::type::unknown;

	while (true) {
		if (!lf_.get_next_file(e.native_name_, is_link, e.type_, &e.size_, &e.mtime_, nullptr))
			break;

		e.name_ = to_utf8(e.native_name_);

		// If conversion to utf8 failed, there's no way we can show this entry to the user. Skip it.
		if (e.name_.empty()) {
			e.type_ = local_filesys::unknown;
			continue;
		}

		e.native_name_ = fz::util::fs::native_path_view(resolved_.native_path) / e.native_name_;

		auto fixup = [perms = resolved_.node.perms](auto &e, auto &r) {
			e.fixup_perms(perms);
			return r(result{result::ok}, std::move(e));
		};

		if (e.type_ == local_filesys::type::link) {
			auto path = e.native_name_;
			return backend_->info(path, true, async_receive(r)
				>> [e = std::move(e), r = std::move(r), fixup]
			(auto, auto, auto, auto size, auto mtime, auto) mutable
			{
				e.size_ = size;
				e.mtime_ = mtime;

				return fixup(e, r);
			});
		}

		return fixup(e, r);
	}

	return r(result{result::other}, std::move(e));
}

void entries_iterator::async_load_next_entry(receiver_handle<> r)
{
	auto iterate_over_mount_nodes = [this] {
		if (mount_nodes_it_ == resolved_.node.children->cend())
			next_entry_ = {};
		else
			next_entry_ = {*(*mount_nodes_it_)++};
	};

	if (mount_nodes_it_) {
		iterate_over_mount_nodes();
		return r();
	}

	return async_next_to_entry(async_reentrant_receive(r) >> [this, r = std::move(r), iterate_over_mount_nodes](auto &&self, auto, auto &entry) {
		// If the entry is also found in the virtual nodes, skip it.
		if (entry && resolved_.node.children && resolved_.node.children->find(entry.name()))
			return async_next_to_entry(std::move(self));

		if (!entry && resolved_.node.children && (resolved_.node.perms & permissions::list_mounts)) {
			mount_nodes_it_ = resolved_.node.children->cbegin();
			iterate_over_mount_nodes();
		}
		else
			next_entry_ = std::move(entry);

		return r();
	});
}

entry entries_iterator::next() {
	entry out_next_entry;

	async_next(sync_receive >> std::tie(std::ignore, out_next_entry));

	return out_next_entry;
}

void entries_iterator::async_next(receiver_handle<entry_result> r)
{
	return async_load_next_entry(async_receive(r) >> [r = std::move(r), next_entry = std::move(next_entry_)]() mutable {
		return r(result{result::ok}, std::move(next_entry));
	});
}

void entries_iterator::end_iteration()
{
	lf_.end_find_files();
	next_entry_ = {};
	resolved_.node.children = {};
	mount_nodes_it_ = {};
	mode_ = traversal_mode::autodetect;
}

void entry_facts::operator()(fz::util::buffer_streamer &bs) const {
	auto fact_type = [this]() {
		switch (e_.type()) {
			case entry_type::file:
				return "file"sv;

			case entry_type::dir:
				return "dir"sv;

			case entry_type::link:
				return "OS.unix=symlink"sv;

			case entry_type::unknown: break;
		}

		assert(e_.type() != local_filesys::type::unknown);

		// This should be never reached. In theory.
		return "X.error=unknown"sv;
	};

	auto fact_perms = [&](util::buffer_streamer &bs) {
		// perm-fact = "Perm" "=" *pvals
		// pvals     = "a" / "c" / "d" / "e" / "f" /
		//             "l" / "m" / "p" / "r" / "w"

		if (e_.type() != entry_type::dir) {
			if (e_.perms() & permissions::write)
				bs << "aw"sv;

			if (e_.perms() & permissions::read)
				bs << 'r';
		}
		else
		if (e_.type() == entry_type::dir) {
			if (e_.perms() & permissions::write)
				bs << "cp"sv;

			if (e_.perms() & (permissions::read | permissions::list_mounts))
				bs << "l"sv;

			if (e_.perms() & (permissions::read | permissions::write | permissions::list_mounts))
				bs << "e"sv;

			if (e_.perms() & permissions::allow_structure_modification)
				bs << "m"sv;
		}

		if (e_.perms() & permissions::rename)
			bs << "f"sv;

		if (e_.perms() & permissions::remove)
			bs << "d"sv;
	};

	if (w_ & type)
		bs << "type="sv << fact_type() << ';';

	if ((w_ & size) && e_.size() >= 0)
		bs << "size="sv << e_.size() << ';';

	if ((w_ & modify) && e_.mtime())
		bs << "modify="sv << e_.timeval(e_.mtime()) << ';';

	if (w_ & perm)
		bs << "perms="sv << fact_perms << ';';

	bs << ' ' << e_.name();
}

entry_facts::feat::feat(entry_facts::which w)
	: w_(w)
{}

void entry_facts::feat::operator()(util::buffer_streamer &bs) const
{
	auto e = [this](which w) {
		return w & w_ ? "*;"sv : ";"sv;
	};

	bs
		<< "type"sv << e(type)
		<< "size"sv << e(size)
		<< "modify"sv << e(modify)
		<< "perm"sv << e(perm);
}

entry_facts::opts::opts(entry_facts::which &w, std::string_view str)
	: w_(w)
{
	auto facts = fz::strtok_view(str, ";");

	for (auto &f: facts) {
		if (equal_insensitive_ascii(f, "type"))
			w_ |= type;
		else
		if (equal_insensitive_ascii(f, "size"))
			w_ |= size;
		else
		if (equal_insensitive_ascii(f, "modify"))
			w_ |= modify;
		else
		if (equal_insensitive_ascii(f, "perm"))
			w_ |= perm;
	}
}

void entry_facts::opts::operator()(util::buffer_streamer &bs) const
{
	auto e = [this](std::string_view s, which w) {
		return w & w_ ? s : ""sv;
	};

	bs
		<< e("type"sv, type)
		<< e("size"sv, size)
		<< e("modify"sv, modify)
		<< e("perm"sv, perm);
}


void entry_stats::operator()(fz::util::buffer_streamer &bs) const {
	#define R_(D) (((permissions::read & e_.perms()) || (e_.type() == entry_type::dir && permissions::list_mounts & e_.perms())) ? 'r' : '-')
	#define W_(D) ((permissions::write & e_.perms()) ? 'w' : '-')
	#define X_(D) ((e_.type() == entry_type::dir && (permissions::read | permissions::list_mounts) & e_.perms()) ? 'x' : '-')

	std::array<char, 10> perms = {
		e_.type() == entry_type::dir ? 'd' : e_.type() == entry_type::link ? 'l' : '-',
		R_(USR), W_(USR), X_(USR),
		R_(GRP), W_(GRP), X_(GRP),
		R_(OTH), W_(OTH), X_(OTH)
	};

	auto date = [&](util::buffer_streamer &bs) {
		static constexpr std::string_view months[] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};

		if (e_.mtime()) {
			auto six_months_ago = datetime::now() - duration::from_days(30*6);
			auto tm = e_.mtime().get_tm(datetime::zone::utc);

			bs << months[tm.tm_mon] << ' ' << bs.dec(tm.tm_mday, 2, '0') << ' ';
			if (e_.mtime() < six_months_ago)
				bs << bs.dec(1900+tm.tm_year, 5);
			else
				bs << bs.dec(tm.tm_hour, 2, '0') << ':' << bs.dec(tm.tm_min, 2, '0');
		}
		else
			bs << "??? ?? ?????"sv;
	};

	// FIXME: we're missing info here
	bs << perms << " 1 ftp ftp "sv << bs.dec(e_.size() >= 0 ? e_.size() : 0, 15) << ' ' << date << ' ' << e_.name();
}

}
