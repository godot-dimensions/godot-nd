#include "array_cell_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "cell_material_nd.h"

void ArrayCellMeshND::_clear_cache_and_validation() {
	cell_mesh_clear_cache();
	reset_mesh_data_validation();
}

bool ArrayCellMeshND::validate_mesh_data() {
	const int64_t cell_vertex_indices_count = _simplex_cell_vertex_indices.size();
	const int dimension = get_dimension();
	const int64_t cell_boundary_normals_count = _simplex_cell_boundary_normals.size();
	ERR_FAIL_COND_V_MSG(dimension == 0 && (cell_vertex_indices_count > 0 || cell_boundary_normals_count > 0), false, "ArrayCellMeshND: Simplex cells and boundary normals require a positive mesh dimension.");
	ERR_FAIL_COND_V_MSG(dimension > 0 && cell_vertex_indices_count % dimension != 0, false, "ArrayCellMeshND: Simplex cell vertex indices size must be a multiple of the dimension.");
	ERR_FAIL_COND_V_MSG(cell_boundary_normals_count > 0 && cell_boundary_normals_count * dimension != cell_vertex_indices_count, false, "ArrayCellMeshND: Simplex cell boundary normals size must be one dimension-th of simplex cell vertex indices size (or empty).");
	for (const VectorN &boundary_normal : _simplex_cell_boundary_normals) {
		ERR_FAIL_COND_V_MSG(boundary_normal.size() > dimension, false, "ArrayCellMeshND: Boundary normals must not exceed the mesh dimension.");
	}
	const int64_t cell_normal_indices_count = _simplex_cell_normal_indices.size();
	ERR_FAIL_COND_V_MSG(cell_normal_indices_count > 0 && cell_normal_indices_count != cell_vertex_indices_count, false, "ArrayCellMeshND: Simplex cell normal indices size must be the same as simplex cell vertex indices size (or empty).");
	const int64_t cell_texture_map_indices_count = _simplex_cell_texture_map_indices.size();
	ERR_FAIL_COND_V_MSG(cell_texture_map_indices_count > 0 && cell_texture_map_indices_count != cell_vertex_indices_count, false, "ArrayCellMeshND: Simplex cell texture map indices size must be the same as simplex cell vertex indices size (or empty).");
	const int64_t vertex_pos_count = _vertex_positions.size();
	for (const VectorN &vertex_position : _vertex_positions) {
		ERR_FAIL_COND_V_MSG(vertex_position.size() > dimension, false, "ArrayCellMeshND: Vertex positions must not exceed the mesh dimension defined by the first vertex.");
	}
	for (int32_t cell_vertex_index : _simplex_cell_vertex_indices) {
		ERR_FAIL_COND_V_MSG(cell_vertex_index < 0 || cell_vertex_index >= vertex_pos_count, false, "ArrayCellMeshND: Simplex cell vertex indices must reference valid vertices.");
	}
	const int64_t normal_value_count = _normal_values.size();
	for (const VectorN &normal_value : _normal_values) {
		ERR_FAIL_COND_V_MSG(normal_value.size() > dimension, false, "ArrayCellMeshND: All normal values must have at most the dimension of the mesh.");
	}
	for (int32_t normal_index : _simplex_cell_normal_indices) {
		ERR_FAIL_COND_V_MSG(normal_index < 0 || normal_index >= normal_value_count, false, "ArrayCellMeshND: Simplex cell normal indices must reference valid normal values.");
	}
	const int64_t texture_map_value_count = _texture_map_values.size();
	const int texture_map_dimension = MAX(dimension - 1, 0);
	for (const VectorM &texture_map_value : _texture_map_values) {
		ERR_FAIL_COND_V_MSG(texture_map_value.size() > texture_map_dimension, false, "ArrayCellMeshND: All texture map values must have at most one fewer dimension than the mesh.");
	}
	for (int32_t texture_map_index : _simplex_cell_texture_map_indices) {
		ERR_FAIL_COND_V_MSG(texture_map_index < 0 || texture_map_index >= texture_map_value_count, false, "ArrayCellMeshND: Simplex cell texture map indices must reference valid texture map values.");
	}
	return true;
}

