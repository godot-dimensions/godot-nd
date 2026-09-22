#include "single_surface_mesh_nd.h"

#include "../../math/vector_nd.h"
#include "wire/array_wire_mesh_nd.h"
#include "wire/wire_material_nd.h"

bool SingleSurfaceMeshND::has_edge_indices(int p_first, int p_second) {
	if (p_first > p_second) {
		SWAP(p_first, p_second);
	}
	PackedInt32Array edge_indices = get_edge_indices();
	for (int i = 0; i < edge_indices.size() - 1; i += 2) {
		if (edge_indices[i] == p_first && edge_indices[i + 1] == p_second) {
			return true;
		}
	}
	return false;
}

Ref<ArrayWireMeshND> SingleSurfaceMeshND::to_array_wire_mesh() {
	Ref<ArrayWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_vertex_positions(get_vertex_positions());
	wire_mesh->set_edge_indices(get_edge_indices());
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<WireMeshND> SingleSurfaceMeshND::to_wire_mesh() {
	return to_array_wire_mesh();
}

Ref<RectND> SingleSurfaceMeshND::get_rect_bounds() {
	if (likely(!_is_rect_bounds_dirty)) {
		return _rect_bounds;
	}
	const Vector<VectorN> vertex_positions = get_vertex_positions();
	const int dimension = vertex_positions.is_empty() ? 0 : vertex_positions[0].size();
	// Start by including the mesh's local origin always, even if the mesh does not cover that point.
	_rect_bounds = RectND::from_position_size(VectorND::zero(dimension), VectorND::zero(dimension));
	for (int vertex_index = 0; vertex_index < vertex_positions.size(); vertex_index++) {
		_rect_bounds->expand_self_to_point(vertex_positions[vertex_index]);
	}
	_is_rect_bounds_dirty = false;
	return _rect_bounds;
}

int SingleSurfaceMeshND::get_dimension() {
	const Vector<VectorN> vertex_positions = get_vertex_positions();
	if (vertex_positions.is_empty()) {
		return 0;
	}
	return vertex_positions[0].size();
}

Ref<MaterialND> SingleSurfaceMeshND::get_material() const {
	return _material;
}

void SingleSurfaceMeshND::set_material(const Ref<MaterialND> &p_material) {
	_material = p_material;
}

void SingleSurfaceMeshND::validate_material_for_mesh(const Ref<MaterialND> &p_material) {
	// Always call the virtual method to allow derived classes to provide more material validation.
	GDVIRTUAL_CALL(_validate_material_for_mesh, p_material);
	// For all SingleSurfaceMeshND-derived meshes: Validate the material's color array against the mesh's edge count.
	const Ref<WireMaterialND> wire_material = p_material;
	if (wire_material.is_valid()) {
		const PackedInt32Array edge_indices = get_edge_indices();
		PackedColorArray color_array = p_material->get_albedo_color_array();
		const int edge_count = edge_indices.size() / 2;
		if (color_array.size() < edge_count) {
			p_material->resize_albedo_color_array(edge_count);
		}
	}
}

PackedInt32Array SingleSurfaceMeshND::get_edge_indices() {
	PackedInt32Array indices;
	GDVIRTUAL_CALL(_get_edge_indices, indices);
	return indices;
}

Vector<VectorN> SingleSurfaceMeshND::get_edge_positions() {
	const PackedInt32Array edge_indices = get_edge_indices();
	const Vector<VectorN> vertex_positions = get_vertex_positions();
	Vector<VectorN> edges;
	for (const int32_t edge_index : edge_indices) {
		edges.append(vertex_positions[edge_index]);
	}
	return edges;
}

Vector<VectorN> SingleSurfaceMeshND::get_vertex_positions() {
	TypedArray<VectorN> typed_array_vertex_positions;
	GDVIRTUAL_CALL(_get_vertex_positions, typed_array_vertex_positions);
	Vector<VectorN> vertex_positions;
	vertex_positions.resize(typed_array_vertex_positions.size());
	for (int i = 0; i < typed_array_vertex_positions.size(); i++) {
		vertex_positions.set(i, typed_array_vertex_positions[i]);
	}
	return vertex_positions;
}

Vector<VectorN> SingleSurfaceMeshND::get_normal_values() {
	TypedArray<VectorN> typed_array_normals;
	GDVIRTUAL_CALL(_get_normal_values, typed_array_normals);
	Vector<VectorN> normals;
	normals.resize(typed_array_normals.size());
	for (int i = 0; i < typed_array_normals.size(); i++) {
		normals.set(i, typed_array_normals[i]);
	}
	return normals;
}

Vector<VectorM> SingleSurfaceMeshND::get_texture_map_values() {
	TypedArray<VectorM> typed_array_texture_maps;
	GDVIRTUAL_CALL(_get_texture_map_values, typed_array_texture_maps);
	Vector<VectorM> texture_maps;
	texture_maps.resize(typed_array_texture_maps.size());
	for (int i = 0; i < typed_array_texture_maps.size(); i++) {
		texture_maps.set(i, typed_array_texture_maps[i]);
	}
	return texture_maps;
}

TypedArray<VectorN> SingleSurfaceMeshND::get_edge_positions_bind() {
	const Vector<VectorN> edge_positions = get_edge_positions();
	TypedArray<VectorN> typed_array_edge_positions;
	typed_array_edge_positions.resize(edge_positions.size());
	for (int i = 0; i < edge_positions.size(); i++) {
		typed_array_edge_positions[i] = edge_positions[i];
	}
	return typed_array_edge_positions;
}

TypedArray<VectorN> SingleSurfaceMeshND::get_vertex_positions_bind() {
	const Vector<VectorN> vertex_positions = get_vertex_positions();
	TypedArray<VectorN> typed_array_vertex_positions;
	typed_array_vertex_positions.resize(vertex_positions.size());
	for (int i = 0; i < vertex_positions.size(); i++) {
		typed_array_vertex_positions[i] = vertex_positions[i];
	}
	return typed_array_vertex_positions;
}

TypedArray<VectorN> SingleSurfaceMeshND::get_normal_values_bind() {
	const Vector<VectorN> normals = get_normal_values();
	TypedArray<VectorN> typed_array_normals;
	typed_array_normals.resize(normals.size());
	for (int i = 0; i < normals.size(); i++) {
		typed_array_normals[i] = normals[i];
	}
	return typed_array_normals;
}

TypedArray<VectorM> SingleSurfaceMeshND::get_texture_map_values_bind() {
	const Vector<VectorM> texture_maps = get_texture_map_values();
	TypedArray<VectorM> typed_array_texture_maps;
	typed_array_texture_maps.resize(texture_maps.size());
	for (int i = 0; i < texture_maps.size(); i++) {
		typed_array_texture_maps[i] = texture_maps[i];
	}
	return typed_array_texture_maps;
}

void SingleSurfaceMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_edge_indices", "first", "second"), &SingleSurfaceMeshND::has_edge_indices);

	ClassDB::bind_method(D_METHOD("to_array_wire_mesh"), &SingleSurfaceMeshND::to_array_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_wire_mesh"), &SingleSurfaceMeshND::to_wire_mesh);

	ClassDB::bind_method(D_METHOD("get_material"), &SingleSurfaceMeshND::get_material);
	ClassDB::bind_method(D_METHOD("set_material", "material"), &SingleSurfaceMeshND::set_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "MaterialND"), "set_material", "get_material");

	ClassDB::bind_method(D_METHOD("get_edge_indices"), &SingleSurfaceMeshND::get_edge_indices);
	ClassDB::bind_method(D_METHOD("get_edge_positions"), &SingleSurfaceMeshND::get_edge_positions_bind);
	ClassDB::bind_method(D_METHOD("get_vertex_positions"), &SingleSurfaceMeshND::get_vertex_positions_bind);
	ClassDB::bind_method(D_METHOD("get_normal_values"), &SingleSurfaceMeshND::get_normal_values_bind);
	ClassDB::bind_method(D_METHOD("get_texture_map_values"), &SingleSurfaceMeshND::get_texture_map_values_bind);

	GDVIRTUAL_BIND(_get_edge_indices);
	GDVIRTUAL_BIND(_get_vertex_positions);
	GDVIRTUAL_BIND(_get_normal_values);
	GDVIRTUAL_BIND(_get_texture_map_values);
}
