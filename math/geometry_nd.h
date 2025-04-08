#pragma once

#include "../godot_nd_defines.h"

// Static helper class for misc ND geometry functions.
class GeometryND : public Object {
	GDCLASS(GeometryND, Object);

protected:
	static GeometryND *singleton;
	static void _bind_methods();

public:
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
