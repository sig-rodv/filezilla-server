#ifndef COMMANDER_HPP
#define COMMANDER_HPP

#include <unordered_map>

#include <libfilezilla/string.hpp>
#include <libfilezilla/logger.hpp>

#include "../buffer_operator/file_reader.hpp"
#include "../buffer_operator/file_writer.hpp"
#include "../buffer_operator/streamed_adder.hpp"
#include "../buffer_operator/line_consumer.hpp"
#include "../buffer_operator/tvfs_entries_lister.hpp"

#include "../channel.hpp"
#include "../tvfs/engine.hpp"
#include "../tcp/session.hpp"

#include "controller.hpp"

namespace fz::ftp {

class commander final
	: public util::invoker_handler
	, private buffer_operator::streamed_adder
	, private buffer_operator::line_consumer<buffer_line_eol::cr_lf>
	, private controller::authenticate_user_response_handler
	, private controller::make_secure_response_handler
	, private controller::data_transfer_handler
	, private controller::data_local_info_handler
	, private channel::progress_notifier
	, public enabled_for_receiving<commander>
{
	using notifier = tcp::session::notifier;

public:
	struct welcome_message_t: std::string
	{
		welcome_message_t()
			: std::string()
			, has_version(true)
		{}

		welcome_message_t(std::string message, bool has_version = true)
			: std::string(std::move(message))
			, has_version(has_version)
		{}

		struct validate_result
		{
			static constexpr std::size_t line_limit = 1024;
			static constexpr std::size_t total_limit = 100*1024;

			enum which
			{
				ok                 = 0,
				total_size_too_big = 1,
				line_too_long      = 2,
			};

			validate_result(which result, std::string_view data)
				: result_(result)
				, data_(data)
			{
			}

			explicit operator bool() const
			{
				return result_ == ok;
			}

			operator which() const
			{
				return result_;
			}

			std::string_view data() const
			{
				return data_;
			}

		private:
			which result_;
			std::string_view data_;
		};

		validate_result validate() const;

		bool has_version;
	};

	commander(event_loop &loop, controller &co, tvfs::engine &tvfs, notifier &notifier,
			  fz::monotonic_clock &last_activity,
			  bool needs_security_before_user_cmd,
			  const welcome_message_t &welcome_message, const std::string &refuse_message,
			  logger_interface &logger);
	~commander() override;

	void set_socket(socket_interface *);
	void set_timeouts(const fz::duration &login_timeout, const fz::duration &activity_timeout);
	void shutdown(int err = 0);

	bool has_empty_buffers();
	bool is_executing_command() const;

private:
	channel channel_;
	controller &controller_;
	tvfs::engine &tvfs_;
	notifier &notifier_;
	const welcome_message_t &welcome_message_;
	const std::string &refuse_message_;
	logger_interface &logger_;

	enum command_flags {
		none                  = 0,
		needs_arg             = 1 << 0,
		needs_auth            = 1 << 1,
		must_be_last_in_queue = 1 << 2,
		needs_security        = 1 << 3,
		trim_arg              = 1 << 4
	};

	enum command_reply {
		positive_preliminary_reply          = 1,
		positive_completion_reply           = 2,
		positive_intermediary_reply         = 3,
		transient_negative_completion_reply = 4,
		permanent_negative_completion_reply = 5
	};

	static bool is_negative(command_reply cr)
	{
		return cr >= transient_negative_completion_reply;
	}

	struct command {
		void (commander::*func)(std::string_view arg);
		command_flags flags;
	};

	static inline std::unordered_map<std::string_view, command> commands_;

	std::string upcase_str_{};
	std::string_view make_upcase(std::string_view str);

	bool is_cmd_illegal(std::string_view cmd);
	int queue_new_cmd();
	bool a_cmd_has_been_queued_{};

	#define FTP_CMD(name, flags) \
		void name(std::string_view arg = std::string_view{}); \
		const inline static decltype(commands_)::const_iterator name##_cmd_ { commands_.try_emplace(#name, command{&commander::name, command_flags(flags)}).first }

	#define FTP_CMD_ALIAS(alias, name) \
		const inline static decltype(commands_)::const_iterator alias##_cmd_ { commands_.try_emplace(#alias, name##_cmd_->second).first }

	FTP_CMD(ABOR, needs_auth);
	FTP_CMD(ADAT, needs_auth | needs_security);
	FTP_CMD(ALLO, needs_auth);
	FTP_CMD(APPE, needs_arg | needs_auth);
	FTP_CMD(AUTH, needs_arg | must_be_last_in_queue);
	FTP_CMD(CDUP, needs_auth);
	FTP_CMD(CLNT, none);
	FTP_CMD(CWD,  needs_arg | needs_auth);
	FTP_CMD(DELE, needs_arg | needs_auth);
	FTP_CMD(EPRT, needs_arg | trim_arg | needs_auth);
	FTP_CMD(EPSV, needs_auth);
	FTP_CMD(FEAT, none);
	FTP_CMD(HELP, none);
	FTP_CMD(LIST, needs_auth);
	FTP_CMD(MDTM, needs_arg | needs_auth);
	FTP_CMD(MFMT, needs_arg | needs_auth);
	FTP_CMD(MKD,  needs_arg | needs_auth);
	FTP_CMD(MLSD, needs_auth);
	FTP_CMD(MLST, needs_auth);
	FTP_CMD(MODE, needs_arg | needs_auth );
	FTP_CMD(NLST, needs_auth);
	FTP_CMD(NOOP, none);
	FTP_CMD(OPTS, needs_arg);
	FTP_CMD(PASS, none);
	FTP_CMD(PASV, needs_auth);
	FTP_CMD(PBSZ, needs_arg | needs_security);
	FTP_CMD(PORT, needs_arg | trim_arg | needs_auth);
	FTP_CMD(PROT, needs_arg | needs_security);
	FTP_CMD(PWD,  needs_auth);
	FTP_CMD(QUIT, none);
	FTP_CMD(REST, needs_arg | needs_auth);
	FTP_CMD(RETR, needs_arg | needs_auth);
	FTP_CMD(RMD,  needs_arg | needs_auth);
	FTP_CMD(RNFR, needs_arg | needs_auth);
	FTP_CMD(RNTO, needs_arg | needs_auth);
	FTP_CMD(SIZE, needs_arg | needs_auth);
	FTP_CMD(STAT, needs_auth);
	FTP_CMD(STOR, needs_arg | needs_auth);
	FTP_CMD(STRU, needs_arg | needs_auth);
	FTP_CMD(SYST, none);
	FTP_CMD(TYPE, needs_arg | needs_auth);
	FTP_CMD(USER, needs_arg);

	FTP_CMD_ALIAS(NOP, NOOP);
	FTP_CMD_ALIAS(XCWD, CWD);
	FTP_CMD_ALIAS(XRMD, RMD);
	FTP_CMD_ALIAS(XMKD, MKD);
	FTP_CMD_ALIAS(XPWD, PWD);

	#undef FTP_CMD_ALIAS
	#undef FTP_CMD

	decltype(commands_)::const_iterator current_cmd_ { commands_.cend() };
	decltype(commands_)::const_iterator cmd_being_aborted_ { commands_.cend() };

	class responder;
	template <unsigned int XYZ>
	responder respond(bool emit_xyz_on_each_line = false);

	int failure_count_{};
	void act_upon_command_reply(command_reply reply);

	std::string user_;
	std::unique_ptr<authentication::authenticator::operation> auth_op_;

	bool only_allow_epsv_{};
	tvfs::entry_size rest_size_{};
	tvfs::entry_facts::which enabled_facts_ = tvfs::entry_facts::all;
	std::string names_prefix_;
	tvfs::entries_iterator entries_iterator_;
	file file_;

	buffer_operator::tvfs_entries_lister<tvfs::entry_facts, tvfs::entry_facts::which&> facts_lister_{event_loop_, entries_iterator_, enabled_facts_};
	buffer_operator::tvfs_entries_lister<tvfs::entry_stats> stats_lister_{event_loop_, entries_iterator_};
	buffer_operator::tvfs_entries_lister<tvfs::entry_name, std::string&> names_lister_{event_loop_, entries_iterator_, names_prefix_};
	buffer_operator::tvfs_entries_lister<tvfs::entry_facts, tvfs::entry_facts::which> mfmt_lister_{event_loop_, entries_iterator_, tvfs::entry_facts::which::modify};

	buffer_operator::file_reader file_reader_{file_, 128*1024};
	buffer_operator::file_writer file_writer_{file_};

	std::string rename_from_{};

	bool data_is_binary_{};

	duration login_timeout_{};
	duration activity_timeout_{};
	timer_id timer_id_{};
	monotonic_clock start_time_{};
	monotonic_clock &last_activity_;
	bool needs_security_before_user_cmd_{};

	// authenticate_user_response_handler interface
private:
	void handle_authenticate_user_response(std::unique_ptr<authentication::authenticator::operation> &&op) override;

	// make_secure_response_handler interface
private:
	void handle_make_secure_response(bool can_secure) override;

	// line_consumer interface
public:
	int process_buffer_line(buffer_string_view line, bool there_is_more_data_to_come) override;

	// event_handler interface
private:
	void operator ()(const event_base &) override;
	void on_channel_done_event(channel &, channel::error_type error);
	void on_timer_event(timer_id id);

	// data_transfer_handler interface
public:
	void handle_data_transfer(status st, channel::error_type error, std::string_view msg) override;

	// data_local_info_handler interface
public:
	void handle_data_local_info(const std::optional<std::pair<std::string, uint16_t>> &info) override;

private:
	void notify_channel_socket_read_amount(const monotonic_clock &time_point, std::int64_t) override;
	void notify_channel_socket_written_amount(const monotonic_clock &time_point, std::int64_t) override;

private:
	static std::string_view to_string_view(result r);

	class async_abortable_receive: async_receive
	{
		enum state {
			idle,
			pending,
			pending_abort
		};

		state state_ = idle;
		commander &owner_;

	public:
		async_abortable_receive(commander &owner);

		template <typename F>
		auto operator >>(F && f);

		void abort();
		bool is_pending() const;
	};

	async_abortable_receive async_receive_{*this};
};

}

#endif // COMMANDS_HPP
