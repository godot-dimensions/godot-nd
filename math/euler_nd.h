#pragma once

#include "transform_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/resource.hpp>
#elif GODOT_MODULE
#include "core/io/resource.h"
#endif

struct EulerRotationND {
	double angle = 0.0;
	int rot_from = 0;
	int rot_to = 1;
};

class EulerND : public Resource {
	GDCLASS(EulerND, Resource);

	Vector<EulerRotationND> _rotations;

	static bool _is_zero_except_indices(const VectorN &p_vector, const int p_index_a, const int p_index_b);

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	static bool does_name_look_like_numbered_rotation_property(const StringName &p_name, int &r_index, String &r_property);
	void get_rotation_property_list(List<PropertyInfo> *p_list) const;

	// Getters and setters.
	Vector<EulerRotationND> get_all_rotations() const { return _rotations; }
	void set_all_rotations(const Vector<EulerRotationND> &p_rotations) { _rotations = p_rotations; }

	PackedFloat64Array get_all_rotation_data() const;
	void set_all_rotation_data(const PackedFloat64Array &p_data);

	EulerRotationND get_rotation(const int p_index) const;
	void set_rotation(const int p_index, const EulerRotationND &p_rotation);

	double get_rotation_angle(const int p_index) const;
	int get_rotation_from(const int p_index) const;
	int get_rotation_to(const int p_index) const;
	void set_rotation_angle(const int p_index, const double p_angle);
	void set_rotation_from(const int p_index, const int p_rot_from);
	void set_rotation_to(const int p_index, const int p_rot_to);

	PackedFloat64Array get_rotation_angle_array() const;
	PackedInt32Array get_rotation_from_array() const;
	PackedInt32Array get_rotation_to_array() const;
	void set_rotation_angle_array(const PackedFloat64Array &p_angle);
	void set_rotation_from_array(const PackedInt32Array &p_rot_from);
	void set_rotation_to_array(const PackedInt32Array &p_rot_to);

	// Count methods.
	int get_dimension() const;
	int get_rotation_count() const { return _rotations.size(); }
	void set_rotation_count(const int p_rotation_count);
	void set_rotation_count_no_signal(const int p_rotation_count);

	// Misc methods.
	void append_rotation(const EulerRotationND &p_rotation);
	bool is_equal_approx(const Ref<EulerND> &p_other) const;
	bool is_equal_exact(const Ref<EulerND> &p_other) const;
	Ref<BasisND> rotate_basis(const Ref<BasisND> &p_basis) const;
	VectorN rotate_point(const VectorN &p_point) const;
	Ref<EulerND> snapped(const double p_step) const;
	Ref<EulerND> wrapped() const;

	// Radians/degrees.
	Ref<EulerND> deg_to_rad() const;
	Ref<EulerND> rad_to_deg() const;

	// Conversion methods.
	Ref<BasisND> to_rotation_basis() const;
	Ref<TransformND> to_rotation_transform() const;
	void set_rotation_of_basis(const Ref<BasisND> &p_basis);
	void set_rotation_of_transform(const Ref<TransformND> &p_transform);

	static Ref<EulerND> decompose_simple_rotations(const Vector<VectorN> &p_columns);
	static Ref<EulerND> decompose_simple_rotations_from_basis(const Ref<BasisND> &p_basis);
	static Ref<EulerND> decompose_simple_rotations_from_transform(const Ref<TransformND> &p_transform);
	void set_from_decomposed_simple_rotations(const Vector<VectorN> &p_columns);
	void set_from_decomposed_simple_rotations_from_basis(const Ref<BasisND> &p_basis);
	void set_from_decomposed_simple_rotations_from_transform(const Ref<TransformND> &p_transform);
};
