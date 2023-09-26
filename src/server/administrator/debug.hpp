#ifndef ADMINISTRATOR_DEBUG_HPP
#define ADMINISTRATOR_DEBUG_HPP

#include "../../filezilla/debug.hpp"

#ifndef ENABLE_ADMINISTRATOR_DEBUG
#   define ENABLE_ADMINISTRATOR_DEBUG 0
#endif

#define ADMINISTRATOR_DEBUG_LOG FZ_DEBUG_LOG("administrator", ENABLE_ADMINISTRATOR_DEBUG)

#endif // ADMINISTRATOR_DEBUG_HPP