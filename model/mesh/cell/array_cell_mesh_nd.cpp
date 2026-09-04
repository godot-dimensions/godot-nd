#include "array_cell_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "cell_material_nd.h"

void ArrayCellMeshND::_clear_cache() {
	cell_mesh_clear_cache();
}

bool ArrayCellMeshND::validate_mesh_data() {
	const int64_t cell_vertex_indices_count = _simplex_cell_vertex_indices.size();
	const int dimension = get_dimension();
	ERR_FAIL_COND_V_MSG(cell_vertex_indices_count % dimension != 0, false, "ArrayCellMeshND: Simplex cell vertex indices size must be a multiple of the dimension.");
	const int64_t cell_boundary_normals_count = _simplex_cell_boundary_normals.size();
	ERR_FAIL_COND_V_MSG(cell_boundary_normals_count > 0 && cell_boundary_normals_count * dimension != cell_vertex_indices_count, false, "ArrayCellMeshND: Simplex cell boundary normals size must be one dimension-th of simplex cell vertex indices size (or empty).");
	const int64_t cell_vertex_normals_count = _simplex_cell_vertex_normals.size();
	ERR_FAIL_COND_V_MSG(cell_vertex_normals_count > 0 && cell_vertex_normals_count != cell_vertex_indices_count, false, "ArrayCellMeshND: Simplex cell vertex normals size must be the same as simplex cell vertex indices size (or empty).");
	const int64_t vertex_count = _vertex_positions.size();
	for (int32_t cell_vertex_index : _simplex_cell_vertex_indices) {
		ERR_FAIL_COND_V_MSG(cell_vertex_index < 0 || cell_vertex_index >= vertex_count, false, "ArrayCellMeshND: Simplex cell vertex indices must reference valid vertices.");
	}
	return true;
}

int ArrayCellMeshND::append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int64_t vertex_count = _vertex_positions.size();
	if (p_deduplicate_vertices) {
		for (int64_t i = 0; i < vertex_count; i++) {
			if (VectorND::is_equal_exact(_vertex_positions[i], p_vertex)) {
				return i;
			}
		}
	}
	_vertex_positions.push_back(p_vertex);
	reset_mesh_data_validation();
	return vertex_count;
}

PackedInt32Array ArrayCellMeshND::append_vertices(const Vector<VectorN> &p_vertex_positions, const bool p_deduplicate_vertices) {
	PackedInt32Array indices;
	for (int64_t i = 0; i < p_vertex_positions.size(); i++) {
		indices.append(append_vertex(p_vertex_positions[i], p_deduplicate_vertices));
	}
	reset_mesh_data_validation();
	return indices;
}

