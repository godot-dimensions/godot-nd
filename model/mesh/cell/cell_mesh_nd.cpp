#include "cell_mesh_nd.h"

#include "../../../math/geometry_nd.h"
#include "../../../math/plane_nd.h"
#include "../../../math/vector_nd.h"
#include "array_cell_mesh_nd.h"
#include "cell_material_nd.h"

int64_t CellMeshND::_binomial_coefficient(const int64_t n, const int64_t k) {
	if (k < 0 || k > n) {
		return 0;
	}
	if (k == 0 || k == n) {
		return 1;
	}
	int64_t result = 1;
	for (int64_t i = 1; i <= k; i++) {
		result *= n - (k - i);
		result /= i;
	}
	return result;
}

// Internal helper function used only by `_generate_combinations`.
void CellMeshND::_generate_combinations_recursive(const PackedInt32Array &p_items, const int64_t p_count, const int64_t p_choose, const int64_t p_start, const int64_t p_depth, int &r_result_index, PackedInt32Array &r_current, Vector<PackedInt32Array> &r_result) {
	if (p_depth == p_choose) {
		r_result.set(r_result_index++, r_current);
		return;
	}
	for (int64_t i = p_start; i < p_count; i++) {
		r_current.set(p_depth, p_items[i]);
		_generate_combinations_recursive(p_items, p_count, p_choose, i + 1, p_depth + 1, r_result_index, r_current, r_result);
	}
}

Vector<PackedInt32Array> CellMeshND::_generate_combinations(const PackedInt32Array &p_items, int64_t p_choose) {
	Vector<PackedInt32Array> result;
	result.resize(_binomial_coefficient(p_items.size(), p_choose));
	int result_index = 0;
	if (p_choose > p_items.size() || p_choose < 0) {
		return result; // Invalid input, return empty.
	}
	PackedInt32Array current;
	current.resize(p_choose);
	_generate_combinations_recursive(p_items, p_items.size(), p_choose, 0, 0, result_index, current, result);
	return result;
}

// Find unique opposing faces of the cell that are not coplanar with the pivot.
Vector<PackedInt32Array> CellMeshND::_determine_opposing_faces(const Vector<VectorN> &p_vertex_positions, const PackedInt32Array &p_poly_cell_indices_without_pivot, const int p_dimension, const int p_pivot_index, const Vector<VectorN> &p_poly_cell_normals, Vector<VectorN> &r_out_normals) {
	Vector<PackedInt32Array> combinations = _generate_combinations(p_poly_cell_indices_without_pivot, p_dimension);
	const VectorN pivot_vertex_position = p_vertex_positions[p_pivot_index];
	Vector<PackedInt32Array> opposing_faces;
	for (const PackedInt32Array &combination : combinations) {
		Vector<VectorN> plane_points;
		plane_points.resize(combination.size() + p_poly_cell_normals.size());
		for (int64_t i = 0; i < combination.size(); i++) {
			plane_points.set(i, p_vertex_positions[combination[i]]);
		}
		for (int64_t i = 0; i < p_poly_cell_normals.size(); i++) {
			plane_points.set(i + combination.size(), VectorND::add(plane_points[0], p_poly_cell_normals[i]));
		}
		const Ref<PlaneND> plane = PlaneND::from_points(plane_points);
		if (plane.is_null()) {
			continue; // Skip invalid planes.
		}
		const double pivot_distance = plane->distance_to(pivot_vertex_position);
		if (Math::is_zero_approx(pivot_distance)) {
			continue; // Skip coplanar faces.
		}
		const int pivot_sign = pivot_distance > 0.0 ? 1 : -1;
		bool cont = false;
		PackedInt32Array face = combination;
		for (int64_t i = 0; i < p_poly_cell_indices_without_pivot.size(); i++) {
			// Skip any indices that are already in the combination.
			if (combination.has(p_poly_cell_indices_without_pivot[i])) {
				continue;
			}
			const VectorN vertex = p_vertex_positions[p_poly_cell_indices_without_pivot[i]];
			const double vertex_distance = plane->distance_to(vertex);
			// Any vertex that is coplanar with this simplex is part of a larger face.
			if (Math::is_zero_approx(vertex_distance)) {
				face.append(p_poly_cell_indices_without_pivot[i]);
				continue;
			}
			const int vertex_sign = vertex_distance > 0.0 ? 1 : -1;
			// If we find a vertex on the opposite side of the pivot, this face cuts through the cell.
			// We are only interested in faces on the surface of the cell, not the interior.
			if (pivot_sign != vertex_sign) {
				cont = true;
				break;
			}
		}
		if (cont) {
			continue;
		}
		face.sort();
		if (!opposing_faces.has(face)) {
			opposing_faces.append(face);
			r_out_normals.append(plane->get_normal());
		}
	}
	return opposing_faces;
}

