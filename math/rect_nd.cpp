#include "rect_nd.h"

#include "vector_nd.h"

#ifdef MATH_CHECKS
void RectND::_check_negative_size(const VectorN &p_size) {
	for (int i = 0; i < p_size.size(); ++i) {
		if (unlikely(p_size[i] < 0.0)) {
			ERR_PRINT("RectND size is negative, this is not supported. Use RectND.abs() to get a RectND with a positive size.");
			return;
		}
	}
}
#endif // MATH_CHECKS

// Returns true if the two intervals do not overlap, for an axis with no relative motion.
// Touching endpoints only count as overlapping when one of the intervals is a single point,
// so that a zero-thickness shape can still collide with what it is touching, while two solid
// rects that merely share a face are free to slide along that face instead of being stopped.
// Note that a rect with a missing size element has a single-point interval on that axis.
bool RectND::_are_motionless_intervals_separated(const double p_self_start, const double p_self_end, const double p_obstacle_start, const double p_obstacle_end) {
	if (p_self_start == p_self_end || p_obstacle_start == p_obstacle_end) {
		return p_self_start > p_obstacle_end || p_self_end < p_obstacle_start;
	}
	return p_self_start >= p_obstacle_end || p_self_end <= p_obstacle_start;
}

// Trivial getters and setters.

VectorN RectND::get_position() const {
	return _position;
}

void RectND::set_position(const VectorN &p_position) {
	_position = p_position;
}

VectorN RectND::get_size() const {
	return _size;
}

void RectND::set_size(const VectorN &p_size) {
	_size = p_size;
}

VectorN RectND::get_end() const {
	return VectorND::add(_position, _size);
}

void RectND::set_end(const VectorN &p_end) {
	_size = VectorND::subtract(p_end, _position);
}

VectorN RectND::get_center() const {
	return VectorND::add(_position, VectorND::multiply_scalar(_size, 0.5));
}

void RectND::set_center(const VectorN &p_center) {
	_position = VectorND::subtract(p_center, VectorND::multiply_scalar(_size, 0.5));
}

int RectND::get_dimension() const {
	return MAX(_position.size(), _size.size());
}

void RectND::set_dimension(const int p_dimension) {
	ERR_FAIL_COND(p_dimension < 0);
	_position.resize(p_dimension);
	_size.resize(p_dimension);
}

// Basic math functions.

Ref<RectND> RectND::abs() const {
	const int dimension = get_dimension();
	VectorN abs_position = VectorND::with_dimension(_position, dimension);
	VectorN abs_size = VectorND::with_dimension(_size, dimension);
	for (int i = 0; i < dimension; ++i) {
		if (abs_size[i] < 0.0) {
			abs_position.set(i, abs_position[i] + abs_size[i]);
			abs_size.set(i, -abs_size[i]);
		}
	}
	Ref<RectND> abs_rect;
	abs_rect.instantiate();
	abs_rect->set_position(abs_position);
	abs_rect->set_size(abs_size);
	return abs_rect;
}

Ref<RectND> RectND::duplicate() const {
	Ref<RectND> dup;
	dup.instantiate();
	dup->set_position(_position);
	dup->set_size(_size);
	return dup;
}

double RectND::get_space() const {
	const int dimension = get_dimension();
	const VectorN size = VectorND::with_dimension(_size, dimension);
	double hypervolume = 1.0;
	for (int i = 0; i < dimension; ++i) {
		hypervolume *= size[i];
	}
	return hypervolume;
}

bool RectND::has_space() const {
	const int dimension = get_dimension();
	const VectorN size = VectorND::with_dimension(_size, dimension);
	for (int i = 0; i < dimension; ++i) {
		if (!(size[i] > 0.0)) {
			return false;
		}
	}
	return true;
}

