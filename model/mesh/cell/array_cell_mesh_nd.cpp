#include "array_cell_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "cell_material_nd.h"

void ArrayCellMeshND::_clear_cache() {
	cell_mesh_clear_cache();
}

bool ArrayCellMeshND::validate_mesh_data() {
	const int64_t cell_indices_count = _simplex_cell_indices.size();
	const int dimension = get_dimension();
	if (cell_indices_count % dimension != 0) {
		return false; // Must be a multiple of _dimension.
	}
	const int64_t cell_boundary_normals_count = _simplex_cell_boundary_normals.size();
	if (cell_boundary_normals_count > 0 && cell_boundary_normals_count * dimension != cell_indices_count) {
		return false; // Must have one normal per cell (dimension indices).
	}
	const int64_t cell_vertex_normals_count = _simplex_cell_vertex_normals.size();
	if (cell_vertex_normals_count > 0 && cell_vertex_normals_count != cell_indices_count) {
		return false; // Must have one normal per cell vertex instance (1 per index).
	}
	const int64_t vertex_count = _vertices.size();
	for (int32_t cell_index : _simplex_cell_indices) {
		if (cell_index < 0 || cell_index >= vertex_count) {
			return false; // Cells must reference valid vertices.
		}
	}
	return true;
}

int ArrayCellMeshND::append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int64_t vertex_count = _vertices.size();
	if (p_deduplicate_vertices) {
		for (int64_t i = 0; i < vertex_count; i++) {
			if (_vertices[i] == p_vertex) {
				return i;
			}
		}
	}
	_vertices.push_back(p_vertex);
	reset_mesh_data_validation();
	return vertex_count;
}

PackedInt32Array ArrayCellMeshND::append_vertices(const Vector<VectorN> &p_vertices, const bool p_deduplicate_vertices) {
	PackedInt32Array indices;
	for (int64_t i = 0; i < p_vertices.size(); i++) {
		indices.append(append_vertex(p_vertices[i], p_deduplicate_vertices));
	}
	reset_mesh_data_validation();
	return indices;
}

