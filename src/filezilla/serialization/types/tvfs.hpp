#ifndef FZ_SERIALIZATION_TYPES_TVFS_HPP
#define FZ_SERIALIZATION_TYPES_TVFS_HPP

#include "../../../filezilla/tvfs/mount.hpp"

#include "../../serialization/helpers.hpp"

namespace fz::serialization {

template <typename Archive>
void serialize(Archive &ar, tvfs::mount_point &mp) {
	using namespace fz::serialization;

	ar.attributes(
		nvp(mp.tvfs_path, "tvfs_path"),
		nvp(mp.native_path, "native_path"),
		nvp(mp.access, "access")
	)
	.optional_attribute(mp.recursive, "recursive")
	.optional_attribute(mp.flags, "flags");
}

}

#endif // FZ_SERIALIZATION_TYPES_TVFS_HPP
