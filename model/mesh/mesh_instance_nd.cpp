#include "mesh_instance_nd.h"

#include "../../render/rendering_server_nd.h"
#include "multi_surface_mesh_nd.h"
#include "single_surface_mesh_nd.h"

void MeshInstanceND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			RenderingServerND::get_singleton()->register_mesh_instance(this);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			// The singleton is already gone if the module was uninitialized first.
			RenderingServerND *rendering_server = RenderingServerND::get_singleton();
			if (rendering_server != nullptr) {
				rendering_server->unregister_mesh_instance(this);
			}
		} break;
	}
}

void MeshInstanceND::_validate_property(PropertyInfo &p_property) const {
	// For material override(s): Always show the single version if the array is empty,
	// always show the plural if the array has more than one element, and if the array
	// has exactly one element, show based on whether the mesh is a MultiSurfaceMeshND.
	if (p_property.name == StringName("material_override") || p_property.name == StringName("material_overrides")) {
		bool show_plural;
		if (_material_overrides.size() == 0) {
			show_plural = false;
		} else if (_material_overrides.size() > 1) {
			show_plural = true;
		} else {
			const Ref<MultiSurfaceMeshND> multi_surface_mesh = _mesh;
			show_plural = multi_surface_mesh.is_valid();
		}
		if (p_property.name == StringName("material_override")) {
			p_property.usage = show_plural ? PROPERTY_USAGE_NONE : PROPERTY_USAGE_DEFAULT;
		} else { // "material_overrides"
			p_property.usage = show_plural ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_NONE;
		}
	}
}

Ref<MaterialND> MeshInstanceND::_get_valid_active_material_for_surface(const Ref<SingleSurfaceMeshND> &p_surface_mesh, Ref<MaterialND> p_material) {
	if (p_material.is_null()) {
		p_material = p_surface_mesh->get_material();
	}
	// If both the surface mesh and material are valid, ensure the material is compatible with the mesh.
	if (p_material.is_valid()) {
		p_surface_mesh->validate_material_for_mesh(p_material);
	}
	return p_material;
}

Ref<MaterialND> MeshInstanceND::get_active_material(const int p_surface_index) const {
	Ref<MaterialND> material;
	const int material_override_count = _material_overrides.size();
	// Check the material overrides. Null here means to use the material from the mesh itself.
	if (material_override_count == 1) {
		// If there is a single material override, use it for all surfaces.
		material = _material_overrides[0];
	} else if (p_surface_index < material_override_count && 0 <= p_surface_index) {
		// If there are multiple material overrides, use the one corresponding to the surface index.
		material = _material_overrides[p_surface_index];
	}
	// Check the mesh for a material, and/or for material validity, as needed.
	if (_mesh.is_valid()) {
		// If the overrides have not provided a material, try to get it from the mesh itself.
		const Ref<SingleSurfaceMeshND> single_surface_mesh = _mesh;
		if (single_surface_mesh.is_valid()) {
			// Single-surface mesh: These have materials defined, so use it if no override is provided.
			material = _get_valid_active_material_for_surface(single_surface_mesh, material);
		} else {
			// Multi-surface mesh: These do not have materials, but their surfaces do.
			const Ref<MultiSurfaceMeshND> multi_surface_mesh = _mesh;
			if (multi_surface_mesh.is_valid()) {
				const Vector<Ref<SingleSurfaceMeshND>> &surface_meshes = multi_surface_mesh->get_surface_meshes();
				if (p_surface_index < surface_meshes.size() && 0 <= p_surface_index) {
					const Ref<SingleSurfaceMeshND> &surface_mesh = surface_meshes[p_surface_index];
					if (surface_mesh.is_valid()) {
						material = _get_valid_active_material_for_surface(surface_mesh, material);
					}
				}
			}
		}
	}
	// Note: It is possible that the returned material is still null, since meshes may have no material.
	// Therefore, rendering engines MUST handle the case of this returning null.
	return material;
}

Ref<MaterialND> MeshInstanceND::get_material_override() const {
	if (_material_overrides.is_empty()) {
		return Ref<MaterialND>();
	}
	return _material_overrides[0];
}

void MeshInstanceND::set_material_override(const Ref<MaterialND> &p_material) {
	if (p_material.is_valid()) {
		// Set the material override for all surfaces by using an array with a single element.
		_material_overrides.resize(1);
		_material_overrides.set(0, p_material);
	} else {
		// Set no material override for all surfaces by clearing the array.
		_material_overrides.clear();
	}
	notify_property_list_changed();
}

Vector<Ref<MaterialND>> MeshInstanceND::get_material_overrides() const {
	return _material_overrides;
}

void MeshInstanceND::set_material_overrides(const Vector<Ref<MaterialND>> &p_material_overrides) {
	_material_overrides = p_material_overrides;
	notify_property_list_changed();
}

TypedArray<MaterialND> MeshInstanceND::get_material_overrides_bind() const {
	TypedArray<MaterialND> bind;
	bind.resize(_material_overrides.size());
	for (int i = 0; i < _material_overrides.size(); ++i) {
		bind[i] = _material_overrides[i];
	}
	return bind;
}

void MeshInstanceND::set_material_overrides_bind(const TypedArray<MaterialND> &p_material_overrides) {
	_material_overrides.resize(p_material_overrides.size());
	for (int i = 0; i < p_material_overrides.size(); ++i) {
		_material_overrides.set(i, p_material_overrides[i]);
	}
	notify_property_list_changed();
}

Ref<MeshND> MeshInstanceND::get_mesh() const {
	return _mesh;
}

void MeshInstanceND::set_mesh(const Ref<MeshND> &p_mesh) {
	_mesh = p_mesh;
	notify_property_list_changed();
}

Ref<RectND> MeshInstanceND::get_rect_bounds(const Ref<TransformND> &p_to_target) const {
	const Ref<TransformND> global_xform = get_global_transform();
	const Ref<TransformND> to_target = p_to_target->compose_square(global_xform);
	const Ref<MeshND> mesh = get_mesh();
	if (mesh.is_null()) {
		return RectND::from_position_size(to_target->get_origin(), VectorN());
	}
	return to_target->xform_rect(mesh->get_rect_bounds());
}

void MeshInstanceND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_active_material", "surface_index"), &MeshInstanceND::get_active_material, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("get_material_override"), &MeshInstanceND::get_material_override);
	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &MeshInstanceND::set_material_override);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "MaterialND"), "set_material_override", "get_material_override");

	ClassDB::bind_method(D_METHOD("get_material_overrides"), &MeshInstanceND::get_material_overrides_bind);
	ClassDB::bind_method(D_METHOD("set_material_overrides", "material_overrides"), &MeshInstanceND::set_material_overrides_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "material_overrides", PROPERTY_HINT_ARRAY_TYPE, "MaterialND", PROPERTY_USAGE_NONE), "set_material_overrides", "get_material_overrides");

	ClassDB::bind_method(D_METHOD("get_mesh"), &MeshInstanceND::get_mesh);
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &MeshInstanceND::set_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "MeshND"), "set_mesh", "get_mesh");
}
