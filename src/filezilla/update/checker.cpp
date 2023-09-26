#include <libfilezilla/process.hpp>

#include "../receiver/async.hpp"
#include "../string.hpp"

#include "checker.hpp"

namespace fz::update {

checker::checker(event_loop &loop, info::retriever &retriever, enabled_for_receiving_base &receiver, logger_interface &logger, options opts)
	: util::invoker_handler(loop)
	, retriever_(retriever)
	, receiver_(receiver)
	, logger_(logger, "Update Checker")
	, opts_(std::move(opts))
	, timer_id_{}
	, is_checking_now_{}
{
}

checker::~checker()
{
	stop_receiving();
	remove_handler();
}

void checker::set_options(options opts)
{
	scoped_lock lock(mutex_);
	opts_ = std::move(opts);

	auto old_check_dt_ = check_dt_;

	reschedule();

	if (old_check_dt_ != check_dt_)
		receiver_handle<result>{receiver_}(last_info_, check_dt_);
}

checker::options checker::get_options()
{
	scoped_lock lock(mutex_);
	return opts_;
}

void checker::start()
{
	scoped_lock lock(mutex_);

	if (!started_) {
		started_ = true;
		check_at(fz::datetime::now());
	}
}

void checker::stop()
{
	scoped_lock lock(mutex_);

	started_ = false;
	check_at({});
}

bool checker::check_now()
{
	scoped_lock lock(mutex_);

	return do_check_now(true);
}

bool checker::is_checking_now() const
{
	scoped_lock lock(mutex_);

	return is_checking_now_;
}

info checker::get_last_checked_info() const
{
	scoped_lock lock(mutex_);

	return last_info_;
}

datetime checker::get_next_check_dt() const
{
	scoped_lock lock(mutex_);

	return check_dt_;
}

auto dt2ms(const fz::datetime &dt)
{
	return (dt-fz::datetime(0, datetime::milliseconds)).get_milliseconds();
}

void checker::reschedule()
{
	datetime next_check;

	// EOL disables automatic checking
	if (opts_.frequency() && !last_info_.is_eol() && started_) {
		if (last_info_.get_timestamp())
			next_check = last_info_.get_timestamp() + opts_.frequency();
		else
		if (auto now = datetime::now(); now.earlier_than(check_dt_))
			next_check = check_dt_;
		else
			next_check = now + opts_.frequency();
	}

	check_at(next_check);
}

/****************************************/

bool checker::do_check_now(bool manual)
{
	if (is_checking_now_) {
		logger_.log_raw(logmsg::debug_warning, L"Already checking, nothing more to do.");
		return false;
	}

	is_checking_now_ = true;

	retriever_.retrieve_info(manual, opts_.allowed_type(), async_receive(this) >> [&](auto &expected_info) {
		scoped_lock lock(mutex_);

		if (expected_info) {
			last_info_ = *expected_info;

			if (!opts_.callback_path().empty() && last_info_.is_newer_than(fz::build_info::version)) {
				logger_.log_u(logmsg::debug_info, L"Running program '%s'", opts_.callback_path());
				fz::spawn_detached_process({opts_.callback_path(), fz::native_string(last_info_->version)});
			}
		}

		reschedule();

		receiver_handle<result>{receiver_}(std::move(expected_info), check_dt_);

		is_checking_now_ = false;
	});

	return true;
}

void checker::check_at(datetime dt)
{
	check_dt_ = dt;

	if (dt) {
		auto delta = dt - datetime::now();

		if (delta > duration::from_days(1))
			delta = duration::from_days(1);
		else
		if (delta > duration::from_minutes(1))
			delta = duration::from_minutes(1);

		timer_id_ = stop_add_timer(timer_id_, delta, true);
	}
	else {
		stop_timer(timer_id_);
		timer_id_ = {};
	}
}

void checker::operator()(const event_base &ev)
{
	if (on_invoker_event(ev))
		return;

	fz::dispatch<timer_event>(ev, [&](timer_id) {
		scoped_lock lock(mutex_);

		if (check_dt_ > datetime::now())
			return check_at(check_dt_);

		do_check_now(false);
	});
}

}
