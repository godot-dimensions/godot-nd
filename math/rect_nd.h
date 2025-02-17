#pragma once

#include "../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/ref_counted.hpp>
#elif GODOT_MODULE
#include "core/object/ref_counted.h"
#endif

class RectND : public RefCounted {
	GDCLASS(RectND, RefCounted);

	VectorN _position;
	VectorN _size;

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
	double get_surface() const;

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

	// Rect comparison functions.
	bool encloses_exclusive(const Ref<RectND> &p_other) const;
	bool encloses_inclusive(const Ref<RectND> &p_other) const;
	bool intersects_exclusive(const Ref<RectND> &p_other) const;
	bool intersects_inclusive(const Ref<RectND> &p_other) const;
	bool is_equal_approx(const Ref<RectND> &p_other) const;
	bool is_finite() const;

	// Constructors.
	static Ref<RectND> from_center_size(const VectorN &p_center, const VectorN &p_size);
	static Ref<RectND> from_position_size(const VectorN &p_position, const VectorN &p_size);
	static Ref<RectND> from_position_end(const VectorN &p_position, const VectorN &p_end);
};
