#ifndef FZ_HTTP_MESSAGE_CONSUMER_HPP
#define FZ_HTTP_MESSAGE_CONSUMER_HPP

#include <libfilezilla/logger.hpp>

#include "../buffer_operator/line_consumer.hpp"

namespace fz::http
{

struct message_consumer: buffer_operator::line_consumer<buffer_line_eol::cr_lf>
{
	message_consumer(logger_interface &logger, std::size_t max_line_size);
	void reset();

protected:
	virtual int process_message_start_line(std::string_view line);
	virtual int process_message_header(std::string_view key, std::string_view value);
	virtual int process_body_chunk(line_consumer::buffer_string_view chunk);
	virtual int process_end_of_message_headers();
	virtual int process_end_of_message();
	virtual int process_error(int err, std::string_view msg);

private:
	int consume_buffer() override final;
	int process_buffer_line(buffer_string_view line, bool there_is_more_data_to_come) override final;

	logger_interface &logger_;

	enum status {
		parse_start_line,
		parse_headers,
		parse_trailer,
		parse_chunk_size,
		parse_end_of_chunk,
		parse_body
	} status_{};

	enum transfer_encoding {
		identity,
		chunked
	} transfer_encoding_{};

	std::size_t remaining_chunk_size_{};
};

}

#endif // FZ_HTTP_MESSAGE_CONSUMER_HPP
