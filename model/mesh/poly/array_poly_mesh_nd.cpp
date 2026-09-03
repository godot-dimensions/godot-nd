#include "array_poly_mesh_nd.h"

#include "../../../math/math_nd.h"
#include "../../../math/vector_nd.h"

// Append and delete functions.

int64_t ArrayPolyMeshND::append_edge_points(const VectorN &p_point_a, const VectorN &p_point_b, const bool p_deduplicate) {
	const int32_t index_a = append_vertex(p_point_a, p_deduplicate);
	const int32_t index_b = append_vertex(p_point_b, p_deduplicate);
	return append_edge_indices(index_a, index_b, p_deduplicate);
}

int64_t ArrayPolyMeshND::append_edge_indices(int32_t p_index_a, int32_t p_index_b, const bool p_deduplicate) {
	const int64_t old_edge_count = _edge_vertex_indices.size() / 2;
	if (p_index_a > p_index_b) {
		SWAP(p_index_a, p_index_b);
	}
	if (p_deduplicate) {
		for (int64_t i = 0; i < old_edge_count; i++) {
			if (_edge_vertex_indices[i * 2] == p_index_a && _edge_vertex_indices[i * 2 + 1] == p_index_b) {
				return i;
			}
		}
	}
	// Append a new edge. We don't need to clear the poly mesh cache, but CellMeshND does cache edges.
	_edge_vertex_indices.append(p_index_a);
	_edge_vertex_indices.append(p_index_b);
	cell_mesh_clear_cache();
	reset_poly_mesh_data_validation();
	return old_edge_count;
}

int64_t ArrayPolyMeshND::append_poly_cell(const int32_t p_dimension, const PackedInt32Array &p_cell, const bool p_deduplicate) {
	ERR_FAIL_COND_V_MSG(_poly_cell_vertex_positions.is_empty() || _edge_vertex_indices.is_empty(), -1, "This ArrayPolyMeshND lacks any 0D vertices or 1D edges, so cannot append a poly cell.");
	ERR_FAIL_COND_V_MSG(p_dimension < 2, -1, "ArrayPolyMeshND: Cannot append " + itos(p_dimension) + "D poly cell. For 0D vertices and 1D edges, use the special functions for those.");
	const int64_t old_mesh_dim = _poly_cell_indices.size() + 1;
	ERR_FAIL_COND_V_MSG(p_dimension > old_mesh_dim + 1, -1, "ArrayPolyMeshND: Cannot append a " + itos(p_dimension) + "D poly cell because the mesh currently only has cells up to " + itos(old_mesh_dim) + "D. Cells must be appended in order of dimension, so append the missing " + itos(old_mesh_dim + 1) + "D cell(s) first.");
	// Check if the elements of the previous dimension referenced by this new cell actually exist.
	const int64_t prev_dim_cell_count = p_dimension == 2 ? _edge_vertex_indices.size() / 2 : _poly_cell_indices[p_dimension - 3].size();
	for (const int32_t cell_element : p_cell) {
		ERR_FAIL_INDEX_V_MSG(cell_element, prev_dim_cell_count, -1, "ArrayPolyMeshND: Cannot append poly cell because it references non-existent elements of the previous dimension.");
	}
	// Append the new cell to the poly cell indices, deduplicating if desired.
	if (p_dimension == old_mesh_dim + 1) {
		Vector<PackedInt32Array> new_dim;
		new_dim.append(p_cell);
		_poly_cell_indices.append(new_dim);
		poly_mesh_clear_cache();
		return 0;
	}
	const int64_t poly_cell_dim_index = p_dimension - 2;
	Vector<PackedInt32Array> existing_dim = _poly_cell_indices[poly_cell_dim_index];
	const int64_t existing_cell_count = existing_dim.size();
	if (p_deduplicate) {
		// Check for cells made of the same elements, regardless of order.
		PackedInt32Array sorted_new_cell = p_cell;
		sorted_new_cell.sort();
		for (int64_t existing_cell_index = 0; existing_cell_index < existing_cell_count; existing_cell_index++) {
			PackedInt32Array existing_cell_copy = PackedInt32Array(existing_dim[existing_cell_index]);
			if (existing_cell_copy.size() != sorted_new_cell.size()) {
				continue;
			}
			existing_cell_copy.sort();
			if (existing_cell_copy == sorted_new_cell) {
				// This existing cell is made of the same elements as the new cell, so consider them duplicates.
				return existing_cell_index;
			}
		}
	}
	existing_dim.append(p_cell);
	_poly_cell_indices.set(poly_cell_dim_index, existing_dim);
	poly_mesh_clear_cache();
	return existing_cell_count;
}

int32_t ArrayPolyMeshND::append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int64_t vertex_pos_count = _poly_cell_vertex_positions.size();
	ERR_FAIL_COND_V_MSG(vertex_pos_count > MAX_POLY_VERTICES, -1, "ArrayPolyMeshND: Cannot add more vertices to the mesh. Maximum vertex count exceeded.");
	if (p_deduplicate_vertices) {
		for (int64_t i = 0; i < vertex_pos_count; i++) {
			if (_poly_cell_vertex_positions[i] == p_vertex) {
				return i;
			}
		}
	}
	_poly_cell_vertex_positions.push_back(p_vertex);
	poly_mesh_clear_cache();
	return (int32_t)vertex_pos_count;
}

PackedInt32Array ArrayPolyMeshND::append_vertices(const TypedArray<VectorN> &p_vertices, const bool p_deduplicate_vertices) {
	PackedInt32Array indices;
	for (int i = 0; i < p_vertices.size(); i++) {
		const VectorN vertex = p_vertices[i];
		indices.append(append_vertex(vertex, p_deduplicate_vertices));
	}
	reset_poly_mesh_data_validation();
	return indices;
}

bool ArrayPolyMeshND::_validate_data_binding_shape_internal(const Vector2i p_key, const Vector<PackedInt32Array> &p_binding, const String &p_binding_name) const {
	ERR_FAIL_COND_V_MSG(p_key.x < 0 || p_key.y < 0 || p_key.y > p_key.x, false, "ArrayPolyMeshND: " + p_binding_name + " binding key " + String(p_key) + " is invalid. The decomposition dimension must be between 0 and the geometry dimension.");
	if (p_binding.is_empty()) {
		return true; // An empty binding means no data for this key, which is treated the same as the key being absent.
	}
	int64_t element_count = 0;
	if (p_key.x == 0) {
		element_count = _poly_cell_vertex_positions.size();
	} else if (p_key.x == 1) {
		element_count = _edge_vertex_indices.size() / 2;
	} else if (p_key.x - 2 < _poly_cell_indices.size()) {
		element_count = _poly_cell_indices[p_key.x - 2].size();
	}
	if (p_key.y == p_key.x) {
		// Non-decomposed case. Only one array, with one value index per element.
		// Fewer entries than elements means the remaining elements have no data.
		ERR_FAIL_COND_V_MSG(p_binding.size() != 1, false, "ArrayPolyMeshND: " + p_binding_name + " binding key " + String(p_key) + " is not decomposed, so it must contain exactly one array.");
		ERR_FAIL_COND_V_MSG(p_binding[0].size() > element_count, false, "ArrayPolyMeshND: " + p_binding_name + " binding key " + String(p_key) + " has " + itos(p_binding[0].size()) + " entries but the geometry dimension only has " + itos(element_count) + " elements.");
	} else {
		// Decomposed case. One array per element of the geometry dimension.
		// Fewer arrays than elements means the remaining elements have no data.
		ERR_FAIL_COND_V_MSG(p_binding.size() > element_count, false, "ArrayPolyMeshND: " + p_binding_name + " binding key " + String(p_key) + " has " + itos(p_binding.size()) + " arrays but the geometry dimension only has " + itos(element_count) + " elements.");
		for (int64_t element_index = 0; element_index < p_binding.size(); element_index++) {
			if (p_binding[element_index].is_empty()) {
				continue; // This element has no data for the binding.
			}
			if (p_key.y == p_key.x - 1) {
				const int64_t member_count = p_key.x == 1 ? 2 : _poly_cell_indices[p_key.x - 2][element_index].size();
				ERR_FAIL_COND_V_MSG(p_binding[element_index].size() != member_count, false, "ArrayPolyMeshND: A direct-member data binding must contain one value index per member, or be empty.");
				continue;
			}
			// Geometry is already validated. Collect sub-elements here because the public traversal helper re-enters validation.
			HashSet<int32_t> sub_elements;
			sub_elements.insert((int32_t)element_index);
			for (int geometry_dimension = p_key.x; geometry_dimension > p_key.y; geometry_dimension--) {
				HashSet<int32_t> next_sub_elements;
				for (const int32_t index : sub_elements) {
					if (geometry_dimension == 1) {
						next_sub_elements.insert(_edge_vertex_indices[index * 2]);
						next_sub_elements.insert(_edge_vertex_indices[index * 2 + 1]);
					} else {
						for (const int32_t member : _poly_cell_indices[geometry_dimension - 2][index]) {
							next_sub_elements.insert(member);
						}
					}
				}
				sub_elements = next_sub_elements;
			}
			ERR_FAIL_COND_V_MSG(p_binding[element_index].size() != sub_elements.size(), false, "ArrayPolyMeshND: " + p_binding_name + " binding key " + String(p_key) + " must have one value index per sub-element, or an empty array for missing data.");
		}
	}
	return true;
}

bool ArrayPolyMeshND::_validate_poly_mesh_data_only() {
	// Validate every binding before the base class samples boundary normals.
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &binding : _all_poly_cell_normal_indices) {
		for (const PackedInt32Array &indices : binding.value) {
			for (const int32_t index : indices) {
				ERR_FAIL_COND_V_MSG(index < 0 || index >= _poly_cell_normal_values.size(), false, "ArrayPolyMeshND: Normal binding references invalid normal value " + itos(index) + ".");
			}
		}
	}
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &binding : _all_poly_cell_texture_map_indices) {
		for (const PackedInt32Array &indices : binding.value) {
			for (const int32_t index : indices) {
				ERR_FAIL_COND_V_MSG(index < 0 || index >= _poly_cell_texture_map_values.size(), false, "ArrayPolyMeshND: Texture map binding references invalid texture map value " + itos(index) + ".");
			}
		}
	}
	if (!PolyMeshND::_validate_poly_mesh_data_only()) {
		return false;
	}
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &binding : _all_poly_cell_normal_indices) {
		if (!_validate_data_binding_shape_internal(binding.key, binding.value, "Normal")) {
			return false;
		}
	}
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &binding : _all_poly_cell_texture_map_indices) {
		if (!_validate_data_binding_shape_internal(binding.key, binding.value, "Texture map")) {
			return false;
		}
	}
	return true;
}

// Internal helpers for the normal and texture map value pools.

PackedInt32Array ArrayPolyMeshND::_normal_indices_for_values_internal(const Vector<VectorN> &p_values) {
	PackedInt32Array indices;
	indices.resize(p_values.size());
	for (int64_t i = 0; i < p_values.size(); i++) {
		indices.set(i, (int32_t)VectorND::array_append_deduplicate(_poly_cell_normal_values, p_values[i]));
	}
	return indices;
}

Vector<VectorN> ArrayPolyMeshND::_sample_normal_values_internal(const PackedInt32Array &p_indices) const {
	Vector<VectorN> values;
	values.resize(p_indices.size());
	const int64_t value_count = _poly_cell_normal_values.size();
	for (int64_t i = 0; i < p_indices.size(); i++) {
		const int32_t value_index = p_indices[i];
		ERR_CONTINUE(value_index < 0 || value_index >= value_count);
		values.set(i, _poly_cell_normal_values[value_index]);
	}
	return values;
}

Vector<Vector<VectorM>> ArrayPolyMeshND::_get_poly_cell_texture_map_dense_internal() const {
	Vector<Vector<VectorM>> dense;
	const Vector2i cell_to_vert_key = const_cast<ArrayPolyMeshND *>(this)->_get_cell_to_vert_key();
	if (!_all_poly_cell_texture_map_indices.has(cell_to_vert_key)) {
		return dense;
	}
	const Vector<PackedInt32Array> &poly_cell_texture_map_indices = _all_poly_cell_texture_map_indices[cell_to_vert_key];
	const int64_t value_count = _poly_cell_texture_map_values.size();
	dense.resize(poly_cell_texture_map_indices.size());
	for (int64_t cell_index = 0; cell_index < poly_cell_texture_map_indices.size(); cell_index++) {
		const PackedInt32Array &cell_indices = poly_cell_texture_map_indices[cell_index];
		Vector<VectorM> cell_values;
		cell_values.resize(cell_indices.size());
		for (int64_t i = 0; i < cell_indices.size(); i++) {
			const int32_t value_index = cell_indices[i];
			ERR_CONTINUE(value_index < 0 || value_index >= value_count);
			cell_values.set(i, _poly_cell_texture_map_values[value_index]);
		}
		dense.set(cell_index, cell_values);
	}
	return dense;
}

void ArrayPolyMeshND::_set_poly_cell_texture_map_dense_internal(const Vector<Vector<VectorM>> &p_poly_cell_texture_map) {
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	Vector<PackedInt32Array> poly_cell_texture_map_indices;
	poly_cell_texture_map_indices.resize(p_poly_cell_texture_map.size());
	for (int64_t cell_index = 0; cell_index < p_poly_cell_texture_map.size(); cell_index++) {
		const Vector<VectorM> &cell_values = p_poly_cell_texture_map[cell_index];
		PackedInt32Array cell_indices;
		cell_indices.resize(cell_values.size());
		for (int64_t i = 0; i < cell_values.size(); i++) {
			cell_indices.set(i, (int32_t)VectorND::array_append_deduplicate(_poly_cell_texture_map_values, cell_values[i]));
		}
		poly_cell_texture_map_indices.set(cell_index, cell_indices);
	}
	if (poly_cell_texture_map_indices.is_empty()) {
		_all_poly_cell_texture_map_indices.erase(cell_to_vert_key);
	} else {
		_all_poly_cell_texture_map_indices.insert(cell_to_vert_key, poly_cell_texture_map_indices);
	}
}

// Note: Compaction is O(n^2) in the pool size because deduplication is a linear scan per value.
// Therefore, compaction is a separate explicit step instead of running after every editing
// operation, so that a sequence of editing operations only needs to pay this cost once at the end.
void ArrayPolyMeshND::_compact_normal_values_internal() {
	// Mark which values are referenced by any data binding.
	const int64_t old_value_count = _poly_cell_normal_values.size();
	Vector<bool> referenced;
	referenced.resize(old_value_count);
	for (int64_t i = 0; i < old_value_count; i++) {
		referenced.set(i, false);
	}
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : _all_poly_cell_normal_indices) {
		for (const PackedInt32Array &indices : kv.value) {
			for (const int32_t value_index : indices) {
				ERR_CONTINUE(value_index < 0 || value_index >= old_value_count);
				referenced.set(value_index, true);
			}
		}
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
		old_to_new.set(i, (int32_t)VectorND::array_append_deduplicate(compacted_values, _poly_cell_normal_values[i]));
	}
	// Remap the indices of all data bindings into the compacted pool.
	for (KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : _all_poly_cell_normal_indices) {
		Vector<PackedInt32Array> &index_arrays = kv.value;
		for (int64_t array_index = 0; array_index < index_arrays.size(); array_index++) {
			PackedInt32Array indices = index_arrays[array_index];
			for (int64_t i = 0; i < indices.size(); i++) {
				const int32_t old_index = indices[i];
				ERR_CONTINUE(old_index < 0 || old_index >= old_value_count);
				indices.set(i, old_to_new[old_index]);
			}
			index_arrays.set(array_index, indices);
		}
	}
	_poly_cell_normal_values = compacted_values;
}

void ArrayPolyMeshND::_compact_texture_map_values_internal() {
	// Mark which values are referenced by any data binding.
	const int64_t old_value_count = _poly_cell_texture_map_values.size();
	Vector<bool> referenced;
	referenced.resize(old_value_count);
	for (int64_t i = 0; i < old_value_count; i++) {
		referenced.set(i, false);
	}
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : _all_poly_cell_texture_map_indices) {
		for (const PackedInt32Array &indices : kv.value) {
			for (const int32_t value_index : indices) {
				ERR_CONTINUE(value_index < 0 || value_index >= old_value_count);
				referenced.set(value_index, true);
			}
		}
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
		old_to_new.set(i, (int32_t)VectorND::array_append_deduplicate(compacted_values, _poly_cell_texture_map_values[i]));
	}
	// Remap the indices of all data bindings into the compacted pool.
	for (KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : _all_poly_cell_texture_map_indices) {
		Vector<PackedInt32Array> &index_arrays = kv.value;
		for (int64_t array_index = 0; array_index < index_arrays.size(); array_index++) {
			PackedInt32Array indices = index_arrays[array_index];
			for (int64_t i = 0; i < indices.size(); i++) {
				const int32_t old_index = indices[i];
				ERR_CONTINUE(old_index < 0 || old_index >= old_value_count);
				indices.set(i, old_to_new[old_index]);
			}
			index_arrays.set(array_index, indices);
		}
	}
	_poly_cell_texture_map_values = compacted_values;
}

// Explicit compaction functions. Editing operations always leave the mesh in a consistent
// valid state, but may leave unreferenced values in the pools, which wastes space when kept.
// Compaction is not run automatically because it is O(n^2) in the pool size, so it is faster
// to run a sequence of editing operations first and only compact once at the end, if desired.

void ArrayPolyMeshND::compact_normal_values() {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot compact normal values of an invalid mesh.");
	_compact_normal_values_internal();
	poly_mesh_clear_cache(true);
	reset_poly_mesh_data_validation();
}

