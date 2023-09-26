#ifndef FZ_BUFFER_OPERATOR_FILE_WRITER_HPP
#define FZ_BUFFER_OPERATOR_FILE_WRITER_HPP

#include <libfilezilla/file.hpp>

#include "../buffer_operator/consumer.hpp"

namespace fz::buffer_operator {

	class file_writer: public consumer {
	public:
		explicit file_writer(file &file): file_{file} {}

		int consume_buffer() override {
			auto buffer = get_buffer();
			if (!buffer)
				return EINVAL;

			int64_t to_write = 0;

			if constexpr (sizeof(std::size_t) >= sizeof(int64_t))
				to_write = int64_t(std::min(buffer->size(), std::size_t(std::numeric_limits<int64_t>::max())));
			else
				to_write = int64_t(buffer->size());

			auto written = file_.write(buffer->get(), to_write);
			if (written < 0)
				return EIO;

			buffer->consume(size_t(written));
			return 0;
		}

	private:
		file &file_;
	};

}

#endif // FZ_BUFFER_OPERATOR_FILE_WRITER_HPP
