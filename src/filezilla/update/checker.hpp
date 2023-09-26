#ifndef FZ_UPDATE_CHECKER_HPP
#define FZ_UPDATE_CHECKER_HPP

#include "../util/options.hpp"
#include "../logger/modularized.hpp"

#include "info.hpp"

namespace fz::update
{

class checker final: public util::invoker_handler, public enabled_for_receiving<checker>
{
public:
	using result = extend_receiver_event_t<checker, info::retriever::result, fz::datetime /*next check*/>;

	struct options: util::options<options>
	{
		opt<allow> allowed_type              = o(allow::release);                ///< Update types whose level is above allowed_type are not accepted
		opt<fz::duration> frequency          = o(fz::duration::from_seconds(5)); ///< Setting the frequency to 0 makes the checker one-shot.
		opt<fz::native_string> callback_path = o();                              ///< Path to a program that is invoked whenever an update is available

		options(){}
	};

	checker(event_loop &loop, info::retriever &retriever, enabled_for_receiving_base &receiver, logger_interface &logger, options opts = {});
	~checker() override;

	void set_options(options opts);
	options get_options();

	void start();
	void stop();

	bool check_now();
	bool is_checking_now() const;

	info get_last_checked_info() const;
	fz::datetime get_next_check_dt() const;

private:
	void reschedule();
	void check_at(datetime dt);
	bool do_check_now(bool manual);

	void operator ()(const event_base &) override;

private:
	mutable fz::mutex mutex_;

	info::retriever &retriever_;
	enabled_for_receiving_base &receiver_;

	logger::modularized logger_;
	options opts_;
	info last_info_;
	fz::datetime check_dt_;

	timer_id timer_id_;
	bool started_{};
	bool is_checking_now_;
};

}

#endif // FZ_UPDATE_CHECKER_HPP
