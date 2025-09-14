#pragma once

#include "../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#elif GODOT_MODULE
#include "core/object/ref_counted.h"
#include "core/variant/typed_array.h"
#endif

class BasisND : public RefCounted {
	GDCLASS(BasisND, RefCounted);

	Vector<VectorN> _columns;

	static void _make_basis_square_in_place(Vector<VectorN> &p_basis);
	static bool _lup_decompose(Vector<VectorN> &p_columns, PackedInt32Array &p_permutations, int p_dimension);
	static Vector<VectorN> _lup_invert(const Vector<VectorN> &p_decomposed, const PackedInt32Array &p_permutations, int p_dimension);

protected:
	static void _bind_methods();

public:
	// Getters and setters.
	Vector<VectorN> get_all_columns() const;
	void set_all_columns(const Vector<VectorN> &p_columns);

	TypedArray<VectorN> get_all_columns_bind() const;
	void set_all_columns_bind(const TypedArray<VectorN> &p_columns);

	VectorN get_flat_array() const;
	void set_flat_array(const VectorN &p_array);

	VectorN get_column_raw(const int p_index) const;
	VectorN get_column(const int p_index) const;
	void set_column(const int p_index, const VectorN &p_column);

	VectorN get_row(const int p_index) const;
	void set_row(const int p_index, const VectorN &p_row);

	double get_element(const int p_column, const int p_row) const;
	void set_element(const int p_column, const int p_row, const double p_value);

	// Dimension methods.
	int get_column_count() const;
	void set_column_count(const int p_column_count);
	int get_dimension() const;
	void set_dimension(const int p_dimension);
	int get_row_count() const;
	void set_row_count(const int p_row_count);
	Ref<BasisND> with_dimension(const int p_dimension) const;

	// Misc methods.
	double determinant() const;
	Ref<BasisND> duplicate() const;
	bool is_equal_approx(const Ref<BasisND> &p_other) const;
	Ref<BasisND> lerp(const Ref<BasisND> &p_to, const double p_weight) const;

	// Transformation methods.
	Ref<BasisND> compose_square(const Ref<BasisND> &p_child_transform) const;
	Ref<BasisND> compose_expand(const Ref<BasisND> &p_child_transform) const;
	Ref<BasisND> compose_shrink(const Ref<BasisND> &p_child_transform) const;
	Ref<BasisND> transform_to(const Ref<BasisND> &p_to) const;

	VectorN xform(const VectorN &p_vector) const;
	Vector<VectorN> xform_many(const Vector<VectorN> &p_vectors) const;
	VectorN xform_axis(const VectorN &p_axis, const int p_axis_index) const;
	VectorN xform_transposed(const VectorN &p_vector) const;

	// Inversion methods.
	Ref<BasisND> inverse() const;
	Ref<BasisND> inverse_transposed() const;

	// Scale methods.
	VectorN get_global_scale_abs() const;
	VectorN get_scale_abs() const;
	void set_scale_abs(const VectorN &p_scale);
	double get_uniform_scale() const;
	double get_uniform_scale_abs() const;
	void scale_global(const VectorN &p_scale);
	Ref<BasisND> scaled_global(const VectorN &p_scale) const;
	void scale_local(const VectorN &p_scale);
	Ref<BasisND> scaled_local(const VectorN &p_scale) const;
	void scale_uniform(const double p_scale);
	Ref<BasisND> scaled_uniform(const double p_scale) const;

	// Validation methods.
	Ref<BasisND> conformalized() const;
	Ref<BasisND> normalized() const;
	Ref<BasisND> orthonormalized() const;
	Ref<BasisND> orthonormalized_axis_aligned() const;
	Ref<BasisND> orthogonalized() const;
	bool is_conformal() const;
	bool is_diagonal() const;
	bool is_normalized() const;
	bool is_orthogonal() const;
	bool is_orthonormal() const;
	bool is_rotation() const;
	bool is_uniform_scale() const;

	// Trivial math. Not useful by itself, but can be a part of a larger expression.
	Ref<BasisND> add(const Ref<BasisND> &p_other) const;
	Ref<BasisND> divide_scalar(const double p_scalar) const;

	// Conversion.
	Transform2D to_2d();
	Basis to_3d();
	Projection to_4d();
	String to_string() MODULE_OVERRIDE;
	static Ref<BasisND> from_2d(const Transform2D &p_transform);
	static Ref<BasisND> from_3d(const Basis &p_basis);
	static Ref<BasisND> from_4d(const Projection &p_basis);

	// Constructors.
	static Ref<BasisND> from_basis_columns(const Vector<VectorN> &p_columns);
	static Ref<BasisND> from_rotation(const int p_rot_from, const int p_rot_to, const double p_rot_angle);
	static Ref<BasisND> from_rotation_scale(const int p_rot_from, const int p_rot_to, const double p_rot_angle, const VectorN &p_scale);
	static Ref<BasisND> from_scale(const VectorN &p_scale);
	static Ref<BasisND> from_swap_rotation(const int p_rot_from, const int p_rot_to);
	static Ref<BasisND> identity(const int p_dimension);
	BasisND() {}
};