double RectND::get_surface() const {
	const int dimension = get_dimension();
	const VectorN size = VectorND::with_dimension(_size, dimension);
	double surface_total = 0.0;
	for (int i = 0; i < dimension; ++i) {
		double surface = 1.0;
		for (int j = 0; j < dimension; ++j) {
			if (i != j) {
				surface *= size[j];
			}
		}
		surface_total += surface;
	}
	return surface_total * 2.0;
}

bool RectND::has_surface() const {
	for (int i = 0; i < _size.size(); ++i) {
		if (_size[i] > 0.0) {
			return true;
		}
	}
	return false;
}

bool RectND::has_any_size() const {
	for (int i = 0; i < _size.size(); ++i) {
		if (_size[i] != 0.0) {
			return true;
		}
	}
	return false;
}

// Point math functions.

void RectND::expand_self_to_point(const VectorN &p_vector) {
#ifdef MATH_CHECKS
	_check_negative_size(_size);
#endif // MATH_CHECKS
	const int dimension = MAX(get_dimension(), (int)p_vector.size());
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN point = VectorND::with_dimension(p_vector, dimension);
	_position = VectorND::with_dimension(_position, dimension);
	_size = VectorND::with_dimension(_size, dimension);
	for (int i = 0; i < dimension; ++i) {
		if (point[i] < _position[i]) {
			_position.set(i, point[i]);
			_size.set(i, end[i] - point[i]);
		} else if (point[i] > end[i]) {
			_size.set(i, point[i] - _position[i]);
		}
	}
}

Ref<RectND> RectND::expand_to_point(const VectorN &p_vector) const {
	Ref<RectND> new_rect = duplicate();
	new_rect->expand_self_to_point(p_vector);
	return new_rect;
}

VectorN RectND::get_nearest_point(const VectorN &p_point) const {
#ifdef MATH_CHECKS
	_check_negative_size(_size);
#endif // MATH_CHECKS
	const int dimension = MAX(get_dimension(), (int)p_point.size());
	const VectorN position = VectorND::with_dimension(_position, dimension);
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	VectorN closest = VectorND::with_dimension(p_point, dimension);
	for (int i = 0; i < dimension; ++i) {
		if (closest[i] < position[i]) {
			closest.set(i, position[i]);
		} else if (closest[i] > end[i]) {
			closest.set(i, end[i]);
		}
	}
	return closest;
}

VectorN RectND::get_support_point(const VectorN &p_direction) const {
	const int dimension = MAX(get_dimension(), (int)p_direction.size());
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN direction = VectorND::with_dimension(p_direction, dimension);
	VectorN support = VectorND::with_dimension(_position, dimension);
	for (int i = 0; i < dimension; ++i) {
		if (direction[i] > 0.0) {
			support.set(i, end[i]);
		}
	}
	return support;
}

bool RectND::has_point(const VectorN &p_point) const {
#ifdef MATH_CHECKS
	_check_negative_size(_size);
#endif // MATH_CHECKS
	const int dimension = MAX(get_dimension(), (int)p_point.size());
	const VectorN position = VectorND::with_dimension(_position, dimension);
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN point = VectorND::with_dimension(p_point, dimension);
	for (int i = 0; i < dimension; ++i) {
		if (point[i] < position[i] || point[i] > end[i]) {
			return false;
		}
	}
	return true;
}

// Rect math functions.

Ref<RectND> RectND::grow(const double p_by) const {
	const int dimension = get_dimension();
	Ref<RectND> grown;
	grown.instantiate();
	grown->set_position(VectorND::add_scalar(VectorND::with_dimension(_position, dimension), -p_by));
	grown->set_size(VectorND::add_scalar(VectorND::with_dimension(_size, dimension), p_by * 2.0));
	return grown;
}

