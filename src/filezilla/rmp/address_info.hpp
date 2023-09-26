#ifndef FZ_RMP_ADDRESS_INFO_HPP
#define FZ_RMP_ADDRESS_INFO_HPP

#include "../tcp/address_info.hpp"

namespace fz::rmp
{

struct address_info: tcp::address_info
{
	bool use_tls = true;

	template <typename Archive>
	void serialize(Archive &ar)
	{
		tcp::address_info::serialize(ar);

		ar(FZ_NVP_O(use_tls));
	}
};

}

#endif // FZ_RMP_ADDRESS_INFO_HPP
