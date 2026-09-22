#include "mesh_nd.h"

#if GDEXTENSION
#include <godot_cpp/templates/hash_set.hpp>
#elif GODOT_MODULE
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR < 6
#include "servers/rendering_server.h"
#else
#include "servers/rendering/rendering_server.h"
#endif
#endif

PackedInt32Array MeshND::deduplicate_edge_indices(const PackedInt32Array &p_items) {
	HashSet<Vector2i> unique_items;
	PackedInt32Array deduplicated_items;
	for (int i = 0; i < p_items.size() - 1; i += 2) {
		Vector2i edge_indices = Vector2i(p_items[i], p_items[i + 1]);
		if (edge_indices.x > edge_indices.y) {
			SWAP(edge_indices.x, edge_indices.y);
		}
		if (unique_items.has(edge_indices)) {
			continue;
		}
		unique_items.insert(edge_indices);
		deduplicated_items.push_back(edge_indices.x);
		deduplicated_items.push_back(edge_indices.y);
	}
	return deduplicated_items;
}

void MeshND::mark_proxy_mesh_3d_dirty() {
	_is_proxy_mesh_3d_dirty = true;
	emit_signal(StringName("proxy_mesh_3d_marked_dirty"));
}

void MeshND::mark_mesh_bounds_and_proxy_mesh_3d_dirty() {
	_is_proxy_mesh_3d_dirty = true;
	_is_rect_bounds_dirty = true;
	emit_signal(StringName("proxy_mesh_3d_marked_dirty"));
	// Only use signals as needed, so no separate rect bounds signal here.
}

bool MeshND::is_mesh_data_valid() {
	if (likely(_is_mesh_data_valid)) {
		return true;
	}
	_is_mesh_data_valid = validate_mesh_data();
	if (!_is_mesh_data_valid) {
		ERR_PRINT("MeshND: Mesh data is invalid on mesh '" + get_name() + "'.");
	}
	return _is_mesh_data_valid;
}

void MeshND::reset_mesh_data_validation() {
	_is_mesh_data_valid = false;
	emit_signal(StringName("mesh_data_validation_reset"));
	// Call this after so that external things which care about mesh validity are notified before things that just need to update their proxy meshes.
	mark_mesh_bounds_and_proxy_mesh_3d_dirty();
}

bool MeshND::validate_mesh_data() {
	bool ret = false;
	GDVIRTUAL_CALL(_validate_mesh_data, ret);
	return ret;
}

void MeshND::update_proxy_mesh_3d() {
	GDVIRTUAL_CALL(_update_proxy_mesh_3d);
}

Ref<ArrayMesh> MeshND::get_proxy_mesh_3d() {
	if (_proxy_mesh_3d.is_null()) {
		_proxy_mesh_3d.instantiate();
	}
	if (_is_proxy_mesh_3d_dirty) {
		const String mesh_path_or_name = get_path().is_empty() ? get_name() : get_path();
		const String proxy_mesh_hint = mesh_path_or_name + String(" Proxy Mesh 3D");
		_proxy_mesh_3d->set_name(proxy_mesh_hint);
		update_proxy_mesh_3d();
		_is_proxy_mesh_3d_dirty = false;
#if GODOT_MODULE
		if (RenderingServer::get_singleton() != nullptr && _proxy_mesh_3d->get_rid().is_valid()) {
			RenderingServer::get_singleton()->mesh_set_path(_proxy_mesh_3d->get_rid(), proxy_mesh_hint);
		}
#endif
	}
	return _proxy_mesh_3d;
}

void MeshND::validate_material_for_mesh(const Ref<MaterialND> &p_material) {
	GDVIRTUAL_CALL(_validate_material_for_mesh, p_material);
}

void MeshND::_bind_methods() {
	ADD_SIGNAL(MethodInfo("mesh_data_validation_reset"));
	ADD_SIGNAL(MethodInfo("proxy_mesh_3d_marked_dirty"));

	ClassDB::bind_static_method("MeshND", D_METHOD("deduplicate_edge_indices", "items"), &MeshND::deduplicate_edge_indices);
	ClassDB::bind_method(D_METHOD("get_rect_bounds"), &MeshND::get_rect_bounds);
	ClassDB::bind_method(D_METHOD("get_dimension"), &MeshND::get_dimension);

	ClassDB::bind_method(D_METHOD("get_proxy_mesh_3d"), &MeshND::get_proxy_mesh_3d);
	ClassDB::bind_method(D_METHOD("update_proxy_mesh_3d"), &MeshND::update_proxy_mesh_3d);
	ClassDB::bind_method(D_METHOD("mark_proxy_mesh_3d_dirty"), &MeshND::mark_proxy_mesh_3d_dirty);
	ClassDB::bind_method(D_METHOD("mark_mesh_bounds_and_proxy_mesh_3d_dirty"), &MeshND::mark_mesh_bounds_and_proxy_mesh_3d_dirty);

	ClassDB::bind_method(D_METHOD("is_mesh_data_valid"), &MeshND::is_mesh_data_valid);
	ClassDB::bind_method(D_METHOD("reset_mesh_data_validation"), &MeshND::reset_mesh_data_validation);
	ClassDB::bind_method(D_METHOD("validate_material_for_mesh", "material"), &MeshND::validate_material_for_mesh);

	GDVIRTUAL_BIND(_update_proxy_mesh_3d);
	GDVIRTUAL_BIND(_validate_mesh_data);
	GDVIRTUAL_BIND(_validate_material_for_mesh, "material");
}