// Nearest point and signed distance.

void CellMeshND::populate_inverse_metric_cache() {
	ERR_FAIL_COND_MSG(!is_mesh_data_valid(), "CellMeshND: Cannot populate closest-point cache for an invalid mesh.");
	const int64_t dimension = get_dimension();
	ERR_FAIL_COND_MSG(dimension < 2, "CellMeshND: Cannot populate closest-point cache for a mesh with less than 2 dimensions.");
	ERR_FAIL_COND_MSG(get_indices_per_simplex_cell() != dimension, "CellMeshND: Cannot populate closest-point cache for a mesh whose simplex cells are not (N-1)-simplexes with N vertex indices.");
	const PackedInt32Array &simplex_cell_vertex_indices = get_simplex_cell_vertex_indices();
	const int64_t simplex_count = simplex_cell_vertex_indices.size() / dimension;
	const int64_t edge_count = dimension - 1;
	const int64_t metric_size = edge_count * (edge_count + 1) / 2;
	if (_nearest_simplex_inverse_metric_cache.size() == simplex_count * metric_size) {
		return;
	}
	_nearest_simplex_inverse_metric_cache.resize(simplex_count * metric_size);
	const Vector<VectorN> &vertex_positions = get_vertex_positions();
	Vector<VectorN> edges;
	edges.resize(edge_count);
	VectorN metric;
	metric.resize(metric_size);
	for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
		// These indices are guaranteed to be within bounds due to mesh validation.
		const VectorN vert0 = VectorND::with_dimension(vertex_positions[simplex_cell_vertex_indices[simplex_index * dimension]], dimension);
		for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
			const VectorN vert = VectorND::with_dimension(vertex_positions[simplex_cell_vertex_indices[simplex_index * dimension + edge_index + 1]], dimension);
			edges.set(edge_index, VectorND::subtract(vert, vert0));
		}
		// Pack the Gram metric matrix G_ij = e_i · e_j in row-major upper-triangular order.
		int64_t packed_index = 0;
		for (int64_t i = 0; i < edge_count; i++) {
			for (int64_t j = i; j < edge_count; j++) {
				metric.set(packed_index++, VectorND::dot(edges[i], edges[j]));
			}
		}
		VectorN inv_metric;
		const bool valid = GeometryND::compute_inverse_metric(metric, inv_metric);
		if (!valid) {
			_nearest_simplex_inverse_metric_cache.clear();
			ERR_PRINT("CellMeshND: Closest-point cache build failed because simplex cell " + itos(simplex_index) + " is degenerate or non-finite.");
			return;
		}
		for (int64_t metric_index = 0; metric_index < metric_size; metric_index++) {
			_nearest_simplex_inverse_metric_cache.set(simplex_index * metric_size + metric_index, inv_metric[metric_index]);
		}
	}
}

