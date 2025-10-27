#include "array_poly_mesh_nd.h"

#include "../../../math/math_nd.h"
#include "../../../math/vector_nd.h"

int64_t ArrayPolyMeshND::append_edge_points(const VectorN &p_point_a, const VectorN &p_point_b, const bool p_deduplicate) {
	const int32_t index_a = append_vertex(p_point_a, p_deduplicate);
	const int32_t index_b = append_vertex(p_point_b, p_deduplicate);
	return append_edge_indices(index_a, index_b, p_deduplicate);
}

int64_t ArrayPolyMeshND::_append_edge_indices_internal(int32_t p_index_a, int32_t p_index_b, const bool p_deduplicate, PackedInt32Array &r_edge_vertex_indices) {
	const int64_t edge_count = r_edge_vertex_indices.size() / 2;
	if (p_index_a > p_index_b) {
		SWAP(p_index_a, p_index_b);
	}
	if (p_deduplicate) {
		for (int64_t i = 0; i < edge_count; i++) {
			if (r_edge_vertex_indices[i * 2] == p_index_a && r_edge_vertex_indices[i * 2 + 1] == p_index_b) {
				return i;
			}
		}
	}
	r_edge_vertex_indices.append(p_index_a);
	r_edge_vertex_indices.append(p_index_b);
	return edge_count;
}

int64_t ArrayPolyMeshND::append_edge_indices(int32_t p_index_a, int32_t p_index_b, const bool p_deduplicate) {
	const int64_t old_edge_count = _edge_vertex_indices.size() / 2;
	const int64_t new_edge_index = _append_edge_indices_internal(p_index_a, p_index_b, p_deduplicate, _edge_vertex_indices);
	if (new_edge_index == old_edge_count) {
		// A new edge was appended! We don't need to clear the poly mesh cache, but CellMeshND does cache edges.
		cell_mesh_clear_cache();
		reset_poly_mesh_data_validation();
	}
	return new_edge_index;
}

int ArrayPolyMeshND::append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int vertex_count = _poly_cell_vertices.size();
	if (p_deduplicate_vertices) {
		for (int i = 0; i < vertex_count; i++) {
			if (_poly_cell_vertices[i] == p_vertex) {
				return i;
			}
		}
	}
	ERR_FAIL_COND_V(_poly_cell_vertices.size() > MAX_POLY_VERTICES, 2147483647);
	_poly_cell_vertices.push_back(p_vertex);
	reset_poly_mesh_data_validation();
	return vertex_count;
}

PackedInt32Array ArrayPolyMeshND::append_vertices(const Vector<VectorN> &p_vertices, const bool p_deduplicate_vertices) {
	PackedInt32Array indices;
	for (int i = 0; i < p_vertices.size(); i++) {
		indices.append(append_vertex(p_vertices[i], p_deduplicate_vertices));
	}
	reset_poly_mesh_data_validation();
	return indices;
}

void ArrayPolyMeshND::calculate_boundary_normals(const ComputeNormalsMode p_mode, const bool p_keep_existing) {
	ERR_FAIL_COND_MSG(_poly_cell_indices.size() < 2, "ArrayPolyMeshND: Cannot calculate boundary normals because there are no boundary cells.");
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot calculate boundary normals for an invalid mesh.");
	const Vector<VectorN> &vertices = get_vertices();
	ERR_FAIL_COND_MSG(vertices.is_empty(), "ArrayPolyMeshND: Cannot calculate boundary normals because there are no vertices.");
	const Vector<PackedInt32Array> cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, true);
	if (cell_vertex_indices.is_empty()) {
		return;
	}
	_poly_cell_boundary_normals = _compute_boundary_normals_based_on_cell_orientation(cell_vertex_indices, p_keep_existing);
	CRASH_COND(_poly_cell_boundary_normals.size() != cell_vertex_indices.size());
	if (p_mode == COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY) {
		return;
	}
	// Ensure normals point outward from the mesh.
	Vector<PackedInt32Array> boundary_cell_indices = _poly_cell_indices[1];
	for (int64_t cell_index = 0; cell_index < cell_vertex_indices.size(); cell_index++) {
		const PackedInt32Array &vertex_indices = cell_vertex_indices[cell_index];
		VectorN average;
		for (int64_t vertex_index : vertex_indices) {
			ERR_FAIL_COND(vertex_index < 0 || vertex_index >= _poly_cell_vertices.size());
			average = VectorND::add(average, _poly_cell_vertices[vertex_index]);
		}
		average = VectorND::divide_scalar(average, (double)vertex_indices.size());
		if (VectorND::dot(average, _poly_cell_boundary_normals[cell_index]) < 0) {
			// Normal points inward, so flip it, and optionally correct the cell orientation.
			_poly_cell_boundary_normals.set(cell_index, VectorND::negate(_poly_cell_boundary_normals[cell_index]));
			if (p_mode == COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION) {
				// Reverse the order of the first two faces in this cell to flip its orientation.
				PackedInt32Array cell_face_indices = boundary_cell_indices[cell_index];
				const int32_t temp = cell_face_indices[0];
				cell_face_indices.set(0, cell_face_indices[1]);
				cell_face_indices.set(1, temp);
				boundary_cell_indices.set(cell_index, cell_face_indices);
			}
		}
	}
	if (p_mode == COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION) {
		_poly_cell_indices.set(1, boundary_cell_indices);
	}
}

void ArrayPolyMeshND::set_flat_shading_normals(const ComputeNormalsMode p_mode, const bool p_recalculate_boundary_normals) {
	_poly_cell_vertex_normals.clear();
	ERR_FAIL_COND_MSG(_poly_cell_indices.size() < 2, "ArrayPolyMeshND: Cannot calculate boundary normals because there are no boundary cells.");
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: Cannot calculate boundary normals for an invalid mesh.");
	if (p_recalculate_boundary_normals || _poly_cell_boundary_normals.size() != _poly_cell_indices[1].size()) {
		calculate_boundary_normals(p_mode);
	}
	const Vector<PackedInt32Array> cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, false);
	const int64_t cell_count = cell_vertex_indices.size();
	CRASH_COND(_poly_cell_boundary_normals.size() != cell_count);
	_poly_cell_vertex_normals.resize(cell_count);
	for (int64_t cell_index = 0; cell_index < cell_count; cell_index++) {
		Vector<VectorN> vertex_normals_for_cell;
		const VectorN &cell_normal = _poly_cell_boundary_normals[cell_index];
		const int64_t cell_vertex_count = cell_vertex_indices[cell_index].size();
		vertex_normals_for_cell.resize(cell_vertex_count);
		for (int64_t vertex_index = 0; vertex_index < cell_vertex_count; vertex_index++) {
			vertex_normals_for_cell.set(vertex_index, cell_normal);
		}
		_poly_cell_vertex_normals.set(cell_index, vertex_normals_for_cell);
	}
}

