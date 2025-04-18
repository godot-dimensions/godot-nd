#include "array_wire_mesh_nd.h"

#include "../../math/vector_nd.h"

bool ArrayWireMeshND::validate_mesh_data() {
	const int64_t edge_indices_count = _edge_indices.size();
	if (edge_indices_count % 2 != 0) {
		return false; // Must be a multiple of 2.
	}
	const int64_t vertex_count = _vertices.size();
	for (int32_t edge_index : _edge_indices) {
		if (edge_index < 0 || edge_index >= vertex_count) {
			return false; // Edges must reference valid vertices.
		}
	}
	return true;
}

void ArrayWireMeshND::append_edge_points(const VectorN &p_point_a, const VectorN &p_point_b, const bool p_deduplicate_vertices) {
	int index_a = append_vertex(p_point_a, p_deduplicate_vertices);
	int index_b = append_vertex(p_point_b, p_deduplicate_vertices);
	append_edge_indices(index_a, index_b);
	reset_mesh_data_validation();
}

void ArrayWireMeshND::append_edge_indices(int p_index_a, int p_index_b) {
	if (p_index_a > p_index_b) {
		SWAP(p_index_a, p_index_b);
	}
	_edge_indices.append(p_index_a);
	_edge_indices.append(p_index_b);
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

int ArrayWireMeshND::append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int vertex_count = _vertices.size();
	if (p_deduplicate_vertices) {
		for (int i = 0; i < vertex_count; i++) {
			if (VectorND::is_equal_approx(_vertices[i], p_vertex)) {
				return i;
			}
		}
	}
	_vertices.push_back(p_vertex);
	reset_mesh_data_validation();
	return vertex_count;
}

PackedInt32Array ArrayWireMeshND::append_vertices(const Vector<VectorN> &p_vertices, const bool p_deduplicate_vertices) {
	PackedInt32Array indices;
	for (int i = 0; i < p_vertices.size(); i++) {
		indices.append(append_vertex(p_vertices[i], p_deduplicate_vertices));
	}
	reset_mesh_data_validation();
	return indices;
}

PackedInt32Array ArrayWireMeshND::append_vertices_bind(const TypedArray<VectorN> &p_vertices, const bool p_deduplicate_vertices) {
	PackedInt32Array indices;
	for (int i = 0; i < p_vertices.size(); i++) {
		indices.append(append_vertex(p_vertices[i], p_deduplicate_vertices));
	}
	reset_mesh_data_validation();
	return indices;
}

void ArrayWireMeshND::merge_with(const Ref<ArrayWireMeshND> &p_other, const Ref<TransformND> &p_transform) {
	const int start_edge_count = _edge_indices.size();
	const int start_vertex_count = _vertices.size();
	const int other_edge_count = p_other->_edge_indices.size();
	const int other_vertex_count = p_other->_vertices.size();
	const int end_edge_count = start_edge_count + other_edge_count;
	const int end_vertex_count = start_vertex_count + other_vertex_count;
	_edge_indices.resize(end_edge_count);
	_vertices.resize(end_vertex_count);
	for (int i = 0; i < other_edge_count; i++) {
		_edge_indices.set(start_edge_count + i, p_other->_edge_indices[i] + start_vertex_count);
	}
	for (int i = 0; i < other_vertex_count; i++) {
		_vertices.set(start_vertex_count + i, p_transform->xform(p_other->_vertices[i]));
	}
	Ref<MaterialND> self_material = get_material();
	if (self_material.is_null()) {
		set_material(p_other->get_material());
	}
	reset_mesh_data_validation();
}

PackedInt32Array ArrayWireMeshND::get_edge_indices() {
	return _edge_indices;
}

void ArrayWireMeshND::set_edge_indices(const PackedInt32Array &p_edge_indices) {
	_edge_indices = p_edge_indices;
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

Vector<VectorN> ArrayWireMeshND::get_vertices() {
	return _vertices;
}

void ArrayWireMeshND::set_vertices(const Vector<VectorN> &p_vertices) {
	_vertices = p_vertices;
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

void ArrayWireMeshND::set_vertices_bind(const TypedArray<VectorN> &p_vertices) {
	_vertices.clear();
	_vertices.resize(p_vertices.size());
	for (int i = 0; i < p_vertices.size(); i++) {
		_vertices.set(i, p_vertices[i]);
	}
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

void ArrayWireMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_edge_points", "point_a", "point_b", "deduplicate"), &ArrayWireMeshND::append_edge_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_edge_indices", "index_a", "index_b"), &ArrayWireMeshND::append_edge_indices);
	ClassDB::bind_method(D_METHOD("append_vertex", "vertex", "deduplicate"), &ArrayWireMeshND::append_vertex, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_vertices", "vertices", "deduplicate"), &ArrayWireMeshND::append_vertices_bind, DEFVAL(true));

	// Only bind the setters here because the getters are already bound in WireMeshND.
	ClassDB::bind_method(D_METHOD("set_edge_indices", "edge_indices"), &ArrayWireMeshND::set_edge_indices);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "edge_indices"), "set_edge_indices", "get_edge_indices");

	ClassDB::bind_method(D_METHOD("set_vertices", "vertices"), &ArrayWireMeshND::set_vertices_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertices", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_vertices", "get_vertices");
}
