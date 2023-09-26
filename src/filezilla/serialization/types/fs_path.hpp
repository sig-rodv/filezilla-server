#ifndef FZ_SERIALIZATION_TYPES_FS_PATH_HPP
#define FZ_SERIALIZATION_TYPES_FS_PATH_HPP

#include "../../util/filesystem.hpp"

#include "../../serialization/helpers.hpp"

namespace fz::util::fs {

template <typename Archive, typename String, path_format Format>
void load_minimal(const Archive &, basic_path<String, Format> &bp, const String & s)
{
	bp = std::move(s);
}

template <typename Archive, typename String, path_format Format>
String save_minimal(const Archive &, const basic_path<String, Format> &bp)
{
	return bp;
}

}

#endif // FZ_SERIALIZATION_TYPES_FS_PATH_HPP