int32_t ArrayCellMeshND::append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int64_t vertex_pos_count = _vertex_positions.size();
	ERR_FAIL_COND_V_MSG(vertex_pos_count > MeshND::MAX_VERTICES, -1, "ArrayCellMeshND: Cannot add more vertices to the mesh. Maximum vertex count exceeded.");
	if (p_deduplicate_vertices) {
		for (int64_t i = 0; i < vertex_pos_count; i++) {
			if (VectorND::is_equal_exact(_vertex_positions[i], p_vertex)) {
				return i;
			}
		}
	}
	_vertex_positions.push_back(p_vertex);
	cell_mesh_clear_cache();
	reset_mesh_data_validation();
	return (int32_t)vertex_pos_count;
}

PackedInt32Array ArrayCellMeshND::append_vertices(const Vector<VectorN> &p_vertex_positions, const bool p_deduplicate_vertices) {
	PackedInt32Array indices;
	for (int64_t i = 0; i < p_vertex_positions.size(); i++) {
		indices.append(append_vertex(p_vertex_positions[i], p_deduplicate_vertices));
	}
	reset_mesh_data_validation();
	return indices;
}

// Explicit compaction functions. Editing operations always leave the mesh in a consistent
// valid state, but may leave unreferenced values in the pools, which wastes space when kept.
// Compaction is not run automatically because it is O(n^2) in the pool size, so it is faster
// to run a sequence of editing operations first and only compact once at the end, if desired.