void ArrayPolyMeshND::compact_texture_map_values() {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot compact texture map values of an invalid mesh.");
	_compact_texture_map_values_internal();
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

void ArrayPolyMeshND::_delete_data_binding_element_internal(const int32_t p_dimension, const int32_t p_index) {
	HashMap<Vector2i, Vector<PackedInt32Array>> *data_binding_maps[2] = { &_all_poly_cell_normal_indices, &_all_poly_cell_texture_map_indices };
	for (HashMap<Vector2i, Vector<PackedInt32Array>> *data_binding_map : data_binding_maps) {
		for (KeyValue<Vector2i, Vector<PackedInt32Array>> &binding : *data_binding_map) {
			if (binding.key.x != p_dimension) {
				continue;
			}
			Vector<PackedInt32Array> &indices = binding.value;
			if (binding.key.y == p_dimension) {
				if (!indices.is_empty() && p_index < indices[0].size()) {
					indices.ptrw()[0].remove_at(p_index);
				}
			} else if (p_index < indices.size()) {
				indices.remove_at(p_index);
			}
		}
	}
}

void ArrayPolyMeshND::_delete_edge_internal(const int32_t p_index) {
	const int32_t edge_count = _edge_vertex_indices.size() / 2;
	ERR_FAIL_COND_MSG(p_index < 0 || p_index >= edge_count, "ArrayPolyMeshND: Edge index is out of range.");
	// Before deleting this edge, we need to delete any poly cells in higher dimensions that reference it.
	if (!_poly_cell_indices.is_empty()) {
		Vector<int32_t> faces_to_delete;
		const Vector<PackedInt32Array> &face_edge_indices = _poly_cell_indices[0];
		for (int32_t face_index = 0; face_index < face_edge_indices.size(); face_index++) {
			if (face_edge_indices[face_index].has(p_index)) {
				faces_to_delete.push_back(face_index);
			}
		}
		for (int32_t i = faces_to_delete.size() - 1; i >= 0; i--) {
			_delete_poly_cell_element_internal(0, faces_to_delete[i]);
		}
	}
	// For 3D meshes, the boundary cells are faces and the seams between them are edges.
	if (_get_boundary_poly_dim_index() == 0 && !_seam_indices.is_empty()) {
		HashSet<int32_t> adjusted_seam_indices;
		for (const int32_t seam_edge_index : _seam_indices) {
			if (seam_edge_index == p_index) {
				continue;
			}
			adjusted_seam_indices.insert(seam_edge_index > p_index ? seam_edge_index - 1 : seam_edge_index);
		}
		_seam_indices = adjusted_seam_indices;
	}
	// Delete the edge's two vertex index entries from the flat edge array.
	_delete_data_binding_element_internal(1, p_index);
	const int32_t edge_vertex_start = p_index * 2;
	_edge_vertex_indices.remove_at(edge_vertex_start + 1);
	_edge_vertex_indices.remove_at(edge_vertex_start);
	// Shift remaining face edge references down to preserve index semantics.
	if (!_poly_cell_indices.is_empty()) {
		Vector<PackedInt32Array> face_edge_indices = _poly_cell_indices[0];
		for (int32_t face_index = 0; face_index < face_edge_indices.size(); face_index++) {
			PackedInt32Array face = face_edge_indices[face_index];
			bool changed = false;
			for (int32_t edge_index_in_face = 0; edge_index_in_face < face.size(); edge_index_in_face++) {
				if (face[edge_index_in_face] > p_index) {
					face.set(edge_index_in_face, face[edge_index_in_face] - 1);
					changed = true;
				}
			}
			if (changed) {
				face_edge_indices.set(face_index, face);
			}
		}
		_poly_cell_indices.set(0, face_edge_indices);
	}
}

void ArrayPolyMeshND::_delete_vertex_internal(const int32_t p_index) {
	const int64_t vertex_pos_count = _poly_cell_vertex_positions.size();
	ERR_FAIL_COND_MSG(p_index < 0 || p_index >= vertex_pos_count, "ArrayPolyMeshND: Vertex index is out of range.");
	const int dimension = get_dimension();
	// Before deleting this vertex, we need to delete any edges that reference it,
	// and any poly cells in higher dimensions that reference those edges.
	const int32_t edge_count = _edge_vertex_indices.size() / 2;
	Vector<int32_t> edges_to_delete;
	for (int32_t edge_index = 0; edge_index < edge_count; edge_index++) {
		if (_edge_vertex_indices[edge_index * 2] == p_index || _edge_vertex_indices[edge_index * 2 + 1] == p_index) {
			edges_to_delete.push_back(edge_index);
		}
	}
	for (int32_t i = edges_to_delete.size() - 1; i >= 0; i--) {
		_delete_edge_internal(edges_to_delete[i]);
	}
	// Delete the vertex itself now that all dependent edges (and higher dimensions) are gone.
	_delete_data_binding_element_internal(0, p_index);
	_poly_cell_vertex_positions.remove_at(p_index);
	if (p_index == 0 && !_poly_cell_vertex_positions.is_empty() && _poly_cell_vertex_positions[0].size() < dimension) {
		// The first position anchors the dimension; other positions may remain compact.
		_poly_cell_vertex_positions.set(0, VectorND::with_dimension(_poly_cell_vertex_positions[0], dimension));
	}
	for (int64_t cell_index = 0; cell_index < _poly_cell_boundary_pivot_overrides.size(); cell_index++) {
		const int32_t pivot = _poly_cell_boundary_pivot_overrides[cell_index];
		if (pivot == p_index) {
			_poly_cell_boundary_pivot_overrides.set(cell_index, -1);
		} else if (pivot > p_index) {
			_poly_cell_boundary_pivot_overrides.set(cell_index, pivot - 1);
		}
	}
	// Shift remaining edge vertex references down to preserve index semantics.
	for (int64_t edge_vertex_index = 0; edge_vertex_index < _edge_vertex_indices.size(); edge_vertex_index++) {
		if (_edge_vertex_indices[edge_vertex_index] > p_index) {
			_edge_vertex_indices.set(edge_vertex_index, _edge_vertex_indices[edge_vertex_index] - 1);
		}
	}
}

void ArrayPolyMeshND::_delete_poly_cell_element_internal(const int32_t p_poly_cell_index, const int32_t p_index) {
	ERR_FAIL_COND_MSG(p_poly_cell_index < 0 || p_poly_cell_index >= _poly_cell_indices.size(), "ArrayPolyMeshND: Dimension is out of range.");
	ERR_FAIL_COND_MSG(p_index < 0 || p_index >= _poly_cell_indices[p_poly_cell_index].size(), "ArrayPolyMeshND: Index is out of range.");
	// Before deleting this poly cell element, we need to delete anything in higher dimensions that reference it.
	const int32_t next_dim_poly_index = p_poly_cell_index + 1;
	if (next_dim_poly_index < _poly_cell_indices.size()) {
		// Collect indices in next_dim_poly_index whose elements reference p_index.
		Vector<int32_t> to_delete;
		const Vector<PackedInt32Array> &next_level = _poly_cell_indices[next_dim_poly_index];
		for (int32_t j = 0; j < next_level.size(); j++) {
			const PackedInt32Array &refs = next_level[j];
			for (int32_t k = 0; k < refs.size(); k++) {
				if (refs[k] == p_index) {
					to_delete.push_back(j);
					break;
				}
			}
		}
		// Delete in reverse order so that earlier indices are not shifted by later removals.
		for (int32_t i = to_delete.size() - 1; i >= 0; i--) {
			_delete_poly_cell_element_internal(next_dim_poly_index, to_delete[i]);
		}
	}
	// Delete any corresponding elements in the associated arrays for this poly cell dimension.
	if (p_poly_cell_index == _get_boundary_poly_dim_index() - 1) {
		// For the members of boundary cells, delete from the seams.
		if (!_seam_indices.is_empty()) {
			HashSet<int32_t> adjusted_seam_indices;
			for (const int32_t seam_index : _seam_indices) {
				if (seam_index == p_index) {
					continue;
				}
				adjusted_seam_indices.insert(seam_index > p_index ? seam_index - 1 : seam_index);
			}
			_seam_indices = adjusted_seam_indices;
		}
	}
	const int geom_dim = p_poly_cell_index + 2;
	_delete_data_binding_element_internal(geom_dim, p_index);
	// Delete from the boundary pivot overrides.
	if (p_poly_cell_index == _get_boundary_poly_dim_index() && p_index < _poly_cell_boundary_pivot_overrides.size()) {
		_poly_cell_boundary_pivot_overrides.remove_at(p_index);
	}
	// Remove the element at p_index from _poly_cell_indices[p_poly_cell_index].
	_poly_cell_indices.ptrw()[p_poly_cell_index].remove_at(p_index);
	// Fix up references in next_dim_poly_index by decrementing any index greater than p_index.
	if (next_dim_poly_index < _poly_cell_indices.size()) {
		Vector<PackedInt32Array> next_dim_data = _poly_cell_indices[next_dim_poly_index];
		for (int32_t j = 0; j < next_dim_data.size(); j++) {
			PackedInt32Array refs = next_dim_data[j];
			bool changed = false;
			for (int32_t k = 0; k < refs.size(); k++) {
				if (refs[k] > p_index) {
					refs.set(k, refs[k] - 1);
					changed = true;
				}
			}
			if (changed) {
				next_dim_data.set(j, refs);
			}
		}
		_poly_cell_indices.set(next_dim_poly_index, next_dim_data);
	}
	// Keep dimensions normalized by trimming from the first empty dimension onward.
	// In a valid poly mesh, once a dimension is empty, all higher dimensions must also be empty.
	for (int32_t dim_index = 0; dim_index < _poly_cell_indices.size(); dim_index++) {
		if (_poly_cell_indices[dim_index].is_empty()) {
			_poly_cell_indices.resize(dim_index);
			break;
		}
	}
}

void ArrayPolyMeshND::delete_poly_element(const int32_t p_dimension, const int32_t p_index) {
	if (p_dimension < 0) {
		ERR_FAIL_MSG("ArrayPolyMeshND: Cannot delete from negative dimension.");
	} else if (p_dimension == 0) {
		_delete_vertex_internal(p_index);
	} else if (p_dimension == 1) {
		_delete_edge_internal(p_index);
	} else {
		const int64_t poly_cell_index = p_dimension - 2;
		if (poly_cell_index >= _poly_cell_indices.size()) {
			ERR_FAIL_MSG("ArrayPolyMeshND: Cannot delete from dimension higher than the highest poly cell dimension.");
		}
		_delete_poly_cell_element_internal(poly_cell_index, p_index);
	}
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

// Normal calculation functions.

void ArrayPolyMeshND::calculate_boundary_normals(const ComputeNormalsMode p_mode, const bool p_keep_existing) {
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, "ArrayPolyMeshND: Cannot calculate boundary normals because there are no boundary cells.");
	ERR_FAIL_COND_MSG(!is_poly_mesh_data_valid(), "ArrayPolyMeshND: Cannot calculate boundary normals for invalid poly mesh data.");
	ERR_FAIL_COND_MSG(_poly_cell_vertex_positions.is_empty(), "ArrayPolyMeshND: Cannot calculate boundary normals because there are no vertices.");
	const Vector<PackedInt32Array> cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, true);
	if (cell_vertex_indices.is_empty()) {
		return;
	}
	Vector<VectorN> poly_cell_boundary_normals = _compute_boundary_normals_based_on_cell_orientation(cell_vertex_indices, p_keep_existing);
	CRASH_COND(poly_cell_boundary_normals.size() != cell_vertex_indices.size());
	const Vector2i per_cell_key = _get_per_cell_key();
	if (p_mode == COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY) {
		_all_poly_cell_normal_indices.insert(per_cell_key, Vector<PackedInt32Array>{ _normal_indices_for_values_internal(poly_cell_boundary_normals) });
		poly_mesh_clear_cache();
		return;
	}
	// Ensure normals point outward from the mesh.
	Vector<PackedInt32Array> boundary_cell_indices = _poly_cell_indices[boundary_dim_index];
	for (int64_t cell_index = 0; cell_index < cell_vertex_indices.size(); cell_index++) {
		const PackedInt32Array &vertex_indices = cell_vertex_indices[cell_index];
		VectorN average;
		for (int64_t vertex_index : vertex_indices) {
			ERR_FAIL_COND(vertex_index < 0 || vertex_index >= _poly_cell_vertex_positions.size());
			average = VectorND::add(average, _poly_cell_vertex_positions[vertex_index]);
		}
		average = VectorND::divide_scalar(average, (double)vertex_indices.size());
		if (VectorND::dot(average, poly_cell_boundary_normals[cell_index]) < 0) {
			// Normal points inward, so flip it, and optionally correct the cell orientation.
			poly_cell_boundary_normals.set(cell_index, VectorND::negate(poly_cell_boundary_normals[cell_index]));
			if (p_mode == COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION) {
				// Flip this cell's orientation so it matches the outward normal.
				PackedInt32Array cell_member_indices = boundary_cell_indices[cell_index];
				flip_poly_cell_orientation(cell_member_indices, boundary_dim_index);
				boundary_cell_indices.set(cell_index, cell_member_indices);
			}
		}
	}
	if (p_mode == COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION) {
		_poly_cell_indices.set(boundary_dim_index, boundary_cell_indices);
	}
	_all_poly_cell_normal_indices.insert(per_cell_key, Vector<PackedInt32Array>{ _normal_indices_for_values_internal(poly_cell_boundary_normals) });
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::set_flat_shading_normals(const ComputeNormalsMode p_mode, const bool p_recalculate_boundary_normals) {
	const Vector2i per_cell_key = _get_per_cell_key();
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	_all_poly_cell_normal_indices.erase(cell_to_vert_key);
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, "ArrayPolyMeshND: Cannot calculate boundary normals because there are no boundary cells.");
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot calculate boundary normals for an invalid mesh.");
	if (p_recalculate_boundary_normals || !_all_poly_cell_normal_indices.has(per_cell_key) || _all_poly_cell_normal_indices[per_cell_key].is_empty() || _all_poly_cell_normal_indices[per_cell_key][0].size() != _poly_cell_indices[boundary_dim_index].size()) {
		calculate_boundary_normals(p_mode);
	}
	const Vector<PackedInt32Array> cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, false);
	const int64_t cell_count = cell_vertex_indices.size();
	const PackedInt32Array &per_cell_normal_indices = _all_poly_cell_normal_indices[per_cell_key][0];
	CRASH_COND(per_cell_normal_indices.size() != cell_count);
	// Flat shading means every vertex of a cell shares the cell's normal, so the
	// vertex normal indices can all point at the cell's boundary normal value.
	Vector<PackedInt32Array> poly_cell_normal_indices;
	poly_cell_normal_indices.resize(cell_count);
	for (int64_t cell_index = 0; cell_index < cell_count; cell_index++) {
		PackedInt32Array normal_indices_for_cell;
		const int32_t cell_normal_index = per_cell_normal_indices[cell_index];
		const int64_t cell_vertex_count = cell_vertex_indices[cell_index].size();
		normal_indices_for_cell.resize(cell_vertex_count);
		for (int64_t vertex_index = 0; vertex_index < cell_vertex_count; vertex_index++) {
			normal_indices_for_cell.set(vertex_index, cell_normal_index);
		}
		poly_cell_normal_indices.set(cell_index, normal_indices_for_cell);
	}
	_all_poly_cell_normal_indices.insert(cell_to_vert_key, poly_cell_normal_indices);
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::set_smooth_shading_normals(const ComputeNormalsMode p_mode, const bool p_recalculate_boundary_normals) {
	const Vector2i per_cell_key = _get_per_cell_key();
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	_all_poly_cell_normal_indices.erase(cell_to_vert_key);
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, "ArrayPolyMeshND: Cannot calculate boundary normals because there are no boundary cells.");
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot calculate boundary normals for an invalid mesh.");
	// Step 1: Prepare the data arrays which will be used by this function.
	if (p_recalculate_boundary_normals || !_all_poly_cell_normal_indices.has(per_cell_key) || _all_poly_cell_normal_indices[per_cell_key].is_empty() || _all_poly_cell_normal_indices[per_cell_key][0].size() != _poly_cell_indices[boundary_dim_index].size()) {
		calculate_boundary_normals(p_mode);
	}
	const Vector<PackedInt32Array> cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, false);
	const Vector<VectorN> poly_cell_boundary_normals = _sample_normal_values_internal(_all_poly_cell_normal_indices[per_cell_key][0]);
	CRASH_COND(poly_cell_boundary_normals.size() != cell_vertex_indices.size());
	Vector<PackedInt32Array> poly_cell_normal_indices;
	poly_cell_normal_indices.resize(poly_cell_boundary_normals.size());
	Vector<VectorN> vertex_normals;
	vertex_normals.resize(_poly_cell_vertex_positions.size());
	PackedInt32Array vertex_normal_value_indices;
	vertex_normal_value_indices.resize(_poly_cell_vertex_positions.size());
	// Step 2: Iterate through each island separately such that seams (if they exist)
	// are respected and are treated as sharp borders that should not be smoothed across.
	const Vector<PackedInt32Array> islands = collect_all_islands();
	for (int64_t island_index = 0; island_index < islands.size(); island_index++) {
		const PackedInt32Array &cells_in_island = islands[island_index];
		// Step 3: Calculate the average normal of each vertex across each usage in cells in this island.
		for (int64_t vertex_index = 0; vertex_index < _poly_cell_vertex_positions.size(); vertex_index++) {
			vertex_normals.set(vertex_index, VectorN());
			vertex_normal_value_indices.set(vertex_index, -1);
		}
		for (const int32_t cell_index : cells_in_island) {
			const PackedInt32Array &vertex_indices_for_cell = cell_vertex_indices[cell_index];
			const VectorN &cell_normal = poly_cell_boundary_normals[cell_index];
			for (const int32_t vertex_index : vertex_indices_for_cell) {
				vertex_normals.set(vertex_index, VectorND::add(vertex_normals[vertex_index], cell_normal));
			}
		}
		for (int64_t vertex_index = 0; vertex_index < _poly_cell_vertex_positions.size(); vertex_index++) {
			vertex_normals.set(vertex_index, VectorND::normalized(vertex_normals[vertex_index]));
		}
		// Step 4: Assign each cell's vertex normals to the average normal of the vertices that make up
		// that cell. Each vertex's normal value is only appended once per island and then referenced.
		for (const int32_t cell_index : cells_in_island) {
			const PackedInt32Array &vertex_indices_for_cell = cell_vertex_indices[cell_index];
			PackedInt32Array normal_indices_for_cell;
			const int64_t cell_vertex_count = vertex_indices_for_cell.size();
			normal_indices_for_cell.resize(cell_vertex_count);
			for (int64_t vertex_in_cell = 0; vertex_in_cell < cell_vertex_count; vertex_in_cell++) {
				const int32_t vertex_index = vertex_indices_for_cell[vertex_in_cell];
				if (vertex_normal_value_indices[vertex_index] == -1) {
					const int64_t normal_index = VectorND::array_append_deduplicate(_poly_cell_normal_values, vertex_normals[vertex_index]);
					vertex_normal_value_indices.set(vertex_index, (int32_t)normal_index);
				}
				normal_indices_for_cell.set(vertex_in_cell, vertex_normal_value_indices[vertex_index]);
			}
			poly_cell_normal_indices.set(cell_index, normal_indices_for_cell);
		}
	}
	_all_poly_cell_normal_indices.insert(cell_to_vert_key, poly_cell_normal_indices);
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::make_double_sided(const bool p_idempotent) {
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, "ArrayPolyMeshND: Cannot make double sided because there are no boundary cells.");
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot make double sided for an invalid mesh.");
	if (_poly_cell_indices[boundary_dim_index].is_empty()) {
		return;
	}
	const Vector2i per_cell_key = _get_per_cell_key();
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	if (!_all_poly_cell_normal_indices.has(per_cell_key) || _all_poly_cell_normal_indices[per_cell_key].is_empty() || _all_poly_cell_normal_indices[per_cell_key][0].size() != _poly_cell_indices[boundary_dim_index].size()) {
		calculate_boundary_normals(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, false);
	}
	Vector<PackedInt32Array> cell_member_indices = Vector<PackedInt32Array>(_poly_cell_indices[boundary_dim_index]);
	const int64_t original_cell_count = cell_member_indices.size();
	const int64_t original_pivot_count = _poly_cell_boundary_pivot_overrides.size();
	// This has to be a copy, it's set back in place at the end of the function.
	PackedInt32Array per_cell_normal_indices = _all_poly_cell_normal_indices[per_cell_key][0];
	CRASH_COND(per_cell_normal_indices.size() != original_cell_count);
	const bool has_vertex_normals = _all_poly_cell_normal_indices.has(cell_to_vert_key) && !_all_poly_cell_normal_indices[cell_to_vert_key].is_empty();
	const bool has_texture_map = _all_poly_cell_texture_map_indices.has(cell_to_vert_key) && !_all_poly_cell_texture_map_indices[cell_to_vert_key].is_empty();
	Vector<PackedInt32Array> original_cell_vertex_indices;
	if (has_vertex_normals || has_texture_map) {
		original_cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, false);
	}
	PackedInt32Array flipped_cell_index_for_original;
	flipped_cell_index_for_original.resize(original_cell_count);
	for (int64_t i = 0; i < original_cell_count; i++) {
		flipped_cell_index_for_original.set(i, -1);
	}
	for (int64_t cell_index = 0; cell_index < original_cell_count; cell_index++) {
		PackedInt32Array flipped_cell_members = PackedInt32Array(cell_member_indices[cell_index]);
		flip_poly_cell_orientation(flipped_cell_members, boundary_dim_index);
		int32_t existing_flipped_index = -1;
		if (p_idempotent) {
			// Check if this flipped cell already exists before adding it, whenever idempotence is requested.
			bool already_exists = false;
			for (int64_t other_cell_index = 0; other_cell_index < original_cell_count; other_cell_index++) {
				if (other_cell_index == cell_index) {
					continue;
				}
				if (cell_member_indices[other_cell_index] == flipped_cell_members) {
					already_exists = true;
					existing_flipped_index = other_cell_index;
					break;
				}
			}
			if (already_exists) {
				flipped_cell_index_for_original.set(cell_index, existing_flipped_index);
				continue;
			}
		}
		// Flipping changes the derived vertex order. Map each flipped vertex back to
		// its original position so normal and texture values stay on the same vertex.
		PackedInt32Array flipped_vertex_order_remap;
		if (has_vertex_normals || has_texture_map) {
			const PackedInt32Array &original_cell_vertices = original_cell_vertex_indices[cell_index];
			Vector<Vector<PackedInt32Array>> flipped_poly_cell_indices = _poly_cell_indices;
			flipped_poly_cell_indices.set(boundary_dim_index, Vector<PackedInt32Array>{ flipped_cell_members });
			const PackedInt32Array flipped_cell_vertices = _get_vertex_indices_of_boundary_cells(flipped_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, false)[0];
			flipped_vertex_order_remap.resize(flipped_cell_vertices.size());
			for (int64_t vertex_in_cell = 0; vertex_in_cell < flipped_cell_vertices.size(); vertex_in_cell++) {
				const int64_t original_position = original_cell_vertices.find(flipped_cell_vertices[vertex_in_cell]);
				CRASH_COND(original_position < 0);
				flipped_vertex_order_remap.set(vertex_in_cell, (int32_t)original_position);
			}
		}
		// Copy the texture map if it exists for this cell before adding the flipped cell.
		// The flipped cell shares the same texture map values, so only the indices are copied.
		if (has_texture_map) {
			// HashMap's indexing operator allows getting a mutable reference, so we don't need to set it back after.
			Vector<PackedInt32Array> &poly_cell_texture_map_indices = _all_poly_cell_texture_map_indices[cell_to_vert_key];
			const PackedInt32Array &original_cell_texture_map_indices = poly_cell_texture_map_indices[cell_index];
			PackedInt32Array flipped_cell_texture_map_indices;
			// Validation guarantees that populated arrays have one index per vertex.
			// Empty arrays remain empty, preserving cells without texture map data.
			if (!original_cell_texture_map_indices.is_empty()) {
				flipped_cell_texture_map_indices.resize(flipped_vertex_order_remap.size());
				for (int64_t vertex_in_cell = 0; vertex_in_cell < flipped_vertex_order_remap.size(); vertex_in_cell++) {
					const int32_t original_position = flipped_vertex_order_remap[vertex_in_cell];
					flipped_cell_texture_map_indices.set(vertex_in_cell, original_cell_texture_map_indices[original_position]);
				}
			}
			poly_cell_texture_map_indices.append(flipped_cell_texture_map_indices);
		}
		// Copy and flip the vertex normals if they exist for this cell before adding the flipped cell.
		if (has_vertex_normals) {
			// HashMap's indexing operator allows getting a mutable reference, so we don't need to set it back after.
			Vector<PackedInt32Array> &poly_cell_normal_indices = _all_poly_cell_normal_indices[cell_to_vert_key];
			const PackedInt32Array &original_cell_normal_indices = poly_cell_normal_indices[cell_index];
			PackedInt32Array flipped_cell_normal_indices;
			flipped_cell_normal_indices.resize(original_cell_normal_indices.size());
			for (int64_t vertex_in_cell = 0; vertex_in_cell < original_cell_normal_indices.size(); vertex_in_cell++) {
				const int32_t original_position = flipped_vertex_order_remap[vertex_in_cell];
				const VectorN flipped_normal = VectorND::negate(_poly_cell_normal_values[original_cell_normal_indices[original_position]]);
				const int64_t normal_index = VectorND::array_append_deduplicate(_poly_cell_normal_values, flipped_normal);
				flipped_cell_normal_indices.set(vertex_in_cell, (int32_t)normal_index);
			}
			poly_cell_normal_indices.append(flipped_cell_normal_indices);
		}
		// Append the flipped cell, and record its index for later when we update volumetric cells.
		const int32_t new_flipped_cell_index = cell_member_indices.size();
		if (cell_index < original_pivot_count) {
			while (_poly_cell_boundary_pivot_overrides.size() < new_flipped_cell_index) {
				_poly_cell_boundary_pivot_overrides.append(-1);
			}
			_poly_cell_boundary_pivot_overrides.append(_poly_cell_boundary_pivot_overrides[cell_index]);
		}
		cell_member_indices.append(flipped_cell_members);
		flipped_cell_index_for_original.set(cell_index, new_flipped_cell_index);
		const VectorN flipped_boundary_normal = VectorND::negate(_poly_cell_normal_values[per_cell_normal_indices[cell_index]]);
		per_cell_normal_indices.append((int32_t)VectorND::array_append_deduplicate(_poly_cell_normal_values, flipped_boundary_normal));
	}
	_poly_cell_indices.set(boundary_dim_index, cell_member_indices);
	if (_poly_cell_indices.size() > boundary_dim_index + 1) {
		Vector<PackedInt32Array> volumetric_cell_indices = _poly_cell_indices[boundary_dim_index + 1];
		for (int64_t volumetric_cell_index = 0; volumetric_cell_index < volumetric_cell_indices.size(); volumetric_cell_index++) {
			PackedInt32Array vol_cell = volumetric_cell_indices[volumetric_cell_index];
			const int64_t original_vol_cell_size = vol_cell.size();
			for (int64_t i = 0; i < original_vol_cell_size; i++) {
				const int32_t original_boundary_cell_index = vol_cell[i];
				if (original_boundary_cell_index < 0 || original_boundary_cell_index >= original_cell_count) {
					continue;
				}
				const int32_t flipped_boundary_cell_index = flipped_cell_index_for_original[original_boundary_cell_index];
				if (flipped_boundary_cell_index == -1 || vol_cell.has(flipped_boundary_cell_index)) {
					continue;
				}
				vol_cell.append(flipped_boundary_cell_index);
			}
			volumetric_cell_indices.set(volumetric_cell_index, vol_cell);
		}
		_poly_cell_indices.set(boundary_dim_index + 1, volumetric_cell_indices);
	}
	_all_poly_cell_normal_indices.insert(per_cell_key, Vector<PackedInt32Array>{ per_cell_normal_indices });
	poly_mesh_clear_cache();
}