Ref<RectND> RectND::intersection(const Ref<RectND> &p_other) const {
	Ref<RectND> intersected;
	intersected.instantiate();
	ERR_FAIL_COND_V(p_other.is_null(), intersected);
#ifdef MATH_CHECKS
	_check_negative_size(_size);
	_check_negative_size(p_other->get_size());
#endif // MATH_CHECKS
	const int dimension = MAX(get_dimension(), p_other->get_dimension());
	const VectorN position = VectorND::with_dimension(_position, dimension);
	const VectorN other_position = VectorND::with_dimension(p_other->get_position(), dimension);
	VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN other_end = VectorND::with_dimension(p_other->get_end(), dimension);
	VectorN intersect_position = position;
	for (int i = 0; i < dimension; ++i) {
		if (position[i] > other_end[i] || end[i] < other_position[i]) {
			// No intersection.
			return intersected;
		}
		if (other_position[i] > position[i]) {
			intersect_position.set(i, other_position[i]);
		}
		if (other_end[i] < end[i]) {
			end.set(i, other_end[i]);
		}
	}
	intersected->set_position(intersect_position);
	intersected->set_end(end);
	return intersected;
}

Ref<RectND> RectND::merge(const Ref<RectND> &p_other) const {
	Ref<RectND> merged;
	merged.instantiate();
	ERR_FAIL_COND_V(p_other.is_null(), merged);
#ifdef MATH_CHECKS
	_check_negative_size(_size);
	_check_negative_size(p_other->get_size());
#endif // MATH_CHECKS
	const int dimension = MAX(get_dimension(), p_other->get_dimension());
	VectorN other_start = VectorND::with_dimension(p_other->get_position(), dimension);
	VectorN other_end = VectorND::with_dimension(p_other->get_end(), dimension);
	VectorN merged_start = VectorND::with_dimension(_position, dimension);
	VectorN merged_end = VectorND::with_dimension(get_end(), dimension);
	for (int i = 0; i < dimension; ++i) {
		if (other_start[i] < merged_start[i]) {
			merged_start.set(i, other_start[i]);
		}
		if (other_end[i] > merged_end[i]) {
			merged_end.set(i, other_end[i]);
		}
	}
	merged->set_position(merged_start);
	merged->set_end(merged_end);
	return merged;
}

// Rect collision functions.

// Note: This function can return values outside of the expected range,
// including values above 1.0, below 0.0, and even below -1.0.
// Handling the meaning of such values is up to the caller.
// Suggestion: For the expected use case of loops, start with `real_t ratio = 1.0f;`
// and then use `ratio = MIN(ratio, result);` on each loop iteration.
double RectND::continuous_collision_depth(const VectorN &p_relative_motion, const Ref<RectND> &p_obstacle, VectorN *r_out_normal) const {
#ifdef MATH_CHECKS
	_check_negative_size(_size);
#endif // MATH_CHECKS
	// A null obstacle cannot be collided with, so all of the motion is allowed.
	ERR_FAIL_COND_V(p_obstacle.is_null(), 1.0);
	const int dimension = MAX(MAX(get_dimension(), (int)p_relative_motion.size()), p_obstacle->get_dimension());
	const VectorN self_position = VectorND::with_dimension(_position, dimension);
	const VectorN self_end = VectorND::with_dimension(get_end(), dimension);
	const VectorN obstacle_position = VectorND::with_dimension(p_obstacle->get_position(), dimension);
	const VectorN obstacle_end = VectorND::with_dimension(p_obstacle->get_end(), dimension);
	const VectorN relative_motion = VectorND::with_dimension(p_relative_motion, dimension);
	VectorN low_ratios;
	VectorN high_ratios;
	low_ratios.resize(dimension);
	high_ratios.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		if (relative_motion[i] == 0.0) {
			if (_are_motionless_intervals_separated(self_position[i], self_end[i], obstacle_position[i], obstacle_end[i])) {
				// No collision is possible in this axis, so we can return early.
				if (r_out_normal) {
					*r_out_normal = VectorND::zero(dimension);
				}
				return 1.0;
			}
			low_ratios.set(i, -Math_INF);
			high_ratios.set(i, Math_INF);
		} else {
			double low_ratio = (obstacle_position[i] - self_end[i]) / (relative_motion[i]);
			double high_ratio = (obstacle_end[i] - self_position[i]) / (relative_motion[i]);
			if (relative_motion[i] < 0.0) {
				SWAP(low_ratio, high_ratio);
			}
			low_ratios.set(i, low_ratio);
			high_ratios.set(i, high_ratio);
		}
	}
	double low_ratio = -Math_INF;
	double high_ratio = Math_INF;
	for (int i = 0; i < dimension; i++) {
		low_ratio = MAX(low_ratio, low_ratios[i]);
		high_ratio = MIN(high_ratio, high_ratios[i]);
	}
	if (low_ratio < high_ratio) {
		// These checks handles depenetration, choosing the quickest way out of the obstacle.
		if (high_ratio > 0.0 && ABS(low_ratio) < high_ratio) {
			// Ok, we know there is a collision and we want to return it.
			// But first we need to set the normal if the caller requested it.
			if (r_out_normal) {
				const int64_t axis = VectorND::max_axis_index(low_ratios);
				const double normal_sign = VectorND::get_component(relative_motion, axis) < 0.0 ? 1.0 : -1.0;
				*r_out_normal = VectorND::value_on_axis_with_dimension(normal_sign, axis, dimension);
			}
			return low_ratio;
		}
	}
	if (r_out_normal) {
		*r_out_normal = VectorND::zero(dimension);
	}
	return 1.0;
}

