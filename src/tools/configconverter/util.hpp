#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdio>
#include <libfilezilla/format.hpp>

template <typename... Args>
void errout(Args &&... args)
{
	auto s = fz::to_string(fz::sprintf(std::forward<Args>(args)...));
	fprintf(stderr, "%s", s.c_str());
}

#endif // UTIL_HPP
