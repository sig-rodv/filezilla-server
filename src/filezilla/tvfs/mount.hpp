#ifndef FZ_TVFS_MOUNT_HPP
#define FZ_TVFS_MOUNT_HPP

#include <string>
#include <memory>

#include <libfilezilla/string.hpp>

#include "permissions.hpp"
#include "canonicalized_path_elements.hpp"
#include "events.hpp"
#include "backend.hpp"

#include "../enum_bitops.hpp"

namespace fz::tvfs {

struct mount_point {
	std::string tvfs_path;
	native_string native_path;

	enum access_t: std::uint8_t {
		read_only,
		read_write,
		disabled
	} access = read_write;

	enum recursive_t: std::uint8_t {
		do_not_apply_permissions_recursively,
		apply_permissions_recursively,
		apply_permissions_recursively_and_allow_structure_modification
	} recursive = apply_permissions_recursively_and_allow_structure_modification;

	enum flags_t: std::uint8_t {
		autocreate = 1
	} flags = {};

	FZ_ENUM_BITOPS_FRIEND_DEFINE_FOR(flags_t)
};

struct mount_table: std::vector<mount_point> {
	using std::vector<mount_point>::vector;
};

class mount_tree {
public:
	struct node;
	struct nodes: std::vector<std::pair<std::string, node>> {
		using std::vector<std::pair<std::string, node>>::vector;

		const node *find(std::string_view name) const noexcept;
		node *find(std::string_view name) noexcept;
		node *insert(std::string_view name, permissions perms = {});
	};

	using shared_const_node = std::shared_ptr<const node>;
	using shared_const_nodes = std::shared_ptr<const nodes>;

	struct node {
		nodes children{};
		native_string target{};
		permissions perms{};
		mount_point::flags_t flags{};

		node(permissions perms = {})
			: perms(perms)
		{}
	};

	std::tuple<const node &, std::size_t /* remainder start pos */> find_node(const canonicalized_path_elements &) const noexcept;

	using placeholders = std::vector<std::pair<fz::native_string, fz::native_string>>;

	mount_tree();
	mount_tree(const mount_table &mt, placeholders placeholders = {});
	~mount_tree();

	mount_tree &merge_with(mount_table mt);

	void set_placeholders(placeholders placeholders);
	placeholders &get_placeholders();

private:
	friend void async_autocreate_directories(std::shared_ptr<mount_tree> mt, std::shared_ptr<backend> b, receiver_handle<> r);

	node root_{permissions::list_mounts};
	placeholders placeholders_;
};

void async_autocreate_directories(std::shared_ptr<mount_tree> mt, std::shared_ptr<backend> b, receiver_handle<> r);

}

#endif // FZ_TVFS_MOUNT_HPP
