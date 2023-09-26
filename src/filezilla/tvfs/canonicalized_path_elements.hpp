#ifndef FZ_TVFS_CANONICALIZED_PATH_ELEMENTS_HPP
#define FZ_TVFS_CANONICALIZED_PATH_ELEMENTS_HPP

#include <string>
#include <vector>

#include <libfilezilla/string.hpp>

namespace fz::tvfs
{

struct canonicalized_path_elements: public std::vector<std::string_view>
{
	canonicalized_path_elements() = default;
	canonicalized_path_elements(std::string_view path);

	template <typename String>
	String to_string(typename String::value_type sep, String prefix_path, std::size_t start_element = 0) const
	{
		if (auto i = start_element; i < size()) {
			if (prefix_path.empty() || prefix_path.back() != sep)
				prefix_path.append(1, sep);

			if constexpr (!std::is_same_v<native_string, std::string> && std::is_same_v<std::decay_t<String>, native_string>) {
				prefix_path.append(fz::to_native(fz::to_wstring_from_utf8(operator[](i))));

				while (++i < size())
					prefix_path.append(1, sep).append(fz::to_native(fz::to_wstring_from_utf8(operator[](i))));
			}
			else {
				prefix_path.append(operator[](i));

				while (++i < size())
					prefix_path.append(1, sep).append(operator[](i));
			}
		}

		return prefix_path;
	}
};

}

#endif // FZ_TVFS_CANONICALIZED_PATH_ELEMENTS_HPP