PackedInt32Array ArrayPolyMeshND::make_single_cell_from_all_cells(const int32_t p_cell_dimension) const {
	ERR_FAIL_COND_V_MSG(p_cell_dimension < 3, PackedInt32Array(), "ArrayPolyMeshND: Cannot make a single cell of dimension " + itos(p_cell_dimension) + " because its members would not be poly cells.");
	const int64_t member_dim_index = p_cell_dimension - 3;
	ERR_FAIL_COND_V_MSG(_poly_cell_indices.size() <= member_dim_index, PackedInt32Array(), "ArrayPolyMeshND: Cannot make a single " + itos(p_cell_dimension) + "D cell because there are no " + itos(p_cell_dimension - 1) + "D cells.");
	const Vector<PackedInt32Array> &members = _poly_cell_indices[member_dim_index];
	const int64_t member_count = members.size();
	PackedInt32Array cell_indices;
	cell_indices.resize(member_count);
	for (int64_t member_index = 0; member_index < member_count; member_index++) {
		cell_indices.set(member_index, member_index);
	}
	// For a deterministic cell orientation, the first two members must share a common element.
	bool success = MathND::ensure_first_two_indices_share_common_int32(cell_indices, members);
	if (!success) {
		ERR_PRINT("ArrayPolyMeshND: Failed to make a single cell from all cells because the first cell does not share a common element with any other cell.");
	}
	return cell_indices;
}

// Texture map and seam functions.

void ArrayPolyMeshND::calculate_seams(const double p_angle_threshold_radians, const bool p_discard_seams_within_islands) {
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, "ArrayPolyMeshND: Cannot calculate seams because there are no boundary cells.");
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot calculate seams for an invalid mesh.");
	const Vector2i per_cell_key = _get_per_cell_key();
	if (!_all_poly_cell_normal_indices.has(per_cell_key) || _all_poly_cell_normal_indices[per_cell_key].is_empty() || _all_poly_cell_normal_indices[per_cell_key][0].size() != _poly_cell_indices[boundary_dim_index].size()) {
		calculate_boundary_normals(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, false);
	}
	_seam_indices.clear();
	HashSet<int32_t> cells_between_volumetric;
	const Vector<PackedInt32Array> &cell_member_indices = _poly_cell_indices[boundary_dim_index];
	const Vector<VectorN> poly_cell_boundary_normals = _sample_normal_values_internal(_all_poly_cell_normal_indices[per_cell_key][0]);
	CRASH_COND(poly_cell_boundary_normals.size() != cell_member_indices.size());
	if (_poly_cell_indices.size() > boundary_dim_index + 1) {
		// If this is a volumetric poly mesh with N-dimensional cells, ignore cells between volumetric cells.
		// Such cells are "virtual", only used to define geometry, and are not part of the visible surface.
		const Vector<PackedInt32Array> &volumetric_cell_indices = _poly_cell_indices[boundary_dim_index + 1];
		for (int64_t volumetric_cell_index = 0; volumetric_cell_index < volumetric_cell_indices.size(); volumetric_cell_index++) {
			const PackedInt32Array &vol_cells = volumetric_cell_indices[volumetric_cell_index];
			for (int64_t other_volumetric_cell_index = volumetric_cell_index + 1; other_volumetric_cell_index < volumetric_cell_indices.size(); other_volumetric_cell_index++) {
				const PackedInt32Array &other_vol_cells = volumetric_cell_indices[other_volumetric_cell_index];
				int64_t index_in_self;
				int64_t index_in_other;
				const int32_t common_cell = MathND::find_common_int32(vol_cells, other_vol_cells, index_in_self, index_in_other);
				if (common_cell != INT32_MIN) {
					cells_between_volumetric.insert(common_cell);
				}
			}
		}
	}
	const Vector<PackedInt32Array> member_to_cell_map = _get_member_to_cell_map();
	// Actually calculate seams based on the angles between normals of adjacent cells.
	for (int64_t cell_index = 0; cell_index < cell_member_indices.size(); cell_index++) {
		if (cells_between_volumetric.has(cell_index)) {
			continue;
		}
		const PackedInt32Array &cell_members = cell_member_indices[cell_index];
		for (int64_t member_index_in_cell = 0; member_index_in_cell < cell_members.size(); member_index_in_cell++) {
			const int32_t member_index = cell_members[member_index_in_cell];
			const PackedInt32Array &cells_using_member = member_to_cell_map[member_index];
			for (int64_t i = 0; i < cells_using_member.size(); i++) {
				const int32_t other_cell_index = cells_using_member[i];
				if (other_cell_index == cell_index || cells_between_volumetric.has(other_cell_index)) {
					continue;
				}
				const VectorN &normal_a = poly_cell_boundary_normals[cell_index];
				const VectorN &normal_b = poly_cell_boundary_normals[other_cell_index];
				if (VectorND::angle_to(normal_a, normal_b) > p_angle_threshold_radians) {
					_seam_indices.insert(member_index);
				}
			}
		}
	}
	if (!p_discard_seams_within_islands) {
		return;
	}
	const Vector<PackedInt32Array> islands = collect_all_islands();
	for (int64_t island_index = 0; island_index < islands.size(); island_index++) {
		const PackedInt32Array &cells_in_island = islands[island_index];
		for (int64_t cell_index_index = 0; cell_index_index < cells_in_island.size(); cell_index_index++) {
			const int32_t cell_index = cells_in_island[cell_index_index];
			const PackedInt32Array &cell_members = cell_member_indices[cell_index];
			for (int64_t other_cell_index_index = cell_index_index + 1; other_cell_index_index < cells_in_island.size(); other_cell_index_index++) {
				const int32_t other_cell_index = cells_in_island[other_cell_index_index];
				const PackedInt32Array &other_cell_members = cell_member_indices[other_cell_index];
				int64_t index_in_self;
				int64_t index_in_other;
				const int32_t common_member = MathND::find_common_int32(cell_members, other_cell_members, index_in_self, index_in_other);
				if (common_member != INT32_MIN) {
					_seam_indices.erase(common_member);
				}
			}
		}
	}
}

int64_t ArrayPolyMeshND::_get_boundary_member_count() const {
	const int64_t dimension = _poly_cell_vertex_positions.is_empty() ? 0 : _poly_cell_vertex_positions[0].size();
	const int64_t boundary_dim_index = dimension - 3;
	if (boundary_dim_index < 0) {
		return 0;
	}
	if (boundary_dim_index == 0) {
		// For 3D meshes, the boundary cells are faces, whose members are edges.
		return _edge_vertex_indices.size() / 2;
	}
	if (_poly_cell_indices.size() <= boundary_dim_index - 1) {
		return 0;
	}
	return _poly_cell_indices[boundary_dim_index - 1].size();
}

Vector<PackedInt32Array> ArrayPolyMeshND::_get_member_to_cell_map() const {
	// For efficiency, pre-compute a mapping of boundary member index to the cells that use it.
	const int64_t dimension = _poly_cell_vertex_positions.is_empty() ? 0 : _poly_cell_vertex_positions[0].size();
	const int64_t boundary_dim_index = dimension - 3;
	Vector<PackedInt32Array> member_to_cells;
	ERR_FAIL_COND_V(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, member_to_cells);
	const Vector<PackedInt32Array> &cell_member_indices = _poly_cell_indices[boundary_dim_index];
	member_to_cells.resize(_get_boundary_member_count());
	for (int64_t cell_index = 0; cell_index < cell_member_indices.size(); cell_index++) {
		const PackedInt32Array &cell_members = cell_member_indices[cell_index];
		for (int64_t member_index_in_cell = 0; member_index_in_cell < cell_members.size(); member_index_in_cell++) {
			const int32_t member_index = cell_members[member_index_in_cell];
			CRASH_COND(member_index < 0 || member_index >= member_to_cells.size());
			PackedInt32Array cells_for_member = member_to_cells[member_index];
			cells_for_member.append(int32_t(cell_index));
			member_to_cells.set(member_index, cells_for_member);
		}
	}
	return member_to_cells;
}

PackedInt32Array ArrayPolyMeshND::_collect_cells_in_island_internal(const int64_t p_start_cell, const Vector<PackedInt32Array> &p_member_to_cell_map) {
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	PackedInt32Array cells_in_island = { int32_t(p_start_cell) };
	PackedInt32Array members_to_search = PackedInt32Array(_poly_cell_indices[boundary_dim_index][p_start_cell]); // Copy.
	HashSet<int32_t> cells_in_island_set; // Redundant with cells_in_island, but faster to check existence for large meshes.
	HashSet<int32_t> members_to_search_set; // Redundant with members_to_search, but faster to check existence for large meshes.
	for (int32_t c : cells_in_island) {
		cells_in_island_set.insert(c);
	}
	for (int32_t m : members_to_search) {
		members_to_search_set.insert(m);
	}
	HashSet<int32_t> members_already_searched;
	int64_t search_index = 0;
	while (search_index < members_to_search.size()) {
		const int32_t member_index = members_to_search[search_index];
		members_already_searched.insert(member_index);
		if (_seam_indices.has(member_index)) {
			// This member is a seam, so do not cross it, as it may be another island.
			search_index++;
			continue;
		}
		const PackedInt32Array &cells_using_member = p_member_to_cell_map[member_index];
		for (int64_t i = 0; i < cells_using_member.size(); i++) {
			const int32_t cell_index = cells_using_member[i];
			if (!cells_in_island_set.has(cell_index)) {
				cells_in_island.append(cell_index);
				cells_in_island_set.insert(cell_index);
				// Add all members of this new cell to the search list, except those already searched.
				const PackedInt32Array &new_cell_members = _poly_cell_indices[boundary_dim_index][cell_index];
				for (int64_t j = 0; j < new_cell_members.size(); j++) {
					const int32_t new_member_index = new_cell_members[j];
					if (!members_already_searched.has(new_member_index) && !members_to_search_set.has(new_member_index)) {
						members_to_search.append(new_member_index);
						members_to_search_set.insert(new_member_index);
					}
				}
			}
		}
		search_index++;
		members_to_search_set.erase(member_index);
	}
	return cells_in_island;
}

