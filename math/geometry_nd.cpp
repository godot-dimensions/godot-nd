#include "geometry_nd.h"

#include "vector_nd.h"

// Barycentric simplex calculations. Don't expose these, it just needs to be efficient
// and shared between CellMeshND, future ND physics shapes, etc.

int64_t GeometryND::_symmetric_matrix_packed_index(const int64_t p_row, const int64_t p_column, const int64_t p_edge_count) {
	// Index into a symmetric matrix packed in row-major upper-triangular order (m00, m01, ..., m11, m12, ...).
	const int64_t row = MIN(p_row, p_column);
	const int64_t column = MAX(p_row, p_column);
	return row * p_edge_count - (row * (row - 1)) / 2 + (column - row);
}

VectorN GeometryND::_get_nearest_point_on_sub_simplex(const Vector<VectorN> &p_vertices, const VectorN &p_point) {
	// Recursive uncached helper for the facets of a full simplex. This is the ND generalization
	// of closest_point_on_triangle to a sub-simplex with fewer vertices than the space's dimension.
	const int64_t vertex_count = p_vertices.size();
	ERR_FAIL_COND_V(vertex_count == 0, VectorN());
	if (vertex_count == 1) {
		return p_vertices[0];
	}
	if (vertex_count == 2) {
		return closest_point_on_line_segment(p_vertices[0], p_vertices[1], p_point);
	}
	const int64_t edge_count = vertex_count - 1;
	Vector<VectorN> edges;
	edges.resize(edge_count);
	for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
		edges.set(edge_index, VectorND::subtract(p_vertices[edge_index + 1], p_vertices[0]));
	}
	VectorN metric;
	metric.resize(edge_count * (edge_count + 1) / 2);
	for (int64_t i = 0; i < edge_count; i++) {
		for (int64_t j = i; j < edge_count; j++) {
			metric.set(_symmetric_matrix_packed_index(i, j, edge_count), VectorND::dot(edges[i], edges[j]));
		}
	}
	VectorN inv_metric;
	if (compute_inverse_metric(metric, inv_metric)) {
		// Solve for the barycentric coordinates, which implicitly solves local to the plane of the sub-simplex.
		const VectorN local = VectorND::subtract(p_point, p_vertices[0]);
		VectorN edge_alignments;
		edge_alignments.resize(edge_count);
		for (int64_t i = 0; i < edge_count; i++) {
			edge_alignments.set(i, VectorND::dot(edges[i], local));
		}
		VectorN bary_edges;
		bary_edges.resize(edge_count);
		double bary_edge_sum = 0.0;
		bool proj_inside = true;
		for (int64_t i = 0; i < edge_count; i++) {
			double bary = 0.0;
			for (int64_t j = 0; j < edge_count; j++) {
				bary += inv_metric[_symmetric_matrix_packed_index(i, j, edge_count)] * edge_alignments[j];
			}
			bary_edges.set(i, bary);
			bary_edge_sum += bary;
			proj_inside = proj_inside && bary >= -CMP_EPSILON;
		}
		const double bary0 = 1.0 - bary_edge_sum;
		// The point is inside the sub-simplex if all barycentric coordinates are non-negative (allowing for a small epsilon).
		if (proj_inside && bary0 >= -CMP_EPSILON) {
			// In this case, the nearest point on the plane lands inside of the sub-simplex.
			VectorN nearest_on_sub_simplex = VectorND::duplicate(p_vertices[0]);
			for (int64_t i = 0; i < edge_count; i++) {
				VectorND::multiply_scalar_and_add_in_place(edges[i], bary_edges[i], nearest_on_sub_simplex);
			}
			return nearest_on_sub_simplex;
		}
	}
	// In this case, the nearest point on the plane lands outside (or the sub-simplex is degenerate),
	// so we need to check the facet borders.
	VectorN nearest_on_sub_simplex;
	double min_dist_sq = Math_INF;
	for (int64_t facet_index = 0; facet_index < vertex_count; facet_index++) {
		Vector<VectorN> facet_vertices;
		for (int64_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
			if (vertex_index != facet_index) {
				facet_vertices.push_back(p_vertices[vertex_index]);
			}
		}
		const VectorN nearest_on_facet = _get_nearest_point_on_sub_simplex(facet_vertices, p_point);
		const double dist_sq_facet = VectorND::distance_squared_to(nearest_on_facet, p_point);
		if (dist_sq_facet < min_dist_sq) {
			nearest_on_sub_simplex = nearest_on_facet;
			min_dist_sq = dist_sq_facet;
		}
	}
	return nearest_on_sub_simplex;
}