double CellMeshND::get_signed_distance_to_mesh(const VectorN &p_local_point, VectorN *r_nearest_point_on_cell, int *r_simplex_cell_index) {
	ERR_FAIL_COND_V_MSG(!is_mesh_data_valid(), Math_INF, "CellMeshND: Cannot get signed distance to an invalid mesh.");
	const int64_t dimension = get_dimension();
	ERR_FAIL_COND_V_MSG(dimension < 2, Math_INF, "CellMeshND: Cannot get signed distance to a mesh with less than 2 dimensions.");
	ERR_FAIL_COND_V_MSG(get_indices_per_simplex_cell() != dimension, Math_INF, "CellMeshND: Cannot get signed distance to a mesh whose simplex cells are not (N-1)-simplexes with N vertex indices.");
	const PackedInt32Array &simplex_cell_vertex_indices = get_simplex_cell_vertex_indices();
	const int64_t simplex_count = simplex_cell_vertex_indices.size() / dimension;
	const int64_t metric_size = (dimension - 1) * dimension / 2;
	if (_nearest_simplex_inverse_metric_cache.size() != simplex_count * metric_size) {
		populate_inverse_metric_cache();
	}
	ERR_FAIL_COND_V_MSG(_nearest_simplex_inverse_metric_cache.size() != simplex_count * metric_size, Math_INF, "CellMeshND: Closest-point cache is invalid for this mesh.");
	ERR_FAIL_COND_V_MSG(simplex_count == 0, Math_INF, "CellMeshND: Cannot get signed distance to a mesh with zero simplex cells.");
	const Vector<VectorN> &vertex_positions = get_vertex_positions();
	const VectorN local_point = VectorND::with_dimension(p_local_point, dimension);
	// Iterate over all simplex cells to find the nearest point on the mesh, keeping track of the best one.
	// Unlike the fixed-size candidate buffer in the 4D module, this uses growable arrays, because the
	// amount of simplex cells tied at a shared border grows with the dimension.
	Vector<VectorN> best_candidate_points_on_cell;
	PackedInt32Array best_candidate_cells;
	VectorN best_point_on_cell;
	double best_distance_sq = Math_INF;
	int best_simplex_index = -1;
	bool best_proj_inside = false;
	Vector<VectorN> simplex_vertex_positions;
	simplex_vertex_positions.resize(dimension);
	// Future: This part could be accelerated with spatial partitioning, and/or accelerated with threading.
	// But those optimizations add a lot of complexity and would only benefit larger meshes.
	for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
		VectorN nearest_on_cell;
		double min_distance_sq = 0.0;
		bool proj_inside = false;
		for (int64_t vertex_num = 0; vertex_num < dimension; vertex_num++) {
			simplex_vertex_positions.set(vertex_num, VectorND::with_dimension(vertex_positions[simplex_cell_vertex_indices[simplex_index * dimension + vertex_num]], dimension));
		}
		GeometryND::get_nearest_point_on_simplex_barycentric(simplex_vertex_positions, local_point, _nearest_simplex_inverse_metric_cache, simplex_index, nearest_on_cell, min_distance_sq, proj_inside);
		const bool less_dist = min_distance_sq < best_distance_sq;
		// If the projection is outside the cell, but the projected point is the same distance as what we
		// already found, then we may have multiple candidates for the closest cell to this point.
		// In this case, we need to collect them all for later disambiguation using the boundary normal.
		if (!proj_inside) {
			if (!best_proj_inside && Math::is_equal_approx(min_distance_sq, best_distance_sq)) {
				best_candidate_points_on_cell.append(nearest_on_cell);
				best_candidate_cells.append(simplex_index);
			} else if (less_dist) {
				best_candidate_points_on_cell.clear();
				best_candidate_cells.clear();
				best_candidate_points_on_cell.append(nearest_on_cell);
				best_candidate_cells.append(simplex_index);
			}
		}
		// If the projection is closer than what we have already found, then this is the new best point.
		// Update the best point and distance regardless of the projection being inside or outside.
		if (less_dist) {
			best_point_on_cell = nearest_on_cell;
			best_distance_sq = min_distance_sq;
			best_simplex_index = simplex_index;
			best_proj_inside = proj_inside;
			if (proj_inside) {
				// If the projection is inside, then this is the single unambiguous nearest point so far.
				best_candidate_points_on_cell.clear();
				best_candidate_cells.clear();
			}
		}
	}
	const Vector<VectorN> &boundary_normals = get_simplex_cell_boundary_normals();
	ERR_FAIL_COND_V_MSG(boundary_normals.size() != simplex_count, Math_INF, "CellMeshND: Cannot get signed distance to a mesh without boundary normals for all simplex cells.");
	if (best_candidate_cells.size() > 1) {
		// We have multiple candidates with the same distance, so we need to disambiguate using
		// the absolute angle to the boundary normal (these are normalized, so use the dot product).
		double best_dot_abs = -1.0;
		for (int64_t candidate_num = 0; candidate_num < best_candidate_cells.size(); candidate_num++) {
			const int32_t candidate_cell = best_candidate_cells[candidate_num];
			const VectorN candidate_point_on_cell = best_candidate_points_on_cell[candidate_num];
			const VectorN cell_point_dir_to_target = VectorND::normalized(VectorND::subtract(local_point, candidate_point_on_cell));
			const double dot_abs = Math::abs(VectorND::dot(cell_point_dir_to_target, boundary_normals[candidate_cell]));
			if (dot_abs > best_dot_abs) {
				best_dot_abs = dot_abs;
				best_simplex_index = candidate_cell;
				best_point_on_cell = candidate_point_on_cell;
			}
		}
	}
	// Write the outputs depending on what the caller requested.
	if (r_nearest_point_on_cell) {
		*r_nearest_point_on_cell = best_point_on_cell;
	}
	if (r_simplex_cell_index) {
		*r_simplex_cell_index = best_simplex_index;
	}
	if (unlikely(best_simplex_index < 0)) {
		// This should be impossible because we check for zero simplex cells above, but just in case.
		return Math_INF;
	}
	// If we found a nearest point with a nearest cell, check its boundary normal to determine the sign of the distance.
	const VectorN best_normal = boundary_normals[best_simplex_index];
	const double side = VectorND::dot(VectorND::subtract(local_point, best_point_on_cell), best_normal);
	double signed_distance = Math::sqrt(best_distance_sq);
	if (side < 0.0) {
		signed_distance = -signed_distance;
	}
	return signed_distance;
}

