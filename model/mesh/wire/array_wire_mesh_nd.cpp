#include "array_wire_mesh_nd.h"

#include "../../../math/vector_nd.h"

#if GDEXTENSION
#include <godot_cpp/templates/hash_map.hpp>
#endif

bool ArrayWireMeshND::validate_mesh_data() {
	const int64_t edge_indices_count = _edge_vertex_indices.size();
	if (edge_indices_count % 2 != 0) {
		return false; // Must be a multiple of 2.
	}
	const int64_t vertex_count = _vertex_positions.size();
	const int dimension = get_dimension();
	for (const VectorN &position : _vertex_positions) {
		if (position.size() > dimension) {
			return false; // The first vertex defines the mesh dimension.
		}
	}
	for (int32_t edge_index : _edge_vertex_indices) {
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
	_edge_vertex_indices.append(p_index_a);
	_edge_vertex_indices.append(p_index_b);
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

int ArrayWireMeshND::append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int vertex_count = _vertex_positions.size();
	if (p_deduplicate_vertices) {
		for (int i = 0; i < vertex_count; i++) {
			if (VectorND::is_equal_approx(_vertex_positions[i], p_vertex)) {
				return i;
			}
		}
	}
	ERR_FAIL_COND_V(_vertex_positions.size() > MAX_VERTICES, 2147483647);
	_vertex_positions.push_back(p_vertex);
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

void ArrayWireMeshND::deduplicate_all_elements() {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayWireMeshND: Cannot deduplicate elements of an invalid mesh.");
	// Deduplicate vertices.
	Vector<VectorN> output_vertices;
	HashMap<int32_t, int32_t> vertex_index_remap;
	for (int64_t input_vertex_index = 0; input_vertex_index < _vertex_positions.size(); input_vertex_index++) {
		const VectorN vertex = _vertex_positions[input_vertex_index];
		bool found_duplicate = false;
		for (int64_t output_vertex_index = 0; output_vertex_index < output_vertices.size(); output_vertex_index++) {
			if (VectorND::is_equal_approx(vertex, output_vertices[output_vertex_index])) {
				vertex_index_remap[input_vertex_index] = (int32_t)output_vertex_index;
				found_duplicate = true;
				break;
			}
		}
		if (!found_duplicate) {
			vertex_index_remap[input_vertex_index] = (int32_t)output_vertices.size();
			output_vertices.push_back(vertex);
		}
	}
	// Update edges that reference those vertices.
	for (int64_t edge_index = 0; edge_index < _edge_vertex_indices.size(); edge_index++) {
		const int64_t input_vertex_index = _edge_vertex_indices[edge_index];
		_edge_vertex_indices.set(edge_index, vertex_index_remap[input_vertex_index]);
	}
	// Deduplicate edges.
	PackedInt32Array output_edge_vertex_indices;
	for (int64_t input_edge_index = 0; input_edge_index < _edge_vertex_indices.size(); input_edge_index += 2) {
		int32_t vertex_index_a = _edge_vertex_indices[input_edge_index];
		int32_t vertex_index_b = _edge_vertex_indices[input_edge_index + 1];
		bool found_duplicate = false;
		for (int64_t output_edge_index = 0; output_edge_index < output_edge_vertex_indices.size(); output_edge_index += 2) {
			const int32_t output_vertex_index_a = output_edge_vertex_indices[output_edge_index];
			const int32_t output_vertex_index_b = output_edge_vertex_indices[output_edge_index + 1];
			// Deduplicate edges in the same order and in the opposite order.
			// Both orders should be considered the same edge in the PolyMeshND code.
			if ((vertex_index_a == output_vertex_index_a && vertex_index_b == output_vertex_index_b) ||
					(vertex_index_a == output_vertex_index_b && vertex_index_b == output_vertex_index_a)) {
				found_duplicate = true;
				break;
			}
		}
		if (!found_duplicate) {
			// Ensure the smaller index is first.
			if (vertex_index_a > vertex_index_b) {
				SWAP(vertex_index_a, vertex_index_b);
			}
			output_edge_vertex_indices.append(vertex_index_a);
			output_edge_vertex_indices.append(vertex_index_b);
		}
	}
	_vertex_positions = output_vertices;
	_edge_vertex_indices = output_edge_vertex_indices;
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

void ArrayWireMeshND::transform_vertices(const Ref<TransformND> &p_transform) {
	ERR_FAIL_COND(p_transform.is_null());
	const int64_t vertex_count = _vertex_positions.size();
	for (int64_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
		_vertex_positions.set(vertex_index, p_transform->xform(_vertex_positions[vertex_index]));
	}
	wire_mesh_clear_cache();
}

void ArrayWireMeshND::merge_with(const Ref<ArrayWireMeshND> &p_other, const Ref<TransformND> &p_transform) {
	const int start_edge_count = _edge_vertex_indices.size();
	const int start_vertex_count = _vertex_positions.size();
	const int other_edge_count = p_other->_edge_vertex_indices.size();
	const int other_vertex_count = p_other->_vertex_positions.size();
	const int end_edge_count = start_edge_count + other_edge_count;
	const int end_vertex_count = start_vertex_count + other_vertex_count;
	_edge_vertex_indices.resize(end_edge_count);
	_vertex_positions.resize(end_vertex_count);
	for (int i = 0; i < other_edge_count; i++) {
		_edge_vertex_indices.set(start_edge_count + i, p_other->_edge_vertex_indices[i] + start_vertex_count);
	}
	for (int i = 0; i < other_vertex_count; i++) {
		_vertex_positions.set(start_vertex_count + i, p_transform->xform(p_other->_vertex_positions[i]));
	}
	Ref<MaterialND> self_material = get_material();
	if (self_material.is_null()) {
		set_material(p_other->get_material());
	}
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

PackedInt32Array ArrayWireMeshND::get_edge_indices() {
	return _edge_vertex_indices;
}

void ArrayWireMeshND::set_edge_indices(const PackedInt32Array &p_edge_indices) {
	_edge_vertex_indices = p_edge_indices;
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

Vector<VectorN> ArrayWireMeshND::get_vertex_positions() {
	return _vertex_positions;
}

void ArrayWireMeshND::set_vertex_positions(const Vector<VectorN> &p_vertex_positions) {
	ERR_FAIL_COND(p_vertex_positions.size() > MAX_VERTICES);
	_vertex_positions = p_vertex_positions;
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

void ArrayWireMeshND::set_vertex_positions_bind(const TypedArray<VectorN> &p_vertex_positions) {
	_vertex_positions.clear();
	_vertex_positions.resize(p_vertex_positions.size());
	for (int i = 0; i < p_vertex_positions.size(); i++) {
		_vertex_positions.set(i, p_vertex_positions[i]);
	}
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

void ArrayWireMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "ArrayWireMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 1000, "ArrayWireMeshND: Too many dimensions for wireframe mesh.");
	for (int i = 0; i < _vertex_positions.size(); i++) {
		if (i == 0 || _vertex_positions[i].size() > p_dimension) {
			_vertex_positions.set(i, VectorND::with_dimension(_vertex_positions[i], p_dimension));
		}
	}
	wire_mesh_clear_cache();
	reset_mesh_data_validation();
}

void ArrayWireMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_edge_points", "point_a", "point_b", "deduplicate"), &ArrayWireMeshND::append_edge_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_edge_indices", "index_a", "index_b"), &ArrayWireMeshND::append_edge_indices);
	ClassDB::bind_method(D_METHOD("append_vertex", "vertex", "deduplicate"), &ArrayWireMeshND::append_vertex, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_vertices", "vertices", "deduplicate"), &ArrayWireMeshND::append_vertices_bind, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("deduplicate_all_elements"), &ArrayWireMeshND::deduplicate_all_elements);
	ClassDB::bind_method(D_METHOD("transform_vertices", "transform"), &ArrayWireMeshND::transform_vertices);
	ClassDB::bind_method(D_METHOD("merge_with", "other", "transform"), &ArrayWireMeshND::merge_with);

	// Only bind the setters here because the getters are already bound in WireMeshND.
	ClassDB::bind_method(D_METHOD("set_edge_indices", "edge_indices"), &ArrayWireMeshND::set_edge_indices);
	ClassDB::bind_method(D_METHOD("set_vertex_positions", "vertex_positions"), &ArrayWireMeshND::set_vertex_positions_bind);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &ArrayWireMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "edge_indices"), "set_edge_indices", "get_edge_indices");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertex_positions", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_vertex_positions", "get_vertex_positions");
#ifndef DISABLE_DEPRECATED
	// Compatibility property to handle reading existing serialized data.
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_INTERNAL), "set_vertex_positions", "get_vertex_positions");
#endif // DISABLE_DEPRECATED
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,1000,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");
}