bool GeometryND::compute_inverse_metric(const VectorN &p_symmetric_metric, VectorN &r_inv_symmetric) {
	// This is the (N-1)x(N-1) symmetric metric matrix G_ij = e_i · e_j for the simplex basis,
	// packed in row-major upper-triangular order (g00, g01, ..., g11, g12, ...).
	// Normalize each basis vector before checking the determinant so that valid small simplices
	// are not mistaken for degenerate ones. The normalized metric is a correlation matrix.
	const int64_t packed_size = p_symmetric_metric.size();
	int64_t edge_count = 0;
	int64_t triangular_size = 0;
	while (triangular_size < packed_size) {
		edge_count++;
		triangular_size += edge_count;
	}
	ERR_FAIL_COND_V_MSG(triangular_size != packed_size, false, "GeometryND::compute_inverse_metric: The metric must be a symmetric matrix packed in row-major upper-triangular order, so its size must be a triangular number.");
	r_inv_symmetric.resize(packed_size);
	if (unlikely(edge_count == 0)) {
		// A full simplex in 0D or 1D space has no edges, so the metric is trivially empty.
		return true;
	}
	VectorN inv_lengths;
	inv_lengths.resize(edge_count);
	for (int64_t i = 0; i < edge_count; i++) {
		const double diagonal = p_symmetric_metric[_symmetric_matrix_packed_index(i, i, edge_count)];
		if (unlikely(!Math::is_finite(diagonal) || diagonal <= 0.0)) {
			return false;
		}
		inv_lengths.set(i, 1.0 / Math::sqrt(diagonal));
	}
	VectorN normalized;
	normalized.resize(packed_size);
	for (int64_t i = 0; i < edge_count; i++) {
		for (int64_t j = i; j < edge_count; j++) {
			const int64_t packed_index = _symmetric_matrix_packed_index(i, j, edge_count);
			normalized.set(packed_index, p_symmetric_metric[packed_index] * inv_lengths[i] * inv_lengths[j]);
		}
	}
	// Invert the correlation matrix with a Cholesky decomposition, the ND generalization of the
	// 3x3 cofactor expansion used by Geometry4D, exploiting that the correlation matrix of any
	// non-degenerate simplex basis is symmetric positive-definite. The correlation matrix has ones
	// on the diagonal, so its determinant is at most the value of any individual Cholesky pivot,
	// meaning a per-pivot epsilon check rejects only matrices the determinant check would reject.
	VectorN cholesky_lower;
	cholesky_lower.resize(edge_count * edge_count);
	double det = 1.0;
	for (int64_t j = 0; j < edge_count; j++) {
		double pivot = normalized[_symmetric_matrix_packed_index(j, j, edge_count)];
		for (int64_t s = 0; s < j; s++) {
			pivot -= cholesky_lower[j * edge_count + s] * cholesky_lower[j * edge_count + s];
		}
		if (unlikely(!Math::is_finite(pivot) || pivot <= CMP_EPSILON)) {
			return false;
		}
		det *= pivot;
		const double diagonal = Math::sqrt(pivot);
		cholesky_lower.set(j * edge_count + j, diagonal);
		const double inv_diagonal = 1.0 / diagonal;
		for (int64_t i = j + 1; i < edge_count; i++) {
			double off_diagonal = normalized[_symmetric_matrix_packed_index(i, j, edge_count)];
			for (int64_t s = 0; s < j; s++) {
				off_diagonal -= cholesky_lower[i * edge_count + s] * cholesky_lower[j * edge_count + s];
			}
			cholesky_lower.set(i * edge_count + j, off_diagonal * inv_diagonal);
		}
	}
	if (unlikely(!Math::is_finite(det) || det <= CMP_EPSILON)) {
		return false;
	}
	// Invert the lower triangular Cholesky factor by forward substitution.
	VectorN inv_cholesky;
	inv_cholesky.resize(edge_count * edge_count);
	for (int64_t j = 0; j < edge_count; j++) {
		inv_cholesky.set(j * edge_count + j, 1.0 / cholesky_lower[j * edge_count + j]);
		for (int64_t i = j + 1; i < edge_count; i++) {
			double sum = 0.0;
			for (int64_t s = j; s < i; s++) {
				sum += cholesky_lower[i * edge_count + s] * inv_cholesky[s * edge_count + j];
			}
			inv_cholesky.set(i * edge_count + j, -sum / cholesky_lower[i * edge_count + i]);
		}
	}
	// The inverse correlation matrix is (L^-1)^T * (L^-1), then un-normalize back to the original basis scale.
	for (int64_t i = 0; i < edge_count; i++) {
		for (int64_t j = i; j < edge_count; j++) {
			double sum = 0.0;
			for (int64_t s = j; s < edge_count; s++) {
				sum += inv_cholesky[s * edge_count + i] * inv_cholesky[s * edge_count + j];
			}
			r_inv_symmetric.set(_symmetric_matrix_packed_index(i, j, edge_count), sum * inv_lengths[i] * inv_lengths[j]);
		}
	}
	for (int64_t i = 0; i < packed_size; i++) {
		if (unlikely(!Math::is_finite(r_inv_symmetric[i]))) {
			return false;
		}
	}
	return true;
}

