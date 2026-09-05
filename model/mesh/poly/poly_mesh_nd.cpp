#include "poly_mesh_nd.h"

#include "../../../math/math_nd.h"
#include "../../../math/vector_nd.h"
#include "../material_nd.h"
#include "array_poly_mesh_nd.h"

bool PolyMeshND::is_poly_mesh_data_valid() {
	if (likely(_is_poly_mesh_data_valid)) {
		return true;
	}
	_is_poly_mesh_data_valid = _validate_poly_mesh_data_only();
	if (!_is_poly_mesh_data_valid) {
		ERR_PRINT("PolyMeshND: Mesh data is invalid on mesh '" + get_name() + "'.");
	}
	return _is_poly_mesh_data_valid;
}

void PolyMeshND::reset_poly_mesh_data_validation() {
	_is_poly_mesh_data_valid = false;
	reset_mesh_data_validation();
}

bool PolyMeshND::validate_mesh_data() {
	if (_validate_poly_mesh_data_only()) {
		_is_poly_mesh_data_valid = true;
	} else {
		_is_poly_mesh_data_valid = false;
		return false;
	}
	// Also check that the result of converting the poly data into simplex cell data is valid.
	// This function is used to validate if a mesh is good for rendering, so we need to check this.
	// Note: This is checked here instead of chaining to CellMeshND::validate_mesh_data,
	// because that resolves to MeshND's script virtual call, which defaults to false.
	const int64_t dimension = get_dimension();
	if (dimension < 3) {
		return true; // Nothing renderable to validate.
	}
	const PackedInt32Array simplex_indices = get_simplex_cell_vertex_indices();
	if (simplex_indices.size() % dimension != 0) {
		return false;
	}
	const int64_t simplex_vertex_count = get_vertex_positions().size();
	for (int64_t i = 0; i < simplex_indices.size(); i++) {
		if (simplex_indices[i] < 0 || simplex_indices[i] >= simplex_vertex_count) {
			return false;
		}
	}
	return true;
}

bool PolyMeshND::_validate_poly_mesh_data_only() {
	const Vector<Vector<PackedInt32Array>> poly_cell_indices = get_poly_cell_indices();
	const PackedInt32Array edge_indices = get_edge_indices();
	const Vector<VectorN> poly_cell_vertices = get_poly_cell_vertex_positions();
	const int64_t edge_index_count = edge_indices.size();
	ERR_FAIL_COND_V_MSG(edge_index_count % 2 != 0, false, "PolyMeshND: Edge index count must be even (pairs of vertices).");
	const int64_t vertex_count = poly_cell_vertices.size();
	const int64_t dimension = vertex_count == 0 ? 0 : poly_cell_vertices[0].size();
	for (int64_t i = 0; i < vertex_count; i++) {
		ERR_FAIL_COND_V_MSG(poly_cell_vertices[i].size() != dimension, false, "PolyMeshND: All vertices must have the same number of dimensions.");
	}
	for (int64_t i = 0; i < edge_index_count; i++) {
		ERR_FAIL_COND_V_MSG(edge_indices[i] < 0 || edge_indices[i] >= vertex_count, false, "PolyMeshND: Edge index " + itos(i) + " references invalid vertex " + itos(edge_indices[i]) + " (valid range: 0-" + itos(vertex_count - 1) + ").");
	}
	const int64_t poly_cell_dims = poly_cell_indices.size();
	const int64_t edge_count = edge_index_count / 2;
	if (poly_cell_dims != 0) {
		Vector<PackedInt32Array> cells_of_prev_dim;
		int64_t prev_dim_count = edge_count;
		for (int64_t poly_dim_index = 0; poly_dim_index < poly_cell_dims; poly_dim_index++) {
			const Vector<PackedInt32Array> &cells_of_dim = poly_cell_indices[poly_dim_index];
			for (int64_t cell_idx = 0; cell_idx < cells_of_dim.size(); cell_idx++) {
				const PackedInt32Array &cell = cells_of_dim[cell_idx];
				const int64_t cell_element_count = cell.size();
				// Faces (poly_dim_index 0) must have at least 3 edges, cells (poly_dim_index 1) must have at least 4 faces, etc.
				ERR_FAIL_COND_V_MSG(cell_element_count < poly_dim_index + 3, false, "PolyMeshND: " + itos(poly_dim_index + 2) + "D cell has insufficient elements (" + itos(cell_element_count) + "<" + itos(poly_dim_index + 3) + ").");
				for (int64_t i = 0; i < cell_element_count; i++) {
					ERR_FAIL_COND_V_MSG(cell[i] < 0 || cell[i] >= prev_dim_count, false, "PolyMeshND: " + itos(poly_dim_index + 2) + "D cell references invalid " + itos(poly_dim_index + 1) + "D element " + itos(cell[i]) + ".");
				}
				bool is_common = false;
				if (poly_dim_index == 0) {
					is_common = _do_edges_have_common_vertex(edge_indices[cell[0] * 2], edge_indices[cell[0] * 2 + 1], edge_indices[cell[1] * 2], edge_indices[cell[1] * 2 + 1]);
				} else {
					int64_t common_in_first = 0;
					int64_t common_in_second = 0;
					int32_t common_edge = MathND::find_common_int32(cells_of_prev_dim[cell[0]], cells_of_prev_dim[cell[1]], common_in_first, common_in_second);
					is_common = common_edge != INT32_MIN;
				}
				if (!is_common) {
					// This problem may be common so let's give a descriptive error message.
					const String cell_str = String(Variant(cell));
					if (poly_dim_index == 0) {
						ERR_PRINT("The first two edges of face " + cell_str + " do not share a common vertex, therefore orientation is not determinable and the face is invalid.");
					} else if (poly_dim_index == 1) {
						ERR_PRINT("The first two faces of cell " + itos(cell_idx) + " with data " + cell_str + " do not share a common edge, therefore orientation is not determinable and the cell is invalid.");
					} else {
						ERR_PRINT("The first two " + itos(poly_dim_index + 1) + "D cells of " + itos(poly_dim_index + 2) + "D cell " + cell_str + " do not share a common " + itos(poly_dim_index) + "D cell, therefore orientation is not determinable and the " + itos(poly_dim_index + 2) + "D cell is invalid.");
					}
					return false;
				}
				if (poly_dim_index == 0 && cell_element_count > 3) {
					// Faces should have their edges in a connected loop order. Reading faces is
					// robust to any order, but other orders are an undesired layout of the data.
					for (int64_t i = 0; i < cell_element_count; i++) {
						const int32_t edge_a = cell[i];
						const int32_t edge_b = cell[(i + 1) % cell_element_count];
						if (!_do_edges_have_common_vertex(edge_indices[edge_a * 2], edge_indices[edge_a * 2 + 1], edge_indices[edge_b * 2], edge_indices[edge_b * 2 + 1])) {
							WARN_PRINT("PolyMeshND: Face " + itos(cell_idx) + " does not have its edges in a connected loop order. This is handled, but it is an undesired layout of the data.");
							break;
						}
					}
				}
			}
			cells_of_prev_dim = cells_of_dim;
			prev_dim_count = cells_of_dim.size();
		}
	}
	// Boundary cell data bindings. The boundary cells of an N-dimensional mesh are the
	// (N-1)-dimensional cells, at poly cell dim index N - 3.
	const int64_t boundary_dim_index = dimension - 3;
	const Vector<VectorN> poly_cell_boundary_normals = get_poly_cell_boundary_normals();
	const int64_t poly_cell_boundary_normals_count = poly_cell_boundary_normals.size();
	if (poly_cell_boundary_normals_count != 0) {
		ERR_FAIL_COND_V_MSG(boundary_dim_index < 0 || poly_cell_dims <= boundary_dim_index, false, "PolyMeshND: Boundary normals provided without any boundary cells.");
		const Vector<PackedInt32Array> &boundary_cells = poly_cell_indices[boundary_dim_index];
		ERR_FAIL_COND_V_MSG(boundary_cells.size() != poly_cell_boundary_normals_count, false, "PolyMeshND: Boundary normals count (" + itos(poly_cell_boundary_normals_count) + ") does not match boundary cells count (" + itos(boundary_cells.size()) + ").");
		for (int64_t i = 0; i < poly_cell_boundary_normals_count; i++) {
			ERR_FAIL_COND_V_MSG(!poly_cell_boundary_normals[i].is_empty() && poly_cell_boundary_normals[i].size() != dimension, false, "PolyMeshND: Boundary normal " + itos(i) + " does not match the dimension of the mesh.");
		}
	}
	const PackedInt32Array poly_cell_boundary_pivot_overrides = get_poly_cell_boundary_pivot_overrides();
	if (poly_cell_boundary_pivot_overrides.size() != 0) {
		ERR_FAIL_COND_V_MSG(boundary_dim_index < 0 || poly_cell_dims <= boundary_dim_index, false, "PolyMeshND: Boundary pivot overrides provided without any boundary cells.");
		ERR_FAIL_COND_V_MSG(poly_cell_boundary_pivot_overrides.size() > poly_cell_indices[boundary_dim_index].size(), false, "PolyMeshND: Boundary pivot overrides must have at most one entry per boundary cell.");
		for (int64_t i = 0; i < poly_cell_boundary_pivot_overrides.size(); i++) {
			const int32_t pivot_vertex_index = poly_cell_boundary_pivot_overrides[i];
			if (pivot_vertex_index == -1) {
				continue; // This cell is allowed to not have a pivot override.
			}
			ERR_FAIL_COND_V_MSG(pivot_vertex_index < 0 || pivot_vertex_index >= vertex_count, false, "PolyMeshND: Boundary pivot override " + itos(i) + " references invalid vertex " + itos(pivot_vertex_index) + " (valid range: 0-" + itos(vertex_count - 1) + ").");
		}
	}
	const Vector<Vector<VectorN>> poly_cell_vertex_normals = get_poly_cell_vertex_normals();
	const int64_t poly_cell_vertex_normals_count = poly_cell_vertex_normals.size();
	if (poly_cell_vertex_normals_count != 0) {
		ERR_FAIL_COND_V_MSG(boundary_dim_index < 0 || poly_cell_dims <= boundary_dim_index, false, "PolyMeshND: Vertex normals provided without any boundary cells to map to.");
		const Vector<PackedInt32Array> &boundary_cells = poly_cell_indices[boundary_dim_index];
		ERR_FAIL_COND_V_MSG(boundary_cells.size() != poly_cell_vertex_normals_count, false, "PolyMeshND: Vertex normals count (" + itos(poly_cell_vertex_normals_count) + ") does not match boundary cells count (" + itos(boundary_cells.size()) + ").");
		const Vector<PackedInt32Array> cell_vert = _get_vertex_indices_of_boundary_cells(poly_cell_indices, edge_indices, boundary_dim_index, false);
		for (int64_t i = 0; i < poly_cell_vertex_normals_count; i++) {
			if (poly_cell_vertex_normals[i].is_empty()) {
				continue; // Allow cells without vertex normals.
			}
			ERR_FAIL_COND_V_MSG(poly_cell_vertex_normals[i].size() != cell_vert[i].size(), false, "PolyMeshND: Vertex normal array " + itos(i) + " has " + itos(poly_cell_vertex_normals[i].size()) + " entries but cell has " + itos(cell_vert[i].size()) + " vertices.");
		}
	}
	const Vector<Vector<VectorM>> poly_cell_texture_map = get_poly_cell_texture_map();
	const int64_t poly_cell_texture_map_count = poly_cell_texture_map.size();
	if (poly_cell_texture_map_count != 0) {
		ERR_FAIL_COND_V_MSG(boundary_dim_index < 0 || poly_cell_dims <= boundary_dim_index, false, "PolyMeshND: Texture map provided without any boundary cells to map to.");
		const Vector<PackedInt32Array> &boundary_cells = poly_cell_indices[boundary_dim_index];
		ERR_FAIL_COND_V_MSG(boundary_cells.size() != poly_cell_texture_map_count, false, "PolyMeshND: Texture maps count (" + itos(poly_cell_texture_map_count) + ") does not match boundary cells count (" + itos(boundary_cells.size()) + ").");
		const Vector<PackedInt32Array> cell_vert = _get_vertex_indices_of_boundary_cells(poly_cell_indices, edge_indices, boundary_dim_index, false);
		for (int64_t i = 0; i < poly_cell_texture_map_count; i++) {
			if (poly_cell_texture_map[i].is_empty()) {
				continue; // Allow unmapped boundary cells.
			}
			ERR_FAIL_COND_V_MSG(poly_cell_texture_map[i].size() != cell_vert[i].size(), false, "PolyMeshND: Texture map " + itos(i) + " has " + itos(poly_cell_texture_map[i].size()) + " entries but cell has " + itos(cell_vert[i].size()) + " vertices.");
		}
	}
	return true;
}

