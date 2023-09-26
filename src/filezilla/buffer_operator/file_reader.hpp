#ifndef FZ_BUFFER_OPERATOR_FILE_READER_HPP
#define FZ_BUFFER_OPERATOR_FILE_READER_HPP

#include <libfilezilla/file.hpp>

#include "adder.hpp"

namespace fz::buffer_operator {

	class file_reader: public adder {
	public:
		file_reader(file &file, unsigned int max_buffer_size): file_{file}, max_buffer_size_{max_buffer_size} {}

		int add_to_buffer() override {
			auto buffer = get_buffer();
			if (!buffer)
				return EINVAL;

			int64_t to_read;

			if constexpr (sizeof(std::size_t) >= sizeof(int64_t))
				to_read = int64_t(std::min(max_buffer_size_-buffer->size(), std::size_t(std::numeric_limits<int64_t>::max())));
			else
				to_read = int64_t(max_buffer_size_-buffer->size());

			if (!to_read)
				return ENOBUFS;

			auto read = file_.read(buffer->get(std::size_t(to_read)), to_read);
			if (read < 0)
				return EIO;

			if (!read)
				return ENODATA;

			buffer->add(size_t(read));
			return 0;
		}

	private:
		file &file_;
		unsigned int max_buffer_size_;
	};

}
#endif // FZ_BUFFER_OPERATOR_FILE_READER_HPP