double CellMeshND::get_signed_distance_to_mesh_bind(const VectorN &p_local_point) {
	return get_signed_distance_to_mesh(p_local_point, nullptr, nullptr);
}

// Raycast.

bool CellMeshND::raycast_intersects_fast(const VectorN &p_local_from, const VectorN &p_local_direction, const double p_max_distance) {
	ERR_FAIL_COND_V_MSG(!is_mesh_data_valid(), false, "CellMeshND: Cannot raycast on an invalid mesh.");
	const int64_t dimension = get_dimension();
	ERR_FAIL_COND_V_MSG(dimension < 2, false, "CellMeshND: Cannot raycast on a mesh with less than 2 dimensions.");
	ERR_FAIL_COND_V_MSG(get_indices_per_simplex_cell() != dimension, false, "CellMeshND: Cannot raycast on a mesh whose simplex cells are not (N-1)-simplexes with N vertex indices.");
	const PackedInt32Array &simplex_cell_vertex_indices = get_simplex_cell_vertex_indices();
	const int64_t simplex_count = simplex_cell_vertex_indices.size() / dimension;
	if (simplex_count == 0) {
		return false; // No simplex cells to raycast against.
	}
	const int64_t metric_size = (dimension - 1) * dimension / 2;
	const PackedFloat64Array &inverse_metric_cache = _nearest_simplex_inverse_metric_cache;
	ERR_FAIL_COND_V_MSG(inverse_metric_cache.size() != simplex_count * metric_size, false, "CellMeshND: Closest-point cache is invalid for this mesh. Call `populate_inverse_metric_cache()` before calling `raycast_intersects_fast()`.");
	const Vector<VectorN> &vertex_positions = get_vertex_positions();
	const Vector<VectorN> &boundary_normals = get_simplex_cell_boundary_normals();
	const int64_t boundary_normals_count = boundary_normals.size();
	const VectorN local_from = VectorND::with_dimension(p_local_from, dimension);
	const VectorN local_direction = VectorND::with_dimension(p_local_direction, dimension);
	Vector<VectorN> simplex_vertex_positions;
	simplex_vertex_positions.resize(dimension);
	// Iterate through all simplex cells to find any ray intersection.
	for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
		// These indices are guaranteed to be within bounds due to mesh validation.
		for (int64_t vertex_num = 0; vertex_num < dimension; vertex_num++) {
			simplex_vertex_positions.set(vertex_num, VectorND::with_dimension(vertex_positions[simplex_cell_vertex_indices[simplex_index * dimension + vertex_num]], dimension));
		}
		VectorN normal;
		if (simplex_count == boundary_normals_count) {
			normal = boundary_normals[simplex_index];
		} else {
			// Fallback in case the boundary normals are not available or are mismatched.
			Vector<VectorN> edges;
			edges.resize(dimension - 1);
			for (int64_t edge_index = 0; edge_index < dimension - 1; edge_index++) {
				edges.set(edge_index, VectorND::subtract(simplex_vertex_positions[edge_index + 1], simplex_vertex_positions[0]));
			}
			normal = VectorND::normalized(VectorND::perpendicular(edges));
		}
		// Intersect the ray with the hyperplane of this simplex cell.
		const double denominator = VectorND::dot(normal, local_direction);
		if (Math::is_zero_approx(denominator)) {
			continue; // The ray is parallel to the hyperplane of this simplex cell.
		}
		const double plane_intersection_factor = (VectorND::dot(normal, simplex_vertex_positions[0]) - VectorND::dot(normal, local_from)) / denominator;
		if (plane_intersection_factor < 0.0) {
			continue; // No intersection with the hyperplane of this simplex cell.
		}
		if (plane_intersection_factor >= p_max_distance) {
			continue; // Intersection is beyond the maximum distance.
		}
		// Check if this candidate intersection is inside the simplex cell.
		const VectorN intersection_point = VectorND::add(local_from, VectorND::multiply_scalar(local_direction, plane_intersection_factor));
		const bool hit = GeometryND::is_point_inside_simplex_barycentric(simplex_vertex_positions, intersection_point, inverse_metric_cache, simplex_index);
		if (hit) {
			// For this fast version, we only care if there is any intersection, so we can return true immediately.
			return true;
		}
	}
	return false;
}

