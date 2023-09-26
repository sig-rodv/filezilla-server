#ifndef FZ_BUFFER_OPERATOR_ADDER_HPP
#define FZ_BUFFER_OPERATOR_ADDER_HPP

#include "../buffer_operator/detail/base.hpp"

namespace fz::buffer_operator {

	using safe_locking_buffer = detail::safe_locking_buffer;
	using unsafe_locking_buffer = detail::unsafe_locking_buffer;
	using locking_buffer = detail::locking_buffer;

	class adder_interface: public detail::base_interface {
	public:
		virtual int /*error*/ add_to_buffer() = 0;

		bool start_adding_to_buffer() {
			return this->send_event(0);
		}
	};

	class adder: public detail::base<adder_interface> { };

}

#endif // FZ_BUFFER_OPERATOR_ADDER_HPP