PackedInt32Array ArrayPolyMeshND::collect_cells_in_island(const int64_t p_start_cell) {
	// This function is exposed so start by performing validation to avoid running with garbage data.
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_V_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, PackedInt32Array(), "ArrayPolyMeshND: Cannot collect islands for a mesh with no boundary cells.");
	ERR_FAIL_INDEX_V_MSG(p_start_cell, _poly_cell_indices[boundary_dim_index].size(), PackedInt32Array(), "ArrayPolyMeshND: The island start cell is not in the mesh.");
	ERR_FAIL_COND_V_MSG(!is_poly_mesh_data_valid(), PackedInt32Array(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot collect islands.");
	const Vector<PackedInt32Array> member_to_cell_map = _get_member_to_cell_map();
	// Use the internal version internally when we know the data is valid.
	return _collect_cells_in_island_internal(p_start_cell, member_to_cell_map);
}

Vector<PackedInt32Array> ArrayPolyMeshND::collect_all_islands() {
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_V_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, Vector<PackedInt32Array>(), "ArrayPolyMeshND: Cannot collect islands for a mesh with no boundary cells.");
	ERR_FAIL_COND_V_MSG(!is_poly_mesh_data_valid(), Vector<PackedInt32Array>(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot collect islands.");
	const Vector<PackedInt32Array> member_to_cell_map = _get_member_to_cell_map();
	Vector<PackedInt32Array> islands;
	for (int64_t cell_index = 0; cell_index < _poly_cell_indices[boundary_dim_index].size(); cell_index++) {
		bool cell_already_in_island = false;
		for (int64_t island_index = 0; island_index < islands.size(); island_index++) {
			if (islands[island_index].has(cell_index)) {
				cell_already_in_island = true;
				break;
			}
		}
		if (cell_already_in_island) {
			continue;
		}
		islands.append(_collect_cells_in_island_internal(cell_index, member_to_cell_map));
	}
	return islands;
}

TypedArray<PackedInt32Array> ArrayPolyMeshND::collect_all_islands_bind() {
	const Vector<PackedInt32Array> islands = collect_all_islands();
	TypedArray<PackedInt32Array> ret;
	ret.resize(islands.size());
	for (int64_t i = 0; i < islands.size(); i++) {
		ret[i] = islands[i];
	}
	return ret;
}

bool ArrayPolyMeshND::_unwrap_texture_map_island_cell(const PackedInt32Array &p_cells_in_island, const int64_t p_current_cell_index_index, const Vector<PackedInt32Array> &p_cell_vert, Vector<Vector<VectorM>> &r_poly_cell_texture_map) {
	const int64_t dimension = get_dimension();
	const int64_t texture_dimension = dimension - 1;
	const int64_t boundary_dim_index = dimension - 3;
	const int32_t cell_index = p_cells_in_island[p_current_cell_index_index];
	const PackedInt32Array &cell_members = _poly_cell_indices[boundary_dim_index][cell_index];
	Vector<Vector<VectorM>> &poly_cell_texture_map = r_poly_cell_texture_map;
	const PackedInt32Array &cell_vertex_list = p_cell_vert[cell_index];
	if (p_current_cell_index_index == 0) {
		// For the first cell in the island, there is nothing to "build on", so project the cell
		// isometrically onto its own hyperplane, using an orthonormal basis of the cell's spans.
		const VectorN base = _poly_cell_vertex_positions[cell_vertex_list[0]];
		Vector<VectorN> ortho_dirs;
		for (int64_t i = 1; i < cell_vertex_list.size(); i++) {
			if (ortho_dirs.size() >= texture_dimension) {
				break;
			}
			VectorN direction = VectorND::subtract(_poly_cell_vertex_positions[cell_vertex_list[i]], base);
			const double original_length = VectorND::length(direction);
			if (Math::is_zero_approx(original_length)) {
				continue;
			}
			for (int64_t ortho_index = 0; ortho_index < ortho_dirs.size(); ortho_index++) {
				direction = VectorND::subtract(direction, VectorND::multiply_scalar(ortho_dirs[ortho_index], VectorND::dot(ortho_dirs[ortho_index], direction)));
			}
			const double residual_length = VectorND::length(direction);
			if (residual_length < original_length * (double)CMP_EPSILON) {
				continue;
			}
			ortho_dirs.append(VectorND::divide_scalar(direction, residual_length));
		}
		ERR_FAIL_COND_V_MSG(ortho_dirs.size() < texture_dimension, false, "ArrayPolyMeshND: Cell is degenerate.");
		Vector<VectorM> cell_texture_map;
		cell_texture_map.resize(cell_vertex_list.size());
		for (int64_t i = 0; i < cell_vertex_list.size(); i++) {
			const VectorN offset = VectorND::subtract(_poly_cell_vertex_positions[cell_vertex_list[i]], base);
			VectorM texcoord = VectorND::fill(texture_dimension, 0.0);
			for (int64_t axis = 0; axis < texture_dimension; axis++) {
				texcoord.set(axis, VectorND::dot(ortho_dirs[axis], offset));
			}
			cell_texture_map.set(i, texcoord);
		}
		poly_cell_texture_map.set(cell_index, cell_texture_map);
		return true;
	}
	// Search for neighboring cells in this island which share a member with this cell.
	for (int64_t already_mapped_cell_index_index = 0; already_mapped_cell_index_index < p_current_cell_index_index; already_mapped_cell_index_index++) {
		const int32_t already_mapped_cell_index = p_cells_in_island[already_mapped_cell_index_index];
		const PackedInt32Array &already_mapped_cell_members = _poly_cell_indices[boundary_dim_index][already_mapped_cell_index];
		const PackedInt32Array &already_mapped_cell_verts = p_cell_vert[already_mapped_cell_index];
		const Vector<VectorM> &already_mapped_texture_map = poly_cell_texture_map[already_mapped_cell_index];
		if (already_mapped_cell_verts.is_empty() || already_mapped_texture_map.is_empty()) {
			continue;
		}
		int64_t member_in_cell;
		int64_t member_in_already_mapped;
		const int32_t common_member = MathND::find_common_int32(cell_members, already_mapped_cell_members, member_in_cell, member_in_already_mapped);
		if (common_member == INT32_MIN) {
			continue;
		}
		// Gather the vertices of the shared member. For 3D meshes the members are edges,
		// which are stored in the flat edge array rather than in the poly cell indices.
		PackedInt32Array member_vertices;
		if (boundary_dim_index == 0) {
			member_vertices.append(_edge_vertex_indices[common_member * 2]);
			member_vertices.append(_edge_vertex_indices[common_member * 2 + 1]);
		} else {
			member_vertices = _get_vertex_indices_of_poly_cell(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index - 1, common_member, false);
		}
		// The shared member is (N-2)-dimensional, so it is spanned by N-2 independent directions.
		PackedInt32Array picked_positions;
		const int64_t rank = _pick_spanning_vertices(_poly_cell_vertex_positions, member_vertices, dimension - 2, picked_positions);
		if (rank < dimension - 2) {
			continue; // The shared member is degenerate, try to build on another neighbor instead.
		}
		const int32_t base_vertex = member_vertices[0];
		const VectorN base = _poly_cell_vertex_positions[base_vertex];
		// Find a vertex of this cell that is not on the shared member, to unfold away from it.
		int32_t off_member_vertex = -1;
		for (const int32_t cell_vertex : cell_vertex_list) {
			if (!member_vertices.has(cell_vertex)) {
				off_member_vertex = cell_vertex;
				break;
			}
		}
		ERR_FAIL_COND_V_MSG(off_member_vertex == -1, false, "ArrayPolyMeshND: Cell is degenerate.");
		// Build the world-space spans: N-2 directions along the shared member, plus one
		// direction perpendicular to the member within the cell's hyperplane.
		Vector<VectorN> world_spans;
		world_spans.resize(rank + 1);
		Vector<VectorN> member_ortho_dirs;
		for (int64_t i = 0; i < rank; i++) {
			VectorN span = VectorND::subtract(_poly_cell_vertex_positions[member_vertices[picked_positions[i]]], base);
			world_spans.set(i, span);
			for (int64_t ortho_index = 0; ortho_index < member_ortho_dirs.size(); ortho_index++) {
				span = VectorND::subtract(span, VectorND::multiply_scalar(member_ortho_dirs[ortho_index], VectorND::dot(member_ortho_dirs[ortho_index], span)));
			}
			member_ortho_dirs.append(VectorND::normalized(span));
		}
		VectorN world_perp = VectorND::subtract(_poly_cell_vertex_positions[off_member_vertex], base);
		for (int64_t ortho_index = 0; ortho_index < member_ortho_dirs.size(); ortho_index++) {
			world_perp = VectorND::subtract(world_perp, VectorND::multiply_scalar(member_ortho_dirs[ortho_index], VectorND::dot(member_ortho_dirs[ortho_index], world_perp)));
		}
		const double world_perp_length = VectorND::length(world_perp);
		ERR_FAIL_COND_V_MSG(Math::is_zero_approx(world_perp_length), false, "ArrayPolyMeshND: Cell is degenerate.");
		world_spans.set(rank, world_perp);
		// Build the texture-space spans from the neighbor's existing texture coordinates.
		const int64_t base_position_in_mapped = already_mapped_cell_verts.find(base_vertex);
		if (base_position_in_mapped < 0) {
			continue;
		}
		const VectorM tex_base = already_mapped_texture_map[base_position_in_mapped];
		Vector<VectorM> tex_spans;
		tex_spans.resize(rank);
		bool found_all_positions = true;
		for (int64_t i = 0; i < rank; i++) {
			const int64_t position_in_mapped = already_mapped_cell_verts.find(member_vertices[picked_positions[i]]);
			if (position_in_mapped < 0) {
				found_all_positions = false;
				break;
			}
			tex_spans.set(i, VectorND::subtract(already_mapped_texture_map[position_in_mapped], tex_base));
		}
		if (!found_all_positions) {
			continue;
		}
		// The unfold direction is perpendicular to the member's texture spans, isometric in
		// length, and points away from the already mapped cell.
		VectorM tex_perp = VectorND::perpendicular(tex_spans);
		const double tex_perp_length = VectorND::length(tex_perp);
		if (Math::is_zero_approx(tex_perp_length)) {
			continue; // The neighbor's mapping of the shared member is degenerate.
		}
		tex_perp = VectorND::multiply_scalar(tex_perp, world_perp_length / tex_perp_length);
		VectorM already_mapped_average;
		for (int64_t i = 0; i < already_mapped_texture_map.size(); i++) {
			already_mapped_average = VectorND::add(already_mapped_average, already_mapped_texture_map[i]);
		}
		already_mapped_average = VectorND::divide_scalar(already_mapped_average, (double)already_mapped_texture_map.size());
		if (VectorND::dot(VectorND::subtract(already_mapped_average, tex_base), tex_perp) > 0.0) {
			tex_perp = VectorND::negate(tex_perp);
		}
		// Map each vertex of this cell by expressing it in the world spans and applying the
		// same coordinates to the texture spans. Vertices on the shared member land exactly
		// on the neighbor's texture coordinates, so the seam matches without cracks.
		Vector<VectorM> cell_texture_map;
		cell_texture_map.resize(cell_vertex_list.size());
		for (int64_t i = 0; i < cell_vertex_list.size(); i++) {
			VectorN coordinates;
			if (!_solve_coordinates_in_span(world_spans, VectorND::subtract(_poly_cell_vertex_positions[cell_vertex_list[i]], base), coordinates)) {
				ERR_FAIL_V_MSG(false, "ArrayPolyMeshND: Cell is degenerate.");
			}
			VectorM texcoord = VectorND::duplicate(tex_base);
			for (int64_t span_index = 0; span_index < rank; span_index++) {
				texcoord = VectorND::add(texcoord, VectorND::multiply_scalar(tex_spans[span_index], coordinates[span_index]));
			}
			texcoord = VectorND::add(texcoord, VectorND::multiply_scalar(tex_perp, coordinates[rank]));
			cell_texture_map.set(i, texcoord);
		}
		poly_cell_texture_map.set(cell_index, cell_texture_map);
		return true;
	}
	ERR_FAIL_V_MSG(false, "ArrayPolyMeshND: Island cells are not contiguous, cannot unwrap.");
}

void ArrayPolyMeshND::_unwrap_texture_map_island_internal(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing, Vector<Vector<VectorM>> &r_poly_cell_texture_map) {
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	CRASH_COND(r_poly_cell_texture_map.size() != _poly_cell_indices[boundary_dim_index].size());
	const Vector<PackedInt32Array> cell_vert = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, false);
	for (int64_t cell_index_index = 0; cell_index_index < p_cells_in_island.size(); cell_index_index++) {
		if (p_keep_existing && !r_poly_cell_texture_map[p_cells_in_island[cell_index_index]].is_empty()) {
			continue;
		}
		if (!_unwrap_texture_map_island_cell(p_cells_in_island, cell_index_index, cell_vert, r_poly_cell_texture_map)) {
			return;
		}
	}
}

void ArrayPolyMeshND::unwrap_texture_map_island(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing) {
	// This function is exposed so start by performing validation to avoid running with garbage data.
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, "ArrayPolyMeshND: Cannot unwrap texture map for a mesh with no boundary cells.");
	ERR_FAIL_COND_MSG(p_cells_in_island.is_empty(), "ArrayPolyMeshND: Cannot unwrap texture map for an empty island of cells.");
	const Vector<PackedInt32Array> cells = _poly_cell_indices[boundary_dim_index];
	for (int64_t i = 0; i < p_cells_in_island.size(); i++) {
		ERR_FAIL_COND_MSG(p_cells_in_island[i] >= cells.size(), "ArrayPolyMeshND: A cell in this island is not in the mesh.");
	}
	ERR_FAIL_COND_MSG(!is_poly_mesh_data_valid(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot unwrap.");
	const int64_t cell_count = cells.size();
	// Unwrapping works with a dense texture map, sampled from the indexed data
	// at the start and converted back to indexed data at the end.
	Vector<Vector<VectorM>> poly_cell_texture_map = _get_poly_cell_texture_map_dense_internal();
	poly_cell_texture_map.resize(cell_count);
	// Use the internal version internally when we know the data is valid.
	_unwrap_texture_map_island_internal(p_cells_in_island, p_keep_existing, poly_cell_texture_map);
	_set_poly_cell_texture_map_dense_internal(poly_cell_texture_map);
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::unwrap_texture_map(const UnwrapTextureMapMode p_mode, const double p_padding, const bool p_proportional, const bool p_keep_existing) {
	ERR_FAIL_COND_MSG(p_padding < 0.0, "ArrayPolyMeshND: Padding must be non-negative.");
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	ERR_FAIL_COND_MSG(boundary_dim_index < 0 || _poly_cell_indices.size() <= boundary_dim_index, "ArrayPolyMeshND: Cannot unwrap texture map for a mesh with no boundary cells.");
	ERR_FAIL_COND_MSG(!is_poly_mesh_data_valid(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot unwrap.");
	const int64_t texture_dimension = get_dimension() - 1;
	const int64_t cell_count = _poly_cell_indices[boundary_dim_index].size();
	// Unwrapping works with a dense texture map, sampled from the indexed data
	// at the start and converted back to indexed data at the end.
	Vector<Vector<VectorM>> poly_cell_texture_map;
	if (p_keep_existing) {
		poly_cell_texture_map = _get_poly_cell_texture_map_dense_internal();
	}
	poly_cell_texture_map.resize(cell_count);
	UnwrapTextureMapMode actual_mode = p_mode;
	if (actual_mode == UNWRAP_MODE_AUTOMATIC) {
		actual_mode = _seam_indices.is_empty() ? UNWRAP_MODE_TILE_CELLS : UNWRAP_MODE_TILE_ISLANDS;
	}
	const double pad_offset = p_padding * 0.5 / (1.0 + p_padding);
	const double pad_size = 1.0 / (1.0 + p_padding);
	// Step 1: What is the list of islands we need to unwrap? This depends on the mode.
	Vector<PackedInt32Array> islands;
	if (actual_mode == UNWRAP_MODE_EACH_CELL_FILLS || actual_mode == UNWRAP_MODE_TILE_CELLS) {
		islands.resize(cell_count);
		for (int32_t cell_index = 0; cell_index < cell_count; cell_index++) {
			islands.set(cell_index, PackedInt32Array{ cell_index });
		}
	} else if (actual_mode == UNWRAP_MODE_EACH_ISLAND_FILLS || actual_mode == UNWRAP_MODE_TILE_ISLANDS) {
		islands = collect_all_islands();
	} else {
		ERR_FAIL_MSG("ArrayPolyMeshND: Unknown unwrap texture map mode.");
	}
	// Step 2: Unwrap each island individually into an unbounded space.
	for (int64_t island_index = 0; island_index < islands.size(); island_index++) {
		_unwrap_texture_map_island_internal(islands[island_index], p_keep_existing, poly_cell_texture_map);
	}
	// Step 3: Fit or tile the islands into the texture space depending on the mode.
	if (actual_mode == UNWRAP_MODE_EACH_CELL_FILLS || actual_mode == UNWRAP_MODE_EACH_ISLAND_FILLS) {
		// Fit each island into the 0-to-1 texture space with padding.
		const VectorM padded_position = VectorND::fill(texture_dimension, pad_offset);
		const VectorM padded_size = VectorND::fill(texture_dimension, pad_size);
		for (int32_t island_index = 0; island_index < islands.size(); island_index++) {
			_fit_island_texture_map_into_box(islands[island_index], padded_position, padded_size, p_proportional, poly_cell_texture_map);
		}
	} else if (actual_mode == UNWRAP_MODE_TILE_CELLS || actual_mode == UNWRAP_MODE_TILE_ISLANDS) {
		// Tile the islands in texture space by rescaling and offsetting them.
		const VectorMi tiles = _tiles_for_island_count(islands.size(), texture_dimension);
		for (int64_t island_index = 0; island_index < islands.size(); island_index++) {
			VectorM tile_position = VectorND::fill(texture_dimension, 0.0);
			VectorM tile_size = VectorND::fill(texture_dimension, 1.0);
			int64_t remaining_index = island_index;
			for (int64_t axis = 0; axis < texture_dimension; axis++) {
				const double unpadded_tile_size = 1.0 / (double)tiles[axis];
				const int64_t tile_coordinate = remaining_index % tiles[axis];
				remaining_index /= tiles[axis];
				tile_position.set(axis, tile_coordinate * unpadded_tile_size + unpadded_tile_size * pad_offset);
				tile_size.set(axis, unpadded_tile_size * pad_size);
			}
			_fit_island_texture_map_into_box(islands[island_index], tile_position, tile_size, p_proportional, poly_cell_texture_map);
		}
	} else {
		ERR_FAIL_MSG("ArrayPolyMeshND: Unknown unwrap texture map mode.");
	}
	_set_poly_cell_texture_map_dense_internal(poly_cell_texture_map);
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::_fit_island_texture_map_into_box(const PackedInt32Array &p_cells_in_island, const VectorM &p_target_position, const VectorM &p_target_size, const bool p_proportional, Vector<Vector<VectorM>> &r_poly_cell_texture_map) {
	Vector<Vector<VectorM>> &poly_cell_texture_map = r_poly_cell_texture_map;
	ERR_FAIL_COND_MSG(poly_cell_texture_map[p_cells_in_island[0]].is_empty(), "ArrayPolyMeshND: Cannot fit island texture map into a box because at least one cell in the island has an empty texture map.");
	const int64_t texture_dimension = p_target_position.size();
	VectorM minimum = VectorND::with_dimension(poly_cell_texture_map[p_cells_in_island[0]][0], texture_dimension);
	VectorM maximum = VectorND::duplicate(minimum);
	for (int64_t cell_index_index = 0; cell_index_index < p_cells_in_island.size(); cell_index_index++) {
		const int32_t cell_index = p_cells_in_island[cell_index_index];
		const Vector<VectorM> &cell_texture_map = poly_cell_texture_map[cell_index];
		for (int64_t vertex_index = 0; vertex_index < cell_texture_map.size(); vertex_index++) {
			const VectorM &texcoord = cell_texture_map[vertex_index];
			for (int64_t axis = 0; axis < texture_dimension; axis++) {
				const double component = VectorND::get_component(texcoord, axis);
				if (component < minimum[axis]) {
					minimum.set(axis, component);
				}
				if (component > maximum[axis]) {
					maximum.set(axis, component);
				}
			}
		}
	}
	VectorM scale = VectorND::fill(texture_dimension, 1.0);
	for (int64_t axis = 0; axis < texture_dimension; axis++) {
		const double current_size = maximum[axis] - minimum[axis];
		scale.set(axis, current_size < (double)CMP_EPSILON2 ? 1.0 : p_target_size[axis] / current_size);
	}
	if (p_proportional) {
		double min_scale = scale[0];
		for (int64_t axis = 1; axis < texture_dimension; axis++) {
			min_scale = MIN(min_scale, scale[axis]);
		}
		scale = VectorND::fill(texture_dimension, min_scale);
	}
	for (int64_t cell_index_index = 0; cell_index_index < p_cells_in_island.size(); cell_index_index++) {
		const int32_t cell_index = p_cells_in_island[cell_index_index];
		Vector<VectorM> cell_texture_map = poly_cell_texture_map[cell_index];
		for (int64_t vertex_index = 0; vertex_index < cell_texture_map.size(); vertex_index++) {
			VectorM texcoord = VectorND::with_dimension(cell_texture_map[vertex_index], texture_dimension);
			for (int64_t axis = 0; axis < texture_dimension; axis++) {
				texcoord.set(axis, p_target_position[axis] + (texcoord[axis] - minimum[axis]) * scale[axis]);
			}
			cell_texture_map.set(vertex_index, texcoord);
		}
		poly_cell_texture_map.set(cell_index, cell_texture_map);
	}
}

VectorMi ArrayPolyMeshND::_tiles_for_island_count(const int32_t p_island_count, const int64_t p_texture_dimension) {
	VectorMi tiles;
	tiles.resize(p_texture_dimension);
	if (p_island_count < 2) {
		for (int64_t i = 0; i < p_texture_dimension; i++) {
			tiles.set(i, 1);
		}
		return tiles;
	}
	int32_t remaining = p_island_count;
	for (int64_t i = p_texture_dimension - 1; i >= 0; i--) {
		// For the last axis (highest index), take the p_texture_dimension-th root.
		// For the next one, take the (p_texture_dimension - 1)-th root, and so on.
		const double root = std::pow((double)remaining, 1.0 / (double)(i + 1));
		const int32_t value = (int32_t)Math::ceil(root);
		tiles.set(i, value);
		remaining = _ceil_div(remaining, value);
	}
	return tiles;
}

void ArrayPolyMeshND::transform_texture_map(const Ref<TransformND> &p_texture_transform) {
	ERR_FAIL_COND(p_texture_transform.is_null());
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	if (!_all_poly_cell_texture_map_indices.has(cell_to_vert_key)) {
		return;
	}
	// Transform a dense copy of the texture map, then convert it back to indexed data.
	// This ensures that values shared with other data bindings are not affected.
	Vector<Vector<VectorM>> poly_cell_texture_map = _get_poly_cell_texture_map_dense_internal();
	const int64_t cell_count = poly_cell_texture_map.size();
	for (int64_t cell_index = 0; cell_index < cell_count; cell_index++) {
		Vector<VectorM> cell_texture_map = poly_cell_texture_map[cell_index];
		for (int64_t vertex_index = 0; vertex_index < cell_texture_map.size(); vertex_index++) {
			cell_texture_map.set(vertex_index, p_texture_transform->xform(cell_texture_map[vertex_index]));
		}
		poly_cell_texture_map.set(cell_index, cell_texture_map);
	}
	_set_poly_cell_texture_map_dense_internal(poly_cell_texture_map);
	poly_mesh_clear_cache();
}

// Misc functions.

void ArrayPolyMeshND::deduplicate_all_elements() {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot deduplicate elements of an invalid mesh.");
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	const Vector2i per_cell_key = _get_per_cell_key();
	const bool has_boundary_cells = boundary_dim_index >= 0 && _poly_cell_indices.size() > boundary_dim_index;
	// We need to ensure the boundary normals stay the same before and after deduplication,
	// which means we need to start with boundary normals calculated from the original data.
	if (has_boundary_cells && (!_all_poly_cell_normal_indices.has(per_cell_key) || _all_poly_cell_normal_indices[per_cell_key].is_empty() || _all_poly_cell_normal_indices[per_cell_key][0].size() != _poly_cell_indices[boundary_dim_index].size())) {
		calculate_boundary_normals();
	}
	// Both data binding maps have the same structure, so most of the code below can process them in a loop.
	HashMap<Vector2i, Vector<PackedInt32Array>> *data_binding_maps[2] = { &_all_poly_cell_normal_indices, &_all_poly_cell_texture_map_indices };
	// Snapshot the sub-element traversal order for all decomposed bindings BEFORE any deduplication.
	// Deduplication can change sub-element ordering within cells (e.g., edge deduplication can reverse
	// an edge's stored vertex order), breaking data indexed by traversal position.
	HashMap<Vector2i, Vector<PackedInt32Array>> pre_dedup_poly;
	for (HashMap<Vector2i, Vector<PackedInt32Array>> *data_binding_map : data_binding_maps) {
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &pre_kv : *data_binding_map) {
			const Vector2i pre_key = pre_kv.key;
			if (!pre_kv.value.is_empty() && pre_key.x != pre_key.y && !pre_dedup_poly.has(pre_key)) {
				pre_dedup_poly.insert(pre_key, get_all_poly_cell_poly_indices(pre_key.x, pre_key.y));
			}
		}
	}
	// Deduplicate vertices.
	Vector<VectorN> output_vertices;
	HashMap<int32_t, int32_t> vertex_index_remap;
	for (int64_t input_vertex_index = 0; input_vertex_index < _poly_cell_vertex_positions.size(); input_vertex_index++) {
		const VectorN vertex = _poly_cell_vertex_positions[input_vertex_index];
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
			output_vertices.append(vertex);
		}
	}
	// Update edges that reference those vertices.
	for (int64_t edge_index = 0; edge_index < _edge_vertex_indices.size(); edge_index++) {
		const int64_t input_vertex_index = _edge_vertex_indices[edge_index];
		_edge_vertex_indices.set(edge_index, vertex_index_remap[input_vertex_index]);
	}
	// Deduplicate edges.
	PackedInt32Array output_edge_vertex_indices;
	HashMap<int32_t, int32_t> edge_index_remap;
	for (int64_t input_edge_index = 0; input_edge_index < _edge_vertex_indices.size(); input_edge_index += 2) {
		const int32_t vertex_index_a = _edge_vertex_indices[input_edge_index];
		const int32_t vertex_index_b = _edge_vertex_indices[input_edge_index + 1];
		bool found_duplicate = false;
		for (int64_t output_edge_index = 0; output_edge_index < output_edge_vertex_indices.size(); output_edge_index += 2) {
			const int32_t output_vertex_index_a = output_edge_vertex_indices[output_edge_index];
			const int32_t output_vertex_index_b = output_edge_vertex_indices[output_edge_index + 1];
			// Deduplicate edges in the same order and in the opposite order.
			// Both orders should be considered the same edge in the PolyMeshND code.
			if ((vertex_index_a == output_vertex_index_a && vertex_index_b == output_vertex_index_b) ||
					(vertex_index_a == output_vertex_index_b && vertex_index_b == output_vertex_index_a)) {
				edge_index_remap[input_edge_index / 2] = output_edge_index / 2;
				found_duplicate = true;
				break;
			}
		}
		if (!found_duplicate) {
			edge_index_remap[input_edge_index / 2] = output_edge_vertex_indices.size() / 2;
			output_edge_vertex_indices.append(vertex_index_a);
			output_edge_vertex_indices.append(vertex_index_b);
		}
	}
	// Deduplicate poly cell indices.
	Vector<Vector<PackedInt32Array>> output_poly_cell_indices;
	Vector<HashMap<int32_t, int32_t>> poly_cell_index_remaps;
	for (int64_t dim_index = 0; dim_index < _poly_cell_indices.size(); dim_index++) {
		Vector<PackedInt32Array> dim_output;
		Vector<PackedInt32Array> dim_output_sorted;
		HashMap<int32_t, int32_t> dim_index_remap;
		const HashMap<int32_t, int32_t> &prev_index_remap = (dim_index == 0) ? edge_index_remap : poly_cell_index_remaps[dim_index - 1];
		Vector<PackedInt32Array> input_cells = _poly_cell_indices[dim_index];
		for (int64_t input_cell_index = 0; input_cell_index < input_cells.size(); input_cell_index++) {
			PackedInt32Array cell = input_cells[input_cell_index];
			// Remap the indices in the cell based on the previous remap.
			for (int64_t i = 0; i < cell.size(); i++) {
				cell.set(i, prev_index_remap[cell[i]]);
			}
			// Deduplicate cells regardless of the order of the indices in the cell.
			PackedInt32Array cell_sorted = PackedInt32Array(cell); // Copy.
			cell_sorted.sort();
			bool found_duplicate = false;
			for (int64_t output_cell_index = 0; output_cell_index < dim_output.size(); output_cell_index++) {
				if (cell_sorted == dim_output_sorted[output_cell_index]) {
					dim_index_remap[input_cell_index] = output_cell_index;
					found_duplicate = true;
					break;
				}
			}
			if (!found_duplicate) {
				dim_index_remap[input_cell_index] = dim_output.size();
				dim_output.append(cell);
				dim_output_sorted.append(cell_sorted);
			}
		}
		output_poly_cell_indices.append(dim_output);
		poly_cell_index_remaps.append(HashMap<int32_t, int32_t>(dim_index_remap));
	}
	// Write back deduplicated geometry now so that get_all_poly_cell_poly_indices reads the new arrays
	// when computing the post-dedup sub-element orderings for remapping data bindings.
	const int64_t input_boundary_cell_count = has_boundary_cells ? _poly_cell_indices[boundary_dim_index].size() : 0;
	_poly_cell_vertex_positions = output_vertices;
	_edge_vertex_indices = output_edge_vertex_indices;
	_poly_cell_indices = output_poly_cell_indices;
	// Snapshot the sub-element traversal order for all decomposed bindings AFTER deduplication.
	HashMap<Vector2i, Vector<PackedInt32Array>> post_dedup_poly;
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &post_kv : pre_dedup_poly) {
		post_dedup_poly.insert(post_kv.key, get_all_poly_cell_poly_indices(post_kv.key.x, post_kv.key.y));
	}
	// Update boundary pivot overrides based on boundary cell remap, then remap pivot vertices.
	PackedInt32Array output_poly_cell_boundary_pivot_overrides;
	if (!_poly_cell_boundary_pivot_overrides.is_empty() && input_boundary_cell_count > 0 && output_poly_cell_indices.size() > boundary_dim_index) {
		const int64_t output_boundary_cell_count = output_poly_cell_indices[boundary_dim_index].size();
		const int64_t input_pivot_count = MIN(_poly_cell_boundary_pivot_overrides.size(), input_boundary_cell_count);
		output_poly_cell_boundary_pivot_overrides.resize(output_boundary_cell_count);
		for (int64_t i = 0; i < output_boundary_cell_count; i++) {
			output_poly_cell_boundary_pivot_overrides.set(i, -1);
		}
		const HashMap<int32_t, int32_t> &boundary_cell_index_remap = poly_cell_index_remaps[boundary_dim_index];
		for (int64_t input_cell_index = 0; input_cell_index < input_pivot_count; input_cell_index++) {
			const int32_t input_pivot_vertex_index = _poly_cell_boundary_pivot_overrides[input_cell_index];
			if (input_pivot_vertex_index < 0) {
				continue;
			}
			const int32_t output_cell_index = boundary_cell_index_remap[input_cell_index];
			if (output_poly_cell_boundary_pivot_overrides[output_cell_index] == -1) {
				output_poly_cell_boundary_pivot_overrides.set(output_cell_index, vertex_index_remap[input_pivot_vertex_index]);
			}
		}
	}
	// Update seam indices based on the boundary member remap.
	HashSet<int32_t> output_seam_indices;
	if (_seam_indices.size() > 0 && boundary_dim_index >= 0) {
		const HashMap<int32_t, int32_t> &member_index_remap = (boundary_dim_index == 0) ? edge_index_remap : ((poly_cell_index_remaps.size() > boundary_dim_index - 1) ? poly_cell_index_remaps[boundary_dim_index - 1] : edge_index_remap);
		for (const int32_t seam_index : _seam_indices) {
			if (member_index_remap.has(seam_index)) {
				output_seam_indices.insert(member_index_remap[seam_index]);
			}
		}
	}
	// Update the poly cell data bindings (normal indices and texture map indices).
	// The bindings reference the geometry elements by position, so they need remapping,
	// but the value pools they index into are unaffected by geometry deduplication.
	for (HashMap<Vector2i, Vector<PackedInt32Array>> *data_binding_map : data_binding_maps) {
		HashMap<Vector2i, Vector<PackedInt32Array>> output_data_bindings;
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : *data_binding_map) {
			if (kv.value.is_empty() || (kv.key.x == kv.key.y && kv.value[0].is_empty())) {
				output_data_bindings.insert(kv.key, kv.value);
				continue;
			}
			const Vector2i key = kv.key;
			ERR_CONTINUE_MSG((key.x - 2) >= poly_cell_index_remaps.size(), "ArrayPolyMeshND: Invalid data binding for geometry dimension " + itos(key.x) + ". Skipping.");
			const HashMap<int32_t, int32_t> &index_remap = (key.x == 0) ? vertex_index_remap : ((key.x == 1) ? edge_index_remap : poly_cell_index_remaps[key.x - 2]);
			const Vector<PackedInt32Array> &input_data = kv.value;
			Vector<PackedInt32Array> output_data;
			if (key.y == key.x && input_data.size() == 1) {
				// Non-decomposed data uses a flat structure, with one value index per element.
				const PackedInt32Array &input_flat_indices = input_data[0];
				PackedInt32Array output_flat_indices;
				for (int64_t input_index = 0; input_index < input_flat_indices.size(); input_index++) {
					const int64_t output_index = index_remap[input_index];
					// New items should be contiguously added in the same order as the input,
					// so the next output index should never be greater than the current output size.
					CRASH_COND(output_index > output_flat_indices.size());
					// When deduplicating, we only want the first copy's attached data,
					// so that it pairs with the geometry that was kept.
					if (output_index == output_flat_indices.size()) {
						output_flat_indices.resize(output_index + 1);
						output_flat_indices.set(output_index, input_flat_indices[input_index]);
					}
				}
				output_data = { output_flat_indices };
			} else {
				for (int64_t input_index = 0; input_index < input_data.size(); input_index++) {
					const int64_t output_index = index_remap[input_index];
					CRASH_COND(output_index > output_data.size());
					if (output_index == output_data.size()) {
						output_data.resize(output_index + 1);
						PackedInt32Array cell_value_indices = input_data[input_index];
						// Remap inner positions if sub-element ordering changed due to deduplication.
						if (pre_dedup_poly.has(key) && input_index < pre_dedup_poly[key].size() && output_index < post_dedup_poly[key].size()) {
							const PackedInt32Array &old_elems = pre_dedup_poly[key][input_index];
							const PackedInt32Array &new_elems = post_dedup_poly[key][output_index];
							if (!cell_value_indices.is_empty()) {
								ERR_CONTINUE_MSG(key.y >= 2 && (key.y - 2) >= poly_cell_index_remaps.size(), "ArrayPolyMeshND: Invalid data binding sub-element dimension " + itos(key.y) + ". Skipping remap.");
								const HashMap<int32_t, int32_t> &subelement_remap = (key.y == 0) ? vertex_index_remap : ((key.y == 1) ? edge_index_remap : poly_cell_index_remaps[key.y - 2]);
								PackedInt32Array remapped_value_indices;
								remapped_value_indices.resize(new_elems.size());
								for (int64_t new_pos = 0; new_pos < new_elems.size(); new_pos++) {
									const int32_t new_elem = new_elems[new_pos];
									bool found = false;
									for (int64_t old_pos = 0; old_pos < old_elems.size(); old_pos++) {
										if (subelement_remap[old_elems[old_pos]] == new_elem) {
											remapped_value_indices.set(new_pos, cell_value_indices[old_pos]);
											found = true;
											break;
										}
									}
									ERR_FAIL_COND_MSG(!found, vformat("ArrayPolyMeshND::deduplicate_all_elements: Failed to remap data binding for cell %d (new sub-element %d not found in pre-dedup traversal).", input_index, new_elem));
								}
								cell_value_indices = remapped_value_indices;
							}
						}
						output_data.set(output_index, cell_value_indices);
					}
				}
			}
			output_data_bindings.insert(key, output_data);
		}
		*data_binding_map = output_data_bindings;
	}
	// Write back the remaining deduplicated data (geometry and data bindings were already written back earlier).
	_poly_cell_boundary_pivot_overrides = output_poly_cell_boundary_pivot_overrides;
	_seam_indices = output_seam_indices;
	if (has_boundary_cells && output_poly_cell_indices.size() > boundary_dim_index) {
		// Ensure boundary normals stay consistent with the geometry after deduplication,
		// since the reordering can cause them to become flipped. This is done by swapping
		// the first two members in any cell whose normal was flipped. This may also affect
		// the binding of other decomposed elements, so those need to be resampled as well.
		const int64_t dimension = get_dimension();
		HashMap<Vector2i, Vector<PackedInt32Array>> remapped_poly_poly;
		for (HashMap<Vector2i, Vector<PackedInt32Array>> *data_binding_map : data_binding_maps) {
			for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : *data_binding_map) {
				const Vector2i key = kv.key;
				if (kv.value.is_empty() || remapped_poly_poly.has(key)) {
					continue; // Already tracked.
				}
				if (key.x < dimension - 1 || key.y >= dimension - 1) {
					continue; // Swapping won't affect the binding of these elements, so we can skip remapping them.
				}
				remapped_poly_poly.insert(key, get_all_poly_cell_poly_indices(key.x, key.y));
			}
		}
		// Now actually check on the boundary cells and see if their first two elements need swapping.
		const Vector<VectorN> remapped_boundary_normals = _all_poly_cell_normal_indices.has(per_cell_key) && !_all_poly_cell_normal_indices[per_cell_key].is_empty() ? _sample_normal_values_internal(_all_poly_cell_normal_indices[per_cell_key][0]) : Vector<VectorN>();
		calculate_boundary_normals();
		const Vector<VectorN> recalculated_boundary_normals = _sample_normal_values_internal(_all_poly_cell_normal_indices[per_cell_key][0]);
		Vector<PackedInt32Array> all_cell_member_indices = output_poly_cell_indices[boundary_dim_index];
		for (int64_t cell_index = 0; cell_index < recalculated_boundary_normals.size(); cell_index++) {
			if (cell_index < remapped_boundary_normals.size() && VectorND::dot(remapped_boundary_normals[cell_index], recalculated_boundary_normals[cell_index]) < 0) {
				// Flip the cell's orientation to flip the normal.
				PackedInt32Array cell_member_indices = all_cell_member_indices[cell_index];
				CRASH_COND(cell_member_indices.size() < 2);
				flip_poly_cell_orientation(cell_member_indices, boundary_dim_index);
				all_cell_member_indices.set(cell_index, cell_member_indices);
			}
		}
		output_poly_cell_indices.set(boundary_dim_index, all_cell_member_indices);
		_poly_cell_indices = output_poly_cell_indices;
		calculate_boundary_normals();
		// Resample the bindings of any decomposed elements that were affected by the swap.
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : remapped_poly_poly) {
			const Vector2i key = kv.key;
			const Vector<PackedInt32Array> &remapped_poly = kv.value;
			const Vector<PackedInt32Array> recalculated_poly = get_all_poly_cell_poly_indices(key.x, key.y);
			CRASH_COND(remapped_poly.size() != recalculated_poly.size());
			for (HashMap<Vector2i, Vector<PackedInt32Array>> *data_binding_map : data_binding_maps) {
				if (!data_binding_map->has(key)) {
					continue;
				}
				Vector<PackedInt32Array> data_bindings = (*data_binding_map)[key];
				if (data_bindings.is_empty()) {
					continue; // An empty binding means no data for this key, so there is nothing to resample.
				}
				CRASH_COND(data_bindings.size() > remapped_poly.size());
				for (int64_t cell_index = 0; cell_index < data_bindings.size(); cell_index++) {
					const PackedInt32Array &remapped_cell_poly = remapped_poly[cell_index];
					const PackedInt32Array &recalculated_cell_poly = recalculated_poly[cell_index];
					if (data_bindings[cell_index].is_empty() || remapped_cell_poly == recalculated_cell_poly) {
						continue; // This cell's poly didn't change, so its binding is still correct.
					}
					CRASH_COND(remapped_cell_poly.size() != recalculated_cell_poly.size());
					// This cell's poly changed, so we need to resample the data binding.
					const PackedInt32Array &old_cell_value_indices = data_bindings[cell_index];
					if (old_cell_value_indices.is_empty()) {
						continue; // Cells without this attribute have nothing to reorder.
					}
					PackedInt32Array new_cell_value_indices;
					new_cell_value_indices.resize(old_cell_value_indices.size());
					for (int64_t elem_index = 0; elem_index < remapped_cell_poly.size(); elem_index++) {
						const int64_t search_element = remapped_cell_poly[elem_index];
						const int64_t dest_index = recalculated_cell_poly.find(search_element);
						new_cell_value_indices.set(dest_index, old_cell_value_indices[elem_index]);
					}
					data_bindings.set(cell_index, new_cell_value_indices);
				}
				data_binding_map->insert(key, data_bindings);
			}
		}
	} else {
		_poly_cell_indices = output_poly_cell_indices;
	}
	// Deduplication can leave the value pools with unreferenced values, so compact them.
	_compact_normal_values_internal();
	_compact_texture_map_values_internal();
	poly_mesh_clear_cache(false);
}