VectorM PolyMeshND::_average_vector_m(const Vector<VectorM> &p_vector_m_array) {
	const int64_t count = p_vector_m_array.size();
	if (count == 0) {
		return VectorM();
	}
	VectorM sum;
	for (int64_t i = 0; i < count; i++) {
		sum = VectorND::add(sum, p_vector_m_array[i]);
	}
	return VectorND::divide_scalar(sum, (double)count);
}

int32_t PolyMeshND::_get_lowest_vertex_of_cell_excluding(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_cell_dim_index, const int64_t p_which_cell, const HashSet<int32_t> &p_excluded_vertices) {
	const PackedInt32Array cell_vertices = _get_vertex_indices_of_poly_cell(p_poly_cell_indices, p_all_edge_indices, p_cell_dim_index, p_which_cell, false);
	int32_t lowest_vertex = -1;
	for (const int32_t cell_vertex : cell_vertices) {
		if (p_excluded_vertices.has(cell_vertex)) {
			continue;
		}
		if (lowest_vertex == -1 || cell_vertex < lowest_vertex) {
			lowest_vertex = cell_vertex;
		}
	}
	return lowest_vertex;
}

PackedInt32Array PolyMeshND::_get_canonical_span_vertex_index_sequence(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_indices_dim_index, const int64_t p_which_cell) {
	const PackedInt32Array &indices = p_poly_cell_indices[p_indices_dim_index][p_which_cell];
	if (p_indices_dim_index == 0) {
		// Given a 2D face (dim index 0) made of edges, get 3 vertex indices from the first 2 edges in the face's winding order.
		return _get_face_edge_3_vertex_index_sequence(
				p_all_edge_indices[indices[0] * 2], p_all_edge_indices[indices[0] * 2 + 1],
				p_all_edge_indices[indices[1] * 2], p_all_edge_indices[indices[1] * 2 + 1]);
	}
	const int64_t prev_dim_index = p_indices_dim_index - 1;
	const Vector<PackedInt32Array> &low_dim = p_poly_cell_indices[prev_dim_index];
	if (p_indices_dim_index == 1) {
		// Given a 3D cell (dim index 1) made of faces of dim 0, get 4 vertex indices from 3 edges from the first 2 faces.
		// This is a fast path: the general case below also handles 3D cells, and produces the
		// same orientation for meshes with sorted edges and planar convex faces, but this
		// special case is faster and established conventions depend on its exact output.
		return _get_cell_face_4_vertex_index_sequence(p_all_edge_indices, low_dim[indices[0]], low_dim[indices[1]]);
	}
	// General case: Cells of geometric dimension d >= 3 (dim index N = d - 2 >= 1).
	// This construction works the same way in every dimension, producing a span of d + 1 vertices:
	//  - The first two members of the cell are (d-1)-dimensional cells, which are required
	//    to share a common (d-2)-dimensional cell, known as the "ridge".
	//  - A (d-2)-dimensional ridge is spanned by d-1 vertices. Use the ridge's d-1 lowest
	//    vertex indices in ascending order. This choice is deterministic and independent of
	//    the order of the data inside of any lower-dimensional cells, so rearranging the
	//    members of lower-dimensional cells cannot change the orientation of this cell.
	//  - Prepend the lowest vertex of the first member that is not on the ridge, and append
	//    the lowest vertex of the second member that is not on the ridge.
	// Swapping the first two members of the cell swaps the prepended and appended vertices,
	// which is a single transposition of the span, so it is guaranteed to flip the cell's
	// orientation. Therefore, the orientation of every cell is controllable at the level of
	// that cell's own data, by the order of its first two members.
	// The 2D and 3D special cases above are instances of the same construction. For a 2D face,
	// the ridge is the vertex shared by the first two edges, which has no ordering concerns.
	// For a 3D cell, the ridge is the edge shared by the first two faces, taken in stored order,
	// which the append functions keep sorted ascending anyway, and the extension vertices are
	// picked using the faces' winding instead of the lowest index.
	// Note: For degenerate geometry (such as the d-1 lowest vertices of a ridge polytope not
	// spanning it, like three collinear vertices of a polygon), the span may be degenerate,
	// which results in a zero normal vector downstream, treated as missing data.
	int64_t ridge_in_first_member = 0;
	int64_t ridge_in_second_member = 0;
	const int32_t ridge_cell = MathND::find_common_int32(low_dim[indices[0]], low_dim[indices[1]], ridge_in_first_member, ridge_in_second_member);
	CRASH_COND_MSG(ridge_cell == INT32_MIN, "PolyMeshND: The first two members of the cell do not share a common cell of the next dimension down, this cell's initial 2 members are invalid.");
	// Gather the ridge's vertices. For 3D cells (dim index 1), the ridge is a 1D edge, which
	// is stored in the flat edge array rather than in the poly cell indices.
	PackedInt32Array ridge_vertices;
	if (p_indices_dim_index == 1) {
		ridge_vertices.append(p_all_edge_indices[ridge_cell * 2]);
		ridge_vertices.append(p_all_edge_indices[ridge_cell * 2 + 1]);
	} else {
		ridge_vertices = _get_vertex_indices_of_poly_cell(p_poly_cell_indices, p_all_edge_indices, p_indices_dim_index - 2, ridge_cell, false);
	}
	HashSet<int32_t> ridge_vertex_set;
	for (const int32_t ridge_vertex : ridge_vertices) {
		ridge_vertex_set.insert(ridge_vertex);
	}
	// A (d-2)-dimensional ridge is spanned by d-1 vertices, where d == p_indices_dim_index + 2.
	const int64_t ridge_span_size = p_indices_dim_index + 1;
	PackedInt32Array ridge_span = ridge_vertices;
	ERR_FAIL_COND_V_MSG(ridge_span.size() < ridge_span_size, PackedInt32Array(), "PolyMeshND: The common cell shared by the first two members of the cell has fewer than " + itos(ridge_span_size) + " vertices, so it cannot span " + itos(p_indices_dim_index) + " dimensions.");
	ridge_span.sort();
	ridge_span.resize(ridge_span_size);
	const int32_t first_extension_vertex = _get_lowest_vertex_of_cell_excluding(p_poly_cell_indices, p_all_edge_indices, prev_dim_index, indices[0], ridge_vertex_set);
	const int32_t second_extension_vertex = _get_lowest_vertex_of_cell_excluding(p_poly_cell_indices, p_all_edge_indices, prev_dim_index, indices[1], ridge_vertex_set);
	ERR_FAIL_COND_V_MSG(first_extension_vertex == -1 || second_extension_vertex == -1, PackedInt32Array(), "PolyMeshND: One of the first two members of the cell has no vertex off of the common cell it shares with the other, so it cannot extend the span.");
	PackedInt32Array ret;
	ret.resize(ridge_span_size + 2);
	ret.set(0, first_extension_vertex);
	for (int64_t i = 0; i < ridge_span_size; i++) {
		ret.set(i + 1, ridge_span[i]);
	}
	ret.set(ridge_span_size + 1, second_extension_vertex);
	return ret;
}

// These internal functions can be fast and exclude most ERR_FAIL_* checks because
// they are only called after `_validate_poly_mesh_data_only()` has returned true.
// If something can crash these functions, it should be caught by that validation first.
PackedInt32Array PolyMeshND::_get_cell_face_4_vertex_index_sequence(const PackedInt32Array &p_all_edge_indices, const PackedInt32Array &p_first_face, const PackedInt32Array &p_second_face) {
	int64_t common_in_first = 0;
	int64_t common_in_second = 0;
	int32_t common_edge = MathND::find_common_int32(p_first_face, p_second_face, common_in_first, common_in_second);
	CRASH_COND_MSG(common_edge == INT32_MIN, "PolyMeshND: Cell faces do not share a common item, this cell's initial 2 faces are invalid.");
	const int64_t first_next_index = (common_in_first + 1) % p_first_face.size();
	const int64_t second_next_index = (common_in_second + 1) % p_second_face.size();
	// Use these 3 edges to get 4 vertex indices in a consistent "winding" order.
	const int32_t common_vertex_start = p_all_edge_indices[common_edge * 2];
	const int32_t common_vertex_end = p_all_edge_indices[common_edge * 2 + 1];
	int32_t first_next_vertex = p_all_edge_indices[p_first_face[first_next_index] * 2];
	if (first_next_vertex == common_vertex_start || first_next_vertex == common_vertex_end) {
		first_next_vertex = p_all_edge_indices[p_first_face[first_next_index] * 2 + 1];
	}
	int32_t second_next_vertex = p_all_edge_indices[p_second_face[second_next_index] * 2];
	if (second_next_vertex == common_vertex_start || second_next_vertex == common_vertex_end) {
		second_next_vertex = p_all_edge_indices[p_second_face[second_next_index] * 2 + 1];
	}
	return PackedInt32Array{ first_next_vertex, common_vertex_start, common_vertex_end, second_next_vertex };
}

PackedInt32Array PolyMeshND::_get_face_edge_3_vertex_index_sequence(const int32_t p_edge1_a, const int32_t p_edge1_b, const int32_t p_edge2_a, const int32_t p_edge2_b) {
	// Deduplicate the matching vertex index and place it in the middle.
	if (p_edge1_a == p_edge2_a) {
		return PackedInt32Array{ p_edge1_b, p_edge1_a, p_edge2_b };
	} else if (p_edge1_a == p_edge2_b) {
		return PackedInt32Array{ p_edge1_b, p_edge1_a, p_edge2_a };
	} else if (p_edge1_b == p_edge2_a) {
		return PackedInt32Array{ p_edge1_a, p_edge1_b, p_edge2_b };
	} else if (p_edge1_b == p_edge2_b) {
		return PackedInt32Array{ p_edge1_a, p_edge1_b, p_edge2_a };
	}
	CRASH_NOW_MSG("PolyMeshND: Edges do not share a vertex, this face's initial 2 edges are invalid.");
	return PackedInt32Array();
}

