#include <libfilezilla/string.hpp>

#include "canonicalized_path_elements.hpp"

namespace fz::tvfs {

using namespace std::string_view_literals;

canonicalized_path_elements::canonicalized_path_elements(std::string_view path)
	: std::vector<std::string_view>(fz::strtok_view(path, '/'))
{
	std::size_t wi = 0;

	for (auto const &e: *this) {
		if (e == ".."sv) {
			if (wi != 0)
				--wi;
		}
		else
		if (!e.empty() && e != "."sv) {
			(*this)[wi++] = e;
		}
	}

	this->resize(wi);
}

}