double RectND::continuous_collision_depth_bind(const VectorN &p_relative_motion, const Ref<RectND> &p_obstacle) const {
	return continuous_collision_depth(p_relative_motion, p_obstacle);
}

bool RectND::continuous_collision_overlaps(const VectorN &p_relative_motion, const Ref<RectND> &p_obstacle) const {
#ifdef MATH_CHECKS
	_check_negative_size(_size);
#endif // MATH_CHECKS
	ERR_FAIL_COND_V(p_obstacle.is_null(), false);
	const int dimension = MAX(MAX(get_dimension(), (int)p_relative_motion.size()), p_obstacle->get_dimension());
	const VectorN self_position = VectorND::with_dimension(_position, dimension);
	const VectorN self_end = VectorND::with_dimension(get_end(), dimension);
	const VectorN obstacle_position = VectorND::with_dimension(p_obstacle->get_position(), dimension);
	const VectorN obstacle_end = VectorND::with_dimension(p_obstacle->get_end(), dimension);
	const VectorN relative_motion = VectorND::with_dimension(p_relative_motion, dimension);
	double low_ratio = -Math_INF;
	double high_ratio = Math_INF;
	for (int i = 0; i < dimension; i++) {
		if (relative_motion[i] == 0.0) {
			if (_are_motionless_intervals_separated(self_position[i], self_end[i], obstacle_position[i], obstacle_end[i])) {
				// No collision is possible in this axis, so we can return early.
				return false;
			}
		} else {
			double axis_low_ratio = (obstacle_position[i] - self_end[i]) / (relative_motion[i]);
			double axis_high_ratio = (obstacle_end[i] - self_position[i]) / (relative_motion[i]);
			if (relative_motion[i] < 0.0) {
				SWAP(axis_low_ratio, axis_high_ratio);
			}
			low_ratio = MAX(low_ratio, axis_low_ratio);
			high_ratio = MIN(high_ratio, axis_high_ratio);
		}
	}
	if (low_ratio < high_ratio && low_ratio < 1.0 && high_ratio > 0.0) {
		return true;
	}
	return false;
}

