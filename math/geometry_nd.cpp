#include "geometry_nd.h"

#include "vector_nd.h"

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
