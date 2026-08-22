#pragma once

#include "../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#elif GODOT_MODULE
#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"
#endif

class RectND : public RefCounted {
	GDCLASS(RectND, RefCounted);

	VectorN _position;
	VectorN _size;

#ifdef MATH_CHECKS
	static void _check_negative_size(const VectorN &p_size);
#endif // MATH_CHECKS
	static bool _are_motionless_intervals_separated(const double p_self_start, const double p_self_end, const double p_obstacle_start, const double p_obstacle_end);

protected:
	static void _bind_methods();

public:
	// Trivial getters and setters.
	VectorN get_position() const;
	void set_position(const VectorN &p_position);

	VectorN get_size() const;
	void set_size(const VectorN &p_size);

	VectorN get_end() const;
	void set_end(const VectorN &p_end);

	VectorN get_center() const;
	void set_center(const VectorN &p_center);

	int get_dimension() const;
	void set_dimension(const int p_dimension);

	// Basic math functions.
	Ref<RectND> abs() const;
	Ref<RectND> duplicate() const;
	double get_space() const;
	bool has_space() const;
	double get_surface() const;
	bool has_surface() const;
	bool has_any_size() const;

	// Point math functions.
	void expand_self_to_point(const VectorN &p_vector);
	Ref<RectND> expand_to_point(const VectorN &p_vector) const;
	VectorN get_nearest_point(const VectorN &p_point) const;
	VectorN get_support_point(const VectorN &p_direction) const;
	bool has_point(const VectorN &p_point) const;

	// Rect math functions.
	Ref<RectND> grow(const double p_by) const;
	Ref<RectND> intersection(const Ref<RectND> &p_other) const;
	Ref<RectND> merge(const Ref<RectND> &p_other) const;

	// Rect collision functions.
	double continuous_collision_depth(const VectorN &p_relative_motion, const Ref<RectND> &p_obstacle, VectorN *r_out_normal = nullptr) const;
	double continuous_collision_depth_bind(const VectorN &p_relative_motion, const Ref<RectND> &p_obstacle) const;
	bool continuous_collision_overlaps(const VectorN &p_relative_motion, const Ref<RectND> &p_obstacle) const;
	bool raycast_intersects(const VectorN &p_from, const VectorN &p_direction, const bool p_inside_is_zero, double *r_out_distance, VectorN *r_out_normal) const;
	Dictionary raycast_intersects_dict(const VectorN &p_from, const VectorN &p_direction, const double p_max_distance = Math_INF, const bool p_inside_is_zero = false) const;

	// Rect comparison functions.
	bool encloses_exclusive(const Ref<RectND> &p_other) const;
	bool encloses_inclusive(const Ref<RectND> &p_other) const;
	bool intersects_exclusive(const Ref<RectND> &p_other) const;
	bool intersects_inclusive(const Ref<RectND> &p_other) const;
	bool is_equal_approx(const Ref<RectND> &p_other) const;
	bool is_finite() const;
	virtual String _to_string() MODULE_OVERRIDE;

	// Constructors.
	static Ref<RectND> from_center_size(const VectorN &p_center, const VectorN &p_size);
	static Ref<RectND> from_position_size(const VectorN &p_position, const VectorN &p_size);
	static Ref<RectND> from_position_end(const VectorN &p_position, const VectorN &p_end);
};