PackedInt32Array PolyMeshND::_get_edges_of_poly_cell(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const int64_t p_cell_dim_index, const int64_t p_which_cell) {
	const PackedInt32Array cell_indices = p_poly_cell_indices[p_cell_dim_index][p_which_cell];
	if (p_cell_dim_index == 0) {
		// Given a 2D face (dim index 0) made of edges, it's already a list of edges.
		return cell_indices;
	}
	// Given a 3D cell (dim index 1) or higher, run this function recursively.
	PackedInt32Array ret;
	HashSet<int32_t> seen_edges;
	for (int64_t i = 0; i < cell_indices.size(); i++) {
		PackedInt32Array face_edges = _get_edges_of_poly_cell(p_poly_cell_indices, p_cell_dim_index - 1, cell_indices[i]);
		if (i == 0) {
			ret = face_edges;
			for (int64_t j = 0; j < face_edges.size(); j++) {
				seen_edges.insert(face_edges[j]);
			}
			continue;
		}
		for (int64_t j = 0; j < face_edges.size(); j++) {
			if (!seen_edges.has(face_edges[j])) {
				seen_edges.insert(face_edges[j]);
				ret.append(face_edges[j]);
			}
		}
	}
	return ret;
}

PackedInt32Array PolyMeshND::_get_vertex_indices_of_poly_cell(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_cell_dim_index, const int64_t p_which_cell, const bool p_start_with_canonical_span) {
	const PackedInt32Array cell_edges = _get_edges_of_poly_cell(p_poly_cell_indices, p_cell_dim_index, p_which_cell);
	PackedInt32Array ret;
	HashSet<int32_t> seen_vertices;
	if (p_start_with_canonical_span) {
		ret = _get_canonical_span_vertex_index_sequence(p_poly_cell_indices, p_all_edge_indices, p_cell_dim_index, p_which_cell);
		for (int64_t i = 0; i < ret.size(); i++) {
			seen_vertices.insert(ret[i]);
		}
	}
	for (int64_t i = 0; i < cell_edges.size(); i++) {
		const int32_t edge_start_vertex = p_all_edge_indices[cell_edges[i] * 2];
		const int32_t edge_end_vertex = p_all_edge_indices[cell_edges[i] * 2 + 1];
		if (!seen_vertices.has(edge_start_vertex)) {
			seen_vertices.insert(edge_start_vertex);
			ret.append(edge_start_vertex);
		}
		if (!seen_vertices.has(edge_end_vertex)) {
			seen_vertices.insert(edge_end_vertex);
			ret.append(edge_end_vertex);
		}
	}
	return ret;
}

PackedInt32Array PolyMeshND::_get_vertex_indices_of_face(const PackedInt32Array &p_all_edge_indices, const PackedInt32Array &p_face_edge_indices) {
	if (p_face_edge_indices.is_empty()) {
		return PackedInt32Array();
	}
	if (p_face_edge_indices.size() == 1) {
		return PackedInt32Array{
			p_all_edge_indices[p_face_edge_indices[0] * 2],
			p_all_edge_indices[p_face_edge_indices[0] * 2 + 1]
		};
	}
	PackedInt32Array ret = _get_face_edge_3_vertex_index_sequence(
			p_all_edge_indices[p_face_edge_indices[0] * 2], p_all_edge_indices[p_face_edge_indices[0] * 2 + 1],
			p_all_edge_indices[p_face_edge_indices[1] * 2], p_all_edge_indices[p_face_edge_indices[1] * 2 + 1]);
	HashSet<int32_t> seen_vertices;
	for (int64_t i = 0; i < ret.size(); i++) {
		seen_vertices.insert(ret[i]);
	}
	// Walk the edge loop by connectivity, so the vertices are in polygon boundary order,
	// even if the edge list is not stored in loop order.
	while (ret.size() < p_face_edge_indices.size()) {
		const int32_t current_vertex = ret[ret.size() - 1];
		bool found = false;
		for (int64_t i = 2; i < p_face_edge_indices.size(); i++) {
			const int32_t edge_start_vertex = p_all_edge_indices[p_face_edge_indices[i] * 2];
			const int32_t edge_end_vertex = p_all_edge_indices[p_face_edge_indices[i] * 2 + 1];
			int32_t other_vertex;
			if (edge_start_vertex == current_vertex) {
				other_vertex = edge_end_vertex;
			} else if (edge_end_vertex == current_vertex) {
				other_vertex = edge_start_vertex;
			} else {
				continue;
			}
			if (seen_vertices.has(other_vertex)) {
				continue;
			}
			seen_vertices.insert(other_vertex);
			ret.append(other_vertex);
			found = true;
			break;
		}
		if (!found) {
			break; // The face is not a closed loop, fall back to appending in edge order.
		}
	}
	for (int64_t i = 2; i < p_face_edge_indices.size(); i++) {
		const int32_t edge_start_vertex = p_all_edge_indices[p_face_edge_indices[i] * 2];
		const int32_t edge_end_vertex = p_all_edge_indices[p_face_edge_indices[i] * 2 + 1];
		if (!seen_vertices.has(edge_start_vertex)) {
			seen_vertices.insert(edge_start_vertex);
			ret.append(edge_start_vertex);
		}
		if (!seen_vertices.has(edge_end_vertex)) {
			seen_vertices.insert(edge_end_vertex);
			ret.append(edge_end_vertex);
		}
	}
	return ret;
}

bool PolyMeshND::_solve_coordinates_in_span(const Vector<VectorN> &p_span_vectors, const VectorN &p_target, VectorN &r_coordinates) {
	const int64_t span_count = p_span_vectors.size();
	r_coordinates = VectorND::fill(span_count, 0.0);
	if (span_count == 0) {
		return false;
	}
	// Modified Gram-Schmidt with a recorded upper-triangular matrix, so that the coordinates
	// can be recovered in terms of the original span vectors by back-substitution. Any
	// component of the target outside of the span is ignored (orthogonal projection).
	Vector<VectorN> ortho_dirs;
	ortho_dirs.resize(span_count);
	Vector<VectorN> upper_rows;
	upper_rows.resize(span_count);
	for (int64_t i = 0; i < span_count; i++) {
		upper_rows.set(i, VectorND::fill(span_count, 0.0));
	}
	for (int64_t j = 0; j < span_count; j++) {
		VectorN working = VectorND::duplicate(p_span_vectors[j]);
		for (int64_t i = 0; i < j; i++) {
			const double coefficient = VectorND::dot(ortho_dirs[i], working);
			upper_rows.ptrw()[i].set(j, coefficient);
			working = VectorND::subtract(working, VectorND::multiply_scalar(ortho_dirs[i], coefficient));
		}
		const double norm = VectorND::length(working);
		if (Math::is_zero_approx(norm)) {
			return false; // The span vectors are not linearly independent.
		}
		upper_rows.ptrw()[j].set(j, norm);
		ortho_dirs.set(j, VectorND::divide_scalar(working, norm));
	}
	// Project the target onto the orthonormal directions, then back-substitute.
	VectorN projected = VectorND::fill(span_count, 0.0);
	for (int64_t i = 0; i < span_count; i++) {
		projected.set(i, VectorND::dot(ortho_dirs[i], p_target));
	}
	for (int64_t j = span_count - 1; j >= 0; j--) {
		double value = projected[j];
		for (int64_t k = j + 1; k < span_count; k++) {
			value -= upper_rows[j][k] * r_coordinates[k];
		}
		r_coordinates.set(j, value / upper_rows[j][j]);
	}
	return true;
}

int64_t PolyMeshND::_pick_spanning_vertices(const Vector<VectorN> &p_all_vertices, const PackedInt32Array &p_candidate_vertex_indices, const int64_t p_max_directions, PackedInt32Array &r_picked_vertex_indices) {
	r_picked_vertex_indices.clear();
	if (p_candidate_vertex_indices.is_empty()) {
		return 0;
	}
	const VectorN base = p_all_vertices[p_candidate_vertex_indices[0]];
	Vector<VectorN> ortho_dirs;
	for (int64_t position = 1; position < p_candidate_vertex_indices.size(); position++) {
		if (ortho_dirs.size() >= p_max_directions) {
			break;
		}
		VectorN direction = VectorND::subtract(p_all_vertices[p_candidate_vertex_indices[position]], base);
		const double original_length = VectorND::length(direction);
		if (Math::is_zero_approx(original_length)) {
			continue;
		}
		for (int64_t i = 0; i < ortho_dirs.size(); i++) {
			direction = VectorND::subtract(direction, VectorND::multiply_scalar(ortho_dirs[i], VectorND::dot(ortho_dirs[i], direction)));
		}
		const double residual_length = VectorND::length(direction);
		if (residual_length < original_length * (double)CMP_EPSILON) {
			continue; // This vertex does not extend the span.
		}
		ortho_dirs.append(VectorND::divide_scalar(direction, residual_length));
		r_picked_vertex_indices.append((int32_t)position);
	}
	return ortho_dirs.size();
}

void PolyMeshND::flip_poly_cell_orientation(PackedInt32Array &r_cell_members, const int64_t p_cell_dim_index) {
	ERR_FAIL_COND(r_cell_members.size() < 2);
	if (p_cell_dim_index == 0) {
		// Faces are flipped by reversing the whole edge loop, keeping it in a connected loop order.
		r_cell_members.reverse();
	} else {
		// Higher-dimensional cells are flipped by swapping the first two members,
		// since the order of the members beyond the first two is free.
		const int32_t temp = r_cell_members[0];
		r_cell_members.set(0, r_cell_members[1]);
		r_cell_members.set(1, temp);
	}
}

void PolyMeshND::_orient_cells_to_match_normals(Vector<Vector<PackedInt32Array>> &r_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const Vector<VectorN> &p_vertices, const Vector<VectorN> &p_target_normals, const int64_t p_cell_dim_index) {
	Vector<PackedInt32Array> cells = r_poly_cell_indices[p_cell_dim_index];
	ERR_FAIL_COND(cells.size() != p_target_normals.size());
	for (int64_t cell_index = 0; cell_index < cells.size(); cell_index++) {
		const PackedInt32Array span = _get_canonical_span_vertex_index_sequence(r_poly_cell_indices, p_all_edge_indices, p_cell_dim_index, cell_index);
		ERR_CONTINUE(span.size() < 2);
		Vector<VectorN> directions;
		directions.resize(span.size() - 1);
		for (int64_t i = 1; i < span.size(); i++) {
			directions.set(i - 1, VectorND::direction_to(p_vertices[span[0]], p_vertices[span[i]]));
		}
		const VectorN cell_perp = VectorND::perpendicular(directions);
		if (VectorND::dot(cell_perp, p_target_normals[cell_index]) < 0.0) {
			// Flip this cell's orientation to match the target normal.
			PackedInt32Array cell = cells[cell_index];
			flip_poly_cell_orientation(cell, p_cell_dim_index);
			cells.set(cell_index, cell);
		}
	}
	r_poly_cell_indices.set(p_cell_dim_index, cells);
}