Dictionary CellMeshND::raycast_intersects(const VectorN &p_local_from, const VectorN &p_local_direction, const double p_max_distance) {
	Dictionary result;
	result["hit"] = false;
	ERR_FAIL_COND_V_MSG(!is_mesh_data_valid(), result, "CellMeshND: Cannot raycast on an invalid mesh.");
	const int64_t dimension = get_dimension();
	ERR_FAIL_COND_V_MSG(dimension < 2, result, "CellMeshND: Cannot raycast on a mesh with less than 2 dimensions.");
	ERR_FAIL_COND_V_MSG(get_indices_per_simplex_cell() != dimension, result, "CellMeshND: Cannot raycast on a mesh whose simplex cells are not (N-1)-simplexes with N vertex indices.");
	const PackedInt32Array &simplex_cell_vertex_indices = get_simplex_cell_vertex_indices();
	const int64_t simplex_count = simplex_cell_vertex_indices.size() / dimension;
	if (simplex_count == 0) {
		return result; // No simplex cells to raycast against.
	}
	const int64_t metric_size = (dimension - 1) * dimension / 2;
	const PackedFloat64Array &inverse_metric_cache = _nearest_simplex_inverse_metric_cache;
	ERR_FAIL_COND_V_MSG(inverse_metric_cache.size() != simplex_count * metric_size, result, "CellMeshND: Closest-point cache is invalid for this mesh. Call `populate_inverse_metric_cache()` before calling `raycast_intersects()`.");
	const Vector<VectorN> &vertex_positions = get_vertex_positions();
	const Vector<VectorN> &boundary_normals = get_simplex_cell_boundary_normals();
	const int64_t boundary_normals_count = boundary_normals.size();
	const VectorN local_from = VectorND::with_dimension(p_local_from, dimension);
	const VectorN local_direction = VectorND::with_dimension(p_local_direction, dimension);
	Vector<VectorN> simplex_vertex_positions;
	simplex_vertex_positions.resize(dimension);
	VectorN best_hit_normal;
	double best_distance = p_max_distance;
	int32_t best_simplex_cell_index = -1;
	// Iterate through all simplex cells to find the closest ray intersection.
	for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
		// These indices are guaranteed to be within bounds due to mesh validation.
		for (int64_t vertex_num = 0; vertex_num < dimension; vertex_num++) {
			simplex_vertex_positions.set(vertex_num, VectorND::with_dimension(vertex_positions[simplex_cell_vertex_indices[simplex_index * dimension + vertex_num]], dimension));
		}
		VectorN normal;
		if (simplex_count == boundary_normals_count) {
			normal = boundary_normals[simplex_index];
		} else {
			// Fallback in case the boundary normals are not available or are mismatched.
			Vector<VectorN> edges;
			edges.resize(dimension - 1);
			for (int64_t edge_index = 0; edge_index < dimension - 1; edge_index++) {
				edges.set(edge_index, VectorND::subtract(simplex_vertex_positions[edge_index + 1], simplex_vertex_positions[0]));
			}
			normal = VectorND::normalized(VectorND::perpendicular(edges));
		}
		// Intersect the ray with the hyperplane of this simplex cell.
		const double denominator = VectorND::dot(normal, local_direction);
		if (Math::is_zero_approx(denominator)) {
			continue; // The ray is parallel to the hyperplane of this simplex cell.
		}
		const double plane_intersection_factor = (VectorND::dot(normal, simplex_vertex_positions[0]) - VectorND::dot(normal, local_from)) / denominator;
		if (plane_intersection_factor < 0.0) {
			continue; // No intersection with the hyperplane of this simplex cell.
		}
		if (plane_intersection_factor > best_distance) {
			continue; // Worse than the best intersection found so far.
		}
		// Check if this candidate intersection is inside the simplex cell.
		const VectorN intersection_point = VectorND::add(local_from, VectorND::multiply_scalar(local_direction, plane_intersection_factor));
		const bool hit = GeometryND::is_point_inside_simplex_barycentric(simplex_vertex_positions, intersection_point, inverse_metric_cache, simplex_index);
		if (hit) {
			best_hit_normal = normal;
			best_distance = plane_intersection_factor;
			best_simplex_cell_index = simplex_index;
		}
	}
	result["hit"] = best_simplex_cell_index != -1;
	if (best_simplex_cell_index != -1) {
		result["distance"] = best_distance;
		result["normal"] = best_hit_normal;
		result["cell_index"] = best_simplex_cell_index;
	}
	return result;
}

void CellMeshND::cell_mesh_clear_cache() {
	_cell_positions_cache.clear();
	_nearest_simplex_inverse_metric_cache.clear();
	_edge_positions_cache.clear();
	_edge_indices_cache.clear();
	mark_mesh_bounds_and_proxy_mesh_3d_dirty();
}

