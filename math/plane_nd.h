#pragma once

#include "../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/ref_counted.hpp>
#elif GODOT_MODULE
#include "core/object/ref_counted.h"
#endif

class PlaneND : public RefCounted {
	GDCLASS(PlaneND, RefCounted);

	VectorN _normal;
	double _distance = 0.0f;

protected:
	static void _bind_methods();

public:
	int64_t get_dimension() const { return _normal.size(); }
	void set_dimension(const int64_t p_dimension);

	double get_distance() const { return _distance; }
	void set_distance(const double p_distance) { _distance = p_distance; }

	VectorN get_normal() const { return _normal; };
	void set_normal(const VectorN &p_normal) { _normal = p_normal; }

	// Point functions.
	double distance_to(const VectorN &p_point) const;
	bool is_point_over(const VectorN &p_point) const;
	bool has_point(const VectorN &p_point) const;
	VectorN project(const VectorN &p_point) const;

	// Plane math functions.
	VectorN get_center() const;
	Variant intersect_line(const VectorN &p_line_origin, const VectorN &p_line_direction) const;
	Variant intersect_line_segment(const VectorN &p_begin, const VectorN &p_end) const;
	Variant intersect_ray(const VectorN &p_ray_origin, const VectorN &p_ray_direction) const;
	Ref<PlaneND> normalized() const;
	void normalize();

	// Plane comparison functions.
	bool is_equal_approx(const Ref<PlaneND> &p_other) const;
	bool is_equal_approx_any_side(const Ref<PlaneND> &p_other) const;

	// Operators.
	Ref<PlaneND> flipped() const;
	String to_string() MODULE_OVERRIDE;

	// Constructors.
	static Ref<PlaneND> from_normal_distance(const VectorN &p_normal, double p_distance = 0.0f);
	static Ref<PlaneND> from_normal_point(const VectorN &p_normal, const VectorN &p_point);
	static Ref<PlaneND> from_points(const Vector<VectorN> &p_points);
	PlaneND() {}
};