bool PolyMeshND::_does_pivot_conflict_with_descendants(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const Vector<Vector<PackedInt32Array>> &p_level_cell_vertices, const Vector<PackedInt32Array> &p_level_pivots, const int64_t p_cell_dim_index, const int64_t p_which_cell, const int32_t p_pivot_vertex) {
	if (p_cell_dim_index == 0) {
		return false; // Faces have no members with pivots of their own (their members are edges).
	}
	const int64_t member_dim_index = p_cell_dim_index - 1;
	for (const int32_t member_index : p_poly_cell_indices[p_cell_dim_index][p_which_cell]) {
		const PackedInt32Array &member_vertices = p_level_cell_vertices[member_dim_index][member_index];
		if (!member_vertices.has(p_pivot_vertex)) {
			continue; // The pivot is not on this member, so the member is free to decompose any way.
		}
		if (member_vertices.size() == member_dim_index + 3) {
			continue; // Simplex members decompose into themselves for any pivot, so no constraint is needed.
		}
		const int32_t member_pivot = p_level_pivots[member_dim_index][member_index];
		if (member_pivot == p_pivot_vertex) {
			continue; // Already constrained to the same pivot, including its own descendants.
		}
		if (member_pivot != -1) {
			return true; // Constrained to a different pivot by another cell.
		}
		if (_does_pivot_conflict_with_descendants(p_poly_cell_indices, p_level_cell_vertices, p_level_pivots, member_dim_index, member_index, p_pivot_vertex)) {
			return true;
		}
	}
	return false;
}

void PolyMeshND::_impose_pivot_on_descendants(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const Vector<Vector<PackedInt32Array>> &p_level_cell_vertices, Vector<PackedInt32Array> &r_level_pivots, const int64_t p_cell_dim_index, const int64_t p_which_cell, const int32_t p_pivot_vertex) {
	if (p_cell_dim_index == 0) {
		return; // Faces have no members with pivots of their own (their members are edges).
	}
	const int64_t member_dim_index = p_cell_dim_index - 1;
	for (const int32_t member_index : p_poly_cell_indices[p_cell_dim_index][p_which_cell]) {
		const PackedInt32Array &member_vertices = p_level_cell_vertices[member_dim_index][member_index];
		if (!member_vertices.has(p_pivot_vertex)) {
			continue; // The pivot is not on this member, so the member is free to decompose any way.
		}
		if (member_vertices.size() == member_dim_index + 3) {
			continue; // Simplex members decompose into themselves for any pivot, so no constraint is needed.
		}
		if (r_level_pivots[member_dim_index][member_index] != -1) {
			continue; // Already constrained, either to the same pivot or unresolvably by an override.
		}
		r_level_pivots.ptrw()[member_dim_index].set(member_index, p_pivot_vertex);
		_impose_pivot_on_descendants(p_poly_cell_indices, p_level_cell_vertices, r_level_pivots, member_dim_index, member_index, p_pivot_vertex);
	}
}

void PolyMeshND::_decompose_boundary_cells_into_simplexes() {
	// This function is required to make the mesh renderable, so it needs to only check the poly mesh data.
	if (!is_poly_mesh_data_valid()) {
		return;
	}
	poly_mesh_clear_cache();
	const int64_t dimension = get_dimension();
	ERR_FAIL_COND_MSG(dimension < 3, "PolyMeshND: Cannot decompose boundary cells into simplexes because the mesh has fewer than 3 dimensions.");
	const int64_t boundary_dim_index = dimension - 3;
	// Step 1: Gather information needed to compute the simplex decomposition.
	_simplex_cell_vertex_positions_cache = get_poly_cell_vertex_positions();
	const PackedInt32Array all_edge_indices = get_edge_indices();
	const Vector<Vector<PackedInt32Array>> poly_cell_indices = get_poly_cell_indices();
	ERR_FAIL_COND_MSG(poly_cell_indices.size() <= boundary_dim_index, "PolyMeshND: Cannot decompose boundary cells into simplexes because there are no boundary cells.");
	const Vector<PackedInt32Array> &boundary_cells = poly_cell_indices[boundary_dim_index];
	const int64_t boundary_cell_count = boundary_cells.size();
	const PackedInt32Array poly_cell_boundary_pivot_overrides = get_poly_cell_boundary_pivot_overrides();
	// Step 2: Drill down into each boundary cell's components to get the vertex indices and normals.
	// The `true` argument makes the first N vertices form the "canonical span" of the cell.
	Vector<PackedInt32Array> boundary_cell_vertex_indices = _get_vertex_indices_of_boundary_cells(poly_cell_indices, all_edge_indices, boundary_dim_index, true);
	Vector<VectorN> poly_cell_boundary_normals = get_poly_cell_boundary_normals();
	if (poly_cell_boundary_normals.size() != boundary_cell_count) {
		poly_cell_boundary_normals = _compute_boundary_normals_based_on_cell_orientation(boundary_cell_vertex_indices, false);
	}
	// Step 3: Determine which boundary cells to use for the simplex decomposition.
	// Don't bother decomposing surface cells completely covered by volumetric cells (2+ uses),
	// because such cells will never be externally visible (like the face between two Minecraft blocks).
	PackedInt32Array surface_cells_to_use;
	surface_cells_to_use.resize(boundary_cell_count);
	if (poly_cell_indices.size() > boundary_dim_index + 1) {
		const Vector<PackedInt32Array> &volumetric_cells = poly_cell_indices[boundary_dim_index + 1];
		PackedInt32Array cell_usage_counts;
		cell_usage_counts.resize_initialized(boundary_cell_count);
		for (int64_t i = 0; i < boundary_cell_count; i++) {
			cell_usage_counts.set(i, 0);
		}
		for (int64_t i = 0; i < volumetric_cells.size(); i++) {
			const PackedInt32Array &volumetric_cell = volumetric_cells[i];
			for (const int32_t vol_cell_part : volumetric_cell) {
				ERR_FAIL_INDEX_MSG(vol_cell_part, boundary_cell_count, "PolyMeshND: Volumetric cell references an invalid boundary cell index.");
				cell_usage_counts.set(vol_cell_part, cell_usage_counts[vol_cell_part] + 1);
			}
		}
		int64_t surface_cell_count = 0;
		for (int64_t boundary_index = 0; boundary_index < boundary_cell_count; boundary_index++) {
			if (cell_usage_counts[boundary_index] < 2) {
				surface_cells_to_use.set(surface_cell_count, boundary_index);
				surface_cell_count++;
			}
		}
		surface_cells_to_use.resize(surface_cell_count);
	} else {
		for (int64_t i = 0; i < boundary_cell_count; i++) {
			surface_cells_to_use.set(i, i);
		}
	}
	HashSet<int32_t> surface_cell_set;
	for (const int32_t surface_cell : surface_cells_to_use) {
		surface_cell_set.insert(surface_cell);
	}
	// Step 4: Cache the vertex indices of every cell at every level up to the boundary cells.
	Vector<Vector<PackedInt32Array>> level_cell_vertices;
	level_cell_vertices.resize(boundary_dim_index + 1);
	for (int64_t level = 0; level <= boundary_dim_index; level++) {
		const int64_t level_count = poly_cell_indices[level].size();
		Vector<PackedInt32Array> vertices_of_level;
		vertices_of_level.resize(level_count);
		for (int64_t cell_index = 0; cell_index < level_count; cell_index++) {
			if (level == 0) {
				vertices_of_level.set(cell_index, _get_vertex_indices_of_face(all_edge_indices, poly_cell_indices[0][cell_index]));
			} else {
				vertices_of_level.set(cell_index, _get_vertex_indices_of_poly_cell(poly_cell_indices, all_edge_indices, level, cell_index, false));
			}
		}
		level_cell_vertices.set(level, vertices_of_level);
	}
	// Step 5: Assign a pivot vertex to every cell at every level, from the top down.
	// Every cell at every level is decomposed exactly once and that decomposition is reused
	// by every higher-dimensional cell that contains it, so adjacent cells always agree on
	// the simplexes of anything they share. The one remaining requirement for the combined
	// result to be a crack-free simplicial complex is: whenever a cell's pivot vertex lies
	// on one of the cells below it, that lower cell must be decomposed as the fan around
	// that same pivot. This is enforced by imposing the pivot as a constraint on all
	// lower-level cells that contain it, transitively. When a cell cannot pick any of its
	// vertices without conflicting with existing constraints, it falls back to a centroid
	// pivot, which is not on any lower cell and therefore imposes no constraints at all.
	Vector<PackedInt32Array> level_pivots;
	level_pivots.resize(boundary_dim_index + 1);
	for (int64_t level = 0; level <= boundary_dim_index; level++) {
		PackedInt32Array pivots_of_level;
		pivots_of_level.resize(poly_cell_indices[level].size());
		for (int64_t cell_index = 0; cell_index < pivots_of_level.size(); cell_index++) {
			pivots_of_level.set(cell_index, -1);
		}
		level_pivots.set(level, pivots_of_level);
	}
	// Step 5a: Assign pivots to the boundary cells on the surface.
	int32_t prev_pivot_vertex = -1;
	for (const int32_t cell_index : surface_cells_to_use) {
		int32_t pivot_vertex = -1;
		if (poly_cell_boundary_pivot_overrides.size() > cell_index && poly_cell_boundary_pivot_overrides[cell_index] != -1) {
			// Pivot overrides are authoritative. If an override conflicts with a constraint
			// already imposed by another cell, the existing constraint is kept, which may
			// produce misaligned simplexes. That is the user's responsibility to avoid.
			pivot_vertex = poly_cell_boundary_pivot_overrides[cell_index];
			_impose_pivot_on_descendants(poly_cell_indices, level_cell_vertices, level_pivots, boundary_dim_index, cell_index, pivot_vertex);
		} else {
			const PackedInt32Array &cell_vertices = boundary_cell_vertex_indices[cell_index];
			// Two passes: strongly prefer not using the same pivot vertex as the previous
			// cell, to support the `"polytopeCells"` option (basically an ND triangle fan).
			for (int64_t pass = 0; pass < 2 && pivot_vertex == -1; pass++) {
				for (const int32_t candidate : cell_vertices) {
					if (pass == 0 && candidate == prev_pivot_vertex) {
						continue;
					}
					if (_does_pivot_conflict_with_descendants(poly_cell_indices, level_cell_vertices, level_pivots, boundary_dim_index, cell_index, candidate)) {
						continue;
					}
					pivot_vertex = candidate;
					break;
				}
			}
			if (pivot_vertex == -1) {
				// Every vertex of this cell conflicts with existing constraints, so decompose
				// this cell from the centroid of its vertices instead. The centroid is not on
				// any lower cell, so it imposes no constraints and can never conflict.
				VectorN centroid = VectorND::fill(dimension, 0.0);
				for (const int32_t vertex_index : cell_vertices) {
					centroid = VectorND::add(centroid, _simplex_cell_vertex_positions_cache[vertex_index]);
				}
				centroid = VectorND::divide_scalar(centroid, (double)cell_vertices.size());
				pivot_vertex = (int32_t)VectorND::array_append_deduplicate(_simplex_cell_vertex_positions_cache, centroid);
			} else {
				_impose_pivot_on_descendants(poly_cell_indices, level_cell_vertices, level_pivots, boundary_dim_index, cell_index, pivot_vertex);
			}
		}
		level_pivots.ptrw()[boundary_dim_index].set(cell_index, pivot_vertex);
		prev_pivot_vertex = pivot_vertex;
	}
	// Step 5b: Assign pivots to all unconstrained cells of the lower levels, from the top down.
	for (int64_t level = boundary_dim_index - 1; level >= 0; level--) {
		const int64_t level_count = poly_cell_indices[level].size();
		for (int64_t cell_index = 0; cell_index < level_count; cell_index++) {
			if (level_pivots[level][cell_index] != -1) {
				continue; // Already constrained, along with its own descendants.
			}
			int32_t pivot_vertex = -1;
			for (const int32_t candidate : level_cell_vertices[level][cell_index]) {
				if (_does_pivot_conflict_with_descendants(poly_cell_indices, level_cell_vertices, level_pivots, level, cell_index, candidate)) {
					continue;
				}
				pivot_vertex = candidate;
				break;
			}
			if (pivot_vertex == -1) {
				const PackedInt32Array &cell_vertices = level_cell_vertices[level][cell_index];
				VectorN centroid = VectorND::fill(dimension, 0.0);
				for (const int32_t vertex_index : cell_vertices) {
					centroid = VectorND::add(centroid, _simplex_cell_vertex_positions_cache[vertex_index]);
				}
				centroid = VectorND::divide_scalar(centroid, (double)cell_vertices.size());
				pivot_vertex = (int32_t)VectorND::array_append_deduplicate(_simplex_cell_vertex_positions_cache, centroid);
			} else {
				_impose_pivot_on_descendants(poly_cell_indices, level_cell_vertices, level_pivots, level, cell_index, pivot_vertex);
			}
			level_pivots.ptrw()[level].set(cell_index, pivot_vertex);
		}
	}
	// Step 6: Build the simplex decomposition of every cell from the bottom up. A cell's
	// decomposition is the cone of its pivot over the decompositions of its members,
	// skipping members that contain the pivot, whose cones would be flat.
	Vector<Vector<PackedInt32Array>> level_decompositions;
	level_decompositions.resize(boundary_dim_index + 1);
	{
		// Level 0: 2D faces decompose into triangles, the cone of the pivot over the face's edges.
		const Vector<PackedInt32Array> &faces = poly_cell_indices[0];
		Vector<PackedInt32Array> face_decompositions;
		face_decompositions.resize(faces.size());
		for (int64_t face_index = 0; face_index < faces.size(); face_index++) {
			const int32_t pivot_vertex = level_pivots[0][face_index];
			if (pivot_vertex == -1) {
				continue; // This face is not on the surface and not used by any surface cell.
			}
			PackedInt32Array triangles;
			for (const int32_t edge_index : faces[face_index]) {
				const int32_t edge_start_vertex = all_edge_indices[edge_index * 2];
				const int32_t edge_end_vertex = all_edge_indices[edge_index * 2 + 1];
				if (edge_start_vertex == pivot_vertex || edge_end_vertex == pivot_vertex) {
					continue;
				}
				triangles.append(pivot_vertex);
				triangles.append(edge_start_vertex);
				triangles.append(edge_end_vertex);
			}
			face_decompositions.set(face_index, triangles);
		}
		level_decompositions.set(0, face_decompositions);
	}
	for (int64_t level = 1; level <= boundary_dim_index; level++) {
		const int64_t member_simplex_size = level + 2;
		const Vector<PackedInt32Array> &cells_of_level = poly_cell_indices[level];
		Vector<PackedInt32Array> decompositions_of_level;
		decompositions_of_level.resize(cells_of_level.size());
		for (int64_t cell_index = 0; cell_index < cells_of_level.size(); cell_index++) {
			if (level == boundary_dim_index && !surface_cell_set.has((int32_t)cell_index)) {
				continue; // Covered boundary cells are never visible, so don't decompose them.
			}
			const int32_t pivot_vertex = level_pivots[level][cell_index];
			PackedInt32Array cell_simplexes;
			for (const int32_t member_index : cells_of_level[cell_index]) {
				if (level_cell_vertices[level - 1][member_index].has(pivot_vertex)) {
					continue; // The cone of the pivot over a member containing the pivot is flat.
				}
				const PackedInt32Array &member_simplexes = level_decompositions[level - 1][member_index];
				for (int64_t i = 0; i < member_simplexes.size(); i += member_simplex_size) {
					cell_simplexes.append(pivot_vertex);
					for (int64_t j = 0; j < member_simplex_size; j++) {
						cell_simplexes.append(member_simplexes[i + j]);
					}
				}
			}
			decompositions_of_level.set(cell_index, cell_simplexes);
		}
		level_decompositions.set(level, decompositions_of_level);
	}
	// Step 7: Emit the boundary cell simplexes, oriented to match each cell's normal vector.
	const int64_t simplex_size = dimension;
	for (const int32_t cell_index : surface_cells_to_use) {
		const PackedInt32Array &cell_simplexes = level_decompositions[boundary_dim_index][cell_index];
		const VectorN &cell_boundary_normal = poly_cell_boundary_normals[cell_index];
		const bool has_cell_boundary_normal = !VectorND::is_zero_approx(cell_boundary_normal);
		VectorN cell_reference_perp;
		bool has_cell_reference_perp = false;
		for (int64_t simplex_start = 0; simplex_start < cell_simplexes.size(); simplex_start += simplex_size) {
			PackedInt32Array new_simplex;
			new_simplex.resize(simplex_size);
			for (int64_t i = 0; i < simplex_size; i++) {
				new_simplex.set(i, cell_simplexes[simplex_start + i]);
			}
			Vector<VectorN> directions;
			directions.resize(simplex_size - 1);
			for (int64_t i = 1; i < simplex_size; i++) {
				directions.set(i - 1, VectorND::direction_to(_simplex_cell_vertex_positions_cache[new_simplex[0]], _simplex_cell_vertex_positions_cache[new_simplex[i]]));
			}
			const VectorN simplex_perp = VectorND::perpendicular(directions);
			if (VectorND::is_zero_approx(simplex_perp)) {
				// Skip zero-measure simplexes, which have no volume to render or collide with.
				// These arise when a cell has collinear or coplanar chains of vertices, such as
				// a cell bordering subdivided cells, conformed by referencing their sub-elements.
				continue;
			}
			bool should_flip = false;
			if (has_cell_boundary_normal && VectorND::dot(simplex_perp, cell_boundary_normal) < 0.0) {
				should_flip = true;
			}
			VectorN oriented_simplex_perp = should_flip ? VectorND::negate(simplex_perp) : simplex_perp;
			if (has_cell_reference_perp && VectorND::dot(oriented_simplex_perp, cell_reference_perp) < 0.0) {
				should_flip = !should_flip;
				oriented_simplex_perp = VectorND::negate(oriented_simplex_perp);
			}
			if (should_flip) {
				// Any single transposition flips the orientation, so swap the last two vertices.
				const int32_t temp = new_simplex[simplex_size - 2];
				new_simplex.set(simplex_size - 2, new_simplex[simplex_size - 1]);
				new_simplex.set(simplex_size - 1, temp);
			}
			if (!has_cell_reference_perp && !VectorND::is_zero_approx(oriented_simplex_perp)) {
				cell_reference_perp = oriented_simplex_perp;
				has_cell_reference_perp = true;
			}
			_simplex_cell_vertex_indices_cache.append_array(new_simplex);
			_simplex_cell_source_poly_cells.append(cell_index);
		}
	}
}