void ArrayCellMeshND::merge_with(const Ref<ArrayCellMeshND> &p_other, const Ref<TransformND> &p_transform) {
	const int64_t start_cell_vertex_index_count = _simplex_cell_vertex_indices.size();
	const int64_t start_cell_face_normal_count = _simplex_cell_boundary_normals.size();
	const int64_t start_cell_vertex_normal_count = _simplex_cell_vertex_normals.size();
	const int64_t start_vertex_count = _vertex_positions.size();
	const int64_t other_cell_vertex_index_count = p_other->_simplex_cell_vertex_indices.size();
	const int64_t other_cell_face_normal_count = p_other->_simplex_cell_boundary_normals.size();
	const int64_t other_cell_vertex_normal_count = p_other->_simplex_cell_vertex_normals.size();
	const int64_t other_vertex_count = p_other->_vertex_positions.size();
	const int64_t end_cell_vertex_index_count = start_cell_vertex_index_count + other_cell_vertex_index_count;
	const int64_t end_vertex_count = start_vertex_count + other_vertex_count;
	const int64_t dimension = get_dimension();
	_simplex_cell_vertex_indices.resize(end_cell_vertex_index_count);
	_vertex_positions.resize(end_vertex_count);
	// Copy in the cell indices and vertices from the other mesh.
	for (int64_t i = 0; i < other_cell_vertex_index_count; i++) {
		_simplex_cell_vertex_indices.set(start_cell_vertex_index_count + i, p_other->_simplex_cell_vertex_indices[i] + start_vertex_count);
	}
	for (int64_t i = 0; i < other_vertex_count; i++) {
		_vertex_positions.set(start_vertex_count + i, p_transform->xform(p_other->_vertex_positions[i]));
	}
	// Can't simply add these together in case the first mesh has no normals.
	if (start_cell_face_normal_count > 0 || other_cell_face_normal_count > 0) {
		const int64_t end_cell_normal_count = end_cell_vertex_index_count / dimension;
		_simplex_cell_boundary_normals.resize(end_cell_normal_count);
		const int64_t start_normal_count = start_cell_vertex_index_count / dimension;
		// Initialize the mesh's face normals to zero if it has none.
		if (start_cell_face_normal_count == 0) {
			for (int64_t i = 0; i < start_normal_count; i++) {
				_simplex_cell_boundary_normals.set(i, VectorN());
			}
		}
		if (other_cell_face_normal_count == 0) {
			for (int64_t i = 0; i < other_cell_vertex_index_count / dimension; i++) {
				_simplex_cell_boundary_normals.set(start_normal_count + i, VectorN());
			}
		}
		// Copy in the face normals from the other mesh.
		if (other_cell_face_normal_count > 0) {
			for (int64_t i = 0; i < other_cell_face_normal_count; i++) {
				_simplex_cell_boundary_normals.set(start_normal_count + i, p_transform->xform_basis(p_other->_simplex_cell_boundary_normals[i]));
			}
		}
	}
	if (start_cell_vertex_normal_count > 0 || other_cell_vertex_normal_count > 0) {
		const int64_t end_cell_vertex_normal_count = end_cell_vertex_index_count;
		_simplex_cell_vertex_normals.resize(end_cell_vertex_normal_count);
		const int64_t start_vertex_normal_count = start_cell_vertex_index_count;
		// Initialize the mesh's vertex normals to zero if it has none.
		if (start_cell_vertex_normal_count == 0) {
			for (int64_t i = 0; i < start_vertex_normal_count; i++) {
				_simplex_cell_vertex_normals.set(i, VectorN());
			}
		}
		if (other_cell_vertex_normal_count == 0) {
			for (int64_t i = 0; i < other_cell_vertex_index_count; i++) {
				_simplex_cell_vertex_normals.set(start_vertex_normal_count + i, VectorN());
			}
		}
		// Copy in the vertex normals from the other mesh.
		if (other_cell_vertex_normal_count > 0) {
			for (int64_t i = 0; i < other_cell_vertex_normal_count; i++) {
				_simplex_cell_vertex_normals.set(start_vertex_normal_count + i, p_transform->xform_basis(p_other->_simplex_cell_vertex_normals[i]));
			}
		}
	}
	_clear_cache();
	reset_mesh_data_validation();
}

PackedInt32Array ArrayCellMeshND::get_simplex_cell_vertex_indices() {
	return _simplex_cell_vertex_indices;
}

void ArrayCellMeshND::set_simplex_cell_vertex_indices(const PackedInt32Array &p_simplex_cell_vertex_indices) {
	_simplex_cell_vertex_indices = p_simplex_cell_vertex_indices;
	_clear_cache();
	reset_mesh_data_validation();
}

Vector<VectorN> ArrayCellMeshND::get_simplex_cell_boundary_normals() {
	return _simplex_cell_boundary_normals;
}

