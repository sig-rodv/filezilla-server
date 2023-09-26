#ifndef FZ_HTTP_REQUEST_HPP
#define FZ_HTTP_REQUEST_HPP

#include <string>
#include <libfilezilla/uri.hpp>

#include "headers.hpp"
#include "../enum_bitops.hpp"

namespace fz::http
{

class request
{
public:
	enum flags_type
	{
		is_confidential = 1 << 0
	};

	std::string method;
	fz::uri uri;
	http::headers headers;
	std::string body;

	flags_type flags;
};

FZ_ENUM_BITOPS_DEFINE_FOR(request::flags_type)

}

#endif // FZ_HTTP_REQUEST_HPP