bool PolyMeshND::_infer_vertex_texcoord_from_cell_pivot_override(const PackedInt32Array &p_source_cell_vertices, const Vector<VectorM> &p_source_cell_texture_map, const int32_t p_target_vertex, VectorM &r_texcoord) {
	const Vector<VectorN> all_vertices = get_poly_cell_vertex_positions();
	const int64_t dimension = get_dimension();
	const int64_t source_count = p_source_cell_vertices.size();
	if (source_count < dimension || p_source_cell_texture_map.size() != source_count) {
		return false;
	}
	if (p_target_vertex < 0 || p_target_vertex >= all_vertices.size()) {
		return false;
	}
	// Pick linearly independent spanning vertices of the source cell, then express the target
	// vertex in that span and map the coordinates into the texture space. Any component of
	// the target outside of the cell's span has no texture image and is discarded.
	PackedInt32Array picked_positions;
	const int64_t rank = _pick_spanning_vertices(all_vertices, p_source_cell_vertices, dimension - 1, picked_positions);
	if (rank < dimension - 1) {
		return false; // The source cell does not span its full hyperplane.
	}
	const VectorN base = all_vertices[p_source_cell_vertices[0]];
	const VectorM tex_base = p_source_cell_texture_map[0];
	Vector<VectorN> world_spans;
	world_spans.resize(rank);
	for (int64_t i = 0; i < rank; i++) {
		world_spans.set(i, VectorND::subtract(all_vertices[p_source_cell_vertices[picked_positions[i]]], base));
	}
	VectorN coordinates;
	if (!_solve_coordinates_in_span(world_spans, VectorND::subtract(all_vertices[p_target_vertex], base), coordinates)) {
		return false;
	}
	VectorM texcoord = VectorND::duplicate(tex_base);
	for (int64_t i = 0; i < rank; i++) {
		const VectorM tex_span = VectorND::subtract(p_source_cell_texture_map[picked_positions[i]], tex_base);
		texcoord = VectorND::add(texcoord, VectorND::multiply_scalar(tex_span, coordinates[i]));
	}
	r_texcoord = texcoord;
	return true;
}

Vector<PackedInt32Array> PolyMeshND::_get_vertex_indices_of_boundary_cells(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_boundary_dim_index, const bool p_start_with_canonical_span) {
	ERR_FAIL_COND_V(p_boundary_dim_index < 0 || p_poly_cell_indices.size() <= p_boundary_dim_index, Vector<PackedInt32Array>());
	Vector<PackedInt32Array> cell_vertex_indices;
	const int64_t boundary_cell_count = p_poly_cell_indices[p_boundary_dim_index].size();
	cell_vertex_indices.resize(boundary_cell_count);
	for (int64_t cell_index = 0; cell_index < boundary_cell_count; cell_index++) {
		if (p_boundary_dim_index == 0 && !p_start_with_canonical_span) {
			cell_vertex_indices.set(cell_index, _get_vertex_indices_of_face(p_all_edge_indices, p_poly_cell_indices[0][cell_index]));
		} else {
			cell_vertex_indices.set(cell_index, _get_vertex_indices_of_poly_cell(p_poly_cell_indices, p_all_edge_indices, p_boundary_dim_index, cell_index, p_start_with_canonical_span));
		}
	}
	return cell_vertex_indices;
}

Vector<VectorN> PolyMeshND::_compute_boundary_normals_based_on_cell_orientation(const Vector<PackedInt32Array> &p_boundary_cell_vertex_indices, const bool p_keep_existing) {
	const Vector<VectorN> poly_cell_vertices = get_poly_cell_vertex_positions();
	ERR_FAIL_COND_V_MSG(poly_cell_vertices.is_empty(), Vector<VectorN>(), "PolyMeshND: Poly cell vertex positions are required to compute boundary normals.");
	const int64_t dimension = get_dimension();
	Vector<VectorN> poly_cell_normals;
	if (p_keep_existing) {
		poly_cell_normals = get_poly_cell_boundary_normals();
	}
	const int64_t boundary_cell_count = p_boundary_cell_vertex_indices.size();
	poly_cell_normals.resize(boundary_cell_count);
	for (int64_t cell_index = 0; cell_index < boundary_cell_count; cell_index++) {
		if (p_keep_existing && !VectorND::is_zero_approx(poly_cell_normals[cell_index])) {
			continue; // Keep existing normal.
		}
		const PackedInt32Array &cell_vertices = p_boundary_cell_vertex_indices[cell_index];
		ERR_FAIL_COND_V_MSG(cell_vertices.size() < dimension, poly_cell_normals, "PolyMeshND: Cannot compute normal for boundary cell because it has fewer than " + itos(dimension) + " vertices.");
		Vector<VectorN> directions;
		directions.resize(dimension - 1);
		for (int64_t i = 1; i < dimension; i++) {
			directions.set(i - 1, VectorND::direction_to(poly_cell_vertices[cell_vertices[0]], poly_cell_vertices[cell_vertices[i]]));
		}
		const VectorN cell_perp = VectorND::perpendicular(directions);
		poly_cell_normals.set(cell_index, VectorND::normalized(cell_perp));
	}
	return poly_cell_normals;
}