void ArrayPolyMeshND::calculate_seam_faces(const double p_angle_threshold_radians, const bool p_discard_seams_within_islands) {
	if (_poly_cell_boundary_normals.is_empty()) {
		calculate_boundary_normals(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, false);
	}
	_seam_indices.clear();
	HashSet<int32_t> cells_between_volumetric;
	const Vector<PackedInt32Array> &cell_face_indices = _poly_cell_indices[1];
	if (_poly_cell_indices.size() > 2) {
		// If this is a volumetric poly mesh with ND cells, ignore cells between volumetric cells.
		// Such cells are "virtual", only used to define geometry, and are not part of the visible surface.
		const Vector<PackedInt32Array> &volumetric_cell_indices = _poly_cell_indices[2];
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
	const Vector<PackedInt32Array> face_to_cell_map = _get_face_to_cell_map();
	// Actually calculate seams based on the angles between normals of adjacent cells.
	for (int64_t cell_index = 0; cell_index < cell_face_indices.size(); cell_index++) {
		if (cells_between_volumetric.has(cell_index)) {
			continue;
		}
		const PackedInt32Array &cell_faces = cell_face_indices[cell_index];
		for (int64_t face_index_in_cell = 0; face_index_in_cell < cell_faces.size(); face_index_in_cell++) {
			const int32_t face_index = cell_faces[face_index_in_cell];
			const PackedInt32Array &cells_using_face = face_to_cell_map[face_index];
			for (int64_t i = 0; i < cells_using_face.size(); i++) {
				const int32_t other_cell_index = cells_using_face[i];
				if (other_cell_index == cell_index || cells_between_volumetric.has(other_cell_index)) {
					continue;
				}
				const VectorN &normal_a = _poly_cell_boundary_normals[cell_index];
				const VectorN &normal_b = _poly_cell_boundary_normals[other_cell_index];
				if (VectorND::angle_to(normal_a, normal_b) > p_angle_threshold_radians) {
					_seam_indices.insert(face_index);
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
			const PackedInt32Array &cell_faces = cell_face_indices[cell_index];
			for (int64_t other_cell_index_index = cell_index + 1; other_cell_index_index < cells_in_island.size(); other_cell_index_index++) {
				const int32_t other_cell_index = cells_in_island[other_cell_index_index];
				const PackedInt32Array &other_cell_faces = cell_face_indices[other_cell_index];
				int64_t index_in_self;
				int64_t index_in_other;
				const int32_t common_face = MathND::find_common_int32(cell_faces, other_cell_faces, index_in_self, index_in_other);
				if (common_face != INT32_MIN) {
					_seam_indices.erase(common_face);
				}
			}
		}
	}
}

Vector<PackedInt32Array> ArrayPolyMeshND::_get_face_to_cell_map() const {
	// For efficiency, pre-compute a mapping of face index to the cells that use it.
	const Vector<PackedInt32Array> &face_edge_indices = _poly_cell_indices[0];
	const Vector<PackedInt32Array> &cell_face_indices = _poly_cell_indices[1];
	Vector<PackedInt32Array> face_to_cells;
	face_to_cells.resize(face_edge_indices.size());
	for (int64_t cell_index = 0; cell_index < cell_face_indices.size(); cell_index++) {
		const PackedInt32Array &cell_faces = cell_face_indices[cell_index];
		for (int64_t face_index_in_cell = 0; face_index_in_cell < cell_faces.size(); face_index_in_cell++) {
			const int32_t face_index = cell_faces[face_index_in_cell];
			CRASH_COND(face_index < 0 || face_index >= face_to_cells.size());
			PackedInt32Array cells_for_face = face_to_cells[face_index];
			cells_for_face.append(int32_t(cell_index));
			face_to_cells.set(face_index, cells_for_face);
		}
	}
	return face_to_cells;
}

PackedInt32Array ArrayPolyMeshND::_collect_cells_in_island_internal(const int64_t p_start_cell, const Vector<PackedInt32Array> &p_face_to_cell_map) {
	PackedInt32Array cells_in_island = { int32_t(p_start_cell) };
	PackedInt32Array faces_to_search = PackedInt32Array(_poly_cell_indices[1][p_start_cell]); // Copy.
	HashSet<int32_t> cells_in_island_set; // Redundant with cells_in_island, but faster to check existence for large meshes.
	HashSet<int32_t> faces_to_search_set; // Redundant with faces_to_search, but faster to check existence for large meshes.
	for (int32_t c : cells_in_island) {
		cells_in_island_set.insert(c);
	}
	for (int32_t f : faces_to_search) {
		faces_to_search_set.insert(f);
	}
	HashSet<int32_t> faces_already_searched;
	int64_t search_index = 0;
	while (search_index < faces_to_search.size()) {
		const int32_t face_index = faces_to_search[search_index];
		faces_already_searched.insert(face_index);
		if (_seam_indices.has(face_index)) {
			// This face is a seam, so do not cross it, as it may be another island.
			search_index++;
			continue;
		}
		const PackedInt32Array &cells_using_face = p_face_to_cell_map[face_index];
		for (int64_t i = 0; i < cells_using_face.size(); i++) {
			const int32_t cell_index = cells_using_face[i];
			if (!cells_in_island_set.has(cell_index)) {
				cells_in_island.append(cell_index);
				cells_in_island_set.insert(cell_index);
				// Add all faces of this new cell to the search list, except those already searched.
				const PackedInt32Array &new_cell_faces = _poly_cell_indices[1][cell_index];
				for (int64_t j = 0; j < new_cell_faces.size(); j++) {
					const int32_t new_face_index = new_cell_faces[j];
					if (!faces_already_searched.has(new_face_index) && !faces_to_search_set.has(new_face_index)) {
						faces_to_search.append(new_face_index);
						faces_to_search_set.insert(new_face_index);
					}
				}
			}
		}
		// If memory constraints are a concern, we could remove items from the start when this gets too big.
		// But in most cases, it's cheaper to just endlessly increase the search index.
		search_index++;
		faces_to_search_set.erase(face_index);
	}
	return cells_in_island;
}

PackedInt32Array ArrayPolyMeshND::collect_cells_in_island(const int64_t p_start_cell) {
	// This function is exposed so start by performing validation to avoid running with garbage data.
	ERR_FAIL_COND_V_MSG(_poly_cell_indices.size() < 2, PackedInt32Array(), "ArrayPolyMeshND: Cannot unwrap texture map for a mesh with no cells.");
	ERR_FAIL_INDEX_V_MSG(p_start_cell, _poly_cell_indices[1].size(), PackedInt32Array(), "ArrayPolyMeshND: The island start cell is not in the mesh.");
	ERR_FAIL_COND_V_MSG(!is_poly_mesh_data_valid(), PackedInt32Array(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot unwrap.");
	const Vector<PackedInt32Array> face_to_cell_map = _get_face_to_cell_map();
	// Use the internal version internally when we know the data is valid.
	return _collect_cells_in_island_internal(p_start_cell, face_to_cell_map);
}

Vector<PackedInt32Array> ArrayPolyMeshND::collect_all_islands() {
	ERR_FAIL_COND_V_MSG(_poly_cell_indices.size() < 2, Vector<PackedInt32Array>(), "ArrayPolyMeshND: Cannot unwrap texture map for a mesh with no cells.");
	ERR_FAIL_COND_V_MSG(!is_poly_mesh_data_valid(), Vector<PackedInt32Array>(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot unwrap.");
	const Vector<PackedInt32Array> face_to_cell_map = _get_face_to_cell_map();
	Vector<PackedInt32Array> islands;
	for (int64_t cell_index = 0; cell_index < _poly_cell_indices[1].size(); cell_index++) {
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
		islands.append(_collect_cells_in_island_internal(cell_index, face_to_cell_map));
	}
	return islands;
}

PackedInt32Array ArrayPolyMeshND::_get_cell_4_vertices_starting_from_face(const int64_t p_which_cell, const int64_t p_start_face_in_cell) const {
	const PackedInt32Array &cell_faces = _poly_cell_indices[1][p_which_cell];
	ERR_FAIL_INDEX_V(p_start_face_in_cell, cell_faces.size(), PackedInt32Array());
	const int32_t first_face_index = cell_faces[p_start_face_in_cell];
	const PackedInt32Array &first_face = _poly_cell_indices[0][first_face_index];
	int64_t common_in_first = 0;
	int64_t common_in_second = 0;
	int32_t common_edge = INT32_MIN;
	for (int64_t cell_face_index = 0; cell_face_index < cell_faces.size(); cell_face_index++) {
		const int32_t face_index = cell_faces[cell_face_index];
		if (face_index == first_face_index) {
			continue;
		}
		const PackedInt32Array &second_face = _poly_cell_indices[0][face_index];
		common_edge = MathND::find_common_int32(first_face, second_face, common_in_first, common_in_second);
		if (common_edge != INT32_MIN) {
			const int64_t first_next_edge = (common_in_first + 1) % first_face.size();
			const int64_t second_next_edge = (common_in_second + 1) % second_face.size();
			// Use these 3 edges to get 4 vertex indices in a consistent "winding" order.
			const int32_t common_vertex_start = _edge_vertex_indices[common_edge * 2];
			const int32_t common_vertex_end = _edge_vertex_indices[common_edge * 2 + 1];
			int32_t first_next_vertex = _edge_vertex_indices[first_face[first_next_edge] * 2];
			if (first_next_vertex == common_vertex_start || first_next_vertex == common_vertex_end) {
				first_next_vertex = _edge_vertex_indices[first_face[first_next_edge] * 2 + 1];
			}
			int32_t second_next_vertex = _edge_vertex_indices[second_face[second_next_edge] * 2];
			if (second_next_vertex == common_vertex_start || second_next_vertex == common_vertex_end) {
				second_next_vertex = _edge_vertex_indices[second_face[second_next_edge] * 2 + 1];
			}
			return PackedInt32Array{ first_next_vertex, common_vertex_start, common_vertex_end, second_next_vertex };
		}
	}
	ERR_FAIL_V_MSG(PackedInt32Array(), "ArrayPolyMeshND: Cell face does not share a common edge with any other face, this cell is invalid.");
}

Vector<VectorN> ArrayPolyMeshND::_get_cell_world_span_seed(const int64_t p_which_cell, int32_t &p_pivot) const {
	//const PackedInt32Array &cell_faces = _poly_cell_indices[1][p_which_cell];
	//const int32_t first_face_index = cell_faces[0];
	//const int32_t second_face_index = cell_faces[1];
	//const PackedInt32Array &first_face = _poly_cell_indices[0][first_face_index];
	//const PackedInt32Array &second_face = _poly_cell_indices[0][second_face_index];
	//int64_t common_in_first = 0;
	//int64_t common_in_second = 0;
	//int32_t common_edge = MathND::find_common_int32(first_face, second_face, common_in_first, common_in_second);
	//ERR_FAIL_COND_MSG(common_edge == INT32_MIN, "ArrayPolyMeshND: First two faces of cell do not share a common edge, this cell is invalid.");
	//const int64_t first_next_edge = (common_in_first + 1) % first_face.size();
	//const int64_t second_next_edge = (common_in_second + 1) % second_face.size();
	//// Use these 3 edges to get 4 vertex indices in a consistent "winding" order.
	//int32_t common_vertex_start = _edge_vertex_indices[common_edge * 2];
	//int32_t common_vertex_end = _edge_vertex_indices[common_edge * 2 + 1];
	//int32_t first_next_vertex = _edge_vertex_indices[first_face[first_next_edge] * 2];
	//int32_t first_common_vertex = _edge_vertex_indices[first_face[first_next_edge] * 2 + 1];
	//if (first_next_vertex == common_vertex_start || first_next_vertex == common_vertex_end) {
	//	SWAP(first_next_vertex, first_common_vertex);
	//}
	//p_pivot = first_next_vertex;
	//r_world_x = _poly_cell_vertices[first_next_vertex].direction_to(_poly_cell_vertices[first_common_vertex]);
	//if (first_common_vertex == common_vertex_start) {
	//	r_world_y = _poly_cell_vertices[first_common_vertex].direction_to(_poly_cell_vertices[common_vertex_end]);
	//} else {
	//	r_world_y = _poly_cell_vertices[first_common_vertex].direction_to(_poly_cell_vertices[common_vertex_start]);
	//}
	//int32_t second_next_vertex = _edge_vertex_indices[second_face[second_next_edge] * 2];
	//int32_t second_common_vertex = _edge_vertex_indices[second_face[second_next_edge] * 2 + 1];
	//if (second_next_vertex == common_vertex_start || second_next_vertex == common_vertex_end) {
	//	SWAP(second_next_vertex, second_common_vertex);
	//}
	//r_world_z = _poly_cell_vertices[second_common_vertex].direction_to(_poly_cell_vertices[second_next_vertex]);
}

void ArrayPolyMeshND::_transform_cell_to_texture_space(const TransformND &p_world_to_texcoord, const Vector<PackedInt32Array> &p_cell_vert, const int64_t p_cell_index, const int32_t p_pivot) {
	const PackedInt32Array &cell_vert = p_cell_vert[p_cell_index];
	Vector<VectorM> cell_texture_map;
	cell_texture_map.resize(cell_vert.size());
	for (int64_t i = 0; i < cell_vert.size(); i++) {
		// If we ever have 3.5-dimensional transforms, this would be a good place to use them.
		// But let's not make a whole new data structure for one use case, we'll use ND transforms instead.
		const VectorN offset = _poly_cell_vertices[cell_vert[i]] - _poly_cell_vertices[p_pivot];
		const VectorN rel = p_world_to_texcoord.xform(offset);
		cell_texture_map.set(i, VectorM(rel.x, rel.y, rel.z));
	}
	_poly_cell_texture_map.set(p_cell_index, cell_texture_map);
}

bool ArrayPolyMeshND::_unwrap_texture_map_island_cell(const PackedInt32Array &p_cells_in_island, const int64_t p_current_cell_index_index, const Vector<PackedInt32Array> &p_cell_vert) {
	const int32_t cell_index = p_cells_in_island[p_current_cell_index_index];
	const PackedInt32Array &cell_data = _poly_cell_indices[1][cell_index];
	// For the first cell in the island, there is nothing to "build on",
	// so just start by directly projecting.
	if (p_current_cell_index_index == 0) {
		//VectorN world_x, world_y, world_z;
		int32_t pivot;
		Vector<VectorN> world_span = _get_cell_world_span_seed(cell_index, pivot);
		const BasisND world_coord = BasisND::from_xyz(world_x, world_y, world_z).orthonormalized();
		ERR_FAIL_COND_V_MSG(Math::is_zero_approx(world_coord.determinant()), false, "ArrayPolyMeshND: Cell is degenerate.");
		// This needs to be "flattened" into the UVW texture space.
		BasisND tex_coord = BasisND(world_coord);
		tex_coord.x.w = 0.0;
		tex_coord.y.w = 0.0;
		tex_coord.z.w = 0.0;
		tex_coord.orthonormalize();
		if (Math::is_zero_approx(tex_coord.determinant())) {
			tex_coord = BasisND();
		}
		const TransformND world_to_texcoord = TransformND(world_coord.transform_to(tex_coord));
		_transform_cell_to_texture_space(world_to_texcoord, p_cell_vert, cell_index, pivot);
		return true;
	}
	// Search for neighboring cells in this island which share a face with this cell.
	for (int64_t already_mapped_cell_index_index = 0; already_mapped_cell_index_index < p_current_cell_index_index; already_mapped_cell_index_index++) {
		const int32_t already_mapped_cell_index = p_cells_in_island[already_mapped_cell_index_index];
		const PackedInt32Array &already_mapped_cell_data = _poly_cell_indices[1][already_mapped_cell_index];
		int64_t cell_data_index;
		int64_t already_mapped_cell_data_index;
		const int32_t common_face = MathND::find_common_int32(cell_data, already_mapped_cell_data, cell_data_index, already_mapped_cell_data_index);
		if (common_face != INT32_MIN) {
			const PackedInt32Array cell_span = _get_cell_4_vertices_starting_from_face(cell_index, cell_data_index);
			ERR_FAIL_COND_V_MSG(cell_span.size() != 4, false, "ArrayPolyMeshND: Failed to get 4 vertex span for cell.");
			//// Don't normalize X and Y because the lengths of these matter.
			//const VectorN world_x = _poly_cell_vertices[cell_span[1]] - _poly_cell_vertices[cell_span[0]];
			//const VectorN world_y = _poly_cell_vertices[cell_span[2]] - _poly_cell_vertices[cell_span[0]];
			//// Z needs to be perpendicular to X and Y and the length proportion needs to be consistent between world and texcoord space.
			//VectorN world_z = _poly_cell_vertices[cell_span[3]] - _poly_cell_vertices[cell_span[0]];
			//const real_t world_z_len = world_x.length() * world_y.length();
			//world_z = VectorND::orthogonal_from_two(world_z, world_x, world_y).normalized() * world_z_len;
			//ERR_FAIL_COND_V_MSG(Math::is_zero_approx(world_z.length()), false, "ArrayPolyMeshND: Cell is degenerate.");
			//const BasisND world_coord = BasisND::from_xyz(world_x, world_y, world_z);
			// Don't use Math::is_zero_approx here because this will be very small for small cells.
			// Instead let's hand-roll our own tolerance epsilon based on the lengths of the axes.
			// world_z_len already includes X and Y, so this accounts for all axes, and therefore
			// the tolerance has an effective dimensionality of length^4.
			real_t tolerance = world_z_len * world_z_len * CMP_EPSILON;
			if (tolerance < 1e-38) {
				tolerance = 1e-38; // Lower bound based on 32-bit floats.
			}
			ERR_FAIL_COND_V_MSG(Math::abs(world_coord.determinant()) < tolerance, false, "ArrayPolyMeshND: Cell is degenerate.");
			const PackedInt32Array &already_mapped_cell_verts = p_cell_vert[already_mapped_cell_index];
			const Vector<VectorM> &already_mapped_texture_map = _poly_cell_texture_map[already_mapped_cell_index];
			const VectorM texcoord_start = already_mapped_texture_map[already_mapped_cell_verts.find(cell_span[0])];
			const VectorM texcoord_x = already_mapped_texture_map[already_mapped_cell_verts.find(cell_span[1])] - texcoord_start;
			const VectorM texcoord_y = already_mapped_texture_map[already_mapped_cell_verts.find(cell_span[2])] - texcoord_start;
			// This could be `length_squared()`, so long as the world's lengths were also changed, but then it would
			// fail for vertex separations around 10^-9 or smaller, or vertex separations around 10^9 or bigger,
			// which I think is... not entirely unreasonable of a use case, so let's just use `length()`.
			VectorM texcoord_z = texcoord_x.cross(texcoord_y).normalized() * (texcoord_x.length() * texcoord_y.length());
			VectorM already_mapped_average;
			for (int64_t i = 0; i < already_mapped_texture_map.size(); i++) {
				already_mapped_average += already_mapped_texture_map[i];
			}
			already_mapped_average /= (real_t)already_mapped_texture_map.size();
			const VectorM to_already_mapped_average = texcoord_start.direction_to(already_mapped_average);
			// We want the Z axis to point "away" from the already mapped cell.
			if (to_already_mapped_average.dot(texcoord_z) > 0) {
				texcoord_z = -texcoord_z;
			}
			const BasisND tex_coord = BasisND::from_xyz(VectorND::from_3d(texcoord_x), VectorND::from_3d(texcoord_y), VectorND::from_3d(texcoord_z));
			const TransformND world_to_texcoord = TransformND(world_coord.transform_to(tex_coord), VectorND::from_3d(texcoord_start));
			_transform_cell_to_texture_space(world_to_texcoord, p_cell_vert, cell_index, cell_span[0]);
			return true;
		}
	}
	ERR_FAIL_V_MSG(false, "ArrayPolyMeshND: Island cells are not contiguous, cannot unwrap.");
}

void ArrayPolyMeshND::_unwrap_texture_map_island_internal(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing) {
	_poly_cell_texture_map.resize(_poly_cell_indices[1].size());
	const Vector<PackedInt32Array> cell_vert = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, false);
	for (int64_t cell_index_index = 0; cell_index_index < p_cells_in_island.size(); cell_index_index++) {
		if (p_keep_existing && !_poly_cell_texture_map[p_cells_in_island[cell_index_index]].is_empty()) {
			continue;
		}
		_unwrap_texture_map_island_cell(p_cells_in_island, cell_index_index, cell_vert);
	}
}

void ArrayPolyMeshND::unwrap_texture_map_island(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing) {
	// This function is exposed so start by performing validation to avoid running with garbage data.
	ERR_FAIL_COND_MSG(_poly_cell_indices.size() < 2, "ArrayPolyMeshND: Cannot unwrap texture map for a mesh with no cells.");
	ERR_FAIL_COND_MSG(p_cells_in_island.is_empty(), "ArrayPolyMeshND: Cannot unwrap texture map for an empty island of cells.");
	const Vector<PackedInt32Array> cells = _poly_cell_indices[1];
	for (int64_t i = 0; i < p_cells_in_island.size(); i++) {
		ERR_FAIL_COND_MSG(p_cells_in_island[i] >= cells.size(), "ArrayPolyMeshND: A cell in this island is not in the mesh.");
	}
	ERR_FAIL_COND_MSG(!is_poly_mesh_data_valid(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot unwrap.");
	// Use the internal version internally when we know the data is valid.
	_unwrap_texture_map_island_internal(p_cells_in_island, p_keep_existing);
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::unwrap_texture_map(const UnwrapTextureMapMode p_mode, const double p_padding, const bool p_keep_existing) {
	ERR_FAIL_COND_MSG(p_padding < 0.0, "ArrayPolyMeshND: Padding must be non-negative.");
	ERR_FAIL_COND_MSG(_poly_cell_indices.size() < 2, "ArrayPolyMeshND: Cannot unwrap texture map for a mesh with no cells.");
	ERR_FAIL_COND_MSG(!is_poly_mesh_data_valid(), "ArrayPolyMeshND: Poly mesh data is invalid, cannot unwrap.");
	if (!p_keep_existing) {
		_poly_cell_texture_map.clear();
	}
	UnwrapTextureMapMode actual_mode = p_mode;
	if (actual_mode == UNWRAP_MODE_AUTOMATIC) {
		actual_mode = _seam_indices.is_empty() ? UNWRAP_MODE_TILE_CELLS : UNWRAP_MODE_ISLANDS_FROM_SEAMS;
	}
	const double pad_offset = p_padding * 0.5 / (1.0 + p_padding);
	const double pad_size = 1.0 / (1.0 + p_padding);
	const int32_t cell_count = _poly_cell_indices[1].size();
	// Handle EACH_CELL_FILLS mode separately since it does not require tiling.
	if (actual_mode == UNWRAP_MODE_EACH_CELL_FILLS) {
		const AABB padded_full_aabb = AABB(VectorM(pad_offset, pad_offset, pad_offset), VectorM(pad_size, pad_size, pad_size));
		for (int32_t cell_index = 0; cell_index < cell_count; cell_index++) {
			const PackedInt32Array single_cell_island = PackedInt32Array{ cell_index };
			_unwrap_texture_map_island_internal(single_cell_island, p_keep_existing);
			_fit_island_texture_map_into_aabb(single_cell_island, padded_full_aabb);
		}
		poly_mesh_clear_cache();
		return;
	}
	// Handle TILE_CELLS and ISLANDS_FROM_SEAMS modes below, since both require tiling.
	Vector<PackedInt32Array> islands;
	if (actual_mode == UNWRAP_MODE_TILE_CELLS) {
		islands.resize(cell_count);
		for (int32_t cell_index = 0; cell_index < cell_count; cell_index++) {
			islands.set(cell_index, PackedInt32Array{ cell_index });
		}
	} else if (actual_mode == UNWRAP_MODE_ISLANDS_FROM_SEAMS) {
		islands = collect_all_islands();
	} else {
		ERR_FAIL_MSG("ArrayPolyMeshND: Unknown unwrap texture map mode.");
	}
	for (int64_t island_index = 0; island_index < islands.size(); island_index++) {
		_unwrap_texture_map_island_internal(islands[island_index], p_keep_existing);
	}
	// Tile the islands in texture space by rescaling and offsetting them.
	const VectorMi tiles = _tiles_for_island_count(islands.size());
	const VectorM unpadded_tile_size = VectorM(1.0 / real_t(tiles.x), 1.0 / real_t(tiles.y), 1.0 / real_t(tiles.z));
	const VectorM padding_offset = unpadded_tile_size * pad_offset;
	const VectorM padded_tile_size = unpadded_tile_size * pad_size;
	for (int32_t x = 0; x < tiles.x; x++) {
		for (int32_t y = 0; y < tiles.y; y++) {
			for (int32_t z = 0; z < tiles.z; z++) {
				const int32_t tile_index = x + y * tiles.x + z * tiles.x * tiles.y;
				if (tile_index >= islands.size()) {
					break;
				}
				const PackedInt32Array &cells_in_island = islands[tile_index];
				const AABB island_aabb = AABB(VectorM(x, y, z) * unpadded_tile_size + padding_offset, padded_tile_size);
				_fit_island_texture_map_into_aabb(cells_in_island, island_aabb);
			}
		}
	}
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::_fit_island_texture_map_into_aabb(const PackedInt32Array &p_cells_in_island, const AABB &p_target_aabb) {
	AABB current_aabb = AABB(_poly_cell_texture_map[p_cells_in_island[0]][0], VectorM());
	for (int64_t cell_index_index = 0; cell_index_index < p_cells_in_island.size(); cell_index_index++) {
		const int32_t cell_index = p_cells_in_island[cell_index_index];
		const Vector<VectorM> &cell_texture_map = _poly_cell_texture_map[cell_index];
		for (int64_t vertex_index = 0; vertex_index < cell_texture_map.size(); vertex_index++) {
			current_aabb.expand_to(cell_texture_map[vertex_index]);
		}
	}
	const Basis scale = Basis::from_scale(p_target_aabb.size / current_aabb.size);
	// Note: xform_inv performs a transposed xform, not an inverse xform, which is what we want in this case.
	// This is badly named in Godot, see this proposal: https://github.com/godotengine/godot-proposals/issues/11235
	const Transform3D to_target = Transform3D(scale, p_target_aabb.position - scale.xform_inv(current_aabb.position));
	for (int64_t cell_index_index = 0; cell_index_index < p_cells_in_island.size(); cell_index_index++) {
		const int32_t cell_index = p_cells_in_island[cell_index_index];
		Vector<VectorM> cell_texture_map = _poly_cell_texture_map[cell_index];
		for (int64_t vertex_index = 0; vertex_index < cell_texture_map.size(); vertex_index++) {
			cell_texture_map.set(vertex_index, to_target.xform(cell_texture_map[vertex_index]));
		}
		_poly_cell_texture_map.set(cell_index, cell_texture_map);
	}
}

VectorMi ArrayPolyMeshND::_tiles_for_island_count(const int32_t p_island_count, const int p_dimension) {
	VectorMi tiles;
	tiles.resize(p_dimension);
	if (p_island_count < 2) {
		for (int i = 0; i < p_dimension; i++) {
			tiles.set(i, 1);
		}
		return tiles;
	}
	int32_t remaining = p_island_count;
	for (int i = p_dimension - 1; i >= 0; i--) {
		// For the last axis (highest index), take the p_dimension-th root.
		// For the next one, (p_dimension - 1)-th root, and so on.
		const double root = std::pow((double)remaining, 1.0 / (i + 1));
		const int32_t value = Math::ceil(root);
		tiles.set(i, value);
		remaining = _ceil_div(remaining, value);
	}
	return tiles;
}

void ArrayPolyMeshND::transform_texture_map(const Ref<TransformND> &p_texture_transform) {
	const int64_t cell_count = _poly_cell_texture_map.size();
	for (int64_t cell_index = 0; cell_index < cell_count; cell_index++) {
		Vector<VectorM> cell_texture_map = _poly_cell_texture_map[cell_index];
		for (int64_t vertex_index = 0; vertex_index < cell_texture_map.size(); vertex_index++) {
			cell_texture_map.set(vertex_index, p_texture_transform->xform(cell_texture_map[vertex_index]));
		}
		_poly_cell_texture_map.set(cell_index, cell_texture_map);
	}
	poly_mesh_clear_cache();
}

void ArrayPolyMeshND::merge_with(const Ref<PolyMeshND> &p_other, const Ref<TransformND> &p_transform) {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "ArrayPolyMeshND: This mesh is invalid, cannot merge another mesh into it.");
	ERR_FAIL_COND_MSG(p_other.is_null() || !p_other->is_mesh_data_valid(), "ArrayPolyMeshND: Cannot merge an invalid PolyMeshND into this mesh.");
	const Vector<Vector<PackedInt32Array>> &other_poly_cell_indices = p_other->get_poly_cell_indices();
	const Vector<Vector<VectorM>> &other_poly_cell_texture_map = p_other->get_poly_cell_texture_map();
	const Vector<Vector<VectorN>> &other_poly_cell_vertex_normals = p_other->get_poly_cell_vertex_normals();
	const Vector<VectorN> &other_poly_cell_boundary_normals = p_other->get_poly_cell_boundary_normals();
	const Vector<VectorN> &other_poly_cell_vertices = p_other->get_poly_cell_vertices();
	const PackedInt32Array &other_edge_indices = p_other->get_edge_indices();
	const HashSet<int32_t> &other_seam_indices = p_other->get_seam_indices();
	const int64_t start_vertex_count = _poly_cell_vertices.size();
	const int64_t start_edge_index_count = _edge_vertex_indices.size();
	const int64_t other_vertex_count = other_poly_cell_vertices.size();
	const int64_t other_edge_index_count = other_edge_indices.size();
	const int64_t end_vertex_count = start_vertex_count + other_vertex_count;
	const int64_t end_edge_index_count = start_edge_index_count + other_edge_index_count;
	// Expand the poly cell indices dimensions if needed and count how many entries are in each dimension.
	int64_t poly_cell_indices_dims = other_poly_cell_indices.size();
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
		other_poly_cell_indices_counts.set(dim_index, other_poly_cell_indices[dim_index].size());
	}
	// Merge vertices.
	_poly_cell_vertices.resize(end_vertex_count);
	for (int64_t i = 0; i < other_vertex_count; i++) {
		_poly_cell_vertices.set(start_vertex_count + i, p_transform->xform(other_poly_cell_vertices[i]));
	}
	// Merge edges.
	_edge_vertex_indices.resize(end_edge_index_count);
	for (int64_t i = 0; i < other_edge_index_count; i += 2) {
		_edge_vertex_indices.set(start_edge_index_count + i, other_edge_indices[i] + int32_t(start_vertex_count));
		_edge_vertex_indices.set(start_edge_index_count + i + 1, other_edge_indices[i + 1] + int32_t(start_vertex_count));
	}
	// Merge poly cell indices.
	for (int64_t dim_index = 0; dim_index < poly_cell_indices_dims; dim_index++) {
		Vector<PackedInt32Array> this_dim = _poly_cell_indices[dim_index];
		const Vector<PackedInt32Array> &other_dim = other_poly_cell_indices[dim_index];
		const int64_t start_count = start_poly_cell_indices_counts[dim_index];
		const int64_t other_count = other_poly_cell_indices_counts[dim_index];
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
	// Merge seams.
	const int32_t adjust_seam_face = start_poly_cell_indices_counts[0];
	for (const int32_t seam_face_index : other_seam_indices) {
		_seam_indices.insert(seam_face_index + adjust_seam_face);
	}
	// Merge cell boundary and vertex normals. These need to stay aligned with the boundary cell indices.
	{
		const Vector<PackedInt32Array> cell_vertex_instances_span_first = _get_vertex_indices_of_boundary_cells(_poly_cell_indices, _edge_vertex_indices, true);
		int64_t start_poly_cell_boundary_normal_count = _poly_cell_boundary_normals.size();
		int64_t other_poly_cell_boundary_normal_count = other_poly_cell_boundary_normals.size();
		int64_t end_poly_cell_boundary_normal_count = start_poly_cell_boundary_normal_count + other_poly_cell_boundary_normal_count;
		int64_t start_poly_cell_vertex_normal_count = _poly_cell_vertex_normals.size();
		int64_t other_poly_cell_vertex_normal_count = other_poly_cell_vertex_normals.size();
		int64_t end_poly_cell_vertex_normal_count = start_poly_cell_vertex_normal_count + other_poly_cell_vertex_normal_count;
		if (end_poly_cell_boundary_normal_count > 0 || end_poly_cell_vertex_normal_count > 0) {
			// Merge cell boundary normals if they exist.
			if (end_poly_cell_boundary_normal_count > 0) {
				if (start_poly_cell_boundary_normal_count < start_poly_cell_indices_counts[1]) {
					// If the first mesh has fewer boundary normals than boundary cells, we need to
					// generate the missing ones first before appending the new ones to the end.
					_poly_cell_boundary_normals = _compute_boundary_normals_based_on_cell_orientation(cell_vertex_instances_span_first, true);
					CRASH_COND(_poly_cell_boundary_normals.size() != start_poly_cell_indices_counts[1]);
					start_poly_cell_boundary_normal_count = start_poly_cell_indices_counts[1];
					end_poly_cell_boundary_normal_count = start_poly_cell_boundary_normal_count + other_poly_cell_boundary_normal_count;
				}
				// Actually merge, transforming the normals with the basis of the provided transform.
				_poly_cell_boundary_normals.resize(end_poly_cell_boundary_normal_count);
				for (int64_t cell_index = 0; cell_index < other_poly_cell_boundary_normals.size(); cell_index++) {
					_poly_cell_boundary_normals.set(start_poly_cell_boundary_normal_count + cell_index, p_transform->xform_basis(other_poly_cell_boundary_normals[cell_index]));
				}
			}
			// We also need to merge vertex normals if they exist, but first check if we need to
			// generate boundary normals, since boundary normals are a prerequisite for vertex normals,
			// in case we need to read from them to generate new vertex normals.
			if (end_poly_cell_vertex_normal_count > 0 || other_poly_cell_boundary_normal_count < other_poly_cell_indices_counts[1]) {
				other_poly_cell_boundary_normal_count = other_poly_cell_indices_counts[1];
				end_poly_cell_boundary_normal_count = start_poly_cell_boundary_normal_count + other_poly_cell_boundary_normal_count;
				_poly_cell_boundary_normals = _compute_boundary_normals_based_on_cell_orientation(cell_vertex_instances_span_first, true);
				CRASH_COND(_poly_cell_boundary_normals.size() != end_poly_cell_boundary_normal_count);
			}
			// Merge cell vertex normals. These need to stay aligned with the boundary cell vertex instances.
			if (end_poly_cell_vertex_normal_count > 0) {
				if (start_poly_cell_vertex_normal_count < start_poly_cell_indices_counts[1]) {
					// If the first mesh has fewer vertex normals than boundary cells, we need to
					// generate the missing ones first before appending the new ones to the end.
					_poly_cell_vertex_normals.resize(start_poly_cell_indices_counts[1]);
					for (int64_t cell_index = start_poly_cell_vertex_normal_count; cell_index < start_poly_cell_indices_counts[1]; cell_index++) {
						Vector<VectorN> flat_shading_vertex_normals;
						const VectorN cell_boundary_normal = _poly_cell_boundary_normals[cell_index];
						const int64_t vertex_instances_in_cell = cell_vertex_instances_span_first[cell_index].size();
						flat_shading_vertex_normals.resize(vertex_instances_in_cell);
						for (int64_t vert_inst = 0; vert_inst < vertex_instances_in_cell; vert_inst++) {
							flat_shading_vertex_normals.set(vert_inst, cell_boundary_normal);
						}
						_poly_cell_vertex_normals.set(cell_index, flat_shading_vertex_normals);
					}
					start_poly_cell_vertex_normal_count = start_poly_cell_indices_counts[1];
					end_poly_cell_vertex_normal_count = start_poly_cell_vertex_normal_count + other_poly_cell_vertex_normal_count;
				}
				// Actually merge, transforming the normals with the basis of the provided transform.
				_poly_cell_vertex_normals.resize(end_poly_cell_vertex_normal_count);
				for (int64_t cell_index = 0; cell_index < other_poly_cell_vertex_normals.size(); cell_index++) {
					Vector<VectorN> transformed_vertex_normals;
					const Vector<VectorN> &other_vertex_normals = other_poly_cell_vertex_normals[cell_index];
					transformed_vertex_normals.resize(other_vertex_normals.size());
					for (int64_t vert_inst = 0; vert_inst < other_vertex_normals.size(); vert_inst++) {
						transformed_vertex_normals.set(vert_inst, p_transform->xform_basis(other_vertex_normals[vert_inst]));
					}
					_poly_cell_vertex_normals.set(start_poly_cell_vertex_normal_count + cell_index, transformed_vertex_normals);
				}
			}
		}
	}
	// Merge UVW texture maps. These need to stay aligned with the boundary cell indices.
	{
		const int64_t start_poly_cell_texture_map_count = _poly_cell_texture_map.size();
		const int64_t other_poly_cell_texture_map_count = other_poly_cell_texture_map.size();
		const int64_t end_poly_cell_texture_map_count = start_poly_cell_texture_map_count + other_poly_cell_texture_map_count;
		if (end_poly_cell_texture_map_count > 0) {
			// We need to keep these aligned, so missing entries need to exist. With the normals, there was a way to
			// generate missing entries, but with texture maps, just let them be default-initialized to empty arrays.
			// PolyMeshND::get_cell_texture_map can turn these into zero vectors for the cellhedral cell UVW maps.
			_poly_cell_texture_map.resize(end_poly_cell_texture_map_count);
			for (int64_t cell_index = 0; cell_index < other_poly_cell_texture_map.size(); cell_index++) {
				_poly_cell_texture_map.set(start_poly_cell_texture_map_count + cell_index, other_poly_cell_texture_map[cell_index]);
			}
		}
	}
	// Merge materials.
	Ref<MaterialND> other_material = p_other->get_material();
	if (other_material.is_valid()) {
		Ref<MaterialND> self_material = get_material();
		if (self_material.is_valid()) {
			self_material->merge_with(other_material, start_vertex_count, other_vertex_count);
		} else if (other_material->get_albedo_color_array().size() > 0) {
			self_material.instantiate();
			self_material->merge_with(other_material, start_vertex_count, other_vertex_count);
			set_material(self_material);
		} else {
			set_material(other_material);
		}
	}
	reset_poly_mesh_data_validation();
}

Vector<PackedInt32Array> ArrayPolyMeshND::_compose_triangles_into_faces(const Vector<VectorN> &p_vertices, const Vector<PackedInt32Array> &p_triangle_vertex_indices, PackedInt32Array &r_edge_vertex_indices) {
	Vector<Vector<PackedInt32Array>> coplanar_triangle_vertex_indices;
	const int64_t triangle_count = p_triangle_vertex_indices.size();
	// Determine which triangles are coplanar with each other so we know what to build the faces with.
	for (int64_t triangle_index = 0; triangle_index < triangle_count; triangle_index++) {
		const PackedInt32Array &triangle = p_triangle_vertex_indices[triangle_index];
		bool is_new_face = true;
		for (int64_t coplanar_index = 0; coplanar_index < coplanar_triangle_vertex_indices.size(); coplanar_index++) {
			const PackedInt32Array &coplanar_representative = coplanar_triangle_vertex_indices[coplanar_index][0];
			bool is_coplanar = true;
			for (int64_t vertex_in_triangle = 0; vertex_in_triangle < 3; vertex_in_triangle++) {
				//const VectorN perp = VectorND::perpendicular(
				//		p_vertices[triangle[vertex_in_triangle]].direction_to(p_vertices[coplanar_representative[0]]),
				//		p_vertices[triangle[vertex_in_triangle]].direction_to(p_vertices[coplanar_representative[1]]),
				//		p_vertices[triangle[vertex_in_triangle]].direction_to(p_vertices[coplanar_representative[2]]));
				if (!perp.is_zero_approx()) {
					is_coplanar = false; // If it's non-zero, it's not coplanar. Contrapositive of: if it's coplanar, all perp calculations are zero.
					break;
				}
			}
			if (is_coplanar) {
				Vector<PackedInt32Array> coplanar_triangles = coplanar_triangle_vertex_indices[coplanar_index];
				coplanar_triangles.append(triangle);
				coplanar_triangle_vertex_indices.set(coplanar_index, coplanar_triangles);
				is_new_face = false; // If it's coplanar, it's not a new face. Contrapositive of: if it's a new face, all vertices are coplanar.
				break;
			}
		}
		if (is_new_face) {
			// If this isn't coplanar with anything so far, it's a new face.
			coplanar_triangle_vertex_indices.append(Vector<PackedInt32Array>{ triangle });
		}
	}
	// Find the edges which border each face.
	Vector<PackedInt32Array> all_face_edges;
	for (int64_t coplanar_tri_set_index = 0; coplanar_tri_set_index < coplanar_triangle_vertex_indices.size(); coplanar_tri_set_index++) {
		HashSet<Vector2i> face_unique_edges;
		const Vector<PackedInt32Array> &coplanar_tri_set = coplanar_triangle_vertex_indices[coplanar_tri_set_index];
		for (const PackedInt32Array &tri_verts : coplanar_tri_set) {
			for (int64_t vertex_in_triangle = 0; vertex_in_triangle < 3; vertex_in_triangle++) {
				Vector2i edge = Vector2i(tri_verts[vertex_in_triangle], tri_verts[(vertex_in_triangle + 1) % 3]);
				if (edge.x > edge.y) {
					SWAP(edge.x, edge.y);
				}
				// This assumes that each edge is only used by up to 2 coplanar triangles.
				// This is true of all sane manifold meshes, other cases are not supported.
				if (face_unique_edges.has(edge)) {
					face_unique_edges.erase(edge);
				} else {
					face_unique_edges.insert(edge);
				}
			}
		}
		// Insert the unique edges into the array, in some consistent winding order. Any winding will work fine.
		PackedInt32Array face_edges;
		face_edges.resize(face_unique_edges.size());
		Vector2i last_edge = *(face_unique_edges.begin());
		face_edges.set(0, _append_edge_indices_internal(last_edge.x, last_edge.y, true, r_edge_vertex_indices));
		face_unique_edges.erase(last_edge);
		int64_t face_edges_index = 1;
		while (!face_unique_edges.is_empty()) {
			bool inserted_something = false;
			for (const Vector2i unique_edge : face_unique_edges) {
				if ((unique_edge.x == last_edge.x) || (unique_edge.x == last_edge.y) || (unique_edge.y == last_edge.x) || (unique_edge.y == last_edge.y)) {
					// This edge shares a vertex with the last edge, so let's insert it.
					const int64_t edge_index = _append_edge_indices_internal(unique_edge.x, unique_edge.y, true, r_edge_vertex_indices);
					face_edges.set(face_edges_index, edge_index);
					face_edges_index++;
					face_unique_edges.erase(unique_edge);
					last_edge = unique_edge;
					inserted_something = true;
					break;
				}
			}
			// Avoid infinite loops, fail if we didn't insert anything.
			ERR_FAIL_COND_V(!inserted_something, Vector<PackedInt32Array>());
		}
		all_face_edges.append(face_edges);
	}
	return all_face_edges;
}

int64_t ArrayPolyMeshND::_append_face_internal(const PackedInt32Array &p_face, Vector<PackedInt32Array> &r_all_face_edge_indices) {
	const int64_t existing_face_count = r_all_face_edge_indices.size();
	const int64_t face_edge_count = p_face.size();
	for (int64_t existing_face_index = 0; existing_face_index < existing_face_count; existing_face_index++) {
		const PackedInt32Array &existing_face = r_all_face_edge_indices[existing_face_index];
		if (face_edge_count != existing_face.size()) {
			continue;
		}
		const int64_t start_index = existing_face.find(p_face[0]);
		if (start_index == -1) {
			continue;
		}
		// Check if the order matches in the same direction.
		bool found = true;
		for (int64_t i = 1; i < face_edge_count; i++) {
			if (p_face[i] != existing_face[(start_index + i) % face_edge_count]) {
				found = false;
				break;
			}
		}
		if (found) {
			return existing_face_index;
		}
		// Check if the order matches in the opposite direction.
		found = true;
		for (int64_t i = 1; i < face_edge_count; i++) {
			int64_t existing_index = start_index - i;
			if (existing_index < 0) {
				existing_index += face_edge_count;
			}
			if (p_face[i] != existing_face[existing_index]) {
				found = false;
				break;
			}
		}
		if (found) {
			return existing_face_index;
		}
	}
	r_all_face_edge_indices.append(p_face);
	return existing_face_count;
}

VectorN ArrayPolyMeshND::_compute_cell_normal(const PackedInt32Array &p_cell_first_face, const PackedInt32Array &p_cell_second_face, const PackedInt32Array &p_edge_vertex_indices, const Vector<VectorN> &p_vertices) {
	int64_t common_in_first;
	int64_t common_in_second;
	const int32_t common_edge = MathND::find_common_int32(p_cell_first_face, p_cell_second_face, common_in_first, common_in_second);
	if (common_edge == INT32_MIN) {
		return VectorN();
	}
	const int64_t first_next_index = (common_in_first + 1) % p_cell_first_face.size();
	const int64_t second_next_index = (common_in_second + 1) % p_cell_second_face.size();
	// Use these 3 edges to get 4 vertex indices in a consistent "winding" order.
	const int32_t common_vertex_start = p_edge_vertex_indices[common_edge * 2];
	const int32_t common_vertex_end = p_edge_vertex_indices[common_edge * 2 + 1];
	int32_t first_next_vertex = p_edge_vertex_indices[p_cell_first_face[first_next_index] * 2];
	if (first_next_vertex == common_vertex_start || first_next_vertex == common_vertex_end) {
		first_next_vertex = p_edge_vertex_indices[p_cell_first_face[first_next_index] * 2 + 1];
	}
	int32_t second_next_vertex = p_edge_vertex_indices[p_cell_second_face[second_next_index] * 2];
	if (second_next_vertex == common_vertex_start || second_next_vertex == common_vertex_end) {
		second_next_vertex = p_edge_vertex_indices[p_cell_second_face[second_next_index] * 2 + 1];
	}
	Vector<PackedFloat64Array> input_vectors;
	//input_vectors.resize(get_dimension() - 1);
	//for (int64_t dim = 1; dim < dimension; dim++) {
	//			const int32_t vert_n = _simplex_cell_indices_cache[offset + dim];
	//			input_vectors.set(dim - 1,
	return VectorND::perpendicular(
			p_vertices[first_next_vertex].direction_to(p_vertices[common_vertex_start]),
			p_vertices[first_next_vertex].direction_to(p_vertices[common_vertex_end]),
			p_vertices[first_next_vertex].direction_to(p_vertices[second_next_vertex]));
}

PackedInt32Array ArrayPolyMeshND::_save_triangle_vertex_indices_as_faces_and_cell(const Vector<PackedInt32Array> &p_last_triangle_vertex_indices, const VectorN &p_last_simplex_normal, const Vector<VectorN> &p_vertices, Vector<PackedInt32Array> &r_all_face_edge_indices, PackedInt32Array &r_edge_vertex_indices) {
	const Vector<PackedInt32Array> composed_faces = _compose_triangles_into_faces(p_vertices, p_last_triangle_vertex_indices, r_edge_vertex_indices);
	PackedInt32Array cell_face_indices;
	const int64_t composed_face_count = composed_faces.size();
	cell_face_indices.resize(composed_face_count);
	for (int64_t i = 0; i < composed_face_count; i++) {
		cell_face_indices.set(i, _append_face_internal(composed_faces[i], r_all_face_edge_indices));
	}
	// Now that we have the cell's faces, we still need to decide in which order they appear in.
	// We want an order that results in the correct cell normal being computed.
	for (int64_t index_of_second_face = 1; index_of_second_face < composed_face_count; index_of_second_face++) {
		const VectorN candidate_normal = _compute_cell_normal(
				r_all_face_edge_indices[cell_face_indices[0]],
				r_all_face_edge_indices[cell_face_indices[index_of_second_face]],
				r_edge_vertex_indices, p_vertices);
		if (candidate_normal.is_zero_approx()) {
			continue;
		}
		const real_t dot = candidate_normal.dot(p_last_simplex_normal);
		if (dot > CMP_EPSILON) {
			// If these are aligned, then this is a correct configuration.
			if (index_of_second_face != 1) {
				const int32_t temp = cell_face_indices[index_of_second_face];
				cell_face_indices.set(index_of_second_face, cell_face_indices[1]);
				cell_face_indices.set(1, temp);
			}
			break;
		}
		if (dot < -CMP_EPSILON) {
			// If these are aligned, then these are a set of correct faces, but in the wrong order.
			const int32_t temp = cell_face_indices[index_of_second_face];
			cell_face_indices.set(index_of_second_face, cell_face_indices[1]);
			cell_face_indices.set(1, cell_face_indices[0]);
			cell_face_indices.set(0, temp);
			break;
		}
	}
	return cell_face_indices;
}

Ref<ArrayPolyMeshND> ArrayPolyMeshND::reconstruct_from_cell_mesh(const Ref<CellMeshND> &p_cell_mesh) {
	ERR_FAIL_COND_V(p_cell_mesh.is_null(), Ref<ArrayPolyMeshND>());
	Ref<ArrayPolyMeshND> ret;
	ret.instantiate();
	const Vector<VectorN> vertices = p_cell_mesh->get_vertices();
	ret->set_poly_cell_vertices(vertices);
	const PackedInt32Array simplex_indices = p_cell_mesh->get_cell_indices();
	const int dimension = vertices[0].size();
	const int64_t simplex_count = simplex_indices.size() / dimension;
	PackedInt32Array edge_vertex_indices;
	Vector<PackedInt32Array> all_face_edge_indices;
	Vector<PackedInt32Array> all_cell_face_indices;
	{
		Vector<PackedInt32Array> last_triangle_vertex_indices;
		VectorN last_simplex_normal;
		int64_t last_pivot = -1;
		for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
			const int64_t offset = simplex_index * 4;
			const int32_t pivot = simplex_indices[offset];
			if (pivot != last_pivot) {
				// This is a new cell, so save what we found so far, and start a new face list.
				if (simplex_index > 0) {
					all_cell_face_indices.append(_save_triangle_vertex_indices_as_faces_and_cell(
							last_triangle_vertex_indices, last_simplex_normal, vertices,
							all_face_edge_indices, edge_vertex_indices));
				}
				// Start a new face list.
				last_triangle_vertex_indices = Vector<PackedInt32Array>();
				const int32_t vert0 = simplex_indices[offset];
				Vector<PackedFloat64Array> input_vectors;
				input_vectors.resize(dimension - 1);
				for (int64_t dim = 1; dim < dimension; dim++) {
					const int32_t vert_n = simplex_indices[offset + dim];
					input_vectors.set(dim - 1, VectorND::direction_to(vertices[vert0], vertices[vert_n]));
				}
				last_simplex_normal = VectorND::perpendicular(input_vectors);
				last_pivot = pivot;
			}
			for (int64_t vertex_in_simplex = 0; vertex_in_simplex < 4; vertex_in_simplex++) {
				PackedInt32Array triangle_vertex_indices = {
					simplex_indices[offset + vertex_in_simplex],
					simplex_indices[offset + (vertex_in_simplex + 1) % 4],
					simplex_indices[offset + (vertex_in_simplex + 2) % 4],
				};
				triangle_vertex_indices.sort();
				const int64_t found_index = last_triangle_vertex_indices.find(triangle_vertex_indices);
				if (found_index == -1) {
					last_triangle_vertex_indices.append(triangle_vertex_indices);
				} else {
					last_triangle_vertex_indices.remove_at(found_index); // Internal face, not up for consideration.
				}
			}
		}
		// Save data for the last cell.
		all_cell_face_indices.append(_save_triangle_vertex_indices_as_faces_and_cell(
				last_triangle_vertex_indices, last_simplex_normal, vertices,
				all_face_edge_indices, edge_vertex_indices));
	}
	ret->set_edge_vertex_indices(edge_vertex_indices);
	ret->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ all_face_edge_indices, all_cell_face_indices });
	return ret;
}

PackedInt32Array ArrayPolyMeshND::get_edge_indices() {
	return _edge_vertex_indices;
}

void ArrayPolyMeshND::set_edge_vertex_indices(const PackedInt32Array &p_edge_indices) {
	_edge_vertex_indices = p_edge_indices;
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

Vector<Vector<PackedInt32Array>> ArrayPolyMeshND::get_poly_cell_indices() {
	return _poly_cell_indices;
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
	return _poly_cell_boundary_normals;
}

void ArrayPolyMeshND::set_poly_cell_boundary_normals(const Vector<VectorN> &p_poly_cell_boundary_normals) {
	_poly_cell_boundary_normals = p_poly_cell_boundary_normals;
	poly_mesh_clear_cache(true);
}

Vector<Vector<VectorN>> ArrayPolyMeshND::get_poly_cell_vertex_normals() {
	return Vector<Vector<VectorN>>(_poly_cell_vertex_normals);
}

void ArrayPolyMeshND::set_poly_cell_vertex_normals(const Vector<Vector<VectorN>> &p_poly_cell_vertex_normals) {
	_poly_cell_vertex_normals = Vector<Vector<VectorN>>(p_poly_cell_vertex_normals);
	poly_mesh_clear_cache(true);
}

void ArrayPolyMeshND::set_poly_cell_vertex_normals_bind(const TypedArray<Vector<VectorN>> &p_poly_cell_vertex_normals) {
	Vector<Vector<VectorN>> normals;
	normals.resize(p_poly_cell_vertex_normals.size());
	for (int i = 0; i < p_poly_cell_vertex_normals.size(); i++) {
		normals.set(i, p_poly_cell_vertex_normals[i]);
	}
	set_poly_cell_vertex_normals(normals);
}

Vector<Vector<VectorM>> ArrayPolyMeshND::get_poly_cell_texture_map() {
	return _poly_cell_texture_map;
}

void ArrayPolyMeshND::set_poly_cell_texture_map(const Vector<Vector<VectorM>> &p_poly_cell_texture_map) {
	_poly_cell_texture_map = p_poly_cell_texture_map;
	reset_poly_mesh_data_validation();
}

void ArrayPolyMeshND::set_poly_cell_texture_map_bind(const TypedArray<Vector<VectorM>> &p_poly_cell_texture_map) {
	Vector<Vector<VectorM>> texture_map;
	texture_map.resize(p_poly_cell_texture_map.size());
	for (int i = 0; i < p_poly_cell_texture_map.size(); i++) {
		texture_map.set(i, p_poly_cell_texture_map[i]);
	}
	set_poly_cell_texture_map(texture_map);
}

PackedInt32Array ArrayPolyMeshND::get_seam_indices_bind() const {
	PackedInt32Array seam_face_indices;
	seam_face_indices.resize(_seam_indices.size());
	int64_t seam_index = 0;
	for (const int32_t face_index : _seam_indices) {
		seam_face_indices.set(seam_index, face_index);
		seam_index++;
	}
	// The order does not matter, but sorting makes it stable and easier to read and compare.
	seam_face_indices.sort();
	return seam_face_indices;
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

Vector<VectorN> ArrayPolyMeshND::get_poly_cell_vertices() {
	return _poly_cell_vertices;
}

void ArrayPolyMeshND::set_poly_cell_vertices(const Vector<VectorN> &p_poly_cell_vertices) {
	ERR_FAIL_COND(p_poly_cell_vertices.size() > MAX_POLY_VERTICES); // Prevent overflow.
	_poly_cell_vertices = p_poly_cell_vertices;
	poly_mesh_clear_cache();
	reset_poly_mesh_data_validation();
}

void ArrayPolyMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_vertex", "vertex", "deduplicate_vertices"), &ArrayPolyMeshND::append_vertex, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("append_vertices", "vertices", "deduplicate_vertices"), &ArrayPolyMeshND::append_vertices, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("calculate_boundary_normals", "normals_mode", "keep_existing"), &ArrayPolyMeshND::calculate_boundary_normals, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("set_flat_shading_normals", "normals_mode", "recalculate_boundary_normals"), &ArrayPolyMeshND::set_flat_shading_normals, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("calculate_seam_faces", "angle_threshold_radians", "discard_seams_within_islands"), &ArrayPolyMeshND::calculate_seam_faces, DEFVAL(Math_TAU / 8.0), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("collect_cells_in_island", "start_cell"), &ArrayPolyMeshND::collect_cells_in_island);
	ClassDB::bind_method(D_METHOD("unwrap_texture_map_island", "cells_in_island", "keep_existing"), &ArrayPolyMeshND::unwrap_texture_map_island, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("unwrap_texture_map", "mode", "padding", "keep_existing"), &ArrayPolyMeshND::unwrap_texture_map, DEFVAL(UNWRAP_MODE_ISLANDS_FROM_SEAMS), DEFVAL(0.0), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("transform_texture_map", "transform"), &ArrayPolyMeshND::transform_texture_map);

	ClassDB::bind_method(D_METHOD("merge_with", "other", "offset", "basis"), &ArrayPolyMeshND::merge_with_bind, DEFVAL(VectorN()), DEFVAL(Projection()));
	ClassDB::bind_static_method("ArrayPolyMeshND", D_METHOD("reconstruct_from_cell_mesh", "cell_mesh"), &ArrayPolyMeshND::reconstruct_from_cell_mesh);

	// Only bind the setters here because the getters are already bound in PolyMeshND.
	ClassDB::bind_method(D_METHOD("set_poly_cell_indices", "poly_cell_indices"), &ArrayPolyMeshND::set_poly_cell_indices_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_indices"), "set_poly_cell_indices", "get_poly_cell_indices");

	ClassDB::bind_method(D_METHOD("set_poly_cell_vertices", "poly_cell_vertices"), &ArrayPolyMeshND::set_poly_cell_vertices);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_vertices"), "set_poly_cell_vertices", "get_poly_cell_vertices");

	ClassDB::bind_method(D_METHOD("set_poly_cell_boundary_normals", "poly_cell_boundary_normals"), &ArrayPolyMeshND::set_poly_cell_boundary_normals);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_boundary_normals"), "set_poly_cell_boundary_normals", "get_poly_cell_boundary_normals");

	ClassDB::bind_method(D_METHOD("set_poly_cell_vertex_normals", "poly_cell_vertex_normals"), &ArrayPolyMeshND::set_poly_cell_vertex_normals_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_vertex_normals"), "set_poly_cell_vertex_normals", "get_poly_cell_vertex_normals");

	ClassDB::bind_method(D_METHOD("set_poly_cell_texture_map", "poly_cell_texture_map"), &ArrayPolyMeshND::set_poly_cell_texture_map_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "poly_cell_texture_map"), "set_poly_cell_texture_map", "get_poly_cell_texture_map");

	ClassDB::bind_method(D_METHOD("get_seam_indices"), &ArrayPolyMeshND::get_seam_indices_bind);
	ClassDB::bind_method(D_METHOD("set_seam_indices", "seam_face_indices"), &ArrayPolyMeshND::set_seam_indices_bind);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "seam_face_indices"), "set_seam_indices", "get_seam_indices");

	ClassDB::bind_method(D_METHOD("set_edge_indices", "edge_indices"), &ArrayPolyMeshND::set_edge_vertex_indices);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "edge_indices"), "set_edge_indices", "get_edge_indices");

	BIND_ENUM_CONSTANT(COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
	BIND_ENUM_CONSTANT(COMPUTE_NORMALS_MODE_FORCE_OUTWARD_OVERRIDE_CELL_ORIENTATION);
	BIND_ENUM_CONSTANT(COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);

	BIND_ENUM_CONSTANT(UNWRAP_MODE_AUTOMATIC);
	BIND_ENUM_CONSTANT(UNWRAP_MODE_EACH_CELL_FILLS);
	BIND_ENUM_CONSTANT(UNWRAP_MODE_TILE_CELLS);
	BIND_ENUM_CONSTANT(UNWRAP_MODE_ISLANDS_FROM_SEAMS);
}