void GeometryND::get_nearest_point_on_simplex_barycentric(const Vector<VectorN> &p_vertices, const VectorN &p_point, const PackedFloat64Array &p_nearest_simplex_inverse_metric_cache, const int64_t p_simplex_index, VectorN &r_nearest_on_simplex, double &r_distance_squared, bool &r_proj_inside) {
	const int64_t dimension = p_vertices.size();
	for (int64_t vertex_index = 0; vertex_index < dimension; vertex_index++) {
		ERR_FAIL_COND_MSG(p_vertices[vertex_index].size() != dimension, "GeometryND::get_nearest_point_on_simplex_barycentric: The simplex must be a full (N-1)-simplex in N-dimensional space, with N vertices, each with N components.");
	}
	if (unlikely(dimension < 2)) {
		// The 0D and 1D cases are degenerate: the simplex is empty or a single point.
		r_proj_inside = true;
		r_nearest_on_simplex = dimension == 0 ? VectorN() : p_vertices[0];
		r_distance_squared = dimension == 0 ? 0.0 : VectorND::distance_squared_to(p_vertices[0], p_point);
		return;
	}
	const int64_t edge_count = dimension - 1;
	const int64_t metric_size = edge_count * (edge_count + 1) / 2;
	ERR_FAIL_COND_MSG(p_simplex_index < 0 || p_nearest_simplex_inverse_metric_cache.size() < p_simplex_index * metric_size + metric_size, "GeometryND::get_nearest_point_on_simplex_barycentric: Inverse metric cache is too small for the given simplex index.");
	Vector<VectorN> edges;
	edges.resize(edge_count);
	for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
		edges.set(edge_index, VectorND::subtract(p_vertices[edge_index + 1], p_vertices[0]));
	}
	// Solve for the barycentric coordinates, which implicitly solves local to the plane of the simplex.
	VectorN bary_edges;
	bary_edges.resize(edge_count);
	double bary0;
	{
		const int64_t cache_offset = p_simplex_index * metric_size;
		const VectorN local = VectorND::subtract(p_point, p_vertices[0]);
		VectorN edge_alignments;
		edge_alignments.resize(edge_count);
		for (int64_t i = 0; i < edge_count; i++) {
			edge_alignments.set(i, VectorND::dot(edges[i], local));
		}
		double bary_edge_sum = 0.0;
		for (int64_t i = 0; i < edge_count; i++) {
			double bary = 0.0;
			for (int64_t j = 0; j < edge_count; j++) {
				bary += p_nearest_simplex_inverse_metric_cache[cache_offset + _symmetric_matrix_packed_index(i, j, edge_count)] * edge_alignments[j];
			}
			bary_edges.set(i, bary);
			bary_edge_sum += bary;
		}
		bary0 = 1.0 - bary_edge_sum;
	}
	// The point is inside the simplex if all barycentric coordinates are non-negative (allowing for a small epsilon).
	bool proj_inside = bary0 >= -CMP_EPSILON;
	for (int64_t i = 0; i < edge_count; i++) {
		proj_inside = proj_inside && bary_edges[i] >= -CMP_EPSILON;
	}
	// Determine the nearest point and/or the min distance based on if it's inside or outside the simplex.
	VectorN nearest_on_simplex;
	double min_dist_sq = Math_INF;
	if (proj_inside) {
		// In this case, the nearest point on the plane lands inside of the simplex.
		nearest_on_simplex = VectorND::duplicate(p_vertices[0]);
		for (int64_t i = 0; i < edge_count; i++) {
			VectorND::multiply_scalar_and_add_in_place(edges[i], bary_edges[i], nearest_on_simplex);
		}
	} else {
		// In this case, the nearest point on the plane lands outside, so we need to check the facet borders.
		for (int64_t facet_index = 0; facet_index < dimension; facet_index++) {
			Vector<VectorN> facet_vertices;
			for (int64_t vertex_index = 0; vertex_index < dimension; vertex_index++) {
				if (vertex_index != facet_index) {
					facet_vertices.push_back(p_vertices[vertex_index]);
				}
			}
			const VectorN nearest_on_facet = _get_nearest_point_on_sub_simplex(facet_vertices, p_point);
			const double dist_sq_facet = VectorND::distance_squared_to(nearest_on_facet, p_point);
			if (dist_sq_facet < min_dist_sq) {
				nearest_on_simplex = nearest_on_facet;
				min_dist_sq = dist_sq_facet;
			}
		}
	}
	// Write the outputs.
	r_proj_inside = proj_inside;
	r_nearest_on_simplex = nearest_on_simplex;
	if (min_dist_sq == Math_INF) {
		min_dist_sq = VectorND::distance_squared_to(nearest_on_simplex, p_point);
	}
	r_distance_squared = min_dist_sq;
}

