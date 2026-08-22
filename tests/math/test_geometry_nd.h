#pragma once

#include "../../math/geometry_nd.h"
#include "../../math/vector_nd.h"

#include "tests/test_macros.h"

namespace TestGeometryND {
// Index into a symmetric matrix packed in row-major upper-triangular order (m00, m01, ..., m11, m12, ...).
inline int64_t symmetric_matrix_packed_index(const int64_t p_row, const int64_t p_column, const int64_t p_edge_count) {
	const int64_t row = MIN(p_row, p_column);
	const int64_t column = MAX(p_row, p_column);
	return row * p_edge_count - (row * (row - 1)) / 2 + (column - row);
}

// Computes the packed symmetric metric matrix G_ij = e_i · e_j of the simplex's edge basis,
// matching the packing order expected by GeometryND::compute_inverse_metric.
inline VectorN compute_simplex_metric(const Vector<VectorN> &p_vertices) {
	const int64_t edge_count = p_vertices.size() - 1;
	Vector<VectorN> edges;
	for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
		edges.push_back(VectorND::subtract(p_vertices[edge_index + 1], p_vertices[0]));
	}
	VectorN metric;
	metric.resize(edge_count * (edge_count + 1) / 2);
	for (int64_t i = 0; i < edge_count; i++) {
		for (int64_t j = i; j < edge_count; j++) {
			metric.set(symmetric_matrix_packed_index(i, j, edge_count), VectorND::dot(edges[i], edges[j]));
		}
	}
	return metric;
}

// Computes an inverse metric cache holding just this one simplex at index 0.
inline PackedFloat64Array compute_simplex_inverse_metric_cache(const Vector<VectorN> &p_vertices) {
	VectorN inv_metric;
	const bool valid = GeometryND::compute_inverse_metric(compute_simplex_metric(p_vertices), inv_metric);
	CHECK_MESSAGE(valid, "GeometryND compute_inverse_metric should succeed for a non-degenerate simplex.");
	return inv_metric;
}

TEST_CASE("[GeometryND] Compute Inverse Metric") {
	{
		// Orthogonal basis: the metric is diagonal, so the inverse is the reciprocal of each diagonal.
		VectorN inv_metric;
		const bool valid = GeometryND::compute_inverse_metric(VectorN{ 4, 0, 0, 4, 0, 4 }, inv_metric);
		CHECK_MESSAGE(valid, "GeometryND compute_inverse_metric should succeed for a diagonal metric.");
		CHECK_MESSAGE(VectorND::is_equal_approx(inv_metric, VectorN{ 0.25, 0, 0, 0.25, 0, 0.25 }), "GeometryND compute_inverse_metric of a diagonal metric should be the reciprocal of each diagonal.");
	}
	{
		// Skewed 5D simplex: multiplying the metric by its inverse should give the identity matrix.
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 1, 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 1, 1, 0, 0, 0 });
		vertices.push_back(VectorN{ 0, 1, 1, 0, 0 });
		vertices.push_back(VectorN{ 0, 0, 1, 1, 1 });
		const int64_t edge_count = vertices.size() - 1;
		const VectorN metric = compute_simplex_metric(vertices);
		VectorN inv_metric;
		const bool valid = GeometryND::compute_inverse_metric(metric, inv_metric);
		CHECK_MESSAGE(valid, "GeometryND compute_inverse_metric should succeed for a skewed 5D simplex.");
		for (int64_t i = 0; i < edge_count; i++) {
			for (int64_t j = 0; j < edge_count; j++) {
				double product = 0.0;
				for (int64_t s = 0; s < edge_count; s++) {
					product += metric[symmetric_matrix_packed_index(i, s, edge_count)] * inv_metric[symmetric_matrix_packed_index(s, j, edge_count)];
				}
				CHECK_MESSAGE(Math::is_equal_approx(product, i == j ? 1.0 : 0.0), "GeometryND compute_inverse_metric multiplied by the metric should give the identity matrix.");
			}
		}
	}
	{
		// Tiny simplex: normalization should prevent valid small simplices from being mistaken for degenerate ones.
		VectorN inv_metric;
		const bool valid = GeometryND::compute_inverse_metric(VectorN{ 1e-16, 0, 1e-16 }, inv_metric);
		CHECK_MESSAGE(valid, "GeometryND compute_inverse_metric should succeed for a tiny but non-degenerate metric.");
		CHECK_MESSAGE(VectorND::is_equal_approx(inv_metric, VectorN{ 1e16, 0, 1e16 }), "GeometryND compute_inverse_metric of a tiny diagonal metric should be the reciprocal of each diagonal.");
	}
	{
		// Degenerate simplex: collinear edges have a singular metric.
		VectorN inv_metric;
		const bool valid = GeometryND::compute_inverse_metric(VectorN{ 1, 2, 4 }, inv_metric);
		CHECK_MESSAGE(!valid, "GeometryND compute_inverse_metric should fail for a singular metric.");
	}
	{
		// Degenerate simplex: a zero-length edge has a non-positive diagonal.
		VectorN inv_metric;
		const bool valid = GeometryND::compute_inverse_metric(VectorN{ 0, 0, 4 }, inv_metric);
		CHECK_MESSAGE(!valid, "GeometryND compute_inverse_metric should fail for a metric with a zero diagonal.");
	}
	{
		// The 0D and 1D cases are degenerate: a full simplex in these spaces has no edges, so the metric is empty.
		VectorN inv_metric;
		const bool valid = GeometryND::compute_inverse_metric(VectorN(), inv_metric);
		CHECK_MESSAGE(valid, "GeometryND compute_inverse_metric should trivially succeed for an empty metric.");
		CHECK_MESSAGE(inv_metric.size() == 0, "GeometryND compute_inverse_metric of an empty metric should be empty.");
	}
}

