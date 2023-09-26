#include <cstring>

#include <libfilezilla/local_filesys.hpp>

#include "../../util/io.hpp"
#include "../../util/filesystem.hpp"

#include "file.hpp"

namespace fz::update::raw_data_retriever {

file::file(const native_string &persist_path, logger_interface &logger)
	: logger_(logger, "Update File Retriever")
	, persist_path_(persist_path)
{
}

void file::retrieve_raw_data(bool manual, fz::receiver_handle<result> h)
{
	auto load_file = [&](fz::file f, const fz::native_string &path, fz::datetime mtime) {
		int err = 0;

		if (auto raw = util::io::read(f, &err)) {
			logger_.log_u(logmsg::debug_info, L"Update info retrieved from cache file '%s'. Timestamp: %s.", path, mtime.get_rfc822());
			return h(std::pair(std::string(raw.to_view()), mtime));
		}

		std::string err_str = fz::sprintf("Couldn't read from cache file '%s': %s.", path, ::strerror(err));
		logger_.log_u(logmsg::debug_warning, L"%s", err_str);
		return h(fz::unexpected(err_str));
	};

	auto now = datetime::now();

	if (manual)
		logger_.log_raw(logmsg::debug_info, L"Caching has been temporarily disabled because the retrieval is being done manually.");
	else {
		if (auto f = fz::file(persist_path_, fz::file::reading); !f)
			logger_.log_u(logmsg::debug_info, L"Cannot open cache file '%s'.", persist_path_);
		else
		if (auto mtime = local_filesys::get_modification_time(persist_path_); !mtime)
			logger_.log_u(logmsg::debug_warning, L"Could not get cache file '%s' modification time, even though the file could previously be opened.", persist_path_);
		else
			return load_file(std::move(f), persist_path_, mtime);
	}

	return h(std::pair(std::string(), now));
}

void file::persist_if_newer(std::string_view raw_data, datetime timestamp)
{
	using namespace fz::util;

	if (!timestamp.later_than(local_filesys::get_modification_time(persist_path_)))
		return;

	auto f = fs::native_path_view(persist_path_).open(fz::file::writing, fz::file::current_user_and_admins_only | fz::file::empty);
	if (!f) {
		logger_.log_u(fz::logmsg::debug_warning, L"Couldn't create cache file '%s'", persist_path_);
		return;
	}

	if (int err = 0; !io::write(f, raw_data, &err)) {
		logger_.log_u(fz::logmsg::debug_warning, L"Couldn't write to cache file '%s': %s.", persist_path_, ::strerror(err));
		return;
	}

	logger_.log_u(fz::logmsg::debug_info, L"Updated cache file '%s'", persist_path_);

	if (!f.set_modification_time(timestamp))
		logger_.log_u(fz::logmsg::debug_warning, L"Couldn't update modification time of cache file '%s'.", persist_path_);
}

}
