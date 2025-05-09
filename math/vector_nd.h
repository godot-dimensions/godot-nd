#pragma once

#include "../godot_nd_defines.h"

// Stateless helper class for operating on VectorN (PackedFloat64Array).
class VectorND : public Object {
	GDCLASS(VectorND, Object);

protected:
	static VectorND *singleton;
	static void _bind_methods();

public:
	// Cosmetic functions.
	static Color axis_color(int64_t p_axis);
	static String axis_letter(int64_t p_axis);

	// VectorN operations.
	static VectorN abs(const VectorN &p_vector);
	static VectorN add(const VectorN &p_a, const VectorN &p_b);
	static void add_in_place(const VectorN &p_a, VectorN &r_result);
	static VectorN add_scalar(const VectorN &p_vector, const double p_scalar);
	static double angle_to(const VectorN &p_from, const VectorN &p_to);
	static VectorN bounce(const VectorN &p_vector, const VectorN &p_normal);
	static VectorN bounce_ratio(const VectorN &p_vector, const VectorN &p_normal, const double p_bounce_ratio);
	static VectorN ceil(const VectorN &p_vector);
	static VectorN clamp(const VectorN &p_vector, const VectorN &p_min, const VectorN &p_max);
	static VectorN clampf(const VectorN &p_vector, const double p_min, const double p_max);
	static double cross(const VectorN &p_a, const VectorN &p_b);
	static VectorN direction_to(const VectorN &p_from, const VectorN &p_to);
	static double distance_to(const VectorN &p_from, const VectorN &p_to);
	static double distance_squared_to(const VectorN &p_from, const VectorN &p_to);
	static VectorN divide_vector(const VectorN &p_a, const VectorN &p_b, const bool p_expand = false);
	static VectorN divide_scalar(const VectorN &p_vector, const double p_scalar);
	static double dot(const VectorN &p_a, const VectorN &p_b);
	static VectorN drop_first_dimensions(const VectorN &p_vector, const int64_t p_dimensions);
	static VectorN duplicate(const VectorN &p_vector);
	static VectorN fill(const double p_value, const int64_t p_dimension);
	static Vector<VectorN> fill_array(const double p_value, const int64_t p_vector_amount, const int64_t p_dimension);
	static TypedArray<VectorN> fill_array_bind(const double p_value, const int64_t p_vector_amount, const int64_t p_dimension);
	static VectorN floor(const VectorN &p_vector);
	static VectorN inverse(const VectorN &p_vector);
	static bool is_equal_approx(const VectorN &p_a, const VectorN &p_b);
	static bool is_equal_exact(const VectorN &p_a, const VectorN &p_b);
	static bool is_finite(const VectorN &p_vector);
	static bool is_zero_approx(const VectorN &p_vector);
	static double length(const VectorN &p_vector);
	static double length_squared(const VectorN &p_vector);
	static VectorN lerp(const VectorN &p_from, const VectorN &p_to, const double p_weight);
	static VectorN limit_length(const VectorN &p_vector, const double p_len = 1.0);
	static int64_t max_absolute_axis_index(const VectorN &p_vector);
	static int64_t max_axis_index(const VectorN &p_vector);
	static int64_t min_axis_index(const VectorN &p_vector);
	static VectorN multiply_vector(const VectorN &p_a, const VectorN &p_b, const bool p_expand = false);
	static VectorN multiply_scalar(const VectorN &p_vector, const double p_scalar);
	static void multiply_scalar_and_add_in_place(const VectorN &p_vector, const double p_scalar, VectorN &r_result);
	static VectorN negate(const VectorN &p_vector);
	static VectorN normalized(const VectorN &p_vector);
	static VectorN perpendicular(const Vector<VectorN> &p_input_vectors);
	static VectorN perpendicular_bind(const TypedArray<VectorN> &p_input_vectors);
	static VectorN posmod(const VectorN &p_vector, const double p_mod);
	static VectorN posmodv(const VectorN &p_vector, const VectorN &p_modv);
	static VectorN project(const VectorN &p_vector, const VectorN &p_on_normal);
	static VectorN reflect(const VectorN &p_vector, const VectorN &p_normal);
	static VectorN round(const VectorN &p_vector);
	static VectorN sign(const VectorN &p_vector);
	static VectorN slide(const VectorN &p_vector, const VectorN &p_normal);
	static VectorN snapped(const VectorN &p_vector, const VectorN &p_by);
	static VectorN snappedf(const VectorN &p_vector, const double p_by);
	static VectorN subtract(const VectorN &p_a, const VectorN &p_b);
	static VectorN value_on_axis(const double p_value, const int64_t p_axis);
	static VectorN value_on_axis_with_dimension(const double p_value, const int64_t p_axis, const int64_t p_dimension);
	static VectorN with_dimension(const VectorN &p_vector, const int64_t p_dimension);
	static VectorN with_length(const VectorN &p_vector, const double p_length = 1.0);

	// Conversion.
	static VectorN from_2d(const Vector2 &p_vector);
	static VectorN from_3d(const Vector3 &p_vector);
	static VectorN from_4d(const Vector4 &p_vector);
	static Vector2 to_2d(const VectorN &p_vector);
	static Vector3 to_3d(const VectorN &p_vector);
	static Vector4 to_4d(const VectorN &p_vector);
	static String to_string(const VectorN &p_vector);

	static VectorND *get_singleton() { return singleton; }
	VectorND() { singleton = this; }
	~VectorND() { singleton = nullptr; }
};