void ArrayPolyMeshND::transform_vertices(const Ref<TransformND> &p_transform) {
	ERR_FAIL_COND(p_transform.is_null());
	const int64_t vertex_pos_count = _poly_cell_vertex_positions.size();
	for (int64_t vertex_index = 0; vertex_index < vertex_pos_count; vertex_index++) {
		_poly_cell_vertex_positions.set(vertex_index, p_transform->xform(_poly_cell_vertex_positions[vertex_index]));
	}
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::merge_with(const Ref<PolyMeshND> &p_other, const Ref<TransformND> &p_transform) {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: This mesh is invalid, cannot merge another mesh into it.");
	ERR_FAIL_COND_MSG(p_other.is_null() || !p_other->is_mesh_data_valid(), "ArrayPolyMeshND: Cannot merge an invalid PolyMeshND into this mesh.");
	if (p_other.ptr() == this) {
		const Ref<ArrayPolyMeshND> source = duplicate();
		merge_with(source, p_transform);
		return;
	}
	const bool has_transform = p_transform.is_valid();
	const Vector<Vector<PackedInt32Array>> other_poly_cell_indices = p_other->get_poly_cell_indices();
	const Vector<VectorN> other_poly_cell_vertices = p_other->get_poly_cell_vertex_positions();
	const PackedInt32Array other_poly_cell_boundary_pivot_overrides = p_other->get_poly_cell_boundary_pivot_overrides();
	const PackedInt32Array other_edge_indices = p_other->get_edge_indices();
	const HashSet<int32_t> other_seam_indices = p_other->get_seam_indices();
	const int64_t start_vertex_pos_count = _poly_cell_vertex_positions.size();
	const int64_t start_edge_index_count = _edge_vertex_indices.size();
	const int64_t other_vertex_pos_count = other_poly_cell_vertices.size();
	const int64_t other_edge_index_count = other_edge_indices.size();
	const int64_t end_vertex_pos_count = start_vertex_pos_count + other_vertex_pos_count;
	const int64_t end_edge_index_count = start_edge_index_count + other_edge_index_count;
	// Expand the poly cell indices dimensions if needed and count how many entries are in each dimension.
	const int64_t poly_cell_indices_dims = MAX(_poly_cell_indices.size(), other_poly_cell_indices.size());
	if (poly_cell_indices_dims > _poly_cell_indices.size()) {
		_poly_cell_indices.resize(poly_cell_indices_dims);
	}
	PackedInt32Array start_poly_cell_indices_counts;
	start_poly_cell_indices_counts.resize(poly_cell_indices_dims);
	for (int64_t dim_index = 0; dim_index < poly_cell_indices_dims; dim_index++) {
		start_poly_cell_indices_counts.set(dim_index, _poly_cell_indices[dim_index].size());
	}
	PackedInt32Array other_poly_cell_indices_counts;
	other_poly_cell_indices_counts.resize(poly_cell_indices_dims);
	for (int64_t dim_index = 0; dim_index < poly_cell_indices_dims; dim_index++) {
		if (dim_index < other_poly_cell_indices.size()) {
			other_poly_cell_indices_counts.set(dim_index, other_poly_cell_indices[dim_index].size());
		} else {
			other_poly_cell_indices_counts.set(dim_index, 0);
		}
	}
	// Merge vertices.
	_poly_cell_vertex_positions.resize(end_vertex_pos_count);
	for (int64_t i = 0; i < other_vertex_pos_count; i++) {
		_poly_cell_vertex_positions.set(start_vertex_pos_count + i, has_transform ? p_transform->xform(other_poly_cell_vertices[i]) : other_poly_cell_vertices[i]);
	}
	// The dimension and data binding keys of the merged mesh, now that the vertices are merged.
	const int64_t dimension = _poly_cell_vertex_positions.is_empty() ? 0 : _poly_cell_vertex_positions[0].size();
	const int64_t boundary_dim_index = dimension - 3;
	const Vector2i per_cell_key = Vector2i(dimension - 1, dimension - 1);
	const Vector2i cell_to_vert_key = Vector2i(dimension - 1, 0);
	// Merge edges.
	_edge_vertex_indices.resize(end_edge_index_count);
	for (int64_t i = 0; i < other_edge_index_count; i += 2) {
		_edge_vertex_indices.set(start_edge_index_count + i, other_edge_indices[i] + int32_t(start_vertex_pos_count));
		_edge_vertex_indices.set(start_edge_index_count + i + 1, other_edge_indices[i + 1] + int32_t(start_vertex_pos_count));
	}
	// Compute cell vertex instances for the original cells before merging poly cell indices.
	// This must happen before the merge so the result only spans the original cells,
	// which is required later when filling in missing boundary normals.
	Vector<PackedInt32Array> cell_vertex_instances_span_first;
	if (boundary_dim_index >= 0 && boundary_dim_index < _poly_cell_indices.size()) {
		cell_vertex_instances_span_first = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, true);
	}
	// Merge poly cell indices.
	for (int64_t dim_index = 0; dim_index < poly_cell_indices_dims; dim_index++) {
		const int64_t other_count = other_poly_cell_indices_counts[dim_index];
		if (other_count == 0) {
			// The other mesh has nothing in this dimension, and may not even have this
			// dimension at all, in which case indexing other_poly_cell_indices would crash.
			continue;
		}
		Vector<PackedInt32Array> this_dim = _poly_cell_indices[dim_index];
		const Vector<PackedInt32Array> &other_dim = other_poly_cell_indices[dim_index];
		const int64_t start_count = start_poly_cell_indices_counts[dim_index];
		const int64_t end_count = start_count + other_count;
		const int32_t adjust_items = (dim_index == 0) ? (start_edge_index_count / 2) : start_poly_cell_indices_counts[dim_index - 1];
		this_dim.resize(end_count);
		for (int64_t other_cell_index = 0; other_cell_index < other_count; other_cell_index++) {
			PackedInt32Array other_cell_copy = PackedInt32Array(other_dim[other_cell_index]); // Copy.
			for (int64_t i = 0; i < other_cell_copy.size(); i++) {
				other_cell_copy.set(i, other_cell_copy[i] + adjust_items);
			}
			this_dim.set(start_count + other_cell_index, other_cell_copy);
		}
		_poly_cell_indices.set(dim_index, this_dim);
	}
	// Merge pivot overrides. Note: Pivot overrides on a mesh without boundary cells are
	// invalid data, so they are ignored here rather than indexing a non-existent dimension.
	if (boundary_dim_index >= 0 && poly_cell_indices_dims > boundary_dim_index && (!_poly_cell_boundary_pivot_overrides.is_empty() || !other_poly_cell_boundary_pivot_overrides.is_empty())) {
		const int64_t start_required_amount = start_poly_cell_indices_counts[boundary_dim_index];
		const int64_t other_required_amount = other_poly_cell_indices_counts[boundary_dim_index];
		const int64_t start_current_amount = _poly_cell_boundary_pivot_overrides.size();
		_poly_cell_boundary_pivot_overrides.resize(start_required_amount + other_required_amount);
		for (int64_t i = start_current_amount; i < start_required_amount; i++) {
			_poly_cell_boundary_pivot_overrides.set(i, -1);
		}
		for (int64_t i = 0; i < other_poly_cell_boundary_pivot_overrides.size(); i++) {
			// A pivot override of -1 means the cell has no override, which must not be offset.
			const int32_t other_pivot = other_poly_cell_boundary_pivot_overrides[i];
			const int32_t new_pivot = other_pivot < 0 ? -1 : other_pivot + int32_t(start_vertex_pos_count);
			_poly_cell_boundary_pivot_overrides.set(start_required_amount + i, new_pivot);
		}
		for (int64_t i = start_required_amount + other_poly_cell_boundary_pivot_overrides.size(); i < _poly_cell_boundary_pivot_overrides.size(); i++) {
			_poly_cell_boundary_pivot_overrides.set(i, -1);
		}
	}
	// Merge seams. Seam indices refer to the members of boundary cells, which are edges for 3D meshes.
	if (!other_seam_indices.is_empty()) {
		if (boundary_dim_index == 0) {
			const int32_t adjust_seam_member = start_edge_index_count / 2;
			for (const int32_t seam_index : other_seam_indices) {
				_seam_indices.insert(seam_index + adjust_seam_member);
			}
		} else if (boundary_dim_index > 0 && poly_cell_indices_dims > boundary_dim_index - 1) {
			const int32_t adjust_seam_member = start_poly_cell_indices_counts[boundary_dim_index - 1];
			for (const int32_t seam_index : other_seam_indices) {
				_seam_indices.insert(seam_index + adjust_seam_member);
			}
		} else {
			WARN_PRINT("ArrayPolyMeshND: Ignoring seam indices while merging because there is no boundary member dimension in the merged poly cell indices.");
		}
	} else if (boundary_dim_index == 0 && !_seam_indices.is_empty()) {
		// Existing seams on edges stay valid because the other mesh's edges are appended after.
	}
	// Merge all normals and texture maps from the HashMaps.
	// This requires the other mesh be ArrayPolyMeshND and the cell vertex instances be calculated.
	Ref<ArrayPolyMeshND> other_array_mesh = p_other;
	if (other_array_mesh.is_null()) {
		other_array_mesh = p_other->to_array_poly_mesh();
	}
	// Merge the value pools first, remembering how the other mesh's value indices map into this
	// mesh's pools. The other mesh's normal values need to be transformed by the merge transform.
	const Vector<VectorN> &other_normal_values = other_array_mesh->_poly_cell_normal_values;
	PackedInt32Array other_normal_value_remap;
	other_normal_value_remap.resize(other_normal_values.size());
	for (int64_t i = 0; i < other_normal_values.size(); i++) {
		const VectorN other_normal = has_transform ? p_transform->xform_basis(other_normal_values[i]) : other_normal_values[i];
		other_normal_value_remap.set(i, (int32_t)VectorND::array_append_deduplicate(_poly_cell_normal_values, other_normal));
	}
	const Vector<VectorM> &other_texture_map_values = other_array_mesh->_poly_cell_texture_map_values;
	PackedInt32Array other_texture_map_value_remap;
	other_texture_map_value_remap.resize(other_texture_map_values.size());
	for (int64_t i = 0; i < other_texture_map_values.size(); i++) {
		const int64_t texture_map_index = VectorND::array_append_deduplicate(_poly_cell_texture_map_values, other_texture_map_values[i]);
		other_texture_map_value_remap.set(i, (int32_t)texture_map_index);
	}
	const HashMap<Vector2i, Vector<PackedInt32Array>> other_poly_cell_normal_indices = other_array_mesh->get_all_poly_cell_normal_indices();
	Vector<VectorN> boundary_normals_cache;
	// Merge all normals.
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &other_normals_kv : other_poly_cell_normal_indices) {
		const Vector2i key = other_normals_kv.key;
		const Vector<PackedInt32Array> &other_normals_data = other_normals_kv.value;
		if (other_normals_data.is_empty() || (key.y == key.x && other_normals_data[0].is_empty())) {
			// Skip before inserting the key below, so an empty binding is not left in the map.
			continue;
		}
		// This usage of HashMap's indexing operator writes to the map if missing.
		Vector<PackedInt32Array> &merged_normals = _all_poly_cell_normal_indices[key];
		if (key.y == key.x && merged_normals.is_empty()) {
			// Non-decomposed normals use a flat structure, which we need to ensure is populated.
			merged_normals.append(PackedInt32Array());
		}
		// Figure out how many entries are needed in this dimension based on the key.
		// Use pre-merge counts to check if the original mesh was missing entries.
		int64_t start_cell_count_for_geom_dim = 0;
		if (key.x == 0) {
			start_cell_count_for_geom_dim = start_vertex_pos_count;
		} else if (key.x == 1) {
			start_cell_count_for_geom_dim = start_edge_index_count / 2;
		} else if (key.x > 1 && (key.x - 2) < poly_cell_indices_dims) {
			start_cell_count_for_geom_dim = start_poly_cell_indices_counts[key.x - 2];
		}
		bool missing_entries = false;
		if (key.y == key.x) {
			// Non-decomposed normals use a flat structure, so read the count from the first array.
			missing_entries = merged_normals[0].size() < start_cell_count_for_geom_dim;
		} else {
			// Decomposed normals use a nested structure, so read the count from the outer array.
			missing_entries = merged_normals.size() < start_cell_count_for_geom_dim;
		}
		if (missing_entries) {
			if (boundary_dim_index >= 0 && poly_cell_indices_dims > boundary_dim_index && key == per_cell_key) {
				// Special case: Generate missing boundary normals if needed.
				if (merged_normals[0].size() < start_poly_cell_indices_counts[boundary_dim_index]) {
					if (boundary_normals_cache.is_empty()) {
						boundary_normals_cache = _compute_boundary_normals_based_on_cell_orientation(cell_vertex_instances_span_first, true);
						CRASH_COND(boundary_normals_cache.size() != start_poly_cell_indices_counts[boundary_dim_index]);
					}
					merged_normals.set(0, _normal_indices_for_values_internal(boundary_normals_cache));
				}
			} else if (boundary_dim_index >= 0 && poly_cell_indices_dims > boundary_dim_index && key == cell_to_vert_key) {
				// Special case: Generate missing vertex normals if needed.
				if (merged_normals.size() < start_poly_cell_indices_counts[boundary_dim_index]) {
					if (boundary_normals_cache.is_empty()) {
						boundary_normals_cache = _compute_boundary_normals_based_on_cell_orientation(cell_vertex_instances_span_first, true);
						CRASH_COND(boundary_normals_cache.size() != start_poly_cell_indices_counts[boundary_dim_index]);
					}
					// Set flat shading normals for all cells without vertex normals.
					const int64_t start_count = merged_normals.size();
					merged_normals.resize(start_poly_cell_indices_counts[boundary_dim_index]);
					for (int64_t cell_index = start_count; cell_index < start_poly_cell_indices_counts[boundary_dim_index]; cell_index++) {
						PackedInt32Array normal_indices_for_cell;
						normal_indices_for_cell.resize(cell_vertex_instances_span_first[cell_index].size());
						const int32_t cell_boundary_normal_index = (int32_t)VectorND::array_append_deduplicate(_poly_cell_normal_values, boundary_normals_cache[cell_index]);
						for (int64_t vert_inst = 0; vert_inst < normal_indices_for_cell.size(); vert_inst++) {
							normal_indices_for_cell.set(vert_inst, cell_boundary_normal_index);
						}
						merged_normals.set(cell_index, normal_indices_for_cell);
					}
				}
			} else {
				WARN_PRINT("ArrayPolyMeshND: The original mesh was missing normal entries for geometry dimension " + itos(key.x) + " and decomposition dimension " + itos(key.y) + ", but the other mesh has entries for this key. Filling missing entries with empty data while merging. Consider updating the original mesh with this data before merging to avoid this warning in the future.");
				if (key.y == key.x) {
					// Fill missing non-decomposed normals with a zero normal value.
					const int32_t zero_normal_value_index = (int32_t)VectorND::array_append_deduplicate(_poly_cell_normal_values, VectorN());
					PackedInt32Array flat_merged_normals = merged_normals[0];
					const int64_t old_flat_count = flat_merged_normals.size();
					flat_merged_normals.resize(start_cell_count_for_geom_dim);
					for (int64_t i = old_flat_count; i < start_cell_count_for_geom_dim; i++) {
						flat_merged_normals.set(i, zero_normal_value_index);
					}
					merged_normals.set(0, flat_merged_normals);
				} else {
					// Fill missing decomposed normals with empty arrays.
					merged_normals.resize(start_cell_count_for_geom_dim);
				}
			}
		}
		// Merge normals by remapping the other mesh's value indices, and insert into the map.
		if (key.y == key.x) {
			// Non-decomposed normals use a flat structure, so we need to merge them into one array.
			PackedInt32Array flat_merged_normals = merged_normals[0];
			const PackedInt32Array &other_flat_normals = other_normals_data[0];
			const int64_t start_count = flat_merged_normals.size();
			flat_merged_normals.resize(start_count + other_flat_normals.size());
			for (int64_t cell_index = 0; cell_index < other_flat_normals.size(); cell_index++) {
				const int32_t other_value_index = other_flat_normals[cell_index];
				ERR_CONTINUE(other_value_index < 0 || other_value_index >= other_normal_value_remap.size());
				flat_merged_normals.set(start_count + cell_index, other_normal_value_remap[other_value_index]);
			}
			merged_normals.set(0, flat_merged_normals);
		} else {
			// Decomposed normals use a nested structure, so we just append the arrays for each cell.
			const int64_t start_count = merged_normals.size();
			merged_normals.resize(start_count + other_normals_data.size());
			for (int64_t cell_index = 0; cell_index < other_normals_data.size(); cell_index++) {
				PackedInt32Array remapped_cell_normal_indices = other_normals_data[cell_index];
				for (int64_t vert_inst = 0; vert_inst < remapped_cell_normal_indices.size(); vert_inst++) {
					const int32_t other_value_index = remapped_cell_normal_indices[vert_inst];
					ERR_CONTINUE(other_value_index < 0 || other_value_index >= other_normal_value_remap.size());
					remapped_cell_normal_indices.set(vert_inst, other_normal_value_remap[other_value_index]);
				}
				merged_normals.set(start_count + cell_index, remapped_cell_normal_indices);
			}
		}
	}
	// Merge all texture maps.
	const HashMap<Vector2i, Vector<PackedInt32Array>> other_poly_cell_texture_map_indices = other_array_mesh->get_all_poly_cell_texture_map_indices();
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &other_tex_map_kv : other_poly_cell_texture_map_indices) {
		const Vector2i key = other_tex_map_kv.key;
		const Vector<PackedInt32Array> &other_texture_map_data = other_tex_map_kv.value;
		if (other_texture_map_data.is_empty() || (key.y == key.x && other_texture_map_data[0].is_empty())) {
			// Skip before inserting the key below, so an empty binding is not left in the map.
			continue;
		}
		// This usage of HashMap's indexing operator writes to the map if missing.
		Vector<PackedInt32Array> &merged_texture_map = _all_poly_cell_texture_map_indices[key];
		if (key.y == key.x && merged_texture_map.is_empty()) {
			// Non-decomposed texture maps use a flat structure, which we need to ensure is populated.
			merged_texture_map.append(PackedInt32Array());
		}
		// Figure out how many entries are needed in this dimension based on the key.
		int64_t start_cell_count_for_geom_dim = 0;
		if (key.x == 0) {
			start_cell_count_for_geom_dim = start_vertex_pos_count;
		} else if (key.x == 1) {
			start_cell_count_for_geom_dim = start_edge_index_count / 2;
		} else if (key.x > 1 && (key.x - 2) < poly_cell_indices_dims) {
			start_cell_count_for_geom_dim = start_poly_cell_indices_counts[key.x - 2];
		}
		bool missing_entries = false;
		if (key.y == key.x) {
			missing_entries = merged_texture_map[0].size() < start_cell_count_for_geom_dim;
		} else {
			missing_entries = merged_texture_map.size() < start_cell_count_for_geom_dim;
		}
		if (missing_entries) {
			// For texture maps, we just pad with empty data if needed, no generation logic.
			if (key.y == key.x) {
				// Fill missing non-decomposed texture maps with a zero texture map value.
				const int32_t zero_texture_map_value_index = (int32_t)VectorND::array_append_deduplicate(_poly_cell_texture_map_values, VectorM());
				PackedInt32Array flat_merged_texture_map = merged_texture_map[0];
				const int64_t old_flat_count = flat_merged_texture_map.size();
				flat_merged_texture_map.resize(start_cell_count_for_geom_dim);
				for (int64_t i = old_flat_count; i < start_cell_count_for_geom_dim; i++) {
					flat_merged_texture_map.set(i, zero_texture_map_value_index);
				}
				merged_texture_map.set(0, flat_merged_texture_map);
			} else {
				merged_texture_map.resize(start_cell_count_for_geom_dim);
			}
		}
		// Merge texture maps by remapping the other mesh's value indices, and insert into the map.
		if (key.y == key.x) {
			PackedInt32Array flat_merged_texture_map = merged_texture_map[0];
			const PackedInt32Array &other_flat_texture_map = other_texture_map_data[0];
			const int64_t start_count = flat_merged_texture_map.size();
			flat_merged_texture_map.resize(start_count + other_flat_texture_map.size());
			for (int64_t cell_index = 0; cell_index < other_flat_texture_map.size(); cell_index++) {
				const int32_t other_value_index = other_flat_texture_map[cell_index];
				ERR_CONTINUE(other_value_index < 0 || other_value_index >= other_texture_map_value_remap.size());
				flat_merged_texture_map.set(start_count + cell_index, other_texture_map_value_remap[other_value_index]);
			}
			merged_texture_map.set(0, flat_merged_texture_map);
		} else {
			const int64_t start_count = merged_texture_map.size();
			merged_texture_map.resize(start_count + other_texture_map_data.size());
			for (int64_t cell_index = 0; cell_index < other_texture_map_data.size(); cell_index++) {
				PackedInt32Array remapped_cell_texture_map_indices = other_texture_map_data[cell_index];
				for (int64_t vert_inst = 0; vert_inst < remapped_cell_texture_map_indices.size(); vert_inst++) {
					const int32_t other_value_index = remapped_cell_texture_map_indices[vert_inst];
					ERR_CONTINUE(other_value_index < 0 || other_value_index >= other_texture_map_value_remap.size());
					remapped_cell_texture_map_indices.set(vert_inst, other_texture_map_value_remap[other_value_index]);
				}
				merged_texture_map.set(start_count + cell_index, remapped_cell_texture_map_indices);
			}
		}
	}
	// The merging above only iterates over the other mesh's data bindings. If this mesh has
	// data bindings that the other mesh lacked, those are now missing entries for the merged
	// geometry, which would make the merged mesh fail validation, so fill them in below.
	Vector<PackedInt32Array> merged_cell_vertex_indices;
	if (boundary_dim_index >= 0 && _poly_cell_indices.size() > boundary_dim_index && _all_poly_cell_normal_indices.has(per_cell_key) && !_all_poly_cell_normal_indices[per_cell_key].is_empty() && !_all_poly_cell_normal_indices[per_cell_key][0].is_empty()) {
		// Special case: Generate missing boundary normals based on the merged cell orientations.
		Vector<PackedInt32Array> &merged_boundary_normals = _all_poly_cell_normal_indices[per_cell_key];
		if (merged_boundary_normals[0].size() < _poly_cell_indices[boundary_dim_index].size()) {
			merged_cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, true);
			merged_boundary_normals.set(0, _normal_indices_for_values_internal(_compute_boundary_normals_based_on_cell_orientation(merged_cell_vertex_indices, true)));
		}
	}
	if (boundary_dim_index >= 0 && _poly_cell_indices.size() > boundary_dim_index && _all_poly_cell_normal_indices.has(cell_to_vert_key) && !_all_poly_cell_normal_indices[cell_to_vert_key].is_empty()) {
		// Special case: Generate missing vertex normals as flat shading normals.
		Vector<PackedInt32Array> &merged_vertex_normals = _all_poly_cell_normal_indices[cell_to_vert_key];
		const int64_t merged_cell_count = _poly_cell_indices[boundary_dim_index].size();
		if (merged_vertex_normals.size() < merged_cell_count) {
			if (merged_cell_vertex_indices.is_empty()) {
				merged_cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, boundary_dim_index, true);
			}
			// Prefer the merged boundary normals when available, else use the cell orientations.
			Vector<VectorN> flat_boundary_normals;
			if (_all_poly_cell_normal_indices.has(per_cell_key) && !_all_poly_cell_normal_indices[per_cell_key].is_empty() && _all_poly_cell_normal_indices[per_cell_key][0].size() == merged_cell_count) {
				flat_boundary_normals = _sample_normal_values_internal(_all_poly_cell_normal_indices[per_cell_key][0]);
			} else {
				flat_boundary_normals = _compute_boundary_normals_based_on_cell_orientation(merged_cell_vertex_indices, false);
			}
			const int64_t start_count = merged_vertex_normals.size();
			merged_vertex_normals.resize(merged_cell_count);
			for (int64_t cell_index = start_count; cell_index < merged_cell_count; cell_index++) {
				PackedInt32Array normal_indices_for_cell;
				normal_indices_for_cell.resize(merged_cell_vertex_indices[cell_index].size());
				const int32_t cell_boundary_normal_index = (int32_t)VectorND::array_append_deduplicate(_poly_cell_normal_values, flat_boundary_normals[cell_index]);
				for (int64_t vert_inst = 0; vert_inst < normal_indices_for_cell.size(); vert_inst++) {
					normal_indices_for_cell.set(vert_inst, cell_boundary_normal_index);
				}
				merged_vertex_normals.set(cell_index, normal_indices_for_cell);
			}
		}
	}
	// General case: Pad any remaining short normal bindings with empty data.
	for (KeyValue<Vector2i, Vector<PackedInt32Array>> &normals_kv : _all_poly_cell_normal_indices) {
		const Vector2i key = normals_kv.key;
		if (normals_kv.value.is_empty() || (key.y == key.x && normals_kv.value[0].is_empty())) {
			continue;
		}
		int64_t cell_count_for_geom_dim;
		if (key.x == 0) {
			cell_count_for_geom_dim = _poly_cell_vertex_positions.size();
		} else if (key.x == 1) {
			cell_count_for_geom_dim = _edge_vertex_indices.size() / 2;
		} else if ((key.x - 2) < _poly_cell_indices.size()) {
			cell_count_for_geom_dim = _poly_cell_indices[key.x - 2].size();
		} else {
			continue; // Invalid binding for a dimension the merged mesh does not have.
		}
		Vector<PackedInt32Array> &merged_normals = normals_kv.value;
		if (key.y == key.x) {
			// Non-decomposed normals use a flat structure, so pad the flat array with a zero normal value.
			if (merged_normals.is_empty()) {
				merged_normals.append(PackedInt32Array());
			}
			if (merged_normals[0].size() < cell_count_for_geom_dim) {
				WARN_PRINT("ArrayPolyMeshND: The other mesh was missing normal entries for geometry dimension " + itos(key.x) + " and decomposition dimension " + itos(key.y) + ", but the original mesh has entries for this key. Filling missing entries with empty data while merging. Consider updating the other mesh with this data before merging to avoid this warning in the future.");
				const int32_t zero_normal_value_index = (int32_t)VectorND::array_append_deduplicate(_poly_cell_normal_values, VectorN());
				PackedInt32Array flat_merged_normals = merged_normals[0];
				const int64_t old_flat_count = flat_merged_normals.size();
				flat_merged_normals.resize(cell_count_for_geom_dim);
				for (int64_t i = old_flat_count; i < cell_count_for_geom_dim; i++) {
					flat_merged_normals.set(i, zero_normal_value_index);
				}
				merged_normals.set(0, flat_merged_normals);
			}
		} else if (merged_normals.size() < cell_count_for_geom_dim) {
			// Decomposed normals use a nested structure, so pad with empty arrays.
			WARN_PRINT("ArrayPolyMeshND: The other mesh was missing normal entries for geometry dimension " + itos(key.x) + " and decomposition dimension " + itos(key.y) + ", but the original mesh has entries for this key. Filling missing entries with empty data while merging. Consider updating the other mesh with this data before merging to avoid this warning in the future.");
			merged_normals.resize(cell_count_for_geom_dim);
		}
	}
	// Texture maps cannot be generated, so pad any short texture map bindings with empty data.
	// Cells with empty texture map entries are valid, they are just unmapped.
	for (KeyValue<Vector2i, Vector<PackedInt32Array>> &texture_map_kv : _all_poly_cell_texture_map_indices) {
		const Vector2i key = texture_map_kv.key;
		if (texture_map_kv.value.is_empty() || (key.y == key.x && texture_map_kv.value[0].is_empty())) {
			continue;
		}
		int64_t cell_count_for_geom_dim;
		if (key.x == 0) {
			cell_count_for_geom_dim = _poly_cell_vertex_positions.size();
		} else if (key.x == 1) {
			cell_count_for_geom_dim = _edge_vertex_indices.size() / 2;
		} else if ((key.x - 2) < _poly_cell_indices.size()) {
			cell_count_for_geom_dim = _poly_cell_indices[key.x - 2].size();
		} else {
			continue; // Invalid binding for a dimension the merged mesh does not have.
		}
		Vector<PackedInt32Array> &merged_texture_map = texture_map_kv.value;
		if (key.y == key.x) {
			// Non-decomposed texture maps use a flat structure, so pad the flat array with a zero texture map value.
			if (merged_texture_map.is_empty()) {
				merged_texture_map.append(PackedInt32Array());
			}
			if (merged_texture_map[0].size() < cell_count_for_geom_dim) {
				const int32_t zero_texture_map_value_index = (int32_t)VectorND::array_append_deduplicate(_poly_cell_texture_map_values, VectorM());
				PackedInt32Array flat_merged_texture_map = merged_texture_map[0];
				const int64_t old_flat_count = flat_merged_texture_map.size();
				flat_merged_texture_map.resize(cell_count_for_geom_dim);
				for (int64_t i = old_flat_count; i < cell_count_for_geom_dim; i++) {
					flat_merged_texture_map.set(i, zero_texture_map_value_index);
				}
				merged_texture_map.set(0, flat_merged_texture_map);
			}
		} else if (merged_texture_map.size() < cell_count_for_geom_dim) {
			// Decomposed texture maps use a nested structure, so pad with empty arrays.
			merged_texture_map.resize(cell_count_for_geom_dim);
		}
	}
	// Merge materials.
	Ref<MaterialND> other_material = p_other->get_material();
	if (other_material.is_valid()) {
		Ref<MaterialND> self_material = get_material();
		if (self_material.is_valid()) {
			self_material->merge_with(other_material, start_vertex_pos_count, other_vertex_pos_count);
		} else if (other_material->get_albedo_color_array().size() > 0) {
			self_material.instantiate();
			self_material->merge_with(other_material, start_vertex_pos_count, other_vertex_pos_count);
			set_material(self_material);
		} else {
			set_material(other_material);
		}
	}
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

