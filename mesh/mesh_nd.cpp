#include "mesh_nd.h"

#include "wire/array_wire_mesh_nd.h"

#if GDEXTENSION
#include <godot_cpp/templates/hash_set.hpp>
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

Ref<ArrayWireMeshND> MeshND::to_array_wire_mesh() {
	Ref<ArrayWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_vertices(get_vertices());
	wire_mesh->set_edge_indices(get_edge_indices());
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<WireMeshND> MeshND::to_wire_mesh() {
	return to_array_wire_mesh();
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
	const Vector<VectorN> vertices = get_vertices();
	Vector<VectorN> edges;
	for (const int32_t edge_index : edge_indices) {
		edges.append(vertices[edge_index]);
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

Vector<VectorN> MeshND::get_vertices() {
	TypedArray<VectorN> typed_array_vertices;
	GDVIRTUAL_CALL(_get_vertices, typed_array_vertices);
	Vector<VectorN> vertices;
	vertices.resize(typed_array_vertices.size());
	for (int i = 0; i < typed_array_vertices.size(); i++) {
		vertices.set(i, typed_array_vertices[i]);
	}
	return vertices;
}

TypedArray<VectorN> MeshND::get_vertices_bind() {
	Vector<VectorN> vertices = get_vertices();
	TypedArray<VectorN> typed_array_vertices;
	typed_array_vertices.resize(vertices.size());
	for (int i = 0; i < vertices.size(); i++) {
		typed_array_vertices[i] = vertices[i];
	}
	return typed_array_vertices;
}

void MeshND::_bind_methods() {
	ClassDB::bind_static_method("MeshND", D_METHOD("deduplicate_edge_indices", "items"), &MeshND::deduplicate_edge_indices);
	ClassDB::bind_method(D_METHOD("has_edge_indices", "first", "second"), &MeshND::has_edge_indices);

	ClassDB::bind_method(D_METHOD("is_mesh_data_valid"), &MeshND::is_mesh_data_valid);
	ClassDB::bind_method(D_METHOD("reset_mesh_data_validation"), &MeshND::reset_mesh_data_validation);

	ClassDB::bind_method(D_METHOD("to_array_wire_mesh"), &MeshND::to_array_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_wire_mesh"), &MeshND::to_wire_mesh);

	ClassDB::bind_method(D_METHOD("get_material"), &MeshND::get_material);
	ClassDB::bind_method(D_METHOD("set_material", "material"), &MeshND::set_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "MaterialND"), "set_material", "get_material");

	ClassDB::bind_method(D_METHOD("get_edge_indices"), &MeshND::get_edge_indices);
	ClassDB::bind_method(D_METHOD("get_edge_positions"), &MeshND::get_edge_positions_bind);
	ClassDB::bind_method(D_METHOD("get_vertices"), &MeshND::get_vertices_bind);

	GDVIRTUAL_BIND(_get_edge_indices);
	GDVIRTUAL_BIND(_get_vertices);
	GDVIRTUAL_BIND(_validate_mesh_data);
}