void CellMeshND::validate_material_for_mesh(const Ref<MaterialND> &p_material) {
	const int dimension = get_dimension();
	const MaterialND::ColorSourceFlagsND albedo_source = p_material->get_albedo_source_flags();
	if (albedo_source & MaterialND::COLOR_SOURCE_FLAG_PER_CELL) {
		const PackedInt32Array cell_indices = get_simplex_cell_vertex_indices();
		PackedColorArray color_array = p_material->get_albedo_color_array();
		const int64_t vertices_per_cell = dimension + 1;
		const int64_t cell_count = cell_indices.size() / vertices_per_cell;
		if (color_array.size() < cell_count) {
			p_material->resize_albedo_color_array(cell_count);
		}
	}
	MeshND::validate_material_for_mesh(p_material);
}

Ref<ArrayCellMeshND> CellMeshND::to_array_cell_mesh() {
	Ref<ArrayCellMeshND> array_mesh;
	array_mesh.instantiate();
	array_mesh->set_vertex_positions(get_vertex_positions());
	array_mesh->set_simplex_cell_vertex_indices(get_simplex_cell_vertex_indices());
	array_mesh->set_cell_boundary_normals(get_simplex_cell_boundary_normals());
	array_mesh->set_simplex_cell_vertex_normals(get_simplex_cell_vertex_normals());
	array_mesh->set_material(get_material());
	return array_mesh;
}

Ref<CellMeshND> CellMeshND::to_cell_mesh() {
	return to_array_cell_mesh();
}

int CellMeshND::get_simplex_cell_count() {
	const int dimension = get_dimension();
	ERR_FAIL_COND_V_MSG(dimension < 1, -1, "CellMeshND: Mesh is empty or 0-dimensional, cannot determine simplex cell count.");
	const PackedInt32Array cell_indices = get_simplex_cell_vertex_indices();
	ERR_FAIL_COND_V_MSG(cell_indices.size() % dimension != 0, -1, "CellMeshND: Cell indices size must be a multiple of the dimension.");
	return cell_indices.size() / dimension;
}

int CellMeshND::get_indices_per_simplex_cell() {
	return get_dimension();
}

PackedInt32Array CellMeshND::get_simplex_cell_vertex_indices() {
	PackedInt32Array indices;
	GDVIRTUAL_CALL(_get_simplex_cell_vertex_indices, indices);
	return indices;
}

Vector<VectorN> CellMeshND::get_simplex_cell_positions() {
	if (_cell_positions_cache.is_empty()) {
		const PackedInt32Array cell_indices = get_simplex_cell_vertex_indices();
		const Vector<VectorN> vertex_positions = get_vertex_positions();
		const int32_t vertices_count = vertex_positions.size();
		for (const int cell_index : cell_indices) {
			ERR_FAIL_INDEX_V(cell_index, vertices_count, _cell_positions_cache);
			_cell_positions_cache.append(vertex_positions[cell_index]);
		}
	}
	return _cell_positions_cache;
}

Vector<VectorN> CellMeshND::get_simplex_cell_boundary_normals() {
	TypedArray<VectorN> boundary_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_boundary_normals, boundary_normals_bind);
	Vector<VectorN> boundary_normals;
	boundary_normals.resize(boundary_normals_bind.size());
	for (int i = 0; i < boundary_normals_bind.size(); i++) {
		const VectorN cell_face_normal = boundary_normals_bind[i];
		boundary_normals.set(i, cell_face_normal);
	}
	return boundary_normals;
}

Vector<VectorN> CellMeshND::get_simplex_cell_vertex_normals() {
	TypedArray<VectorN> vertex_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_vertex_normals, vertex_normals_bind);
	Vector<VectorN> vertex_normals;
	vertex_normals.resize(vertex_normals_bind.size());
	for (int i = 0; i < vertex_normals_bind.size(); i++) {
		const VectorN cell_vertex_normal = vertex_normals_bind[i];
		vertex_normals.set(i, cell_vertex_normal);
	}
	return vertex_normals;
}

Vector<VectorM> CellMeshND::get_simplex_cell_texture_map() {
	TypedArray<VectorM> texture_map_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_texture_map, texture_map_bind);
	Vector<VectorM> texture_map;
	texture_map.resize(texture_map_bind.size());
	for (int i = 0; i < texture_map_bind.size(); i++) {
		const VectorM cell_vertex_texture = texture_map_bind[i];
		texture_map.set(i, cell_vertex_texture);
	}
	return texture_map;
}

TypedArray<VectorN> CellMeshND::get_simplex_cell_boundary_normals_bind() {
	TypedArray<VectorN> boundary_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_boundary_normals, boundary_normals_bind);
	if (!boundary_normals_bind.is_empty()) {
		return boundary_normals_bind;
	}
	const Vector<VectorN> boundary_normals = get_simplex_cell_boundary_normals();
	boundary_normals_bind.resize(boundary_normals.size());
	for (int i = 0; i < boundary_normals.size(); i++) {
		const VectorN &cell_face_normal = boundary_normals[i];
		boundary_normals_bind[i] = cell_face_normal;
	}
	return boundary_normals_bind;
}