// Getters and setters.

HashMap<Vector2i, Vector<PackedInt32Array>> ArrayPolyMeshND::get_all_poly_cell_normal_indices() const {
	return HashMap<Vector2i, Vector<PackedInt32Array>>(_all_poly_cell_normal_indices);
}

void ArrayPolyMeshND::set_all_poly_cell_normal_indices(const HashMap<Vector2i, Vector<PackedInt32Array>> &p_all_poly_cell_normal_indices) {
	_all_poly_cell_normal_indices = HashMap<Vector2i, Vector<PackedInt32Array>>(p_all_poly_cell_normal_indices);
	poly_mesh_clear_cache(true);
}

HashMap<Vector2i, Vector<PackedInt32Array>> ArrayPolyMeshND::get_all_poly_cell_texture_map_indices() const {
	return HashMap<Vector2i, Vector<PackedInt32Array>>(_all_poly_cell_texture_map_indices);
}

void ArrayPolyMeshND::set_all_poly_cell_texture_map_indices(const HashMap<Vector2i, Vector<PackedInt32Array>> &p_all_poly_cell_texture_map_indices) {
	_all_poly_cell_texture_map_indices = HashMap<Vector2i, Vector<PackedInt32Array>>(p_all_poly_cell_texture_map_indices);
	poly_mesh_clear_cache(false);
}