Vector<PackedInt32Array> PolyMeshND::get_all_face_vertex_indices() {
	ERR_FAIL_COND_V(!is_mesh_data_valid(), Vector<PackedInt32Array>());
	const Vector<Vector<PackedInt32Array>> poly_cell_indices = get_poly_cell_indices();
	ERR_FAIL_COND_V(poly_cell_indices.is_empty(), Vector<PackedInt32Array>());
	const Vector<PackedInt32Array> face_edge_indices = poly_cell_indices[0];
	const PackedInt32Array all_edge_indices = get_edge_indices();
	ERR_FAIL_COND_V(all_edge_indices.is_empty(), Vector<PackedInt32Array>());
	Vector<PackedInt32Array> ret;
	ret.resize(face_edge_indices.size());
	for (int64_t face_index = 0; face_index < face_edge_indices.size(); face_index++) {
		ret.set(face_index, _get_vertex_indices_of_face(all_edge_indices, face_edge_indices[face_index]));
	}
	return ret;
}

TypedArray<PackedInt32Array> PolyMeshND::get_all_face_vertex_indices_bind() {
	const Vector<PackedInt32Array> vec = get_all_face_vertex_indices();
	TypedArray<PackedInt32Array> ret;
	ret.resize(vec.size());
	for (int64_t i = 0; i < vec.size(); i++) {
		ret[i] = vec[i];
	}
	return ret;
}

Vector<PackedInt32Array> PolyMeshND::get_all_boundary_cell_vertex_indices(const bool p_start_with_canonical_span) {
	ERR_FAIL_COND_V(!is_mesh_data_valid(), Vector<PackedInt32Array>());
	const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
	const Vector<Vector<PackedInt32Array>> poly_cell_indices = get_poly_cell_indices();
	ERR_FAIL_COND_V(boundary_dim_index < 0 || poly_cell_indices.size() <= boundary_dim_index, Vector<PackedInt32Array>());
	const PackedInt32Array all_edge_indices = get_edge_indices();
	ERR_FAIL_COND_V(all_edge_indices.is_empty(), Vector<PackedInt32Array>());
	return _get_vertex_indices_of_boundary_cells(poly_cell_indices, all_edge_indices, boundary_dim_index, p_start_with_canonical_span);
}

TypedArray<PackedInt32Array> PolyMeshND::get_all_boundary_cell_vertex_indices_bind(const bool p_start_with_canonical_span) {
	const Vector<PackedInt32Array> vec = get_all_boundary_cell_vertex_indices(p_start_with_canonical_span);
	TypedArray<PackedInt32Array> ret;
	ret.resize(vec.size());
	for (int64_t i = 0; i < vec.size(); i++) {
		ret[i] = vec[i];
	}
	return ret;
}

Vector<PackedInt32Array> PolyMeshND::get_all_poly_cell_vertex_indices(const int p_cell_dimension, const bool p_start_with_canonical_span) {
	Vector<PackedInt32Array> ret;
	ERR_FAIL_COND_V(!is_mesh_data_valid(), ret);
	const Vector<Vector<PackedInt32Array>> &poly_cell_indices = get_poly_cell_indices();
	ERR_FAIL_COND_V(p_cell_dimension >= poly_cell_indices.size() + 2, ret);
	if (p_cell_dimension == 0) {
		// Degenerate case: the "decomposition" of a vertex into its own dimension is just itself.
		const int64_t vertex_count = get_poly_cell_vertex_positions().size();
		ret.resize(vertex_count);
		for (int64_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
			ret.set(vertex_index, PackedInt32Array{ (int32_t)vertex_index });
		}
		return ret;
	}
	const PackedInt32Array &all_edge_indices = get_edge_indices();
	if (p_cell_dimension == 1) {
		const int64_t edge_count = all_edge_indices.size() / 2;
		ret.resize(edge_count);
		for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
			ret.set(edge_index, PackedInt32Array{ all_edge_indices[edge_index * 2], all_edge_indices[edge_index * 2 + 1] });
		}
		return ret;
	}
	ERR_FAIL_COND_V(all_edge_indices.is_empty(), ret);
	const Vector<PackedInt32Array> &cells = poly_cell_indices[p_cell_dimension - 2];
	ret.resize(cells.size());
	for (int64_t cell_index = 0; cell_index < cells.size(); cell_index++) {
		ret.set(cell_index, _get_vertex_indices_of_poly_cell(poly_cell_indices, all_edge_indices, p_cell_dimension - 2, cell_index, p_start_with_canonical_span));
	}
	return ret;
}

TypedArray<PackedInt32Array> PolyMeshND::get_all_poly_cell_vertex_indices_bind(const int p_cell_dimension, const bool p_start_with_canonical_span) {
	const Vector<PackedInt32Array> vec = get_all_poly_cell_vertex_indices(p_cell_dimension, p_start_with_canonical_span);
	TypedArray<PackedInt32Array> ret;
	ret.resize(vec.size());
	for (int64_t i = 0; i < vec.size(); i++) {
		ret[i] = vec[i];
	}
	return ret;
}

Vector<PackedInt32Array> PolyMeshND::get_all_poly_cell_poly_indices(const int p_cell_dimension, const int p_decomposition_dimension) {
	Vector<PackedInt32Array> ret;
	ERR_FAIL_COND_V(!is_mesh_data_valid(), ret);
	ERR_FAIL_COND_V(p_decomposition_dimension > p_cell_dimension || p_decomposition_dimension < 0, ret);
	const Vector<Vector<PackedInt32Array>> &poly_cell_indices = get_poly_cell_indices();
	ERR_FAIL_INDEX_V(p_cell_dimension, poly_cell_indices.size() + 2, ret);
	const PackedInt32Array &all_edge_indices = get_edge_indices();
	const int64_t cells_in_dimension = (p_cell_dimension == 0) ? get_poly_cell_vertex_positions().size() : ((p_cell_dimension == 1) ? all_edge_indices.size() / 2 : poly_cell_indices[p_cell_dimension - 2].size());
	if (p_decomposition_dimension == p_cell_dimension) {
		// Degenerate case: the "decomposition" of a cell into its own dimension is just itself.
		ret.resize(cells_in_dimension);
		for (int64_t cell_index = 0; cell_index < cells_in_dimension; cell_index++) {
			ret.set(cell_index, PackedInt32Array{ (int32_t)cell_index });
		}
	} else if (p_decomposition_dimension == p_cell_dimension - 1) {
		// The "decomposition" of a cell into the cells of the next-lower dimension is just its direct components.
		if (p_cell_dimension == 1) {
			// Special case: Edges stored in a flat way, and need to be repacked.
			ret.resize(all_edge_indices.size() / 2);
			for (int64_t edge_index = 0; edge_index < all_edge_indices.size() / 2; edge_index++) {
				ret.set(edge_index, PackedInt32Array{ all_edge_indices[edge_index * 2], all_edge_indices[edge_index * 2 + 1] });
			}
		} else {
			ret = poly_cell_indices[p_cell_dimension - 2];
		}
	} else if (p_decomposition_dimension == 0) {
		ret = get_all_poly_cell_vertex_indices(p_cell_dimension, false);
	} else {
		// In the general case, run this function recursively.
		const Vector<PackedInt32Array> all_level_up = get_all_poly_cell_poly_indices(p_cell_dimension, p_decomposition_dimension + 1);
		const Vector<PackedInt32Array> all_level_down = get_all_poly_cell_poly_indices(p_decomposition_dimension + 1, p_decomposition_dimension);
		ret.resize(cells_in_dimension);
		for (int64_t cell_index = 0; cell_index < cells_in_dimension; cell_index++) {
			PackedInt32Array level_up = all_level_up[cell_index];
			HashSet<int32_t> seen_elements;
			PackedInt32Array elements;
			for (const int32_t level_up_element : level_up) {
				const PackedInt32Array in_level_down = all_level_down[level_up_element];
				for (const int32_t level_down_element : in_level_down) {
					if (!seen_elements.has(level_down_element)) {
						seen_elements.insert(level_down_element);
						elements.append(level_down_element);
					}
				}
			}
			ret.set(cell_index, elements);
		}
	}
	return ret;
}

TypedArray<PackedInt32Array> PolyMeshND::get_all_poly_cell_poly_indices_bind(const int p_cell_dimension, const int p_decomposition_dimension) {
	const Vector<PackedInt32Array> vec = get_all_poly_cell_poly_indices(p_cell_dimension, p_decomposition_dimension);
	TypedArray<PackedInt32Array> ret;
	ret.resize(vec.size());
	for (int64_t cell_index = 0; cell_index < vec.size(); cell_index++) {
		ret[cell_index] = vec[cell_index];
	}
	return ret;
}

void PolyMeshND::poly_mesh_clear_cache(const bool p_normals_only) {
	_simplex_cell_boundary_normals_cache.clear();
	_simplex_cell_vertex_normals_cache.clear();
	reset_poly_mesh_data_validation();
	// Normals can be computed separately from the rest, so allow resetting just them (and mark the proxy mesh 3D dirty).
	if (p_normals_only) {
		mark_proxy_mesh_3d_dirty();
		return;
	}
	_simplex_cell_vertex_indices_cache.clear();
	_simplex_cell_source_poly_cells.clear();
	_simplex_cell_uvw_texture_map_cache.clear();
	_simplex_cell_vertex_positions_cache.clear();
	cell_mesh_clear_cache();
}

Ref<ArrayPolyMeshND> PolyMeshND::to_array_poly_mesh() {
	// Copy all of this data to a new ArrayPolyMeshND.
	Ref<ArrayPolyMeshND> array_poly_mesh;
	array_poly_mesh.instantiate();
	array_poly_mesh->set_poly_cell_vertex_positions(get_poly_cell_vertex_positions());
	array_poly_mesh->set_edge_vertex_indices(get_edge_indices());
	array_poly_mesh->set_poly_cell_indices(get_poly_cell_indices());
	array_poly_mesh->set_poly_cell_boundary_normals(get_poly_cell_boundary_normals());
	array_poly_mesh->set_poly_cell_vertex_normals(get_poly_cell_vertex_normals());
	array_poly_mesh->set_poly_cell_texture_map(get_poly_cell_texture_map());
	array_poly_mesh->set_material(get_material());
	return array_poly_mesh;
}

int32_t PolyMeshND::get_source_poly_cell_for_simplex_cell(const int32_t p_simplex_cell_index) const {
	if (p_simplex_cell_index < 0 || p_simplex_cell_index >= _simplex_cell_source_poly_cells.size()) {
		return -1;
	}
	return _simplex_cell_source_poly_cells[p_simplex_cell_index];
}