TypedArray<VectorN> CellMeshND::get_simplex_cell_vertex_normals_bind() {
	TypedArray<VectorN> vertex_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_vertex_normals, vertex_normals_bind);
	if (!vertex_normals_bind.is_empty()) {
		return vertex_normals_bind;
	}
	const Vector<VectorN> vertex_normals = get_simplex_cell_vertex_normals();
	vertex_normals_bind.resize(vertex_normals.size());
	for (int i = 0; i < vertex_normals.size(); i++) {
		const VectorN &cell_vertex_normal = vertex_normals[i];
		vertex_normals_bind[i] = cell_vertex_normal;
	}
	return vertex_normals_bind;
}

TypedArray<VectorM> CellMeshND::get_simplex_cell_texture_map_bind() {
	TypedArray<VectorM> texture_map_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_texture_map, texture_map_bind);
	if (!texture_map_bind.is_empty()) {
		return texture_map_bind;
	}
	const Vector<VectorM> texture_map = get_simplex_cell_texture_map();
	texture_map_bind.resize(texture_map.size());
	for (int i = 0; i < texture_map.size(); i++) {
		const VectorM &cell_vertex_texture = texture_map[i];
		texture_map_bind[i] = cell_vertex_texture;
	}
	return texture_map_bind;
}

TypedArray<VectorN> CellMeshND::get_simplex_cell_positions_bind() {
	TypedArray<VectorN> cell_positions_bind;
	const Vector<VectorN> cell_positions = get_simplex_cell_positions();
	cell_positions_bind.resize(cell_positions.size());
	for (int i = 0; i < cell_positions.size(); i++) {
		const VectorN &cell_position = cell_positions[i];
		cell_positions_bind[i] = cell_position;
	}
	return cell_positions_bind;
}

// Recursive function that decomposes a polytope cell into simplexes.
// Each simplex will be a set of p_dimension indices in the returned array,
// therefore the returned array will be a multiple of p_dimension.
// The algorithm is as follows:
// 1. Pick a pivot index from the cell indices (must be different from the last pivot).
// 2. Find unique opposing faces of the cell that are not coplanar with the pivot.
// 3. For each face, recursively call this function with the face as the new cell.
// 4. Each of the returned simplexes will have the pivot index prepended to it.
// This function has atrocious time complexity, so avoid using it on large cells or at runtime.
Vector<PackedInt32Array> CellMeshND::decompose_polytope_cell_into_simplexes(const Vector<VectorN> &p_vertex_positions, const PackedInt32Array &p_poly_cell_indices, const int p_dimension, const int p_last_pivot, const Vector<VectorN> &p_poly_cell_normals) {
	Vector<PackedInt32Array> simplexes;
	if (p_poly_cell_indices.size() < 2) {
		return simplexes; // No simplexes can be formed.
	}
	PackedInt32Array cell_ind_without_pivot = p_poly_cell_indices;
	int pivot_item;
	if (p_poly_cell_indices[0] == p_last_pivot) {
		pivot_item = p_poly_cell_indices[1];
		cell_ind_without_pivot.remove_at(1);
	} else {
		pivot_item = p_poly_cell_indices[0];
		cell_ind_without_pivot.remove_at(0);
	}
	Vector<VectorN> out_normals;
	Vector<PackedInt32Array> opposing_faces = _determine_opposing_faces(p_vertex_positions, cell_ind_without_pivot, p_dimension, pivot_item, p_poly_cell_normals, out_normals);
	for (int i = 0; i < opposing_faces.size(); i++) {
		PackedInt32Array face = opposing_faces[i];
		if (face.size() == p_dimension) {
			// This is a flat simplex of the next lowest dimension, just add the pivot.
			face.insert(0, pivot_item);
			simplexes.append(face);
			continue;
		}
		// This face is not a simplex, so we need to recurse.
		Vector<VectorN> boundary_normals = p_poly_cell_normals;
		boundary_normals.append(out_normals[i]);
		Vector<PackedInt32Array> lower_simplexes = decompose_polytope_cell_into_simplexes(p_vertex_positions, face, p_dimension - 1, pivot_item, boundary_normals);
		for (PackedInt32Array &lower_simplex : lower_simplexes) {
			lower_simplex.insert(0, pivot_item);
			simplexes.append(lower_simplex);
		}
	}
	return simplexes;
}