Vector<Vector<VectorN>> ArrayPolyMeshND::get_poly_cell_dense_normals(const Vector2i &p_key) const {
	Vector<Vector<VectorN>> dense;
	if (!_all_poly_cell_normal_indices.has(p_key)) {
		return dense;
	}
	const Vector<PackedInt32Array> &index_arrays = _all_poly_cell_normal_indices[p_key];
	dense.resize(index_arrays.size());
	for (int64_t i = 0; i < index_arrays.size(); i++) {
		dense.set(i, _sample_normal_values_internal(index_arrays[i]));
	}
	return dense;
}

void ArrayPolyMeshND::set_poly_cell_dense_normals(const Vector2i &p_key, const Vector<Vector<VectorN>> &p_dense_normals) {
	if (p_dense_normals.is_empty()) {
		_all_poly_cell_normal_indices.erase(p_key);
	} else {
		Vector<PackedInt32Array> index_arrays;
		index_arrays.resize(p_dense_normals.size());
		for (int64_t i = 0; i < p_dense_normals.size(); i++) {
			index_arrays.set(i, _normal_indices_for_values_internal(p_dense_normals[i]));
		}
		_all_poly_cell_normal_indices.insert(p_key, index_arrays);
	}
	poly_mesh_clear_cache(true);
}

Vector<Vector<VectorM>> ArrayPolyMeshND::get_poly_cell_dense_texture_map(const Vector2i &p_key) const {
	Vector<Vector<VectorM>> dense;
	if (!_all_poly_cell_texture_map_indices.has(p_key)) {
		return dense;
	}
	const Vector<PackedInt32Array> &index_arrays = _all_poly_cell_texture_map_indices[p_key];
	const int64_t value_count = _poly_cell_texture_map_values.size();
	dense.resize(index_arrays.size());
	for (int64_t i = 0; i < index_arrays.size(); i++) {
		const PackedInt32Array &indices = index_arrays[i];
		Vector<VectorM> values;
		values.resize(indices.size());
		for (int64_t j = 0; j < indices.size(); j++) {
			const int32_t value_index = indices[j];
			ERR_CONTINUE(value_index < 0 || value_index >= value_count);
			values.set(j, _poly_cell_texture_map_values[value_index]);
		}
		dense.set(i, values);
	}
	return dense;
}

void ArrayPolyMeshND::set_poly_cell_dense_texture_map(const Vector2i &p_key, const Vector<Vector<VectorM>> &p_dense_texture_map) {
	if (p_dense_texture_map.is_empty()) {
		_all_poly_cell_texture_map_indices.erase(p_key);
	} else {
		Vector<PackedInt32Array> index_arrays;
		index_arrays.resize(p_dense_texture_map.size());
		for (int64_t i = 0; i < p_dense_texture_map.size(); i++) {
			const Vector<VectorM> &values = p_dense_texture_map[i];
			PackedInt32Array indices;
			indices.resize(values.size());
			for (int64_t j = 0; j < values.size(); j++) {
				indices.set(j, (int32_t)VectorND::array_append_deduplicate(_poly_cell_texture_map_values, values[j]));
			}
			index_arrays.set(i, indices);
		}
		_all_poly_cell_texture_map_indices.insert(p_key, index_arrays);
	}
	poly_mesh_clear_cache(false);
}

ArrayPolyMeshND::PolyDataDictionary ArrayPolyMeshND::get_all_poly_cell_normal_indices_bind() const {
	PolyDataDictionary result;
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : _all_poly_cell_normal_indices) {
		const Vector2i &key = kv.key;
		const Vector<PackedInt32Array> &normal_indices_data = kv.value;
		Array normal_indices_array;
		normal_indices_array.resize(normal_indices_data.size());
		for (int64_t i = 0; i < normal_indices_data.size(); i++) {
			normal_indices_array[i] = normal_indices_data[i];
		}
		result[key] = normal_indices_array;
	}
	return result;
}

void ArrayPolyMeshND::set_all_poly_cell_normal_indices_bind(const PolyDataDictionary &p_all_poly_cell_normal_indices) {
	HashMap<Vector2i, Vector<PackedInt32Array>> normal_indices_hashmap;
	const Array normal_indices_keys = p_all_poly_cell_normal_indices.keys();
	for (int64_t key_index = 0; key_index < normal_indices_keys.size(); key_index++) {
		const Vector2i key = normal_indices_keys[key_index];
		const Array normal_indices_array = p_all_poly_cell_normal_indices[key];
		Vector<PackedInt32Array> normal_indices_data;
		normal_indices_data.resize(normal_indices_array.size());
		for (int64_t i = 0; i < normal_indices_array.size(); i++) {
			const PackedInt32Array cell_normal_indices = normal_indices_array[i];
			normal_indices_data.set(i, cell_normal_indices);
		}
		normal_indices_hashmap.insert(key, normal_indices_data);
	}
	set_all_poly_cell_normal_indices(normal_indices_hashmap);
}

ArrayPolyMeshND::PolyDataDictionary ArrayPolyMeshND::get_all_poly_cell_texture_map_indices_bind() const {
	PolyDataDictionary result;
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : _all_poly_cell_texture_map_indices) {
		const Vector2i &key = kv.key;
		const Vector<PackedInt32Array> &texture_map_indices_data = kv.value;
		Array texture_map_indices_array;
		texture_map_indices_array.resize(texture_map_indices_data.size());
		for (int64_t i = 0; i < texture_map_indices_data.size(); i++) {
			texture_map_indices_array[i] = texture_map_indices_data[i];
		}
		result[key] = texture_map_indices_array;
	}
	return result;
}

void ArrayPolyMeshND::set_all_poly_cell_texture_map_indices_bind(const PolyDataDictionary &p_all_poly_cell_texture_map_indices) {
	HashMap<Vector2i, Vector<PackedInt32Array>> texture_map_indices_hashmap;
	const Array texture_map_indices_keys = p_all_poly_cell_texture_map_indices.keys();
	for (int64_t key_index = 0; key_index < texture_map_indices_keys.size(); key_index++) {
		const Vector2i key = texture_map_indices_keys[key_index];
		const Array texture_map_indices_array = p_all_poly_cell_texture_map_indices[key];
		Vector<PackedInt32Array> texture_map_indices_data;
		texture_map_indices_data.resize(texture_map_indices_array.size());
		for (int64_t i = 0; i < texture_map_indices_array.size(); i++) {
			const PackedInt32Array cell_texture_map_indices = texture_map_indices_array[i];
			texture_map_indices_data.set(i, cell_texture_map_indices);
		}
		texture_map_indices_hashmap.insert(key, texture_map_indices_data);
	}
	set_all_poly_cell_texture_map_indices(texture_map_indices_hashmap);
}