bool GeometryND::is_point_inside_simplex_barycentric(const Vector<VectorN> &p_vertices, const VectorN &p_point, const PackedFloat64Array &p_nearest_simplex_inverse_metric_cache, const int64_t p_simplex_index) {
	const int64_t dimension = p_vertices.size();
	for (int64_t vertex_index = 0; vertex_index < dimension; vertex_index++) {
		ERR_FAIL_COND_V_MSG(p_vertices[vertex_index].size() != dimension, false, "GeometryND::is_point_inside_simplex_barycentric: The simplex must be a full (N-1)-simplex in N-dimensional space, with N vertices, each with N components.");
	}
	if (unlikely(dimension < 2)) {
		// The 0D and 1D cases are degenerate: the projection onto the simplex's affine hull is always inside.
		return true;
	}
	const int64_t edge_count = dimension - 1;
	const int64_t metric_size = edge_count * (edge_count + 1) / 2;
	ERR_FAIL_COND_V_MSG(p_simplex_index < 0 || p_nearest_simplex_inverse_metric_cache.size() < p_simplex_index * metric_size + metric_size, false, "GeometryND::is_point_inside_simplex_barycentric: Inverse metric cache is too small for the given simplex index.");
	Vector<VectorN> edges;
	edges.resize(edge_count);
	for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
		edges.set(edge_index, VectorND::subtract(p_vertices[edge_index + 1], p_vertices[0]));
	}
	// Solve for the barycentric coordinates, which implicitly solves local to the plane of the simplex.
	VectorN bary_edges;
	bary_edges.resize(edge_count);
	double bary0;
	{
		const int64_t cache_offset = p_simplex_index * metric_size;
		const VectorN local = VectorND::subtract(p_point, p_vertices[0]);
		VectorN edge_alignments;
		edge_alignments.resize(edge_count);
		for (int64_t i = 0; i < edge_count; i++) {
			edge_alignments.set(i, VectorND::dot(edges[i], local));
		}
		double bary_edge_sum = 0.0;
		for (int64_t i = 0; i < edge_count; i++) {
			double bary = 0.0;
			for (int64_t j = 0; j < edge_count; j++) {
				bary += p_nearest_simplex_inverse_metric_cache[cache_offset + _symmetric_matrix_packed_index(i, j, edge_count)] * edge_alignments[j];
			}
			bary_edges.set(i, bary);
			bary_edge_sum += bary;
		}
		bary0 = 1.0 - bary_edge_sum;
	}
	// The point is inside the simplex if all barycentric coordinates are non-negative (allowing for a small epsilon).
	if (bary0 < -CMP_EPSILON) {
		return false;
	}
	for (int64_t i = 0; i < edge_count; i++) {
		if (bary_edges[i] < -CMP_EPSILON) {
			return false;
		}
	}
	return true;
}