TEST_CASE("[GeometryND] Is Point Inside Simplex Barycentric") {
	{
		// 2D: a line segment. "Inside" means the projection onto the segment's line lands between the endpoints.
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0 });
		vertices.push_back(VectorN{ 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 1, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true for a point on the segment.");
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 1, -5 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true when the projection lands inside the segment.");
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 3, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return false for a point beyond the end of the segment.");
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ -0.1, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return false for a point before the start of the segment.");
	}
	{
		// 3D: a triangle.
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0, 0 });
		vertices.push_back(VectorN{ 2, 0, 0 });
		vertices.push_back(VectorN{ 0, 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 0.5, 0.5, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true for a point inside the triangle.");
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 0.5, 0.5, -3 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true when the projection lands inside the triangle.");
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 2, 2, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return false for a point outside the triangle.");
	}
	{
		// 4D: a tetrahedron. This case matches the tests of the specialized Geometry4D functions.
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 2, 0, 0, 0 });
		vertices.push_back(VectorN{ 0, 2, 0, 0 });
		vertices.push_back(VectorN{ 0, 0, 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 0.5, 0.5, 0.5, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true for a point inside the tetrahedron.");
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 0.5, 0.5, 0.5, 9 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true when the projection lands inside the tetrahedron.");
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ -0.1, 0.5, 0.5, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return false for a point outside the tetrahedron.");
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 2, 2, 2, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return false for a point beyond the far cell of the tetrahedron.");
	}
	{
		// 5D: a pentachoron (4-simplex, 5 vertices).
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 2, 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 0, 2, 0, 0, 0 });
		vertices.push_back(VectorN{ 0, 0, 2, 0, 0 });
		vertices.push_back(VectorN{ 0, 0, 0, 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 0.4, 0.4, 0.4, 0.4, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true for a point inside the pentachoron.");
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 0.4, 0.4, 0.4, 0.4, -2 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return true when the projection lands inside the pentachoron.");
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 2, 2, 2, 2, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return false for a point outside the pentachoron.");
	}
	{
		// The 0D and 1D cases are degenerate: the projection onto the simplex's affine hull is always inside.
		Vector<VectorN> vertices_1d;
		vertices_1d.push_back(VectorN{ 5 });
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices_1d, VectorN{ 7 }, PackedFloat64Array(), 0), "GeometryND is_point_inside_simplex_barycentric should return true for the degenerate 1D case.");
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(Vector<VectorN>(), VectorN(), PackedFloat64Array(), 0), "GeometryND is_point_inside_simplex_barycentric should return true for the degenerate 0D case.");
	}
	{
		// A cache holding multiple simplices should be read at the correct offset for the simplex index.
		Vector<VectorN> vertices_a;
		vertices_a.push_back(VectorN{ 0, 0 });
		vertices_a.push_back(VectorN{ 2, 0 });
		Vector<VectorN> vertices_b;
		vertices_b.push_back(VectorN{ 0, 0 });
		vertices_b.push_back(VectorN{ 0, 4 });
		PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices_a);
		cache.append_array(compute_simplex_inverse_metric_cache(vertices_b));
		CHECK_MESSAGE(GeometryND::is_point_inside_simplex_barycentric(vertices_b, VectorN{ 0, 3 }, cache, 1), "GeometryND is_point_inside_simplex_barycentric should read the cache at the offset of the given simplex index.");
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices_b, VectorN{ 0, 5 }, cache, 1), "GeometryND is_point_inside_simplex_barycentric should read the cache at the offset of the given simplex index.");
	}
	{
		// Mismatched dimensions should fail with an error.
		ERR_PRINT_OFF;
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0 });
		vertices.push_back(VectorN{ 2, 0, 0 });
		const PackedFloat64Array cache = { 0.25 };
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices, VectorN{ 1, 0 }, cache, 0), "GeometryND is_point_inside_simplex_barycentric should return false when a vertex dimension does not match the vertex count.");
		Vector<VectorN> vertices_valid;
		vertices_valid.push_back(VectorN{ 0, 0 });
		vertices_valid.push_back(VectorN{ 2, 0 });
		CHECK_MESSAGE(!GeometryND::is_point_inside_simplex_barycentric(vertices_valid, VectorN{ 1, 0 }, PackedFloat64Array(), 0), "GeometryND is_point_inside_simplex_barycentric should return false when the inverse metric cache is too small.");
		ERR_PRINT_ON;
	}
}