bool RectND::raycast_intersects(const VectorN &p_from, const VectorN &p_direction, const bool p_inside_is_zero, double *r_out_distance, VectorN *r_out_normal) const {
#ifdef MATH_CHECKS
	_check_negative_size(_size);
	ERR_FAIL_COND_V_MSG(!Math::is_equal_approx(VectorND::length_squared(p_direction), 1.0), false, "RectND::raycast_intersects: Ray direction must be normalized.");
#endif // MATH_CHECKS
	const int dimension = MAX(MAX(get_dimension(), (int)p_from.size()), (int)p_direction.size());
	const VectorN self_position = VectorND::with_dimension(_position, dimension);
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN from = VectorND::with_dimension(p_from, dimension);
	const VectorN direction = VectorND::with_dimension(p_direction, dimension);
	if (p_inside_is_zero) {
		// Same as `has_point`, but reusing the already dimension-expanded vectors.
		bool is_inside = true;
		for (int i = 0; i < dimension; i++) {
			if (from[i] < self_position[i] || from[i] > end[i]) {
				is_inside = false;
				break;
			}
		}
		if (is_inside) {
			// The ray starts inside the box, so we can return a distance of 0.0f.
			if (r_out_distance != nullptr) {
				*r_out_distance = 0.0;
			}
			if (r_out_normal != nullptr) {
				*r_out_normal = VectorND::zero(dimension);
			}
			return true;
		}
	}
	double distance_min = -Math_INF;
	double distance_max = Math_INF;
	int64_t distance_min_axis = 0;
	int64_t distance_max_axis = 0;
	for (int i = 0; i < dimension; i++) {
		if (direction[i] == 0.0) {
			// Ray is parallel in this axis, so there is no casting: just check if the from point is inside the box in this axis.
			if (from[i] < self_position[i] || from[i] > end[i]) {
				return false;
			}
		} else {
			double low_ratio = (self_position[i] - from[i]) / direction[i];
			double high_ratio = (end[i] - from[i]) / direction[i];
			// Ray is not parallel in this axis, so we can calculate the intersection distances.
			if (low_ratio > high_ratio) {
				// Swap the ratios if they are in the wrong order.
				SWAP(low_ratio, high_ratio);
			}
			if (low_ratio > distance_min) {
				distance_min = low_ratio;
				distance_min_axis = i;
			}
			if (high_ratio < distance_max) {
				if (high_ratio < 0.0) {
					// The ray is pointing away from the box, so it will never hit.
					return false;
				}
				distance_max = high_ratio;
				distance_max_axis = i;
			}
			if (distance_min > distance_max) {
				// The ray misses the box.
				return false;
			}
		}
	}
	// If we haven't returned false by now, then the ray hits the box.
	const bool hit_from_inside = distance_min < 0.0;
	const double hit_distance = hit_from_inside ? distance_max : distance_min;
	const int64_t hit_axis = hit_from_inside ? distance_max_axis : distance_min_axis;
	if (r_out_distance != nullptr) {
		*r_out_distance = hit_distance;
	}
	if (r_out_normal != nullptr) {
		const double direction_sign = (VectorND::get_component(direction, hit_axis) >= 0.0) ? 1.0 : -1.0;
		*r_out_normal = VectorND::value_on_axis_with_dimension(hit_from_inside ? direction_sign : -direction_sign, hit_axis, dimension);
	}
	return true;
}

Dictionary RectND::raycast_intersects_dict(const VectorN &p_from, const VectorN &p_direction, const double p_max_distance, const bool p_inside_is_zero) const {
	const int dimension = MAX(MAX(get_dimension(), (int)p_from.size()), (int)p_direction.size());
	const VectorN from = VectorND::with_dimension(p_from, dimension);
	const VectorN direction = VectorND::with_dimension(p_direction, dimension);
	double distance = -Math_INF;
	VectorN normal;
	const bool hit_intersects = raycast_intersects(from, direction, p_inside_is_zero, &distance, &normal) && distance < p_max_distance;
	Dictionary result;
	result["hit"] = hit_intersects;
	if (hit_intersects) {
		result["distance"] = distance;
		result["normal"] = normal;
		result["point"] = VectorND::add(from, VectorND::multiply_scalar(direction, distance));
	}
	return result;
}