VectorN GeometryND::closest_point_on_line(const VectorN &p_line_position, const VectorN &p_line_direction, const VectorN &p_point) {
	const VectorN vector_to_point = VectorND::subtract(p_point, p_line_position);
	return VectorND::add(p_line_position, VectorND::project(vector_to_point, p_line_direction));
}

VectorN GeometryND::closest_point_on_line_segment(const VectorN &p_line_a, const VectorN &p_line_b, const VectorN &p_point) {
	const VectorN line_direction = VectorND::subtract(p_line_b, p_line_a);
	const VectorN vector_to_point = VectorND::subtract(p_point, p_line_a);
	const double projection_factor = VectorND::dot(vector_to_point, line_direction) / VectorND::length_squared(line_direction);
	if (projection_factor < 0.0) {
		return p_line_a;
	} else if (projection_factor > 1.0) {
		return p_line_b;
	}
	return VectorND::add(p_line_a, VectorND::multiply_scalar(line_direction, projection_factor));
}

VectorN GeometryND::closest_point_on_ray(const VectorN &p_ray_origin, const VectorN &p_ray_direction, const VectorN &p_point) {
	const VectorN vector_to_point = VectorND::subtract(p_point, p_ray_origin);
	const double projection_factor = VectorND::dot(vector_to_point, p_ray_direction) / VectorND::length_squared(p_ray_direction);
	if (projection_factor < 0.0) {
		return p_ray_origin;
	}
	return VectorND::add(p_ray_origin, VectorND::multiply_scalar(p_ray_direction, projection_factor));
}

VectorN GeometryND::closest_point_between_lines(const VectorN &p_line1_point, const VectorN &p_line1_dir, const VectorN &p_line2_point, const VectorN &p_line2_dir) {
	const VectorN difference_between_points = VectorND::subtract(p_line1_point, p_line2_point);
	const double line1_len_sq = VectorND::length_squared(p_line1_dir);
	const double line2_len_sq = VectorND::length_squared(p_line2_dir);
	const double line1_projection = VectorND::dot(p_line1_dir, difference_between_points);
	const double line2_projection = VectorND::dot(p_line2_dir, difference_between_points);
	const double direction_dot = VectorND::dot(p_line1_dir, p_line2_dir);
	const double denominator = line1_len_sq * line2_len_sq - direction_dot * direction_dot;
	if (Math::is_zero_approx(denominator)) {
		// Lines are parallel, handling it as a special case.
		return p_line1_point;
	}
	const double line1_factor = (direction_dot * line2_projection - line2_len_sq * line1_projection) / denominator;
	const VectorN closest_point_on_line1 = VectorND::add(p_line1_point, VectorND::multiply_scalar(p_line1_dir, line1_factor));
	return closest_point_on_line1;
}

