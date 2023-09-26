#include <utility>

#include "../util/filesystem.hpp"
#include "../logger/null.hpp"

#include "../debug.hpp"

#include "mount.hpp"
#include "backends/local_filesys.hpp"

#define FZ_TVFS_DEBUG_LOG FZ_DEBUG_LOG("TVFS", 0)

namespace fz::tvfs {

const mount_tree::node *tvfs::mount_tree::nodes::find(std::string_view name) const noexcept
{
	for (const auto &v: *this) {
		if (v.first == name) {
			return &v.second;
		}
	}

	return nullptr;
}

mount_tree::node *tvfs::mount_tree::nodes::find(std::string_view name) noexcept
{
	return const_cast<node*>(std::as_const(*this).find(name));
}

mount_tree::node *tvfs::mount_tree::nodes::insert(std::string_view name, permissions perms)
{
	return &emplace_back(name, node{perms}).second;
}

std::tuple<const tvfs::mount_tree::node &, std::size_t> mount_tree::find_node(const tvfs::canonicalized_path_elements &elements) const noexcept
{
	const auto *node = &root_;
	std::size_t ei = 0;

	for (; ei < elements.size(); ++ei) {
		auto new_node = node->children.find(elements[ei]);
		if (!new_node)
			break;

		node = new_node;
	}

	return {*node, ei};
}


mount_tree::mount_tree()
{
	FZ_TVFS_DEBUG_LOG(L"Mount tree created");
}

mount_tree::mount_tree(const tvfs::mount_table &mt, placeholders placeholders)
{
	set_placeholders(std::move(placeholders));
	merge_with(mt);

	FZ_TVFS_DEBUG_LOG(L"Mount tree merged");
}

mount_tree::~mount_tree()
{
	FZ_TVFS_DEBUG_LOG(L"Mount tree destroyed");
}

mount_tree &tvfs::mount_tree::merge_with(tvfs::mount_table mt)
{
	static auto access2perms = [](mount_point::access_t access) constexpr -> permissions {
		using a = mount_point::access_t;

		switch (access) {
			case a::read_only: return permissions::read | permissions::list_mounts;
			case a::read_write: return permissions::read | permissions::list_mounts | permissions::write;
			case a::disabled: return permissions{};
		}

		return permissions{};
	};

	static auto recursive2perms = [](mount_point::recursive_t recursive) constexpr -> permissions {
		using r = mount_point::recursive_t;

		switch (recursive) {
			case r::apply_permissions_recursively: return permissions::apply_recursively;
			case r::apply_permissions_recursively_and_allow_structure_modification: return permissions::apply_recursively | permissions::allow_structure_modification;
			case r::do_not_apply_permissions_recursively: return permissions{};
		}

		return permissions{};
	};

	std::sort(mt.begin(), mt.end(), [](const tvfs::mount_point &lhs, const tvfs::mount_point &rhs) {
		return canonicalized_path_elements(lhs.tvfs_path) < canonicalized_path_elements(rhs.tvfs_path);
	});

	for (const auto &mp: mt) {
		canonicalized_path_elements elements{mp.tvfs_path};

		node *n = &root_;

		for (const auto &e: elements) {
			auto next_node = n->children.find(e);

			if (!next_node) {
				next_node = n->children.insert(e);

				if (&e != &elements.back()) {
					if (util::fs::native_path_view target_path = n->target; !target_path.str().empty()) {
						next_node->target = target_path / fz::to_native(e);
						if (n->perms & permissions::apply_recursively)
							next_node->perms = n->perms;
						else
							next_node->perms = permissions::list_mounts;
					}
					else
						next_node->perms = permissions::list_mounts;
				}
			}

			n = next_node;
		}

		n->target = mp.native_path;
		n->perms = access2perms(mp.access) | recursive2perms(mp.recursive);
		n->flags = mp.flags;

		for(const auto &p: placeholders_)
			fz::replace_substrings(n->target, p.first, p.second);
	}

	return *this;
}


void mount_tree::set_placeholders(tvfs::mount_tree::placeholders placeholders)
{
	placeholders_ = std::move(placeholders);
}

mount_tree::placeholders &tvfs::mount_tree::get_placeholders()
{
	return placeholders_;
}


static void async_autocreate_directory(mount_tree::shared_const_node n, std::shared_ptr<backend> b, receiver_handle<> r);
static void async_autocreate_directories(mount_tree::shared_const_nodes ns, std::shared_ptr<backend> b, receiver_handle<> r);

static void async_autocreate_directories(mount_tree::shared_const_nodes ns, std::shared_ptr<backend> b, receiver_handle<> r)
{
	if (ns->empty())
		return r();

	auto cur = ns->begin();

	return async_autocreate_directory(mount_tree::shared_const_node(ns, &cur->second), b, async_reentrant_receive(r)
	>> [r = std::move(r), cur, ns, b] (auto && self) mutable {
		if (++cur != ns->end())
			return async_autocreate_directory(mount_tree::shared_const_node(ns, &cur->second), b, std::move(self));

		return r();
	});
}

static void async_autocreate_directory(mount_tree::shared_const_node n, std::shared_ptr<backend> b, receiver_handle<> r)
{
	if ((n->flags & mount_point::autocreate) && !n->target.empty()) {
		return b->mkdir(n->target, true, mkdir_permissions::normal, async_receive(r)
		>> [r = std::move(r), b, n](auto) mutable {
			return async_autocreate_directories(mount_tree::shared_const_nodes(n, &n->children), std::move(b), std::move(r));
		});
	}

	return async_autocreate_directories(mount_tree::shared_const_nodes(n, &n->children), std::move(b), std::move(r));
}


void async_autocreate_directories(std::shared_ptr<mount_tree> mt, std::shared_ptr<backend> b, receiver_handle<> r)
{
	if (!mt)
		return r();

	if (!b) {
		thread_local auto static_b = std::make_shared<backends::local_filesys>(logger::null);
		b = static_b;
	}

	return async_autocreate_directory(mount_tree::shared_const_node(mt, &mt->root_), std::move(b), std::move(r));
}

}
