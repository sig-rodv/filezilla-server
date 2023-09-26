#include <cassert>

#include "enabled_for_receiving.hpp"

namespace fz {

enabled_for_receiving_base::~enabled_for_receiving_base()
{
	assert(!get_receiving_context() && "You must invoke stop_receiving() from the most derived class' destructor.");
}

}
