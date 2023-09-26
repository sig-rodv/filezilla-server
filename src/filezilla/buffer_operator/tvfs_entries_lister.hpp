#ifndef FZ_FTP_TVFS_ENTRIES_LISTER_HPP
#define FZ_FTP_TVFS_ENTRIES_LISTER_HPP

#include <libfilezilla/event_handler.hpp>

#include "../tvfs/entry.hpp"
#include "../buffer_operator/adder.hpp"

namespace fz::buffer_operator {

	template <typename EntryStreamer, typename... Args>
	class tvfs_entries_lister: public adder {
		struct next_response_tag{};
		using next_response = simple_event<next_response_tag>;
	public:
		tvfs_entries_lister(event_loop &loop, tvfs::entries_iterator &it, Args... args)
			: h_(loop)
			, it_{it}
			, args_{args...}
		{}

		tvfs_entries_lister &prepend_space(bool v = true) {
			prepend_space_ = v;
			return *this;
		}

		int add_to_buffer() override
		{
			if (!it_.has_next())
				return ENODATA;

			it_.async_next(async_receive(h_) >> [this](auto result, tvfs::entry &entry) {
				if (!result) {
					adder::send_event(EINVAL);
					return;
				}

				std::apply([&](auto& ...args) {
					auto buffer = get_buffer();
					if (!buffer) {
						adder::send_event(EINVAL);
						return;
					}

					auto out = util::buffer_streamer(*buffer);
					if (prepend_space_)
						out << ' ';
					out << EntryStreamer(entry, args...) << "\r\n";

					adder::send_event(0);
				}, args_);
			});

			return EAGAIN;
		}

	private:
		async_handler h_;
		tvfs::entries_iterator &it_;
		std::tuple<Args...> args_;
		bool prepend_space_{};
	};

}

#endif // FZ_FTP_TVFS_ENTRIES_LISTER_HPP