PackedInt32Array CellMeshND::calculate_edge_indices_from_simplex_cell_vertex_indices(const PackedInt32Array &p_simplex_cell_vertex_indices, const int p_dimension, const bool p_deduplicate) {
	PackedInt32Array edge_indices;
	ERR_FAIL_COND_V_MSG(p_dimension < 1, edge_indices, "CellMeshND: Dimension must be greater than 0.");
	ERR_FAIL_COND_V_MSG(p_simplex_cell_vertex_indices.size() % p_dimension != 0, edge_indices, "CellMeshND: Simplex cell vertex indices size must be a multiple of the dimension.");
	const int cell_count = p_simplex_cell_vertex_indices.size() / p_dimension;
	// The number of edges is the triangular number of the dimension per cell.
	const int edge_index_count = cell_count * (p_dimension * (p_dimension - 1));
	edge_indices.resize(edge_index_count);
	int edge_index = 0;
	for (int cell_index = 0; cell_index < cell_count; cell_index++) {
		const int cell_start = cell_index * p_dimension;
		for (int i = 0; i < p_dimension; i++) {
			for (int j = i + 1; j < p_dimension; j++) {
				edge_indices.set(edge_index++, p_simplex_cell_vertex_indices[cell_start + i]);
				edge_indices.set(edge_index++, p_simplex_cell_vertex_indices[cell_start + j]);
			}
		}
	}
	CRASH_COND(edge_index != edge_index_count);
	if (p_deduplicate) {
		edge_indices = deduplicate_edge_indices(edge_indices);
	}
	return edge_indices;
}

PackedInt32Array CellMeshND::get_edge_indices() {
	const int dimension = get_dimension();
	if (_edge_indices_cache.is_empty()) {
		_edge_indices_cache = calculate_edge_indices_from_simplex_cell_vertex_indices(get_simplex_cell_vertex_indices(), dimension, true);
	}
	return _edge_indices_cache;
}

Vector<VectorN> CellMeshND::get_edge_positions() {
	if (_edge_positions_cache.is_empty()) {
		const PackedInt32Array edge_indices = get_edge_indices();
		const Vector<VectorN> vertex_positions = get_vertex_positions();
		const int32_t vertices_count = vertex_positions.size();
		for (const int edge_index : edge_indices) {
			ERR_FAIL_INDEX_V(edge_index, vertices_count, _edge_positions_cache);
			_edge_positions_cache.append(vertex_positions[edge_index]);
		}
	}
	return _edge_positions_cache;
}

void CellMeshND::_bind_methods() {
	// Nearest point and distance.
	ClassDB::bind_method(D_METHOD("populate_inverse_metric_cache"), &CellMeshND::populate_inverse_metric_cache);
	ClassDB::bind_method(D_METHOD("get_signed_distance_to_mesh", "local_point"), &CellMeshND::get_signed_distance_to_mesh_bind);
	// Raycast.
	ClassDB::bind_method(D_METHOD("raycast_intersects_fast", "local_from", "local_direction", "max_distance"), &CellMeshND::raycast_intersects_fast, DEFVAL(BINDING_SAFE_INF));
	ClassDB::bind_method(D_METHOD("raycast_intersects", "local_from", "local_direction", "max_distance"), &CellMeshND::raycast_intersects, DEFVAL(BINDING_SAFE_INF));

	ClassDB::bind_method(D_METHOD("cell_mesh_clear_cache"), &CellMeshND::cell_mesh_clear_cache);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_count"), &CellMeshND::get_simplex_cell_count);
	ClassDB::bind_method(D_METHOD("get_indices_per_simplex_cell"), &CellMeshND::get_indices_per_simplex_cell);
	ClassDB::bind_method(D_METHOD("to_array_cell_mesh"), &CellMeshND::to_array_cell_mesh);

	ClassDB::bind_static_method("CellMeshND", D_METHOD("calculate_edge_indices_from_simplex_cell_vertex_indices", "simplex_cell_vertex_indices", "dimension", "deduplicate"), &CellMeshND::calculate_edge_indices_from_simplex_cell_vertex_indices);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_vertex_indices"), &CellMeshND::get_simplex_cell_vertex_indices);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_boundary_normals"), &CellMeshND::get_simplex_cell_boundary_normals_bind);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_vertex_normals"), &CellMeshND::get_simplex_cell_vertex_normals_bind);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_texture_map"), &CellMeshND::get_simplex_cell_texture_map_bind);

	GDVIRTUAL_BIND(_get_simplex_cell_vertex_indices);
	GDVIRTUAL_BIND(_get_simplex_cell_boundary_normals);
	GDVIRTUAL_BIND(_get_simplex_cell_vertex_normals);
	GDVIRTUAL_BIND(_get_simplex_cell_texture_map);
}
