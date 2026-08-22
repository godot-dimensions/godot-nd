#pragma once

#include "../godot_nd_defines.h"

// Static helper class for misc ND geometry functions.
class GeometryND : public Object {
	GDCLASS(GeometryND, Object);

	static int64_t _symmetric_matrix_packed_index(const int64_t p_row, const int64_t p_column, const int64_t p_edge_count);
	static VectorN _get_nearest_point_on_sub_simplex(const Vector<VectorN> &p_vertices, const VectorN &p_point);

protected:
	static GeometryND *singleton;
	static void _bind_methods();

public:
	// Barycentric simplex calculations. Don't expose these, it just needs to be efficient
	// and shared between CellMeshND, future ND physics shapes, etc. The simplex is a full
	// (N-1)-simplex in N-dimensional space: N vertices, each with N components.
	static bool compute_inverse_metric(const VectorN &p_symmetric_metric, VectorN &r_inv_symmetric);
	static void get_nearest_point_on_simplex_barycentric(const Vector<VectorN> &p_vertices, const VectorN &p_point, const PackedFloat64Array &p_nearest_simplex_inverse_metric_cache, const int64_t p_simplex_index, VectorN &r_nearest_on_simplex, double &r_distance_squared, bool &r_proj_inside);
	static bool is_point_inside_simplex_barycentric(const Vector<VectorN> &p_vertices, const VectorN &p_point, const PackedFloat64Array &p_nearest_simplex_inverse_metric_cache, const int64_t p_simplex_index);

	static VectorN closest_point_on_line(const VectorN &p_line_position, const VectorN &p_line_direction, const VectorN &p_point);
	static VectorN closest_point_on_line_segment(const VectorN &p_line_a, const VectorN &p_line_b, const VectorN &p_point);
	static VectorN closest_point_on_ray(const VectorN &p_ray_origin, const VectorN &p_ray_direction, const VectorN &p_point);
	static VectorN closest_point_between_lines(const VectorN &p_line1_point, const VectorN &p_line1_dir, const VectorN &p_line2_point, const VectorN &p_line2_dir);
	static VectorN closest_point_between_line_segments(const VectorN &p_line1_a, const VectorN &p_line1_b, const VectorN &p_line2_a, const VectorN &p_line2_b);
	static Vector<VectorN> closest_points_between_lines(const VectorN &p_line1_point, const VectorN &p_line1_dir, const VectorN &p_line2_point, const VectorN &p_line2_dir);
	static Vector<VectorN> closest_points_between_line_segments(const VectorN &p_line1_a, const VectorN &p_line1_b, const VectorN &p_line2_a, const VectorN &p_line2_b);
	static Vector<VectorN> closest_points_between_line_and_segment(const VectorN &p_line_point, const VectorN &p_line_direction, const VectorN &p_segment_a, const VectorN &p_segment_b);

	static GeometryND *get_singleton() { return singleton; }
	GeometryND() { singleton = this; }
	~GeometryND() { singleton = nullptr; }
};