void ArrayCellMeshND::set_simplex_cell_boundary_normals(const Vector<VectorN> &p_simplex_cell_boundary_normals) {
	_simplex_cell_boundary_normals = p_simplex_cell_boundary_normals;
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_simplex_cell_boundary_normals_bind(const TypedArray<VectorN> &p_simplex_cell_boundary_normals) {
	_simplex_cell_boundary_normals.clear();
	_simplex_cell_boundary_normals.resize(p_simplex_cell_boundary_normals.size());
	for (int i = 0; i < p_simplex_cell_boundary_normals.size(); i++) {
		_simplex_cell_boundary_normals.set(i, p_simplex_cell_boundary_normals[i]);
	}
	reset_mesh_data_validation();
}

Vector<VectorN> ArrayCellMeshND::get_simplex_cell_vertex_normals() {
	return _simplex_cell_vertex_normals;
}

void ArrayCellMeshND::set_simplex_cell_vertex_normals(const Vector<VectorN> &p_simplex_cell_vertex_normals) {
	_simplex_cell_vertex_normals = p_simplex_cell_vertex_normals;
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_simplex_cell_vertex_normals_bind(const TypedArray<VectorN> &p_simplex_cell_vertex_normals) {
	_simplex_cell_vertex_normals.clear();
	_simplex_cell_vertex_normals.resize(p_simplex_cell_vertex_normals.size());
	for (int i = 0; i < p_simplex_cell_vertex_normals.size(); i++) {
		_simplex_cell_vertex_normals.set(i, p_simplex_cell_vertex_normals[i]);
	}
	reset_mesh_data_validation();
}

Vector<VectorM> ArrayCellMeshND::get_simplex_cell_texture_map() {
	return _simplex_cell_texture_map;
}

void ArrayCellMeshND::set_simplex_cell_texture_map(const Vector<VectorM> &p_simplex_cell_texture_map) {
	_simplex_cell_texture_map = p_simplex_cell_texture_map;
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_simplex_cell_texture_map_bind(const TypedArray<VectorM> &p_simplex_cell_texture_map) {
	_simplex_cell_texture_map.clear();
	_simplex_cell_texture_map.resize(p_simplex_cell_texture_map.size());
	for (int i = 0; i < p_simplex_cell_texture_map.size(); i++) {
		_simplex_cell_texture_map.set(i, p_simplex_cell_texture_map[i]);
	}
	reset_mesh_data_validation();
}

Vector<VectorN> ArrayCellMeshND::get_vertex_positions() {
	return _vertex_positions;
}

void ArrayCellMeshND::set_vertex_positions(const Vector<VectorN> &p_vertex_positions) {
	_vertex_positions = p_vertex_positions;
	_clear_cache();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_vertex_positions_bind(const TypedArray<VectorN> &p_vertex_positions) {
	_vertex_positions.clear();
	_vertex_positions.resize(p_vertex_positions.size());
	for (int i = 0; i < p_vertex_positions.size(); i++) {
		_vertex_positions.set(i, p_vertex_positions[i]);
	}
	_clear_cache();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "ArrayCellMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 1000, "ArrayCellMeshND: Too many dimensions for cell mesh.");
	for (int i = 0; i < _vertex_positions.size(); i++) {
		_vertex_positions.set(i, VectorND::with_dimension(_vertex_positions[i], p_dimension));
	}
	_clear_cache();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_vertex", "vertex", "deduplicate_vertices"), &ArrayCellMeshND::append_vertex, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("merge_with", "other", "transform"), &ArrayCellMeshND::merge_with);

	// Only bind the setters here because the getters are already bound in CellMeshND.
	ClassDB::bind_method(D_METHOD("set_simplex_cell_vertex_indices", "simplex_cell_vertex_indices"), &ArrayCellMeshND::set_simplex_cell_vertex_indices);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_boundary_normals", "simplex_cell_boundary_normals"), &ArrayCellMeshND::set_simplex_cell_boundary_normals_bind);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_vertex_normals", "simplex_cell_vertex_normals"), &ArrayCellMeshND::set_simplex_cell_vertex_normals_bind);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_texture_map", "simplex_cell_texture_map"), &ArrayCellMeshND::set_simplex_cell_texture_map_bind);
	ClassDB::bind_method(D_METHOD("set_vertex_positions", "vertex_positions"), &ArrayCellMeshND::set_vertex_positions_bind);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &ArrayCellMeshND::set_dimension);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "simplex_cell_vertex_indices"), "set_simplex_cell_vertex_indices", "get_simplex_cell_vertex_indices");
#ifndef DISABLE_DEPRECATED
	// Compatibility property to handle reading existing serialized data.
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "simplex_cell_indices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_INTERNAL), "set_simplex_cell_vertex_indices", "get_simplex_cell_vertex_indices");
#endif // DISABLE_DEPRECATED
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "simplex_cell_boundary_normals", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_simplex_cell_boundary_normals", "get_simplex_cell_boundary_normals");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "simplex_cell_vertex_normals", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_simplex_cell_vertex_normals", "get_simplex_cell_vertex_normals");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "simplex_cell_texture_map", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_simplex_cell_texture_map", "get_simplex_cell_texture_map");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertex_positions", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_vertex_positions", "get_vertex_positions");
#ifndef DISABLE_DEPRECATED
	// Compatibility property to handle reading existing serialized data.
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_INTERNAL), "set_vertex_positions", "get_vertex_positions");
#endif // DISABLE_DEPRECATED
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,1000,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");
}