VectorN GeometryND::closest_point_between_line_segments(const VectorN &p_line1_a, const VectorN &p_line1_b, const VectorN &p_line2_a, const VectorN &p_line2_b) {
	const VectorN difference_between_points = VectorND::subtract(p_line1_a, p_line2_a);
	const VectorN line1_dir = VectorND::subtract(p_line1_b, p_line1_a);
	const VectorN line2_dir = VectorND::subtract(p_line2_b, p_line2_a);
	const double line1_len_sq = VectorND::length_squared(line1_dir);
	const double line2_len_sq = VectorND::length_squared(line2_dir);
	const double line1_projection = VectorND::dot(line1_dir, difference_between_points);
	const double line2_projection = VectorND::dot(line2_dir, difference_between_points);
	const double direction_dot = VectorND::dot(line1_dir, line2_dir);
	const double denominator = line1_len_sq * line2_len_sq - direction_dot * direction_dot;
	if (Math::is_zero_approx(denominator)) {
		// Lines are parallel, handling it as a special case.
		return p_line1_a;
	}
	const double line1_factor = (direction_dot * line2_projection - line2_len_sq * line1_projection) / denominator;
	const VectorN closest_point_on_line1 = VectorND::add(p_line1_a, VectorND::multiply_scalar(line1_dir, CLAMP(line1_factor, (double)0.0, (double)1.0)));
	return closest_point_on_line1;
}

Vector<VectorN> GeometryND::closest_points_between_lines(const VectorN &p_line1_point, const VectorN &p_line1_dir, const VectorN &p_line2_point, const VectorN &p_line2_dir) {
	const VectorN difference_between_points = VectorND::subtract(p_line1_point, p_line2_point);
	const double line1_len_sq = VectorND::length_squared(p_line1_dir);
	const double line2_len_sq = VectorND::length_squared(p_line2_dir);
	const double line1_projection = VectorND::dot(p_line1_dir, difference_between_points);
	const double line2_projection = VectorND::dot(p_line2_dir, difference_between_points);
	const double direction_dot = VectorND::dot(p_line1_dir, p_line2_dir);
	const double denominator = line1_len_sq * line2_len_sq - direction_dot * direction_dot;
	if (Math::is_zero_approx(denominator)) {
		// Lines are parallel, handling it as a special case.
		return Vector<VectorN>{ p_line1_point, p_line2_point };
	}
	const double line1_factor = (direction_dot * line2_projection - line2_len_sq * line1_projection) / denominator;
	const double line2_factor = (line1_len_sq * line2_projection - direction_dot * line1_projection) / denominator;
	const VectorN closest_point_on_line1 = VectorND::add(p_line1_point, VectorND::multiply_scalar(p_line1_dir, line1_factor));
	const VectorN closest_point_on_line2 = VectorND::add(p_line2_point, VectorND::multiply_scalar(p_line2_dir, line2_factor));
	return Vector<VectorN>{ closest_point_on_line1, closest_point_on_line2 };
}