int PolyMeshND::get_dimension() {
	const Vector<VectorN> vertex_positions = get_poly_cell_vertex_positions();
	if (vertex_positions.is_empty()) {
		return 0;
	}
	return vertex_positions[0].size();
}

Vector<Vector<PackedInt32Array>> PolyMeshND::get_poly_cell_indices() {
	TypedArray<Array> indices_bind;
	GDVIRTUAL_CALL(_get_poly_cell_indices, indices_bind);
	Vector<Vector<PackedInt32Array>> indices;
	indices.resize(indices_bind.size());
	for (int i = 0; i < indices_bind.size(); i++) {
		Array dim_array = indices_bind[i];
		Vector<PackedInt32Array> dim_vector;
		dim_vector.resize(dim_array.size());
		for (int j = 0; j < dim_array.size(); j++) {
			const PackedInt32Array cell_indices = dim_array[j];
			dim_vector.set(j, cell_indices);
		}
		indices.set(i, dim_vector);
	}
	return indices;
}

Vector<VectorN> PolyMeshND::get_poly_cell_vertex_positions() {
	TypedArray<VectorN> vertex_positions_bind;
	GDVIRTUAL_CALL(_get_poly_cell_vertex_positions, vertex_positions_bind);
	Vector<VectorN> vertex_positions;
	vertex_positions.resize(vertex_positions_bind.size());
	for (int i = 0; i < vertex_positions_bind.size(); i++) {
		const VectorN vertex = vertex_positions_bind[i];
		vertex_positions.set(i, vertex);
	}
	return vertex_positions;
}

Vector<VectorN> PolyMeshND::get_poly_cell_boundary_normals() {
	TypedArray<VectorN> normals_bind;
	GDVIRTUAL_CALL(_get_poly_cell_boundary_normals, normals_bind);
	Vector<VectorN> normals;
	normals.resize(normals_bind.size());
	for (int i = 0; i < normals_bind.size(); i++) {
		const VectorN normal = normals_bind[i];
		normals.set(i, normal);
	}
	return normals;
}

PackedInt32Array PolyMeshND::get_poly_cell_boundary_pivot_overrides() {
	PackedInt32Array pivot_overrides;
	GDVIRTUAL_CALL(_get_poly_cell_boundary_pivot_overrides, pivot_overrides);
	return pivot_overrides;
}

Vector<Vector<VectorN>> PolyMeshND::get_poly_cell_vertex_normals() {
	TypedArray<Array> vertex_normals_bind;
	GDVIRTUAL_CALL(_get_poly_cell_vertex_normals, vertex_normals_bind);
	Vector<Vector<VectorN>> vertex_normals;
	vertex_normals.resize(vertex_normals_bind.size());
	for (int i = 0; i < vertex_normals_bind.size(); i++) {
		const Array cell_vertex_normals_array = vertex_normals_bind[i];
		Vector<VectorN> cell_vertex_normals;
		cell_vertex_normals.resize(cell_vertex_normals_array.size());
		for (int j = 0; j < cell_vertex_normals_array.size(); j++) {
			const VectorN cell_vertex_normal = cell_vertex_normals_array[j];
			cell_vertex_normals.set(j, cell_vertex_normal);
		}
		vertex_normals.set(i, cell_vertex_normals);
	}
	return vertex_normals;
}

Vector<Vector<VectorM>> PolyMeshND::get_poly_cell_texture_map() {
	TypedArray<Array> uvw_texture_map_bind;
	GDVIRTUAL_CALL(_get_poly_cell_texture_map, uvw_texture_map_bind);
	Vector<Vector<VectorM>> uvw_texture_map;
	uvw_texture_map.resize(uvw_texture_map_bind.size());
	for (int i = 0; i < uvw_texture_map_bind.size(); i++) {
		const Array cell_texture_map_array = uvw_texture_map_bind[i];
		Vector<VectorM> cell_texture_map;
		cell_texture_map.resize(cell_texture_map_array.size());
		for (int j = 0; j < cell_texture_map_array.size(); j++) {
			const VectorM cell_texcoord = cell_texture_map_array[j];
			cell_texture_map.set(j, cell_texcoord);
		}
		uvw_texture_map.set(i, cell_texture_map);
	}
	return uvw_texture_map;
}

TypedArray<Array> PolyMeshND::get_poly_cell_indices_bind() {
	TypedArray<Array> indices_bind;
	GDVIRTUAL_CALL(_get_poly_cell_indices, indices_bind);
	if (!indices_bind.is_empty()) {
		return indices_bind;
	}
	const Vector<Vector<PackedInt32Array>> indices = get_poly_cell_indices();
	indices_bind.resize(indices.size());
	for (int i = 0; i < indices.size(); i++) {
		const Vector<PackedInt32Array> &dim_vector = indices[i];
		Array dim_array;
		dim_array.resize(dim_vector.size());
		for (int j = 0; j < dim_vector.size(); j++) {
			const PackedInt32Array &cell_indices = dim_vector[j];
			dim_array[j] = cell_indices;
		}
		indices_bind[i] = dim_array;
	}
	return indices_bind;
}

TypedArray<VectorN> PolyMeshND::get_poly_cell_vertex_positions_bind() {
	TypedArray<VectorN> vertex_positions_bind;
	GDVIRTUAL_CALL(_get_poly_cell_vertex_positions, vertex_positions_bind);
	if (!vertex_positions_bind.is_empty()) {
		return vertex_positions_bind;
	}
	const Vector<VectorN> vertex_positions = get_poly_cell_vertex_positions();
	vertex_positions_bind.resize(vertex_positions.size());
	for (int i = 0; i < vertex_positions.size(); i++) {
		vertex_positions_bind[i] = vertex_positions[i];
	}
	return vertex_positions_bind;
}

TypedArray<VectorN> PolyMeshND::get_poly_cell_boundary_normals_bind() {
	TypedArray<VectorN> normals_bind;
	GDVIRTUAL_CALL(_get_poly_cell_boundary_normals, normals_bind);
	if (!normals_bind.is_empty()) {
		return normals_bind;
	}
	const Vector<VectorN> normals = get_poly_cell_boundary_normals();
	normals_bind.resize(normals.size());
	for (int i = 0; i < normals.size(); i++) {
		normals_bind[i] = normals[i];
	}
	return normals_bind;
}

TypedArray<Array> PolyMeshND::get_poly_cell_vertex_normals_bind() {
	TypedArray<Array> vertex_normals_bind;
	GDVIRTUAL_CALL(_get_poly_cell_vertex_normals, vertex_normals_bind);
	if (!vertex_normals_bind.is_empty()) {
		return vertex_normals_bind;
	}
	const Vector<Vector<VectorN>> vertex_normals = get_poly_cell_vertex_normals();
	vertex_normals_bind.resize(vertex_normals.size());
	for (int i = 0; i < vertex_normals.size(); i++) {
		const Vector<VectorN> &cell_vertex_normals = vertex_normals[i];
		Array cell_vertex_normals_array;
		cell_vertex_normals_array.resize(cell_vertex_normals.size());
		for (int j = 0; j < cell_vertex_normals.size(); j++) {
			cell_vertex_normals_array[j] = cell_vertex_normals[j];
		}
		vertex_normals_bind[i] = cell_vertex_normals_array;
	}
	return vertex_normals_bind;
}

TypedArray<Array> PolyMeshND::get_poly_cell_texture_map_bind() {
	TypedArray<Array> uvw_texture_map_bind;
	GDVIRTUAL_CALL(_get_poly_cell_texture_map, uvw_texture_map_bind);
	if (!uvw_texture_map_bind.is_empty()) {
		return uvw_texture_map_bind;
	}
	const Vector<Vector<VectorM>> texture_map = get_poly_cell_texture_map();
	uvw_texture_map_bind.resize(texture_map.size());
	for (int i = 0; i < texture_map.size(); i++) {
		const Vector<VectorM> &cell_texture_map = texture_map[i];
		Array cell_texture_map_array;
		cell_texture_map_array.resize(cell_texture_map.size());
		for (int j = 0; j < cell_texture_map.size(); j++) {
			cell_texture_map_array[j] = cell_texture_map[j];
		}
		uvw_texture_map_bind[i] = cell_texture_map_array;
	}
	return uvw_texture_map_bind;
}

PackedInt32Array PolyMeshND::get_simplex_cell_vertex_indices() {
	if (_simplex_cell_vertex_indices_cache.is_empty()) {
		// Only attempt to decompose boundary cells into simplexes if there are boundary cells to decompose.
		const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
		if (boundary_dim_index >= 0 && get_poly_cell_indices().size() > boundary_dim_index) {
			// This will populate the simplex indices cache as a side effect.
			_decompose_boundary_cells_into_simplexes();
		}
	}
	return _simplex_cell_vertex_indices_cache;
}

Vector<VectorN> PolyMeshND::get_simplex_cell_boundary_normals() {
	if (_simplex_cell_boundary_normals_cache.is_empty()) {
		if (_simplex_cell_vertex_indices_cache.is_empty()) {
			_decompose_boundary_cells_into_simplexes();
		}
		const int64_t dimension = get_dimension();
		if (dimension < 3 || _simplex_cell_vertex_indices_cache.is_empty()) {
			return _simplex_cell_boundary_normals_cache;
		}
		const int64_t simplex_count = _simplex_cell_vertex_indices_cache.size() / dimension;
		_simplex_cell_boundary_normals_cache.resize(simplex_count);
		for (int64_t i = 0; i < simplex_count; i++) {
			const int64_t offset = i * dimension;
			const int32_t vert0 = _simplex_cell_vertex_indices_cache[offset];
			Vector<VectorN> directions;
			directions.resize(dimension - 1);
			for (int64_t dim = 1; dim < dimension; dim++) {
				const int32_t vert_n = _simplex_cell_vertex_indices_cache[offset + dim];
				directions.set(dim - 1, VectorND::direction_to(_simplex_cell_vertex_positions_cache[vert0], _simplex_cell_vertex_positions_cache[vert_n]));
			}
			const VectorN perp = VectorND::perpendicular(directions);
			_simplex_cell_boundary_normals_cache.set(i, VectorND::normalized(perp));
		}
	}
	return _simplex_cell_boundary_normals_cache;
}