void ArrayCellMeshND::merge_with(const Ref<ArrayCellMeshND> &p_other, const Ref<TransformND> &p_transform) {
	const int64_t start_cell_index_count = _simplex_cell_indices.size();
	const int64_t start_cell_face_normal_count = _simplex_cell_boundary_normals.size();
	const int64_t start_cell_vertex_normal_count = _simplex_cell_vertex_normals.size();
	const int64_t start_vertex_count = _vertices.size();
	const int64_t other_cell_index_count = p_other->_simplex_cell_indices.size();
	const int64_t other_cell_face_normal_count = p_other->_simplex_cell_boundary_normals.size();
	const int64_t other_cell_vertex_normal_count = p_other->_simplex_cell_vertex_normals.size();
	const int64_t other_vertex_count = p_other->_vertices.size();
	const int64_t end_cell_index_count = start_cell_index_count + other_cell_index_count;
	const int64_t end_vertex_count = start_vertex_count + other_vertex_count;
	const int64_t dimension = get_dimension();
	_simplex_cell_indices.resize(end_cell_index_count);
	_vertices.resize(end_vertex_count);
	// Copy in the cell indices and vertices from the other mesh.
	for (int64_t i = 0; i < other_cell_index_count; i++) {
		_simplex_cell_indices.set(start_cell_index_count + i, p_other->_simplex_cell_indices[i] + start_vertex_count);
	}
	for (int64_t i = 0; i < other_vertex_count; i++) {
		_vertices.set(start_vertex_count + i, p_transform->xform(p_other->_vertices[i]));
	}
	// Can't simply add these together in case the first mesh has no normals.
	if (start_cell_face_normal_count > 0 || other_cell_face_normal_count > 0) {
		const int64_t end_cell_normal_count = end_cell_index_count / dimension;
		_simplex_cell_boundary_normals.resize(end_cell_normal_count);
		const int64_t start_normal_count = start_cell_index_count / dimension;
		// Initialize the mesh's face normals to zero if it has none.
		if (start_cell_face_normal_count == 0) {
			for (int64_t i = 0; i < start_normal_count; i++) {
				_simplex_cell_boundary_normals.set(i, VectorN());
			}
		}
		if (other_cell_face_normal_count == 0) {
			for (int64_t i = 0; i < other_cell_index_count / dimension; i++) {
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
		const int64_t end_cell_vertex_normal_count = end_cell_index_count;
		_simplex_cell_vertex_normals.resize(end_cell_vertex_normal_count);
		const int64_t start_vertex_normal_count = start_cell_index_count;
		// Initialize the mesh's vertex normals to zero if it has none.
		if (start_cell_vertex_normal_count == 0) {
			for (int64_t i = 0; i < start_vertex_normal_count; i++) {
				_simplex_cell_vertex_normals.set(i, VectorN());
			}
		}
		if (other_cell_vertex_normal_count == 0) {
			for (int64_t i = 0; i < other_cell_index_count; i++) {
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
	reset_mesh_data_validation();
}

PackedInt32Array ArrayCellMeshND::get_simplex_cell_indices() {
	return _simplex_cell_indices;
}

void ArrayCellMeshND::set_simplex_cell_indices(const PackedInt32Array &p_simplex_cell_indices) {
	_simplex_cell_indices = p_simplex_cell_indices;
	_clear_cache();
	reset_mesh_data_validation();
}

Vector<VectorN> ArrayCellMeshND::get_simplex_cell_boundary_normals() {
	return _simplex_cell_boundary_normals;
}

void ArrayCellMeshND::set_cell_boundary_normals(const Vector<VectorN> &p_simplex_cell_normals) {
	_simplex_cell_boundary_normals = p_simplex_cell_normals;
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

Vector<VectorN> ArrayCellMeshND::get_vertices() {
	return _vertices;
}

void ArrayCellMeshND::set_vertices(const Vector<VectorN> &p_vertices) {
	_vertices = p_vertices;
	_clear_cache();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_vertices_bind(const TypedArray<VectorN> &p_vertices) {
	_vertices.clear();
	_vertices.resize(p_vertices.size());
	for (int i = 0; i < p_vertices.size(); i++) {
		_vertices.set(i, p_vertices[i]);
	}
	_clear_cache();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "ArrayCellMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 1000, "ArrayCellMeshND: Too many dimensions for cell mesh.");
	for (int i = 0; i < _vertices.size(); i++) {
		_vertices.set(i, VectorND::with_dimension(_vertices[i], p_dimension));
	}
	_clear_cache();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_vertex", "vertex", "deduplicate_vertices"), &ArrayCellMeshND::append_vertex, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("merge_with", "other", "transform"), &ArrayCellMeshND::merge_with);

	// Only bind the setters here because the getters are already bound in CellMeshND.
	ClassDB::bind_method(D_METHOD("set_simplex_cell_indices", "simplex_cell_indices"), &ArrayCellMeshND::set_simplex_cell_indices);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_boundary_normals", "simplex_cell_normals"), &ArrayCellMeshND::set_simplex_cell_boundary_normals_bind);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_vertex_normals", "simplex_cell_vertex_normals"), &ArrayCellMeshND::set_simplex_cell_vertex_normals_bind);
	ClassDB::bind_method(D_METHOD("set_vertices", "vertices"), &ArrayCellMeshND::set_vertices_bind);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &ArrayCellMeshND::set_dimension);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "simplex_cell_indices"), "set_simplex_cell_indices", "get_simplex_cell_indices");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "simplex_cell_boundary_normals", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_simplex_cell_boundary_normals", "get_simplex_cell_boundary_normals");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "simplex_cell_vertex_normals", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_simplex_cell_vertex_normals", "get_simplex_cell_vertex_normals");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertices", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_vertices", "get_vertices");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,1000,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");
}
