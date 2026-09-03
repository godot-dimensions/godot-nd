#include "mesh_nd.h"

#include "../../math/rect_nd.h"
#include "../../math/vector_nd.h"
#include "wire/array_wire_mesh_nd.h"
#include "wire/wire_material_nd.h"

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

bool MeshND::has_edge_indices(int p_first, int p_second) {
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
}

bool MeshND::validate_mesh_data() {
	bool ret = false;
	GDVIRTUAL_CALL(_validate_mesh_data, ret);
	return ret;
}

void MeshND::update_proxy_mesh_3d() {
	GDVIRTUAL_CALL(_update_proxy_mesh_3d);
}

void MeshND::validate_material_for_mesh(const Ref<MaterialND> &p_material) {
	GDVIRTUAL_CALL(_validate_material_for_mesh, p_material);
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

Ref<ArrayWireMeshND> MeshND::to_array_wire_mesh() {
	Ref<ArrayWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_vertex_positions(get_vertex_positions());
	wire_mesh->set_edge_indices(get_edge_indices());
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<WireMeshND> MeshND::to_wire_mesh() {
	return to_array_wire_mesh();
}

Ref<RectND> MeshND::get_rect_bounds() {
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

Ref<MaterialND> MeshND::get_material() const {
	return _material;
}

void MeshND::set_material(const Ref<MaterialND> &p_material) {
	_material = p_material;
}

PackedInt32Array MeshND::get_edge_indices() {
	PackedInt32Array indices;
	GDVIRTUAL_CALL(_get_edge_indices, indices);
	return indices;
}

Vector<VectorN> MeshND::get_edge_positions() {
	const PackedInt32Array edge_indices = get_edge_indices();
	const Vector<VectorN> vertex_positions = get_vertex_positions();
	Vector<VectorN> edges;
	for (const int32_t edge_index : edge_indices) {
		edges.append(vertex_positions[edge_index]);
	}
	return edges;
}

TypedArray<VectorN> MeshND::get_edge_positions_bind() {
	Vector<VectorN> edge_positions = get_edge_positions();
	TypedArray<VectorN> typed_array_edge_positions;
	typed_array_edge_positions.resize(edge_positions.size());
	for (int i = 0; i < edge_positions.size(); i++) {
		typed_array_edge_positions[i] = edge_positions[i];
	}
	return typed_array_edge_positions;
}

Vector<VectorN> MeshND::get_vertex_positions() {
	TypedArray<VectorN> typed_array_vertex_positions;
	GDVIRTUAL_CALL(_get_vertex_positions, typed_array_vertex_positions);
	Vector<VectorN> vertex_positions;
	vertex_positions.resize(typed_array_vertex_positions.size());
	for (int i = 0; i < typed_array_vertex_positions.size(); i++) {
		vertex_positions.set(i, typed_array_vertex_positions[i]);
	}
	return vertex_positions;
}

Vector<VectorN> MeshND::get_normal_values() {
	TypedArray<VectorN> typed_array_normals;
	GDVIRTUAL_CALL(_get_normal_values, typed_array_normals);
	Vector<VectorN> normals;
	normals.resize(typed_array_normals.size());
	for (int i = 0; i < typed_array_normals.size(); i++) {
		normals.set(i, typed_array_normals[i]);
	}
	return normals;
}

Vector<VectorM> MeshND::get_texture_map_values() {
	TypedArray<VectorM> typed_array_texture_maps;
	GDVIRTUAL_CALL(_get_texture_map_values, typed_array_texture_maps);
	Vector<VectorM> texture_maps;
	texture_maps.resize(typed_array_texture_maps.size());
	for (int i = 0; i < typed_array_texture_maps.size(); i++) {
		texture_maps.set(i, typed_array_texture_maps[i]);
	}
	return texture_maps;
}

TypedArray<VectorN> MeshND::get_vertex_positions_bind() {
	const Vector<VectorN> vertex_positions = get_vertex_positions();
	TypedArray<VectorN> typed_array_vertex_positions;
	typed_array_vertex_positions.resize(vertex_positions.size());
	for (int i = 0; i < vertex_positions.size(); i++) {
		typed_array_vertex_positions[i] = vertex_positions[i];
	}
	return typed_array_vertex_positions;
}

TypedArray<VectorN> MeshND::get_normal_values_bind() {
	const Vector<VectorN> normals = get_normal_values();
	TypedArray<VectorN> typed_array_normals;
	typed_array_normals.resize(normals.size());
	for (int i = 0; i < normals.size(); i++) {
		typed_array_normals[i] = normals[i];
	}
	return typed_array_normals;
}

TypedArray<VectorM> MeshND::get_texture_map_values_bind() {
	const Vector<VectorM> texture_maps = get_texture_map_values();
	TypedArray<VectorM> typed_array_texture_maps;
	typed_array_texture_maps.resize(texture_maps.size());
	for (int i = 0; i < texture_maps.size(); i++) {
		typed_array_texture_maps[i] = texture_maps[i];
	}
	return typed_array_texture_maps;
}

int MeshND::get_dimension() {
	const Vector<VectorN> vertex_positions = get_vertex_positions();
	if (vertex_positions.is_empty()) {
		return 0;
	}
	return vertex_positions[0].size();
}

void MeshND::_bind_methods() {
	ClassDB::bind_static_method("MeshND", D_METHOD("deduplicate_edge_indices", "items"), &MeshND::deduplicate_edge_indices);
	ClassDB::bind_method(D_METHOD("has_edge_indices", "first", "second"), &MeshND::has_edge_indices);

	ClassDB::bind_method(D_METHOD("is_mesh_data_valid"), &MeshND::is_mesh_data_valid);
	ClassDB::bind_method(D_METHOD("reset_mesh_data_validation"), &MeshND::reset_mesh_data_validation);
	ClassDB::bind_method(D_METHOD("validate_material_for_mesh", "material"), &MeshND::validate_material_for_mesh);
	ClassDB::bind_method(D_METHOD("mark_proxy_mesh_3d_dirty"), &MeshND::mark_proxy_mesh_3d_dirty);
	ClassDB::bind_method(D_METHOD("mark_mesh_bounds_and_proxy_mesh_3d_dirty"), &MeshND::mark_mesh_bounds_and_proxy_mesh_3d_dirty);
	ClassDB::bind_method(D_METHOD("update_proxy_mesh_3d"), &MeshND::update_proxy_mesh_3d);

	ClassDB::bind_method(D_METHOD("to_array_wire_mesh"), &MeshND::to_array_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_wire_mesh"), &MeshND::to_wire_mesh);
	ClassDB::bind_method(D_METHOD("get_rect_bounds"), &MeshND::get_rect_bounds);
	ClassDB::bind_method(D_METHOD("get_proxy_mesh_3d"), &MeshND::get_proxy_mesh_3d);

	ClassDB::bind_method(D_METHOD("get_material"), &MeshND::get_material);
	ClassDB::bind_method(D_METHOD("set_material", "material"), &MeshND::set_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "MaterialND"), "set_material", "get_material");

	ClassDB::bind_method(D_METHOD("get_edge_indices"), &MeshND::get_edge_indices);
	ClassDB::bind_method(D_METHOD("get_edge_positions"), &MeshND::get_edge_positions_bind);
	ClassDB::bind_method(D_METHOD("get_vertex_positions"), &MeshND::get_vertex_positions_bind);
	ClassDB::bind_method(D_METHOD("get_normal_values"), &MeshND::get_normal_values_bind);
	ClassDB::bind_method(D_METHOD("get_texture_map_values"), &MeshND::get_texture_map_values_bind);
	ClassDB::bind_method(D_METHOD("get_dimension"), &MeshND::get_dimension);

	GDVIRTUAL_BIND(_get_edge_indices);
	GDVIRTUAL_BIND(_get_vertex_positions);
	GDVIRTUAL_BIND(_get_normal_values);
	GDVIRTUAL_BIND(_get_texture_map_values);
	GDVIRTUAL_BIND(_validate_material_for_mesh, "material");
	GDVIRTUAL_BIND(_validate_mesh_data);
	GDVIRTUAL_BIND(_update_proxy_mesh_3d);
}
