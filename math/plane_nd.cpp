#include "plane_nd.h"

#include "transform_nd.h"
#include "vector_nd.h"

void PlaneND::set_dimension(const int64_t p_dimension) {
	_normal = VectorND::with_dimension(_normal, p_dimension);
	normalize();
}

double PlaneND::distance_to(const VectorN &p_point) const {
	return VectorND::dot(_normal, p_point) - _distance;
}

bool PlaneND::is_point_over(const VectorN &p_point) const {
	return VectorND::dot(_normal, p_point) > _distance;
}

bool PlaneND::has_point(const VectorN &p_point) const {
	return Math::is_zero_approx(distance_to(p_point));
}

VectorN PlaneND::project(const VectorN &p_point) const {
	return VectorND::subtract(p_point, VectorND::multiply_scalar(_normal, distance_to(p_point)));
}

VectorN PlaneND::get_center() const {
	return VectorND::multiply_scalar(_normal, _distance);
}

Variant PlaneND::intersect_line(const VectorN &p_line_origin, const VectorN &p_line_direction) const {
	const double denominator = VectorND::dot(_normal, p_line_direction);
	if (Math::is_zero_approx(denominator)) {
		return Variant();
	}
	const double factor = (_distance - VectorND::dot(_normal, p_line_origin)) / denominator;
	return VectorND::add(p_line_origin, VectorND::multiply_scalar(p_line_direction, factor));
}

Variant PlaneND::intersect_line_segment(const VectorN &p_begin, const VectorN &p_end) const {
	const VectorN direction = VectorND::subtract(p_end, p_begin);
	const double denominator = VectorND::dot(_normal, direction);
	if (Math::is_zero_approx(denominator)) {
		return Variant();
	}
	const double factor = (_distance - VectorND::dot(_normal, p_begin)) / denominator;
	if (factor < 0.0 || factor > 1.0) {
		return Variant();
	}
	return VectorND::add(p_begin, VectorND::multiply_scalar(direction, factor));
}

Variant PlaneND::intersect_ray(const VectorN &p_ray_origin, const VectorN &p_ray_direction) const {
	const double denominator = VectorND::dot(_normal, p_ray_direction);
	if (Math::is_zero_approx(denominator)) {
		return Variant();
	}
	const double factor = (_distance - VectorND::dot(_normal, p_ray_origin)) / denominator;
	if (factor < 0.0) {
		return Variant();
	}
	return VectorND::add(p_ray_origin, VectorND::multiply_scalar(p_ray_direction, factor));
}

Ref<PlaneND> PlaneND::normalized() const {
	const double len = VectorND::length(_normal);
	if (len == 0.0f) {
		return Ref<PlaneND>();
	}
	return from_normal_distance(VectorND::divide_scalar(_normal, len), _distance * len);
}

void PlaneND::normalize() {
	const double len = VectorND::length(_normal);
	if (len == 0.0f) {
		ERR_FAIL_MSG("PlaneND.normalize: Cannot normalize a zero-length normal.");
	}
	_normal = VectorND::divide_scalar(_normal, len);
	_distance *= len;
}

// Plane comparison functions.

bool PlaneND::is_equal_approx(const Ref<PlaneND> &p_other) const {
	return VectorND::is_equal_approx(_normal, p_other->get_normal()) && Math::is_equal_approx(_distance, p_other->get_distance());
}

bool PlaneND::is_equal_approx_any_side(const Ref<PlaneND> &p_other) const {
	return (VectorND::is_equal_approx(_normal, p_other->get_normal()) && Math::is_equal_approx(_distance, p_other->get_distance())) || (VectorND::is_equal_approx(_normal, VectorND::negate(p_other->get_normal())) && Math::is_equal_approx(_distance, -p_other->get_distance()));
}

// Operators.

Ref<PlaneND> PlaneND::flipped() const {
	Ref<PlaneND> neg;
	neg.instantiate();
	neg->set_normal(VectorND::negate(_normal));
	neg->set_distance(-_distance);
	return neg;
}

String PlaneND::to_string() {
	return "[N: " + String(Variant(_normal)) + ", D: " + String::num_real(_distance, false) + "]";
}

// Constructors.

Ref<PlaneND> PlaneND::from_normal_distance(const VectorN &p_normal, double p_distance) {
	Ref<PlaneND> plane;
	plane.instantiate();
	plane->set_normal(p_normal);
	plane->set_distance(p_distance);
	return plane;
}

Ref<PlaneND> PlaneND::from_normal_point(const VectorN &p_normal, const VectorN &p_point) {
	Ref<PlaneND> plane;
	plane.instantiate();
	plane->set_normal(p_normal);
	plane->set_distance(VectorND::dot(p_normal, p_point));
	return plane;
}

