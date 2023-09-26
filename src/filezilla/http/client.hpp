#ifndef FZ_HTTP_CLIENT_HPP
#define FZ_HTTP_CLIENT_HPP

#include <deque>
#include <libfilezilla/tls_system_trust_store.hpp>

#include "config.hpp"

#include "../channel.hpp"
#include "../logger/modularized.hpp"
#include "../buffer_operator/adder.hpp"
#include "../buffer_operator/line_consumer.hpp"
#include "../util/options.hpp"
#include "../securable_socket.hpp"

#include "request.hpp"
#include "response.hpp"
#include "message_consumer.hpp"

namespace fz::http
{

class client
	: fz::event_handler
	, private logger::modularized
	, private buffer_operator::adder
	, private message_consumer
	, private channel::progress_notifier
{
	class performer;

public:
	using cert_verifier = std::function<bool (const tls_session_info &)>;

	struct options: util::options<options, client>
	{
		opt<bool>                    follow_redirects         = o(false);
		opt<uint8_t>                 redirects_limits         = o(10);
		opt<std::string>             user_agent               = o(fz::replaced_substrings(PACKAGE_NAME, " ", "-").append("/" PACKAGE_VERSION));
		opt<std::size_t>             max_body_size            = o(10*1024*1024);
		opt<response::handler>       default_response_handler = o();
		opt<headers>                 default_request_headers  = o();
		opt<fz::duration>            default_timeout          = o();
		opt<tls_system_trust_store*> trust_store              = o();
		opt<client::cert_verifier>   cert_verifier            = o();

		options(){}
	};

	client(thread_pool &pool, event_loop &loop, logger_interface &logger, options opts = {});
	~client() override;

	performer perform(std::string verb, fz::uri uri, headers headers = {}, std::string body = {});

	static inline const auto do_not_verify = [](const tls_session_info &) {	return true; };

private:
	friend performer;

	void enqueue_request(request &&request, response::partial_handler &&handler, fz::duration &&timeout, fz::datetime &&at);
	void process_queue();

private:
	void on_connection_error(int error);
	void on_certificate_verification_event(tls_layer *, tls_session_info &info);
	void on_socket_event(fz::socket_event_source *, fz::socket_event_flag type, int error);
	void on_channel_done_event(channel &, channel::error_type error);
	void on_timer(timer_id id);
	void operator ()(const event_base &ev) override;

private:
	thread_pool &pool_;
	logger_interface &logger_;
	options opts_;
	std::unique_ptr<securable_socket> socket_;
	channel channel_;

private:
	fz::mutex mutex_;

	struct reqres
	{
		request req;
		response::partial_handler res_handler;
		fz::duration timeout;
		fz::monotonic_clock at;
	};
	std::deque<reqres> reqres_;

	request current_request_{};
	std::optional<response> current_response_{};
	bool current_response_headers_have_been_parsed_{};
	response::partial_handler current_response_handler_{};
	fz::duration current_timeout_{};
	fz::monotonic_clock perform_at_{};
	timer_id timeout_id_{};
	timer_id perform_at_id_{};

	bool must_secure_socket_{};
	uint8_t redirection_num_{};

private:
	int add_to_buffer() override;
	int process_message_start_line(std::string_view line) override;
	int process_message_header(std::string_view key, std::string_view value) override;
	int process_body_chunk(line_consumer::buffer_string_view chunk) override;
	int process_end_of_message_headers() override;
	int process_end_of_message() override;
	int process_error(int err, std::string_view msg) override;

private:
	void notify_channel_socket_read_amount(const monotonic_clock &time_point, std::int64_t amount) override;
	void notify_channel_socket_written_amount(const monotonic_clock &time_point, std::int64_t amount) override;
};

class client::performer final
{
public:
	~performer();

	performer &&with_timeout(fz::duration timeout) &&;
	performer &&confidentially() &&;
	performer &&at(fz::datetime dt) &&;

	void and_then(response::handler handler) &&;
	void and_then(response::partial_handler partial_handler) &&;

private:
	friend client;

	performer(client &client, request &&request);

private:
	response::partial_handler to_partial(response::handler handler);

	client &client_;
	request request_;
	response::partial_handler response_partial_handler_;
	fz::duration timeout_;
	fz::datetime at_;
};


inline client::performer client::perform(std::string verb, fz::uri uri, headers headers,  std::string body)
{
	return { *this, { std::move(verb), std::move(uri), std::move(headers), std::move(body), {} } };
}

inline client::performer::performer(client &client, request &&request)
	: client_(client)
	, request_(std::move(request))
	, response_partial_handler_(to_partial(client_.opts_.default_response_handler()))
	, timeout_(client_.opts_.default_timeout())
{}

inline client::performer::~performer()
{
	client_.enqueue_request(std::move(request_), std::move(response_partial_handler_), std::move(timeout_), std::move(at_));
}

inline client::performer &&client::performer::with_timeout(fz::duration timeout) &&
{
	timeout_ = std::move(timeout);
	return std::move(*this);
}

inline client::performer &&client::performer::confidentially() &&
{
	request_.flags |= request_.is_confidential;
	return std::move(*this);
}

inline fz::http::client::performer &&client::performer::at(datetime dt) &&
{
	at_ = dt;
	return std::move(*this);
}

inline void client::performer::and_then(response::handler handler) &&
{
	response_partial_handler_ = to_partial(std::move(handler));
}

inline void client::performer::and_then(response::partial_handler handler) &&
{
	response_partial_handler_ = std::move(handler);
}

}

#endif // FZ_HTTP_CLIENT_HPP