TEST_CASE("[GeometryND] Get Nearest Point On Simplex Barycentric") {
	VectorN nearest;
	double distance_squared = 0.0;
	bool proj_inside = false;
	{
		// 2D: a line segment.
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0 });
		vertices.push_back(VectorN{ 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 1, 1 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 1, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should project onto the inside of the segment.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(1.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as inside the segment.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 3, 1 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 2, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the endpoint for a point beyond the end of the segment.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(2.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(!proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as outside the segment.");
	}
	{
		// 3D: a triangle.
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0, 0 });
		vertices.push_back(VectorN{ 2, 0, 0 });
		vertices.push_back(VectorN{ 0, 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 0.5, 0.5, 1 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0.5, 0.5, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should project onto the inside of the triangle.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(1.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as inside the triangle.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 2, 2, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 1, 1, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest point on the border edge.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(2.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(!proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as outside the triangle.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ -1, -1, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0, 0, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest vertex.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(2.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(!proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as outside the triangle.");
	}
	{
		// 4D: a tetrahedron. This case matches the tests of the specialized Geometry4D functions.
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 2, 0, 0, 0 });
		vertices.push_back(VectorN{ 0, 2, 0, 0 });
		vertices.push_back(VectorN{ 0, 0, 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 0.5, 0.5, 0.5, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0.5, 0.5, 0.5, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the point itself when inside the tetrahedron.");
		CHECK_MESSAGE(Math::is_zero_approx(distance_squared), "GeometryND get_nearest_point_on_simplex_barycentric should return zero distance for a point inside the tetrahedron.");
		CHECK_MESSAGE(proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as inside the tetrahedron.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 0.5, 0.5, 0.5, 1 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0.5, 0.5, 0.5, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should project onto the inside of the tetrahedron.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(1.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as inside the tetrahedron.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 2, 2, 2, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest point on the far facet.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(16.0 / 3.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(!proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as outside the tetrahedron.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 2, 2, 0, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 1, 1, 0, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest point on the border edge.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(2.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 4, 0, 0, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 2, 0, 0, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest vertex.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(4.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ -1, -1, -1, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0, 0, 0, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest vertex.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(3.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
	}
	{
		// 5D: a pentachoron (4-simplex, 5 vertices).
		Vector<VectorN> vertices;
		vertices.push_back(VectorN{ 0, 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 2, 0, 0, 0, 0 });
		vertices.push_back(VectorN{ 0, 2, 0, 0, 0 });
		vertices.push_back(VectorN{ 0, 0, 2, 0, 0 });
		vertices.push_back(VectorN{ 0, 0, 0, 2, 0 });
		const PackedFloat64Array cache = compute_simplex_inverse_metric_cache(vertices);
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 0.4, 0.4, 0.4, 0.4, 1 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0.4, 0.4, 0.4, 0.4, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should project onto the inside of the pentachoron.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(1.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as inside the pentachoron.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ 2, 2, 2, 2, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0.5, 0.5, 0.5, 0.5, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest point on the far facet.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(9.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
		CHECK_MESSAGE(!proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as outside the pentachoron.");
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices, VectorN{ -1, -1, -1, -1, 0 }, cache, 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 0, 0, 0, 0, 0 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the nearest vertex.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(4.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance.");
	}
	{
		// The 0D and 1D cases are degenerate: return the single point in 1D, and an empty VectorN in 0D.
		Vector<VectorN> vertices_1d;
		vertices_1d.push_back(VectorN{ 5 });
		GeometryND::get_nearest_point_on_simplex_barycentric(vertices_1d, VectorN{ 7 }, PackedFloat64Array(), 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest, VectorN{ 5 }), "GeometryND get_nearest_point_on_simplex_barycentric should return the single point for the degenerate 1D case.");
		CHECK_MESSAGE(distance_squared == doctest::Approx(4.0), "GeometryND get_nearest_point_on_simplex_barycentric should return the correct squared distance for the degenerate 1D case.");
		CHECK_MESSAGE(proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as inside for the degenerate 1D case.");
		GeometryND::get_nearest_point_on_simplex_barycentric(Vector<VectorN>(), VectorN(), PackedFloat64Array(), 0, nearest, distance_squared, proj_inside);
		CHECK_MESSAGE(nearest.size() == 0, "GeometryND get_nearest_point_on_simplex_barycentric should return an empty VectorN for the degenerate 0D case.");
		CHECK_MESSAGE(Math::is_zero_approx(distance_squared), "GeometryND get_nearest_point_on_simplex_barycentric should return zero distance for the degenerate 0D case.");
		CHECK_MESSAGE(proj_inside, "GeometryND get_nearest_point_on_simplex_barycentric should report the projection as inside for the degenerate 0D case.");
	}
}
} // namespace TestGeometryND