Vector<VectorN> PolyMeshND::get_simplex_cell_vertex_normals() {
	if (_simplex_cell_vertex_normals_cache.is_empty()) {
		const Vector<Vector<VectorN>> poly_cell_vertex_normals = get_poly_cell_vertex_normals();
		if (poly_cell_vertex_normals.is_empty()) {
			return Vector<VectorN>(); // No vertex normal data available.
		}
		const int64_t dimension = get_dimension();
		ERR_FAIL_COND_V(dimension < 3, Vector<VectorN>());
		int64_t simplex_count = _simplex_cell_source_poly_cells.size();
		if (simplex_count == 0 || simplex_count * dimension != _simplex_cell_vertex_indices_cache.size()) {
			_decompose_boundary_cells_into_simplexes();
			simplex_count = _simplex_cell_source_poly_cells.size();
			if (simplex_count == 0) {
				return Vector<VectorN>(); // Nothing on the surface to compute vertex normals for.
			}
			CRASH_COND_MSG(simplex_count * dimension != _simplex_cell_vertex_indices_cache.size(), "PolyMeshND: Simplex cell indices cache is corrupt.");
		}
		const int64_t boundary_dim_index = dimension - 3;
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = get_poly_cell_indices();
		ERR_FAIL_COND_V_MSG(poly_cell_indices.size() <= boundary_dim_index, Vector<VectorN>(), "PolyMeshND: No boundary cells available, cannot compute simplex vertex normals.");
		const PackedInt32Array all_edge_indices = get_edge_indices();
		const Vector<PackedInt32Array> cell_vert = _get_vertex_indices_of_boundary_cells(poly_cell_indices, all_edge_indices, boundary_dim_index, false);
		_simplex_cell_vertex_normals_cache.resize(simplex_count * dimension);
		bool has_some_vertex_normals = false;
		bool missing_some_vertex_normals = false;
		for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
			const int32_t source_cell = _simplex_cell_source_poly_cells[simplex_index];
			const Vector<VectorN> &source_poly_vertex_normals = poly_cell_vertex_normals[source_cell];
			if (source_poly_vertex_normals.is_empty()) {
				missing_some_vertex_normals = true;
				continue;
			}
			has_some_vertex_normals = true;
			const PackedInt32Array &source_cell_vertices = cell_vert[source_cell];
			CRASH_COND_MSG(source_poly_vertex_normals.size() != source_cell_vertices.size(), "PolyMeshND: Source polytope cell vertex normals size does not match cell vertex count.");
			const int64_t offset = simplex_index * dimension;
			for (int64_t vertex_in_simplex = 0; vertex_in_simplex < dimension; vertex_in_simplex++) {
				const int32_t vertex_index = _simplex_cell_vertex_indices_cache[offset + vertex_in_simplex];
				const int64_t vertex_in_source_poly = source_cell_vertices.find(vertex_index);
				VectorN normal;
				if (vertex_in_source_poly == -1) {
					normal = VectorND::average(source_poly_vertex_normals);
				} else {
					normal = source_poly_vertex_normals[vertex_in_source_poly];
				}
				_simplex_cell_vertex_normals_cache.set(offset + vertex_in_simplex, normal);
			}
		}
		if (missing_some_vertex_normals) {
			if (has_some_vertex_normals) {
				WARN_PRINT("PolyMeshND: Some polytope cells are missing vertex normal data, the corresponding simplex vertex normals will be averaged from the available data which may produce inaccurate lighting.");
			} else {
				_simplex_cell_vertex_normals_cache.clear(); // No vertex normal data available.
			}
		}
	}
	return _simplex_cell_vertex_normals_cache;
}

Vector<VectorM> PolyMeshND::get_simplex_cell_texture_map() {
	if (_simplex_cell_uvw_texture_map_cache.is_empty()) {
		const Vector<Vector<VectorM>> poly_cell_texture_map = get_poly_cell_texture_map();
		if (poly_cell_texture_map.is_empty()) {
			return Vector<VectorM>(); // No texture map data available.
		}
		const int64_t dimension = get_dimension();
		ERR_FAIL_COND_V(dimension < 3, Vector<VectorM>());
		int64_t simplex_count = _simplex_cell_source_poly_cells.size();
		if (simplex_count == 0 || simplex_count * dimension != _simplex_cell_vertex_indices_cache.size()) {
			_decompose_boundary_cells_into_simplexes();
			simplex_count = _simplex_cell_source_poly_cells.size();
			if (simplex_count == 0) {
				return Vector<VectorM>(); // Nothing on the surface to texture map.
			}
			CRASH_COND_MSG(simplex_count * dimension != _simplex_cell_vertex_indices_cache.size(), "PolyMeshND: Simplex cell indices cache is corrupt.");
		}
		const int64_t boundary_dim_index = dimension - 3;
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = get_poly_cell_indices();
		ERR_FAIL_COND_V_MSG(poly_cell_indices.size() <= boundary_dim_index, Vector<VectorM>(), "PolyMeshND: No boundary cells available, cannot compute simplex UVW map.");
		const PackedInt32Array all_edge_indices = get_edge_indices();
		const Vector<PackedInt32Array> cell_vert = _get_vertex_indices_of_boundary_cells(poly_cell_indices, all_edge_indices, boundary_dim_index, false);
		// Prepare a cache for inferred vertex texcoords for pivot overrides.
		const PackedInt32Array poly_cell_boundary_pivot_overrides = get_poly_cell_boundary_pivot_overrides();
		const int64_t cell_vert_count = cell_vert.size();
		Vector<int8_t> cached_inference_state;
		cached_inference_state.resize_initialized(cell_vert_count);
		Vector<VectorM> cached_inferred_texcoord;
		cached_inferred_texcoord.resize(cell_vert_count);
		for (int64_t cell_vert_index = 0; cell_vert_index < cell_vert_count; cell_vert_index++) {
			cached_inference_state.set(cell_vert_index, (int8_t)0);
			cached_inferred_texcoord.set(cell_vert_index, VectorM());
		}
		// Fill the texture map cache for each simplex cell using data from the corresponding source polytope cell.
		_simplex_cell_uvw_texture_map_cache.resize(simplex_count * dimension);
		bool has_some_texture_map = false;
		bool missing_some_texture_map = false;
		for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
			const int32_t source_cell = _simplex_cell_source_poly_cells[simplex_index];
			const Vector<VectorM> &source_poly_texture_map = poly_cell_texture_map[source_cell];
			if (source_poly_texture_map.is_empty()) {
				missing_some_texture_map = true;
				continue;
			}
			has_some_texture_map = true;
			const PackedInt32Array &source_cell_vertices = cell_vert[source_cell];
			CRASH_COND_MSG(source_poly_texture_map.size() != source_cell_vertices.size(), "PolyMeshND: Source polytope cell texture map size does not match cell vertex count.");
			const int64_t offset = simplex_index * dimension;
			for (int64_t vertex_in_simplex = 0; vertex_in_simplex < dimension; vertex_in_simplex++) {
				const int32_t vertex_index = _simplex_cell_vertex_indices_cache[offset + vertex_in_simplex];
				const int64_t vertex_in_source_poly = source_cell_vertices.find(vertex_index);
				VectorM texcoord;
				if (vertex_in_source_poly == -1) {
					// If the simplexes contain a vertex that is not on the original polytope cell surface,
					// then it is either a pivot override, or a computed centroid. Check for overrides first.
					bool used_pivot_override = false;
					if (poly_cell_boundary_pivot_overrides.size() > source_cell) {
						const int32_t pivot_override_vertex = poly_cell_boundary_pivot_overrides[source_cell];
						if (pivot_override_vertex >= 0 && vertex_index == pivot_override_vertex) {
							//  0: Not inferred yet (should try to attempt inference, then leads to 1 or -1).
							//  1: Inferred successfully (use the cached value).
							// -1: Inference attempted but failed (use average as fallback).
							const int8_t inference_state = cached_inference_state[source_cell];
							if (inference_state == 1) {
								texcoord = cached_inferred_texcoord[source_cell];
								used_pivot_override = true;
							} else if (inference_state == 0) {
								used_pivot_override = _infer_vertex_texcoord_from_cell_pivot_override(source_cell_vertices, source_poly_texture_map, pivot_override_vertex, texcoord);
								cached_inference_state.set(source_cell, used_pivot_override ? (int8_t)1 : (int8_t)-1);
								if (used_pivot_override) {
									cached_inferred_texcoord.set(source_cell, texcoord);
								}
							}
						}
					}
					// If this vertex is not a pivot override, or if it is but we couldn't infer a texcoord for it,
					// then just average the existing texcoords for this cell as a fallback.
					if (!used_pivot_override) {
						texcoord = _average_vector_m(source_poly_texture_map);
					}
				} else {
					texcoord = source_poly_texture_map[vertex_in_source_poly];
				}
				_simplex_cell_uvw_texture_map_cache.set(offset + vertex_in_simplex, texcoord);
			}
		}
		if (missing_some_texture_map) {
			if (has_some_texture_map) {
				WARN_PRINT("PolyMeshND: Some polytope cells are missing UVW texture map data, the texture mapping will be missing for the corresponding simplexes.");
			} else {
				_simplex_cell_uvw_texture_map_cache.clear(); // No texture map data available.
			}
		}
	}
	return _simplex_cell_uvw_texture_map_cache;
}

Vector<VectorN> PolyMeshND::get_vertex_positions() {
	if (_simplex_cell_vertex_positions_cache.is_empty()) {
		// Only attempt to decompose boundary cells into simplexes if there are boundary cells to decompose.
		const int64_t boundary_dim_index = _get_boundary_poly_dim_index();
		if (boundary_dim_index >= 0 && get_poly_cell_indices().size() > boundary_dim_index) {
			// This will populate the simplex vertices cache as a side effect.
			_decompose_boundary_cells_into_simplexes();
		} else {
			// If there are no boundary cells, then we can just use the poly cell vertices directly as the simplex vertices.
			_simplex_cell_vertex_positions_cache = get_poly_cell_vertex_positions();
		}
	}
	return _simplex_cell_vertex_positions_cache;
}

void PolyMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_all_face_vertex_indices"), &PolyMeshND::get_all_face_vertex_indices_bind);
	ClassDB::bind_method(D_METHOD("get_all_cell_vertex_indices", "start_with_canonical_span"), &PolyMeshND::get_all_boundary_cell_vertex_indices_bind);
	ClassDB::bind_method(D_METHOD("get_all_poly_cell_vertex_indices", "cell_dimension", "start_with_canonical_span"), &PolyMeshND::get_all_poly_cell_vertex_indices_bind);
	ClassDB::bind_method(D_METHOD("get_all_poly_cell_poly_indices", "cell_dimension", "decomposition_dimension"), &PolyMeshND::get_all_poly_cell_poly_indices_bind);
	ClassDB::bind_method(D_METHOD("poly_mesh_clear_cache", "normals_only"), &PolyMeshND::poly_mesh_clear_cache, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("to_array_poly_mesh"), &PolyMeshND::to_array_poly_mesh);

	ClassDB::bind_method(D_METHOD("get_source_poly_cell_for_simplex_cell", "simplex_cell_index"), &PolyMeshND::get_source_poly_cell_for_simplex_cell);

	ClassDB::bind_method(D_METHOD("get_poly_cell_indices"), &PolyMeshND::get_poly_cell_indices_bind);
	ClassDB::bind_method(D_METHOD("get_poly_cell_vertex_positions"), &PolyMeshND::get_poly_cell_vertex_positions_bind);
	ClassDB::bind_method(D_METHOD("get_poly_cell_boundary_normals"), &PolyMeshND::get_poly_cell_boundary_normals_bind);
	ClassDB::bind_method(D_METHOD("get_poly_cell_boundary_pivot_overrides"), &PolyMeshND::get_poly_cell_boundary_pivot_overrides);
	ClassDB::bind_method(D_METHOD("get_poly_cell_vertex_normals"), &PolyMeshND::get_poly_cell_vertex_normals_bind);
	ClassDB::bind_method(D_METHOD("get_poly_cell_texture_map"), &PolyMeshND::get_poly_cell_texture_map_bind);

	GDVIRTUAL_BIND(_get_poly_cell_indices);
	GDVIRTUAL_BIND(_get_poly_cell_vertex_positions);
	GDVIRTUAL_BIND(_get_poly_cell_boundary_normals);
	GDVIRTUAL_BIND(_get_poly_cell_boundary_pivot_overrides);
	GDVIRTUAL_BIND(_get_poly_cell_vertex_normals);
	GDVIRTUAL_BIND(_get_poly_cell_texture_map);
}
