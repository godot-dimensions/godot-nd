#include "mesh_instance_nd.h"

Ref<MaterialND> MeshInstanceND::get_active_material() const {
	if (_material_override.is_valid()) {
		return _material_override;
	}
	if (_mesh.is_valid()) {
		return _mesh->get_material();
	}
	return Ref<MaterialND>();
}

Ref<MaterialND> MeshInstanceND::get_material_override() const {
	return _material_override;
}

void MeshInstanceND::set_material_override(const Ref<MaterialND> &p_material) {
	_material_override = p_material;
}

Ref<MeshND> MeshInstanceND::get_mesh() const {
	return _mesh;
}

void MeshInstanceND::set_mesh(const Ref<MeshND> &p_mesh) {
	_mesh = p_mesh;
}

Ref<RectND> MeshInstanceND::get_rect_bounds(const Ref<TransformND> &p_inv_relative_to) const {
	const Ref<TransformND> global_transform = get_global_transform();
	Ref<RectND> bounds;
	bounds.instantiate();
	bounds->set_position(p_inv_relative_to->xform(global_transform->get_origin()));
	const Ref<MeshND> mesh = get_mesh();
	if (mesh.is_null()) {
		return bounds;
	}
	const Ref<TransformND> to_target = p_inv_relative_to->compose_square(global_transform);
	const Vector<VectorN> vertices = mesh->get_vertices();
	for (int vert_index = 0; vert_index < vertices.size(); vert_index++) {
		bounds = bounds->expand_to_point(to_target->xform(vertices[vert_index]));
	}
	return bounds;
}

void MeshInstanceND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_active_material"), &MeshInstanceND::get_active_material);

	ClassDB::bind_method(D_METHOD("get_material_override"), &MeshInstanceND::get_material_override);
	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &MeshInstanceND::set_material_override);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "MaterialND"), "set_material_override", "get_material_override");

	ClassDB::bind_method(D_METHOD("get_mesh"), &MeshInstanceND::get_mesh);
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &MeshInstanceND::set_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "MeshND"), "set_mesh", "get_mesh");
}