Vector<VectorN> GeometryND::closest_points_between_line_segments(const VectorN &p_line1_a, const VectorN &p_line1_b, const VectorN &p_line2_a, const VectorN &p_line2_b) {
	const VectorN difference_between_points = VectorND::subtract(p_line1_a, p_line2_a);
	const VectorN line1_dir = VectorND::subtract(p_line1_b, p_line1_a);
	const VectorN line2_dir = VectorND::subtract(p_line2_b, p_line2_a);
	const double line1_len_sq = VectorND::length_squared(line1_dir);
	const double line2_len_sq = VectorND::length_squared(line2_dir);
	const double line1_projection = VectorND::dot(line1_dir, difference_between_points);
	const double line2_projection = VectorND::dot(line2_dir, difference_between_points);
	const double direction_dot = VectorND::dot(line1_dir, line2_dir);
	const double denominator = line1_len_sq * line2_len_sq - direction_dot * direction_dot;
	if (Math::is_zero_approx(denominator)) {
		// Lines are parallel, handling it as a special case.
		return Vector<VectorN>{ p_line1_a, p_line2_a };
	}
	const double line1_factor = (direction_dot * line2_projection - line2_len_sq * line1_projection) / denominator;
	const double line2_factor = (line1_len_sq * line2_projection - direction_dot * line1_projection) / denominator;
	const VectorN closest_point_on_line1 = VectorND::add(p_line1_a, VectorND::multiply_scalar(line1_dir, CLAMP(line1_factor, (double)0.0, (double)1.0)));
	const VectorN closest_point_on_line2 = VectorND::add(p_line2_a, VectorND::multiply_scalar(line2_dir, CLAMP(line2_factor, (double)0.0, (double)1.0)));
	return Vector<VectorN>{ closest_point_on_line1, closest_point_on_line2 };
}

Vector<VectorN> GeometryND::closest_points_between_line_and_segment(const VectorN &p_line_point, const VectorN &p_line_direction, const VectorN &p_segment_a, const VectorN &p_segment_b) {
	const VectorN difference_between_points = VectorND::subtract(p_line_point, p_segment_a);
	const VectorN segment_dir = VectorND::subtract(p_segment_b, p_segment_a);
	const double segment_len_sq = VectorND::length_squared(segment_dir);
	const double line_projection = VectorND::dot(p_line_direction, difference_between_points);
	const double segment_projection = VectorND::dot(segment_dir, difference_between_points);
	const double direction_dot = VectorND::dot(p_line_direction, segment_dir);
	const double denominator = VectorND::length_squared(p_line_direction) * segment_len_sq - direction_dot * direction_dot;
	if (Math::is_zero_approx(denominator)) {
		// Lines are parallel, handling it as a special case.
		return Vector<VectorN>{ p_line_point, p_segment_a };
	}
	const double line_factor = (direction_dot * segment_projection - segment_len_sq * line_projection) / denominator;
	const double segment_factor = (VectorND::length_squared(p_line_direction) * segment_projection - direction_dot * line_projection) / denominator;
	const VectorN closest_point_on_line = VectorND::add(p_line_point, VectorND::multiply_scalar(p_line_direction, line_factor));
	const VectorN closest_point_on_segment = VectorND::add(p_segment_a, VectorND::multiply_scalar(segment_dir, CLAMP(segment_factor, (double)0.0, (double)1.0)));
	return Vector<VectorN>{ closest_point_on_line, closest_point_on_segment };
}

GeometryND *GeometryND::singleton = nullptr;

void GeometryND::_bind_methods() {
	ClassDB::bind_static_method("GeometryND", D_METHOD("closest_point_on_line", "line_position", "line_direction", "point"), &GeometryND::closest_point_on_line);
	ClassDB::bind_static_method("GeometryND", D_METHOD("closest_point_on_line_segment", "line_a", "line_b", "point"), &GeometryND::closest_point_on_line_segment);
	ClassDB::bind_static_method("GeometryND", D_METHOD("closest_point_on_ray", "ray_origin", "ray_direction", "point"), &GeometryND::closest_point_on_ray);
	ClassDB::bind_static_method("GeometryND", D_METHOD("closest_point_between_lines", "line1_point", "line1_dir", "line2_point", "line2_dir"), &GeometryND::closest_point_between_lines);
	ClassDB::bind_static_method("GeometryND", D_METHOD("closest_point_between_line_segments", "line1_a", "line1_b", "line2_a", "line2_b"), &GeometryND::closest_point_between_line_segments);
}