// Rect comparison functions.

bool RectND::encloses_exclusive(const Ref<RectND> &p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), false);
	const int dimension = MAX(get_dimension(), p_other->get_dimension());
	const VectorN position = VectorND::with_dimension(_position, dimension);
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN other_position = VectorND::with_dimension(p_other->get_position(), dimension);
	const VectorN other_end = VectorND::with_dimension(p_other->get_end(), dimension);
	for (int i = 0; i < dimension; ++i) {
		if (position[i] >= other_position[i] || end[i] <= other_end[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::encloses_inclusive(const Ref<RectND> &p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), false);
	const int dimension = MAX(get_dimension(), p_other->get_dimension());
	const VectorN position = VectorND::with_dimension(_position, dimension);
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN other_position = VectorND::with_dimension(p_other->get_position(), dimension);
	const VectorN other_end = VectorND::with_dimension(p_other->get_end(), dimension);
	for (int i = 0; i < dimension; ++i) {
		if (position[i] > other_position[i] || end[i] < other_end[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::intersects_exclusive(const Ref<RectND> &p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), false);
	const int dimension = MAX(get_dimension(), p_other->get_dimension());
	const VectorN position = VectorND::with_dimension(_position, dimension);
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN other_position = VectorND::with_dimension(p_other->get_position(), dimension);
	const VectorN other_end = VectorND::with_dimension(p_other->get_end(), dimension);
	for (int i = 0; i < dimension; ++i) {
		if (position[i] >= other_end[i] || end[i] <= other_position[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::intersects_inclusive(const Ref<RectND> &p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), false);
	const int dimension = MAX(get_dimension(), p_other->get_dimension());
	const VectorN position = VectorND::with_dimension(_position, dimension);
	const VectorN end = VectorND::with_dimension(get_end(), dimension);
	const VectorN other_position = VectorND::with_dimension(p_other->get_position(), dimension);
	const VectorN other_end = VectorND::with_dimension(p_other->get_end(), dimension);
	for (int i = 0; i < dimension; ++i) {
		if (position[i] > other_end[i] || end[i] < other_position[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::is_equal_approx(const Ref<RectND> &p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), false);
	return VectorND::is_equal_approx(_position, p_other->get_position()) && VectorND::is_equal_approx(_size, p_other->get_size());
}

bool RectND::is_finite() const {
	return VectorND::is_finite(_position) && VectorND::is_finite(_size);
}

String RectND::_to_string() {
	return "RectND(P: " + VectorND::vec_to_string(_position) + ", S: " + VectorND::vec_to_string(_size) + ")";
}

// Constructors.

Ref<RectND> RectND::from_center_size(const VectorN &p_center, const VectorN &p_size) {
	Ref<RectND> rect;
	rect.instantiate();
	rect->set_size(p_size);
	rect->set_center(p_center);
	return rect;
}

Ref<RectND> RectND::from_position_size(const VectorN &p_position, const VectorN &p_size) {
	Ref<RectND> rect;
	rect.instantiate();
	rect->set_position(p_position);
	rect->set_size(p_size);
	return rect;
}

Ref<RectND> RectND::from_position_end(const VectorN &p_position, const VectorN &p_end) {
	Ref<RectND> rect;
	rect.instantiate();
	rect->set_position(p_position);
	rect->set_end(p_end);
	return rect;
}

void RectND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_position"), &RectND::get_position);
	ClassDB::bind_method(D_METHOD("set_position", "position"), &RectND::set_position);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "position", PROPERTY_HINT_NONE, "suffix:m"), "set_position", "get_position");

	ClassDB::bind_method(D_METHOD("get_size"), &RectND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &RectND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");

	ClassDB::bind_method(D_METHOD("get_end"), &RectND::get_end);
	ClassDB::bind_method(D_METHOD("set_end", "end"), &RectND::set_end);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "end", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_end", "get_end");

	ClassDB::bind_method(D_METHOD("get_center"), &RectND::get_center);
	ClassDB::bind_method(D_METHOD("set_center", "center"), &RectND::set_center);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "center", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_center", "get_center");

	ClassDB::bind_method(D_METHOD("get_dimension"), &RectND::get_dimension);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &RectND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_dimension", "get_dimension");
	// Basic math functions.
	ClassDB::bind_method(D_METHOD("abs"), &RectND::abs);
	ClassDB::bind_method(D_METHOD("duplicate"), &RectND::duplicate);
	ClassDB::bind_method(D_METHOD("get_space"), &RectND::get_space);
	ClassDB::bind_method(D_METHOD("has_space"), &RectND::has_space);
	ClassDB::bind_method(D_METHOD("get_surface"), &RectND::get_surface);
	ClassDB::bind_method(D_METHOD("has_surface"), &RectND::has_surface);
	ClassDB::bind_method(D_METHOD("has_any_size"), &RectND::has_any_size);
	// Point math functions.
	ClassDB::bind_method(D_METHOD("expand_self_to_point", "vector"), &RectND::expand_self_to_point);
	ClassDB::bind_method(D_METHOD("expand_to_point", "vector"), &RectND::expand_to_point);
	ClassDB::bind_method(D_METHOD("get_nearest_point", "point"), &RectND::get_nearest_point);
	ClassDB::bind_method(D_METHOD("get_support_point", "direction"), &RectND::get_support_point);
	ClassDB::bind_method(D_METHOD("has_point", "point"), &RectND::has_point);
	// Rect math functions.
	ClassDB::bind_method(D_METHOD("grow", "by"), &RectND::grow);
	ClassDB::bind_method(D_METHOD("intersection", "other"), &RectND::intersection);
	ClassDB::bind_method(D_METHOD("merge", "other"), &RectND::merge);
	// Rect collision functions.
	ClassDB::bind_method(D_METHOD("continuous_collision_depth", "relative_motion", "obstacle"), &RectND::continuous_collision_depth_bind);
	ClassDB::bind_method(D_METHOD("continuous_collision_overlaps", "relative_motion", "obstacle"), &RectND::continuous_collision_overlaps);
	// TODO: These should be `Math_INF` but Godot's bindings do not like infinity,
	// and also values above max float32 will overflow to infinity in the bindings.
	// See https://github.com/godotengine/godot-cpp/pull/2030
	ClassDB::bind_method(D_METHOD("raycast_intersects_dict", "from", "direction", "max_distance", "inside_is_zero"), &RectND::raycast_intersects_dict, DEFVAL(3.4e38), DEFVAL(false));
	// Rect comparison functions.
	ClassDB::bind_method(D_METHOD("encloses_exclusive", "other"), &RectND::encloses_exclusive);
	ClassDB::bind_method(D_METHOD("encloses_inclusive", "other"), &RectND::encloses_inclusive);
	ClassDB::bind_method(D_METHOD("intersects_exclusive", "other"), &RectND::intersects_exclusive);
	ClassDB::bind_method(D_METHOD("intersects_inclusive", "other"), &RectND::intersects_inclusive);
	ClassDB::bind_method(D_METHOD("is_equal_approx", "other"), &RectND::is_equal_approx);
	ClassDB::bind_method(D_METHOD("is_finite"), &RectND::is_finite);
	// Constructors.
	ClassDB::bind_static_method("RectND", D_METHOD("from_center_size", "center", "size"), &RectND::from_center_size);
	ClassDB::bind_static_method("RectND", D_METHOD("from_position_size", "position", "size"), &RectND::from_position_size);
	ClassDB::bind_static_method("RectND", D_METHOD("from_position_end", "position", "end"), &RectND::from_position_end);
}