Ref<PlaneND> PlaneND::from_points(const Vector<VectorN> &p_points) {
	Ref<PlaneND> plane;
	ERR_FAIL_COND_V_MSG(p_points.is_empty(), Ref<PlaneND>(), "PlaneND.from_points: No points provided.");
	const int point_count = p_points.size();
	VectorN added = p_points[0];
	for (int i = 1; i < point_count; i++) {
		added = VectorND::add(added, p_points[i]);
	}
	Vector<VectorN> basis_columns;
	basis_columns.resize(point_count);
	for (int i = 1; i < point_count; i++) {
		basis_columns.set(i - 1, VectorND::subtract(p_points[i], p_points[0]));
	}
	const int64_t last_column = point_count - 1;
	basis_columns.set(last_column, added);
	Ref<TransformND> basis = TransformND::from_basis_columns(basis_columns);
	basis = basis->orthonormalized();
	// Iterate until we have a valid positive determinant basis.
	double det = basis->determinant();
	if (!Math::is_equal_approx(det, 1.0)) {
		for (int i = 0; i < point_count && Math::is_zero_approx(det); i++) {
			const VectorN new_column = VectorND::value_on_axis_with_dimension(1.0, i, point_count);
			basis->set_basis_column(last_column, new_column);
			det = basis->determinant();
		}
		if (det < 0.0) {
			basis->set_basis_column(last_column, VectorND::negate(basis->get_basis_column(last_column)));
		}
		basis = basis->orthonormalized();
		det = basis->determinant();
	}
	if (!Math::is_equal_approx(det, 1.0)) {
		return Ref<PlaneND>();
	}
	// Now that we have a valid basis, we can get the normal.
	const VectorN normal = basis->get_basis_column(last_column);
	plane.instantiate();
	plane->set_normal(normal);
	plane->set_distance(VectorND::dot(normal, p_points[0]));
	return plane;
}

void PlaneND::_bind_methods() {
	// Trivial getters and setters.
	ClassDB::bind_method(D_METHOD("get_dimension"), &PlaneND::get_dimension);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &PlaneND::set_dimension);
	ClassDB::bind_method(D_METHOD("get_distance"), &PlaneND::get_distance);
	ClassDB::bind_method(D_METHOD("set_distance", "distance"), &PlaneND::set_distance);
	ClassDB::bind_method(D_METHOD("get_normal"), &PlaneND::get_normal);
	ClassDB::bind_method(D_METHOD("set_normal", "normal"), &PlaneND::set_normal);
	// Point functions.
	ClassDB::bind_method(D_METHOD("distance_to", "point"), &PlaneND::distance_to);
	ClassDB::bind_method(D_METHOD("is_point_over", "point"), &PlaneND::is_point_over);
	ClassDB::bind_method(D_METHOD("has_point", "point"), &PlaneND::has_point);
	ClassDB::bind_method(D_METHOD("project", "point"), &PlaneND::project);
	// Plane math functions.
	ClassDB::bind_method(D_METHOD("get_center"), &PlaneND::get_center);
	ClassDB::bind_method(D_METHOD("intersect_line", "line_origin", "line_direction"), &PlaneND::intersect_line);
	ClassDB::bind_method(D_METHOD("intersect_line_segment", "begin", "end"), &PlaneND::intersect_line_segment);
	ClassDB::bind_method(D_METHOD("intersect_ray", "ray_origin", "ray_direction"), &PlaneND::intersect_ray);
	ClassDB::bind_method(D_METHOD("normalized"), &PlaneND::normalized);
	ClassDB::bind_method(D_METHOD("normalize"), &PlaneND::normalize);
	// Plane comparison functions.
	ClassDB::bind_method(D_METHOD("is_equal_approx", "other"), &PlaneND::is_equal_approx);
	ClassDB::bind_method(D_METHOD("is_equal_approx_any_side", "other"), &PlaneND::is_equal_approx_any_side);
	// Plane negation.
	ClassDB::bind_method(D_METHOD("flipped"), &PlaneND::flipped);
	// Constructors.
	ClassDB::bind_static_method("PlaneND", D_METHOD("from_normal_distance", "normal", "distance"), &PlaneND::from_normal_distance);
	ClassDB::bind_static_method("PlaneND", D_METHOD("from_normal_point", "normal", "point"), &PlaneND::from_normal_point);
	// Properties.
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension"), "set_dimension", "get_dimension");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance"), "set_distance", "get_distance");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "normal"), "set_normal", "get_normal");
}