void ArrayCellMeshND::compact_normal_values() {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayCellMeshND: Cannot compact normal values of an invalid mesh.");
	// Mark which values are referenced by the normal indices.
	const int64_t old_value_count = _normal_values.size();
	Vector<bool> referenced;
	referenced.resize(old_value_count);
	for (int64_t i = 0; i < old_value_count; i++) {
		referenced.set(i, false);
	}
	for (const int32_t value_index : _simplex_cell_normal_indices) {
		referenced.set(value_index, true);
	}
	// Build the compacted pool, dropping unreferenced values and deduplicating
	// identical values, while preserving the relative order of the kept values.
	Vector<VectorN> compacted_values;
	PackedInt32Array old_to_new;
	old_to_new.resize(old_value_count);
	for (int64_t i = 0; i < old_value_count; i++) {
		if (!referenced[i]) {
			old_to_new.set(i, -1);
			continue;
		}
		old_to_new.set(i, (int32_t)VectorND::array_append_deduplicate(compacted_values, _normal_values[i]));
	}
	// Remap the normal indices into the compacted pool.
	for (int64_t i = 0; i < _simplex_cell_normal_indices.size(); i++) {
		_simplex_cell_normal_indices.set(i, old_to_new[_simplex_cell_normal_indices[i]]);
	}
	_normal_values = compacted_values;
	mark_proxy_mesh_3d_dirty();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::compact_texture_map_values() {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayCellMeshND: Cannot compact texture map values of an invalid mesh.");
	// Mark which values are referenced by the texture map indices.
	const int64_t old_value_count = _texture_map_values.size();
	Vector<bool> referenced;
	referenced.resize(old_value_count);
	for (int64_t i = 0; i < old_value_count; i++) {
		referenced.set(i, false);
	}
	for (const int32_t value_index : _simplex_cell_texture_map_indices) {
		referenced.set(value_index, true);
	}
	// Build the compacted pool, dropping unreferenced values and deduplicating
	// identical values, while preserving the relative order of the kept values.
	Vector<VectorM> compacted_values;
	PackedInt32Array old_to_new;
	old_to_new.resize(old_value_count);
	for (int64_t i = 0; i < old_value_count; i++) {
		if (!referenced[i]) {
			old_to_new.set(i, -1);
			continue;
		}
		old_to_new.set(i, (int32_t)VectorND::array_append_deduplicate(compacted_values, _texture_map_values[i]));
	}
	// Remap the texture map indices into the compacted pool.
	for (int64_t i = 0; i < _simplex_cell_texture_map_indices.size(); i++) {
		_simplex_cell_texture_map_indices.set(i, old_to_new[_simplex_cell_texture_map_indices[i]]);
	}
	_texture_map_values = compacted_values;
	mark_proxy_mesh_3d_dirty();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::merge_with(const Ref<ArrayCellMeshND> &p_other, const Ref<TransformND> &p_transform) {
	ERR_FAIL_COND_MSG(p_other.is_null(), "ArrayCellMeshND: Cannot merge a null mesh.");
	ERR_FAIL_COND_MSG(p_transform.is_null(), "ArrayCellMeshND: Cannot merge with a null transform.");
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayCellMeshND: Cannot merge into an invalid mesh.");
	ERR_FAIL_COND_MSG(!p_other->is_mesh_data_valid(), "ArrayCellMeshND: Cannot merge an invalid mesh.");
	if (p_other.ptr() == this) {
		const Ref<ArrayCellMeshND> source = duplicate();
		merge_with(source, p_transform);
		return;
	}
	const int64_t start_cell_vertex_index_count = _simplex_cell_vertex_indices.size();
	const int64_t start_cell_normal_index_count = _simplex_cell_normal_indices.size();
	const int64_t start_cell_texture_map_index_count = _simplex_cell_texture_map_indices.size();
	const int64_t start_cell_boundary_normal_count = _simplex_cell_boundary_normals.size();
	const int64_t start_normal_value_count = _normal_values.size();
	const int64_t start_texture_map_value_count = _texture_map_values.size();
	const int64_t start_vertex_pos_count = _vertex_positions.size();
	const int64_t other_cell_vertex_index_count = p_other->_simplex_cell_vertex_indices.size();
	const int64_t other_cell_normal_index_count = p_other->_simplex_cell_normal_indices.size();
	const int64_t other_cell_texture_map_index_count = p_other->_simplex_cell_texture_map_indices.size();
	const int64_t other_cell_boundary_normal_count = p_other->_simplex_cell_boundary_normals.size();
	const int64_t other_normal_value_count = p_other->_normal_values.size();
	const int64_t other_texture_map_value_count = p_other->_texture_map_values.size();
	const int64_t other_vertex_pos_count = p_other->_vertex_positions.size();
	const int64_t end_cell_vertex_index_count = start_cell_vertex_index_count + other_cell_vertex_index_count;
	const int64_t end_vertex_pos_count = start_vertex_pos_count + other_vertex_pos_count;
	const int64_t start_dimension = get_dimension();
	const int64_t other_dimension = p_other->get_dimension();
	ERR_FAIL_COND_MSG(start_vertex_pos_count > 0 && other_vertex_pos_count > 0 && start_dimension != other_dimension, "ArrayCellMeshND: Cannot merge meshes with different dimensions.");
	const int64_t dimension = start_vertex_pos_count == 0 ? other_dimension : start_dimension;
	_simplex_cell_vertex_indices.resize(end_cell_vertex_index_count);
	_vertex_positions.resize(end_vertex_pos_count);
	// Copy in the cell indices and vertices from the other mesh.
	for (int64_t i = 0; i < other_cell_vertex_index_count; i++) {
		_simplex_cell_vertex_indices.set(start_cell_vertex_index_count + i, p_other->_simplex_cell_vertex_indices[i] + start_vertex_pos_count);
	}
	for (int64_t i = 0; i < other_vertex_pos_count; i++) {
		_vertex_positions.set(start_vertex_pos_count + i, p_transform->xform(p_other->_vertex_positions[i]));
	}
	// Merge the value pools. The other mesh's normal values need to be transformed.
	_normal_values.resize(start_normal_value_count + other_normal_value_count);
	for (int64_t i = 0; i < other_normal_value_count; i++) {
		_normal_values.set(start_normal_value_count + i, p_transform->xform_basis(p_other->_normal_values[i]));
	}
	_texture_map_values.resize(start_texture_map_value_count + other_texture_map_value_count);
	for (int64_t i = 0; i < other_texture_map_value_count; i++) {
		_texture_map_values.set(start_texture_map_value_count + i, p_other->_texture_map_values[i]);
	}
	// Can't simply add these together in case the first mesh has no normals or texture maps.
	if (start_cell_boundary_normal_count > 0 || other_cell_boundary_normal_count > 0) {
		const int64_t end_cell_boundary_normal_count = end_cell_vertex_index_count / dimension;
		_simplex_cell_boundary_normals.resize(end_cell_boundary_normal_count);
		const int64_t start_boundary_normal_count = start_cell_vertex_index_count / dimension;
		// Initialize the mesh's boundary normals to zero if it has none.
		if (start_cell_boundary_normal_count == 0) {
			for (int64_t i = 0; i < start_boundary_normal_count; i++) {
				_simplex_cell_boundary_normals.set(i, VectorN());
			}
		}
		if (other_cell_boundary_normal_count == 0) {
			for (int64_t i = 0; i < other_cell_vertex_index_count / dimension; i++) {
				_simplex_cell_boundary_normals.set(start_boundary_normal_count + i, VectorN());
			}
		}
		// Copy in the boundary normals from the other mesh.
		if (other_cell_boundary_normal_count > 0) {
			for (int64_t i = 0; i < other_cell_boundary_normal_count; i++) {
				_simplex_cell_boundary_normals.set(start_boundary_normal_count + i, p_transform->xform_basis(p_other->_simplex_cell_boundary_normals[i]));
			}
		}
	}
	if (start_cell_normal_index_count > 0 || other_cell_normal_index_count > 0) {
		_simplex_cell_normal_indices.resize(end_cell_vertex_index_count);
		const bool fill_start_normals = start_cell_normal_index_count < start_cell_vertex_index_count;
		const bool fill_other_normals = other_cell_normal_index_count == 0 && other_cell_vertex_index_count > 0;
		if (fill_start_normals || fill_other_normals) {
			// At least one of the meshes is missing normal indices, so point the missing entries at an empty normal value.
			const int32_t zero_normal_value_index = (int32_t)VectorND::array_append_deduplicate(_normal_values, VectorN());
			for (int64_t i = start_cell_normal_index_count; i < start_cell_vertex_index_count; i++) {
				_simplex_cell_normal_indices.set(i, zero_normal_value_index);
			}
			if (fill_other_normals) {
				for (int64_t i = start_cell_vertex_index_count; i < end_cell_vertex_index_count; i++) {
					_simplex_cell_normal_indices.set(i, zero_normal_value_index);
				}
			}
		}
		for (int64_t i = 0; i < other_cell_normal_index_count; i++) {
			_simplex_cell_normal_indices.set(start_cell_vertex_index_count + i, p_other->_simplex_cell_normal_indices[i] + int32_t(start_normal_value_count));
		}
	}
	if (start_cell_texture_map_index_count > 0 || other_cell_texture_map_index_count > 0) {
		_simplex_cell_texture_map_indices.resize(end_cell_vertex_index_count);
		const bool fill_start_texture_maps = start_cell_texture_map_index_count < start_cell_vertex_index_count;
		const bool fill_other_texture_maps = other_cell_texture_map_index_count == 0 && other_cell_vertex_index_count > 0;
		if (fill_start_texture_maps || fill_other_texture_maps) {
			// At least one of the meshes is missing texture map indices, so point the missing entries at an empty texture map value.
			const int32_t zero_texture_map_value_index = (int32_t)VectorND::array_append_deduplicate(_texture_map_values, VectorM());
			for (int64_t i = start_cell_texture_map_index_count; i < start_cell_vertex_index_count; i++) {
				_simplex_cell_texture_map_indices.set(i, zero_texture_map_value_index);
			}
			if (fill_other_texture_maps) {
				for (int64_t i = start_cell_vertex_index_count; i < end_cell_vertex_index_count; i++) {
					_simplex_cell_texture_map_indices.set(i, zero_texture_map_value_index);
				}
			}
		}
		for (int64_t i = 0; i < other_cell_texture_map_index_count; i++) {
			_simplex_cell_texture_map_indices.set(start_cell_vertex_index_count + i, p_other->_simplex_cell_texture_map_indices[i] + int32_t(start_texture_map_value_count));
		}
	}
	_clear_cache_and_validation();
}

PackedInt32Array ArrayCellMeshND::get_simplex_cell_vertex_indices() {
	return _simplex_cell_vertex_indices;
}

void ArrayCellMeshND::set_simplex_cell_vertex_indices(const PackedInt32Array &p_simplex_cell_vertex_indices) {
	_simplex_cell_vertex_indices = p_simplex_cell_vertex_indices;
	_clear_cache_and_validation();
}

PackedInt32Array ArrayCellMeshND::get_simplex_cell_normal_indices() {
	return _simplex_cell_normal_indices;
}

void ArrayCellMeshND::set_simplex_cell_normal_indices(const PackedInt32Array &p_simplex_cell_normal_indices) {
	_simplex_cell_normal_indices = p_simplex_cell_normal_indices;
	mark_proxy_mesh_3d_dirty();
	reset_mesh_data_validation();
}

PackedInt32Array ArrayCellMeshND::get_simplex_cell_texture_map_indices() {
	return _simplex_cell_texture_map_indices;
}

void ArrayCellMeshND::set_simplex_cell_texture_map_indices(const PackedInt32Array &p_simplex_cell_texture_map_indices) {
	_simplex_cell_texture_map_indices = p_simplex_cell_texture_map_indices;
	mark_proxy_mesh_3d_dirty();
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

Vector<VectorN> ArrayCellMeshND::get_normal_values() {
	return _normal_values;
}

void ArrayCellMeshND::set_normal_values(const Vector<VectorN> &p_normal_values) {
	_normal_values = p_normal_values;
	mark_proxy_mesh_3d_dirty();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_normal_values_bind(const TypedArray<VectorN> &p_normal_values) {
	_normal_values.clear();
	_normal_values.resize(p_normal_values.size());
	for (int i = 0; i < p_normal_values.size(); i++) {
		_normal_values.set(i, p_normal_values[i]);
	}
	mark_proxy_mesh_3d_dirty();
	reset_mesh_data_validation();
}

Vector<VectorM> ArrayCellMeshND::get_texture_map_values() {
	return _texture_map_values;
}

void ArrayCellMeshND::set_texture_map_values(const Vector<VectorM> &p_texture_map_values) {
	_texture_map_values = p_texture_map_values;
	mark_proxy_mesh_3d_dirty();
	reset_mesh_data_validation();
}

void ArrayCellMeshND::set_texture_map_values_bind(const TypedArray<VectorM> &p_texture_map_values) {
	_texture_map_values.clear();
	_texture_map_values.resize(p_texture_map_values.size());
	for (int i = 0; i < p_texture_map_values.size(); i++) {
		_texture_map_values.set(i, p_texture_map_values[i]);
	}
	mark_proxy_mesh_3d_dirty();
	reset_mesh_data_validation();
}

Vector<VectorN> ArrayCellMeshND::get_vertex_positions() {
	return _vertex_positions;
}

void ArrayCellMeshND::set_vertex_positions(const Vector<VectorN> &p_vertex_positions) {
	_vertex_positions = p_vertex_positions;
	_clear_cache_and_validation();
}

void ArrayCellMeshND::set_vertex_positions_bind(const TypedArray<VectorN> &p_vertex_positions) {
	_vertex_positions.clear();
	_vertex_positions.resize(p_vertex_positions.size());
	for (int i = 0; i < p_vertex_positions.size(); i++) {
		_vertex_positions.set(i, p_vertex_positions[i]);
	}
	_clear_cache_and_validation();
}

void ArrayCellMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "ArrayCellMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 1000, "ArrayCellMeshND: Too many dimensions for cell mesh.");
	const int old_dimension = get_dimension();
	// Resize all data to the new dimension, even if the dimension is the same, to ensure consistency of the data.
	for (int i = 0; i < _vertex_positions.size(); i++) {
		// For vertex positions only, always resize the first vertex to set the mesh dimension.
		if (i == 0 || _vertex_positions[i].size() > p_dimension) {
			_vertex_positions.set(i, VectorND::with_dimension(_vertex_positions[i], p_dimension));
		}
	}
	for (int i = 0; i < _normal_values.size(); i++) {
		// Empty normal values mean missing data, which should stay empty rather than becoming zero-filled.
		if (_normal_values[i].size() > p_dimension) {
			_normal_values.set(i, VectorND::with_dimension(_normal_values[i], p_dimension));
		}
	}
	const int texture_map_dimension = MAX(p_dimension - 1, 0);
	for (int i = 0; i < _texture_map_values.size(); i++) {
		// Empty texture map values mean missing data, which should stay empty rather than becoming zero-filled.
		if (_texture_map_values[i].size() > texture_map_dimension) {
			_texture_map_values.set(i, VectorND::with_dimension(_texture_map_values[i], texture_map_dimension));
		}
	}
	if (p_dimension != old_dimension) {
		// Simplex cells are invalidated by a change in dimension, since the number of vertices per cell is equal to the dimension.
		// Callers that wish to resolve this will need to come up with their own resolution strategy.
		_simplex_cell_vertex_indices.clear();
		_simplex_cell_normal_indices.clear();
		_simplex_cell_texture_map_indices.clear();
		_simplex_cell_boundary_normals.clear();
	} else {
		for (int i = 0; i < _simplex_cell_boundary_normals.size(); i++) {
			if (_simplex_cell_boundary_normals[i].size() > p_dimension) {
				_simplex_cell_boundary_normals.set(i, VectorND::with_dimension(_simplex_cell_boundary_normals[i], p_dimension));
			}
		}
	}
	_clear_cache_and_validation();
}

void ArrayCellMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_vertex", "vertex", "deduplicate_vertices"), &ArrayCellMeshND::append_vertex, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("compact_normal_values"), &ArrayCellMeshND::compact_normal_values);
	ClassDB::bind_method(D_METHOD("compact_texture_map_values"), &ArrayCellMeshND::compact_texture_map_values);

	ClassDB::bind_method(D_METHOD("merge_with", "other", "transform"), &ArrayCellMeshND::merge_with);

	// Only bind the setters here because the getters are already bound in CellMeshND.
	ClassDB::bind_method(D_METHOD("set_simplex_cell_vertex_indices", "simplex_cell_vertex_indices"), &ArrayCellMeshND::set_simplex_cell_vertex_indices);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_normal_indices", "simplex_cell_normal_indices"), &ArrayCellMeshND::set_simplex_cell_normal_indices);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_texture_map_indices", "simplex_cell_texture_map_indices"), &ArrayCellMeshND::set_simplex_cell_texture_map_indices);
	ClassDB::bind_method(D_METHOD("set_simplex_cell_boundary_normals", "simplex_cell_boundary_normals"), &ArrayCellMeshND::set_simplex_cell_boundary_normals_bind);
	ClassDB::bind_method(D_METHOD("set_normal_values", "normal_values"), &ArrayCellMeshND::set_normal_values_bind);
	ClassDB::bind_method(D_METHOD("set_texture_map_values", "texture_map_values"), &ArrayCellMeshND::set_texture_map_values_bind);
	ClassDB::bind_method(D_METHOD("set_vertex_positions", "vertex_positions"), &ArrayCellMeshND::set_vertex_positions_bind);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &ArrayCellMeshND::set_dimension);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "simplex_cell_vertex_indices"), "set_simplex_cell_vertex_indices", "get_simplex_cell_vertex_indices");
#ifndef DISABLE_DEPRECATED
	// Compatibility property to handle reading existing serialized data.
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "simplex_cell_indices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_INTERNAL), "set_simplex_cell_vertex_indices", "get_simplex_cell_vertex_indices");
#endif // DISABLE_DEPRECATED
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "simplex_cell_normal_indices"), "set_simplex_cell_normal_indices", "get_simplex_cell_normal_indices");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "simplex_cell_texture_map_indices"), "set_simplex_cell_texture_map_indices", "get_simplex_cell_texture_map_indices");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "simplex_cell_boundary_normals", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_simplex_cell_boundary_normals", "get_simplex_cell_boundary_normals");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "normal_values", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_normal_values", "get_normal_values");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "texture_map_values", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_texture_map_values", "get_texture_map_values");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertex_positions", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_vertex_positions", "get_vertex_positions");
#ifndef DISABLE_DEPRECATED
	// Compatibility property to handle reading existing serialized data.
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "vertices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_INTERNAL), "set_vertex_positions", "get_vertex_positions");
#endif // DISABLE_DEPRECATED
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,1000,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");
}
