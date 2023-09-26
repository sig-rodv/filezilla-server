#include <libfilezilla/socket.hpp>

#include "message_consumer.hpp"
#include "../util/parser.hpp"

namespace fz::http
{

message_consumer::message_consumer(logger_interface &logger, std::size_t max_line_size)
	: line_consumer(max_line_size)
	, logger_(logger)
{
}

void message_consumer::reset()
{
	status_ = parse_start_line;
	transfer_encoding_ = identity;
	remaining_chunk_size_ = std::numeric_limits<std::size_t>::max();
}

int message_consumer::process_message_start_line(std::string_view)
{
	return 0;
}

int message_consumer::process_message_header(std::string_view, std::string_view)
{
	return 0;
}

int message_consumer::process_body_chunk(buffer_string_view)
{
	return 0;
}

int message_consumer::process_end_of_message_headers()
{
	return 0;
}

int message_consumer::process_end_of_message()
{
	return 0;
}

int message_consumer::process_error(int err, std::string_view msg)
{
	logger_.log_u(logmsg::error, L"%s - %s", fz::socket_error_string(err), msg);
	return err;
}

int message_consumer::consume_buffer()
{
	if (status_ != parse_body)
		return line_consumer::consume_buffer();

	auto buffer = line_consumer::get_buffer();
	auto to_consume = std::min(remaining_chunk_size_, buffer->size());

	if (int res = process_body_chunk({buffer->get(), to_consume}))
		return res;

	remaining_chunk_size_ -= to_consume;
	buffer->consume(to_consume);

	if (remaining_chunk_size_ == 0) {
		if (transfer_encoding_ == chunked)
			status_ = parse_end_of_chunk;
		else {
			reset();
			return process_end_of_message();
		}
	}

	return 0;
}

int message_consumer::process_buffer_line(buffer_string_view bline, bool)
{
	auto line = std::string_view((const char*)bline.data(), bline.size());

	logger_.log_u(logmsg::debug_debug, L"[Status: %d] %s", int(status_), line);

	if (status_ == parse_start_line) {
		status_ = parse_headers;
		return process_message_start_line(line);
	}

	if (status_ == parse_headers || status_ == parse_trailer) {
		if (line.empty()) {
			if (status_ == parse_headers) {
				if (int err = process_end_of_message_headers())
					return err;

				if (transfer_encoding_ == chunked) {
					if (remaining_chunk_size_ != std::numeric_limits<std::size_t>::max())
						return process_error(EINVAL, "Content-Length and chunked Transfer-Encoding are not compatible");

					status_ = parse_chunk_size;
					return 0;
				}

				if (remaining_chunk_size_ != 0) {
					status_ = parse_body;
					return 0;
				}

				// Else fall through and end the message.
			}

			reset();
			return process_end_of_message();
		}

		auto const epos = line.find_first_of(':');
		if (epos == std::string::npos)
			return process_error(EINVAL, fz::sprintf("Invalid header line: %s", line));

		const auto & key = std::string(line.substr(0, epos));
		auto value = line.substr(epos+2);
		if (int err = process_message_header(key, value))
			return err;

		if (fz::equal_insensitive_ascii(key, "Transfer-Encoding")) {
			if (fz::equal_insensitive_ascii(value, "identity"))
				transfer_encoding_ = identity;
			else
			if (fz::equal_insensitive_ascii(value, "chunked"))
				transfer_encoding_ = chunked;
			else
				return process_error(EINVAL, fz::sprintf("Unsupported Transfer-Encoding: %s", value));
		}
		else
		if (fz::equal_insensitive_ascii(key, "Content-Length")) {
			util::parseable_range r(value);

			if (!(parse_int(r, remaining_chunk_size_) && eol(r)))
				return process_error(EINVAL, fz::sprintf("Invalid Content-Length: %s", value));
		}

		return 0;
	}

	if (status_ == parse_chunk_size) {
		util::parseable_range r(line);

		if (!(parse_int(r, remaining_chunk_size_, 16) && eol(r)))
			return process_error(EINVAL, fz::sprintf("Invalid chunk size: %s", line));

		if (remaining_chunk_size_ > 0)
			status_ = parse_body;
		else
			status_ = parse_trailer;

		return 0;
	}

	if (status_ == parse_end_of_chunk) {
		if (!line.empty())
			return process_error(EINVAL, fz::sprintf("Spurious data after end of chunk: %s", line));

		status_ = parse_chunk_size;

		return 0;
	}

	return process_error(EINVAL, fz::sprintf("Invalid internal status: %d.", status_));
}

}