void ArrayPolyMeshND::set_edge_vertex_indices(const PackedInt32Array &p_edge_indices) {
	_edge_vertex_indices = PackedInt32Array(p_edge_indices);
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

void ArrayPolyMeshND::set_poly_cell_indices(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices) {
	_poly_cell_indices = p_poly_cell_indices;
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

#define IS_NOT_INDICES_ARRAY(m_var) (m_var.get_type() != Variant::PACKED_INT32_ARRAY && m_var.get_type() != Variant::ARRAY)

void ArrayPolyMeshND::set_poly_cell_indices_bind(const TypedArray<Array> &p_poly_cell_indices) {
	Vector<Vector<PackedInt32Array>> indices;
	indices.resize(p_poly_cell_indices.size());
	for (int i = 0; i < p_poly_cell_indices.size(); i++) {
		const Array &dim_array = p_poly_cell_indices[i];
		Vector<PackedInt32Array> dim_vector;
		dim_vector.resize(dim_array.size());
		for (int j = 0; j < dim_array.size(); j++) {
			const Variant &cell_indices_variant = dim_array[j];
			ERR_FAIL_COND_MSG(IS_NOT_INDICES_ARRAY(cell_indices_variant),
					"ArrayPolyMeshND: Poly cell indices must be an array of polytope dimensions, "
					"containing an array of cells, each of which contains indices. For example, "
					"`[[[0, 1, 2]]]` encodes a single triangle made of edges 0, 1, and 2.");
			const PackedInt32Array cell_indices = cell_indices_variant;
			dim_vector.set(j, cell_indices);
		}
		indices.set(i, dim_vector);
	}
	set_poly_cell_indices(indices);
}

Vector<VectorN> ArrayPolyMeshND::get_poly_cell_boundary_normals() {
	const Vector2i per_cell_key = _get_per_cell_key();
	if (!_all_poly_cell_normal_indices.has(per_cell_key) || _all_poly_cell_normal_indices[per_cell_key].is_empty()) {
		return Vector<VectorN>();
	}
	return _sample_normal_values_internal(_all_poly_cell_normal_indices[per_cell_key][0]);
}

void ArrayPolyMeshND::set_poly_cell_boundary_normals(const Vector<VectorN> &p_poly_cell_boundary_normals) {
	const Vector2i per_cell_key = _get_per_cell_key();
	if (p_poly_cell_boundary_normals.is_empty()) {
		_all_poly_cell_normal_indices.erase(per_cell_key);
	} else {
		_all_poly_cell_normal_indices.insert(per_cell_key, Vector<PackedInt32Array>{ _normal_indices_for_values_internal(p_poly_cell_boundary_normals) });
	}
	poly_mesh_clear_cache(true);
}

void ArrayPolyMeshND::set_poly_cell_boundary_normals_bind(const TypedArray<VectorN> &p_poly_cell_boundary_normals) {
	Vector<VectorN> normals;
	normals.resize(p_poly_cell_boundary_normals.size());
	for (int i = 0; i < p_poly_cell_boundary_normals.size(); i++) {
		const VectorN normal = p_poly_cell_boundary_normals[i];
		normals.set(i, normal);
	}
	set_poly_cell_boundary_normals(normals);
}

PackedInt32Array ArrayPolyMeshND::get_poly_cell_boundary_pivot_overrides() {
	return _poly_cell_boundary_pivot_overrides;
}

void ArrayPolyMeshND::set_poly_cell_boundary_pivot_overrides(const PackedInt32Array &p_poly_cell_boundary_pivot_overrides) {
	_poly_cell_boundary_pivot_overrides = p_poly_cell_boundary_pivot_overrides;
	poly_mesh_clear_cache(false);
}

Vector<PackedInt32Array> ArrayPolyMeshND::get_poly_cell_normal_indices() {
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	if (!_all_poly_cell_normal_indices.has(cell_to_vert_key)) {
		return Vector<PackedInt32Array>();
	}
	return Vector<PackedInt32Array>(_all_poly_cell_normal_indices[cell_to_vert_key]);
}

void ArrayPolyMeshND::set_poly_cell_normal_indices(const Vector<PackedInt32Array> &p_poly_cell_normal_indices) {
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	if (p_poly_cell_normal_indices.is_empty()) {
		_all_poly_cell_normal_indices.erase(cell_to_vert_key);
	} else {
		_all_poly_cell_normal_indices.insert(cell_to_vert_key, p_poly_cell_normal_indices);
	}
	poly_mesh_clear_cache(true);
}

void ArrayPolyMeshND::set_poly_cell_normal_indices_bind(const TypedArray<PackedInt32Array> &p_poly_cell_normal_indices) {
	Vector<PackedInt32Array> normal_indices;
	normal_indices.resize(p_poly_cell_normal_indices.size());
	for (int i = 0; i < p_poly_cell_normal_indices.size(); i++) {
		normal_indices.set(i, p_poly_cell_normal_indices[i]);
	}
	set_poly_cell_normal_indices(normal_indices);
}

Vector<PackedInt32Array> ArrayPolyMeshND::get_poly_cell_texture_map_indices() {
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	if (!_all_poly_cell_texture_map_indices.has(cell_to_vert_key)) {
		return Vector<PackedInt32Array>();
	}
	return Vector<PackedInt32Array>(_all_poly_cell_texture_map_indices[cell_to_vert_key]);
}

void ArrayPolyMeshND::set_poly_cell_texture_map_indices(const Vector<PackedInt32Array> &p_poly_cell_texture_map_indices) {
	const Vector2i cell_to_vert_key = _get_cell_to_vert_key();
	if (p_poly_cell_texture_map_indices.is_empty()) {
		_all_poly_cell_texture_map_indices.erase(cell_to_vert_key);
	} else {
		_all_poly_cell_texture_map_indices.insert(cell_to_vert_key, p_poly_cell_texture_map_indices);
	}
	poly_mesh_clear_cache(false);
}

void ArrayPolyMeshND::set_poly_cell_texture_map_indices_bind(const TypedArray<PackedInt32Array> &p_poly_cell_texture_map_indices) {
	Vector<PackedInt32Array> texture_map_indices;
	texture_map_indices.resize(p_poly_cell_texture_map_indices.size());
	for (int i = 0; i < p_poly_cell_texture_map_indices.size(); i++) {
		texture_map_indices.set(i, p_poly_cell_texture_map_indices[i]);
	}
	set_poly_cell_texture_map_indices(texture_map_indices);
}

Vector<VectorN> ArrayPolyMeshND::get_poly_cell_normal_values() {
	return _poly_cell_normal_values;
}

void ArrayPolyMeshND::set_poly_cell_normal_values(const Vector<VectorN> &p_poly_cell_normal_values) {
	_poly_cell_normal_values = p_poly_cell_normal_values;
	poly_mesh_clear_cache(true);
}

void ArrayPolyMeshND::set_poly_cell_normal_values_bind(const TypedArray<VectorN> &p_poly_cell_normal_values) {
	Vector<VectorN> normal_values;
	normal_values.resize(p_poly_cell_normal_values.size());
	for (int i = 0; i < p_poly_cell_normal_values.size(); i++) {
		const VectorN normal_value = p_poly_cell_normal_values[i];
		normal_values.set(i, normal_value);
	}
	set_poly_cell_normal_values(normal_values);
}

Vector<VectorM> ArrayPolyMeshND::get_poly_cell_texture_map_values() {
	return _poly_cell_texture_map_values;
}

void ArrayPolyMeshND::set_poly_cell_texture_map_values(const Vector<VectorM> &p_poly_cell_texture_map_values) {
	_poly_cell_texture_map_values = p_poly_cell_texture_map_values;
	poly_mesh_clear_cache(false);
}

void ArrayPolyMeshND::set_poly_cell_texture_map_values_bind(const TypedArray<VectorM> &p_poly_cell_texture_map_values) {
	Vector<VectorM> texture_map_values;
	texture_map_values.resize(p_poly_cell_texture_map_values.size());
	for (int i = 0; i < p_poly_cell_texture_map_values.size(); i++) {
		const VectorM texture_map_value = p_poly_cell_texture_map_values[i];
		texture_map_values.set(i, texture_map_value);
	}
	set_poly_cell_texture_map_values(texture_map_values);
}

PackedInt32Array ArrayPolyMeshND::get_seam_indices_bind() const {
	PackedInt32Array seam_indices;
	seam_indices.resize(_seam_indices.size());
	int64_t seam_index = 0;
	for (const int32_t member_index : _seam_indices) {
		seam_indices.set(seam_index, member_index);
		seam_index++;
	}
	// The order does not matter, but sorting makes it stable and easier to read and compare.
	seam_indices.sort();
	return seam_indices;
}

void ArrayPolyMeshND::set_seam_indices(const HashSet<int32_t> &p_seam_indices) {
	_seam_indices = HashSet<int32_t>(p_seam_indices);
	// No need to reset validation, even invalid seams do not affect mesh operations.
}

void ArrayPolyMeshND::set_seam_indices_bind(const PackedInt32Array &p_seam_indices) {
	_seam_indices.clear();
	for (int64_t i = 0; i < p_seam_indices.size(); i++) {
		_seam_indices.insert(p_seam_indices[i]);
	}
}

Vector<VectorN> ArrayPolyMeshND::get_poly_cell_vertex_positions() {
	return _poly_cell_vertex_positions;
}

void ArrayPolyMeshND::set_poly_cell_vertex_positions(const Vector<VectorN> &p_vertex_positions) {
	ERR_FAIL_COND(p_vertex_positions.size() > MAX_POLY_VERTICES); // Prevent overflow.
	_poly_cell_vertex_positions = p_vertex_positions;
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

void ArrayPolyMeshND::set_poly_cell_vertex_positions_bind(const TypedArray<VectorN> &p_vertex_positions) {
	Vector<VectorN> vertices;
	vertices.resize(p_vertex_positions.size());
	for (int i = 0; i < p_vertex_positions.size(); i++) {
		const VectorN vertex = p_vertex_positions[i];
		vertices.set(i, vertex);
	}
	set_poly_cell_vertex_positions(vertices);
}

void ArrayPolyMeshND::_bind_methods() {
	// Append and delete functions.
	ClassDB::bind_method(D_METHOD("append_edge_points", "point_a", "point_b", "deduplicate"), &ArrayPolyMeshND::append_edge_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_edge_indices", "index_a", "index_b", "deduplicate"), &ArrayPolyMeshND::append_edge_indices, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_poly_cell", "dimension", "cell", "deduplicate"), &ArrayPolyMeshND::append_poly_cell, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_vertex", "vertex", "deduplicate_vertices"), &ArrayPolyMeshND::append_vertex, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_vertices", "vertices", "deduplicate_vertices"), &ArrayPolyMeshND::append_vertices, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("delete_poly_element", "dimension", "index"), &ArrayPolyMeshND::delete_poly_element);

	ClassDB::bind_method(D_METHOD("compact_normal_values"), &ArrayPolyMeshND::compact_normal_values);
	ClassDB::bind_method(D_METHOD("compact_texture_map_values"), &ArrayPolyMeshND::compact_texture_map_values);

	// Normal calculation functions.
	ClassDB::bind_method(D_METHOD("calculate_boundary_normals", "normals_mode", "keep_existing"), &ArrayPolyMeshND::calculate_boundary_normals, DEFVAL(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("set_flat_shading_normals", "normals_mode", "recalculate_boundary_normals"), &ArrayPolyMeshND::set_flat_shading_normals, DEFVAL(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_smooth_shading_normals", "normals_mode", "recalculate_boundary_normals"), &ArrayPolyMeshND::set_smooth_shading_normals, DEFVAL(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("make_double_sided", "idempotent"), &ArrayPolyMeshND::make_double_sided, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("make_single_cell_from_all_cells", "cell_dimension"), &ArrayPolyMeshND::make_single_cell_from_all_cells);

	// Texture map and seam functions.
	ClassDB::bind_method(D_METHOD("calculate_seams", "angle_threshold_radians", "discard_seams_within_islands"), &ArrayPolyMeshND::calculate_seams, DEFVAL(Math_TAU / 8.0), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("collect_cells_in_island", "start_cell"), &ArrayPolyMeshND::collect_cells_in_island);
	ClassDB::bind_method(D_METHOD("collect_all_islands"), &ArrayPolyMeshND::collect_all_islands_bind);
	ClassDB::bind_method(D_METHOD("unwrap_texture_map_island", "cells_in_island", "keep_existing"), &ArrayPolyMeshND::unwrap_texture_map_island, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("unwrap_texture_map", "mode", "padding", "proportional", "keep_existing"), &ArrayPolyMeshND::unwrap_texture_map, DEFVAL(UNWRAP_MODE_TILE_ISLANDS), DEFVAL(0.0), DEFVAL(true), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("transform_texture_map", "transform"), &ArrayPolyMeshND::transform_texture_map);

	// Misc functions.
	ClassDB::bind_method(D_METHOD("deduplicate_all_elements"), &ArrayPolyMeshND::deduplicate_all_elements);
	ClassDB::bind_method(D_METHOD("transform_vertices", "transform"), &ArrayPolyMeshND::transform_vertices);
	ClassDB::bind_method(D_METHOD("merge_with", "other", "transform"), &ArrayPolyMeshND::merge_with, DEFVAL(Ref<TransformND>()));

	// Properties. Only bind the setters here because the getters are already bound in PolyMeshND.
	ClassDB::bind_method(D_METHOD("set_poly_cell_indices", "poly_cell_indices"), &ArrayPolyMeshND::set_poly_cell_indices_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_indices"), "set_poly_cell_indices", "get_poly_cell_indices");

	ClassDB::bind_method(D_METHOD("set_poly_cell_vertex_positions", "poly_cell_vertex_positions"), &ArrayPolyMeshND::set_poly_cell_vertex_positions_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_vertex_positions"), "set_poly_cell_vertex_positions", "get_poly_cell_vertex_positions");
#ifndef DISABLE_DEPRECATED
	// Compatibility property to handle reading existing serialized data.
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_vertices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_INTERNAL), "set_poly_cell_vertex_positions", "get_poly_cell_vertex_positions");
#endif // DISABLE_DEPRECATED

	ClassDB::bind_method(D_METHOD("set_poly_cell_boundary_pivot_overrides", "poly_cell_boundary_pivot_overrides"), &ArrayPolyMeshND::set_poly_cell_boundary_pivot_overrides);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "poly_cell_boundary_pivot_overrides"), "set_poly_cell_boundary_pivot_overrides", "get_poly_cell_boundary_pivot_overrides");

	ClassDB::bind_method(D_METHOD("get_seam_indices"), &ArrayPolyMeshND::get_seam_indices_bind);
	ClassDB::bind_method(D_METHOD("set_seam_indices", "seam_indices"), &ArrayPolyMeshND::set_seam_indices_bind);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "seam_indices"), "set_seam_indices", "get_seam_indices");

	ClassDB::bind_method(D_METHOD("set_edge_indices", "edge_indices"), &ArrayPolyMeshND::set_edge_vertex_indices);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "edge_indices"), "set_edge_indices", "get_edge_indices");

	// Normals and texture maps. The "all" ones need the getters bound here.
	ClassDB::bind_method(D_METHOD("get_all_poly_cell_normal_indices"), &ArrayPolyMeshND::get_all_poly_cell_normal_indices_bind);
	ClassDB::bind_method(D_METHOD("set_all_poly_cell_normal_indices", "all_poly_cell_normal_indices"), &ArrayPolyMeshND::set_all_poly_cell_normal_indices_bind);
	ClassDB::bind_method(D_METHOD("get_all_poly_cell_texture_map_indices"), &ArrayPolyMeshND::get_all_poly_cell_texture_map_indices_bind);
	ClassDB::bind_method(D_METHOD("set_all_poly_cell_texture_map_indices", "all_poly_cell_texture_map_indices"), &ArrayPolyMeshND::set_all_poly_cell_texture_map_indices_bind);
#if GODOT_HAS_TYPED_DICTIONARY
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "all_poly_cell_normal_indices", PROPERTY_HINT_TYPE_STRING, "Vector2i:Array"), "set_all_poly_cell_normal_indices", "get_all_poly_cell_normal_indices");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "all_poly_cell_texture_map_indices", PROPERTY_HINT_TYPE_STRING, "Vector2i:Array"), "set_all_poly_cell_texture_map_indices", "get_all_poly_cell_texture_map_indices");
#else
	// Godot 4.3 and earlier do not support type hints on Dictionary properties.
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "all_poly_cell_normal_indices"), "set_all_poly_cell_normal_indices", "get_all_poly_cell_normal_indices");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "all_poly_cell_texture_map_indices"), "set_all_poly_cell_texture_map_indices", "get_all_poly_cell_texture_map_indices");
#endif // GODOT_HAS_TYPED_DICTIONARY

	// The value pools that the index dictionaries reference into.
	ClassDB::bind_method(D_METHOD("set_poly_cell_normal_values", "poly_cell_normal_values"), &ArrayPolyMeshND::set_poly_cell_normal_values_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_normal_values", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_poly_cell_normal_values", "get_poly_cell_normal_values");

	ClassDB::bind_method(D_METHOD("set_poly_cell_texture_map_values", "poly_cell_texture_map_values"), &ArrayPolyMeshND::set_poly_cell_texture_map_values_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_texture_map_values", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array"), "set_poly_cell_texture_map_values", "get_poly_cell_texture_map_values");

	// The dictionaries and value pools are the stored source of truth, so the high-level views are editor-only.
	ClassDB::bind_method(D_METHOD("set_poly_cell_boundary_normals", "poly_cell_boundary_normals"), &ArrayPolyMeshND::set_poly_cell_boundary_normals_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_boundary_normals", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_poly_cell_boundary_normals", "get_poly_cell_boundary_normals");

	ClassDB::bind_method(D_METHOD("set_poly_cell_normal_indices", "poly_cell_normal_indices"), &ArrayPolyMeshND::set_poly_cell_normal_indices_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_normal_indices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_poly_cell_normal_indices", "get_poly_cell_normal_indices");

	ClassDB::bind_method(D_METHOD("set_poly_cell_texture_map_indices", "poly_cell_texture_map_indices"), &ArrayPolyMeshND::set_poly_cell_texture_map_indices_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_texture_map_indices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_poly_cell_texture_map_indices", "get_poly_cell_texture_map_indices");

	// Enums.
	BIND_ENUM_CONSTANT(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
	BIND_ENUM_CONSTANT(COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);
	BIND_ENUM_CONSTANT(COMPUTE_NORMALS_MODE_FORCE_OUTWARD_OVERRIDE_CELL_ORIENTATION);

	BIND_ENUM_CONSTANT(UNWRAP_MODE_AUTOMATIC);
	BIND_ENUM_CONSTANT(UNWRAP_MODE_EACH_CELL_FILLS);
	BIND_ENUM_CONSTANT(UNWRAP_MODE_TILE_CELLS);
	BIND_ENUM_CONSTANT(UNWRAP_MODE_EACH_ISLAND_FILLS);
	BIND_ENUM_CONSTANT(UNWRAP_MODE_TILE_ISLANDS);
}
