#ifndef FZ_UTIL_TOOLS_HPP
#define FZ_UTIL_TOOLS_HPP

#include "filesystem.hpp"

namespace fz::util {

fs::native_path get_own_executable_directory();
fs::native_path find_tool(fz::native_string name, fs::native_path build_rel_path, const char *env);

}

#endif // FZ_UTIL_TOOLS_HPP
