#pragma once

#include "basis_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/ref_counted.hpp>
#elif GODOT_MODULE
#include "core/object/ref_counted.h"
#endif

class TransformND : public RefCounted {
	GDCLASS(TransformND, RefCounted);

	Vector<VectorN> _columns;
	VectorN _origin;

	static void _make_basis_square_in_place(Vector<VectorN> &p_basis);
	static bool _lup_decompose(Vector<VectorN> &p_columns, PackedInt32Array &p_permutations, int p_dimension);
	static Vector<VectorN> _lup_invert(const Vector<VectorN> &p_decomposed, const PackedInt32Array &p_permutations, int p_dimension);

protected:
	static void _bind_methods();

public:
	// Trivial getters and setters.
	Ref<BasisND> get_basis() const;
	void set_basis(const Ref<BasisND> &p_basis);

	Vector<VectorN> get_all_basis_columns() const;
	void set_all_basis_columns(const Vector<VectorN> &p_columns);

	TypedArray<VectorN> get_all_basis_columns_bind() const;
	void set_all_basis_columns_bind(const TypedArray<VectorN> &p_columns);

	VectorN get_basis_column_raw(const int p_index) const;
	VectorN get_basis_column(const int p_index) const;
	void set_basis_column(const int p_index, const VectorN &p_column);

	VectorN get_basis_row(const int p_index) const;
	void set_basis_row(const int p_index, const VectorN &p_row);

	double get_basis_element(const int p_column, const int p_row) const;
	void set_basis_element(const int p_column, const int p_row, const double p_value);

	VectorN get_origin() const;
	void set_origin(const VectorN &p_origin);

	double get_origin_element(const int p_index) const;
	void set_origin_element(const int p_index, const double p_value);

	// Dimension methods.
	int get_basis_column_count() const;
	void set_basis_column_count(const int p_column_count);
	int get_basis_dimension() const;
	void set_basis_dimension(const int p_basis_dimension);
	int get_basis_row_count() const;
	void set_basis_row_count(const int p_row_count);
	int get_dimension() const;
	void set_dimension(const int p_dimension);
	int get_origin_dimension() const;
	void set_origin_dimension(const int p_origin_dimension);
	Ref<TransformND> with_dimension(const int p_dimension) const;

	// Misc methods.
	double determinant() const;
	Ref<TransformND> duplicate() const;
	bool is_equal_approx(const Ref<TransformND> &p_other) const;
	Ref<TransformND> lerp(const Ref<TransformND> &p_to, const double p_weight) const;

	// Transformation methods.
	Ref<TransformND> compose_square(const Ref<TransformND> &p_child_transform) const;
	Ref<TransformND> compose_expand(const Ref<TransformND> &p_child_transform) const;
	Ref<TransformND> compose_shrink(const Ref<TransformND> &p_child_transform) const;
	Ref<TransformND> transform_to(const Ref<TransformND> &p_to) const;

	void translate_global(const VectorN &p_translation);
	void translate_local(const VectorN &p_translation);

	VectorN xform(const VectorN &p_vector) const;
	Vector<VectorN> xform_many(const Vector<VectorN> &p_vectors) const;
	VectorN xform_basis(const VectorN &p_vector) const;
	VectorN xform_basis_axis(const VectorN &p_axis, const int p_axis_index) const;
	VectorN xform_transposed(const VectorN &p_vector) const;
	VectorN xform_transposed_basis(const VectorN &p_vector) const;

	// Inversion methods.
	Ref<TransformND> inverse() const;
	Ref<TransformND> inverse_basis() const;
	Ref<TransformND> inverse_basis_transposed() const;
	Ref<TransformND> inverse_transposed() const;

	// Scale methods.
	VectorN get_global_scale_abs() const;
	VectorN get_scale_abs() const;
	void set_scale_abs(const VectorN &p_scale);
	double get_uniform_scale() const;
	double get_uniform_scale_abs() const;
	void scale_global(const VectorN &p_scale);
	Ref<TransformND> scaled_global(const VectorN &p_scale) const;
	void scale_local(const VectorN &p_scale);
	Ref<TransformND> scaled_local(const VectorN &p_scale) const;
	void scale_uniform(const double p_scale);
	Ref<TransformND> scaled_uniform(const double p_scale) const;

	// Validation methods.
	Ref<TransformND> conformalized() const;
	Ref<TransformND> normalized() const;
	Ref<TransformND> orthonormalized() const;
	Ref<TransformND> orthonormalized_axis_aligned() const;
	Ref<TransformND> orthogonalized() const;
	bool is_conformal() const;
	bool is_diagonal() const;
	bool is_normalized() const;
	bool is_orthogonal() const;
	bool is_orthonormal() const;
	bool is_rotation() const;
	bool is_uniform_scale() const;

	// Trivial math. Not useful by itself, but can be a part of a larger expression.
	Ref<TransformND> add(const Ref<TransformND> &p_other) const;
	Ref<TransformND> divide_scalar(const double p_scalar) const;

	// Conversion.
	Transform2D to_2d();
	Transform3D to_3d();
	Projection to_4d();
	String to_string() MODULE_OVERRIDE;
	static Ref<TransformND> from_2d(const Transform2D &p_transform);
	static Ref<TransformND> from_3d(const Transform3D &p_transform);
	static Ref<TransformND> from_4d(const Projection &p_basis, const Vector4 &p_origin = Vector4(0, 0, 0, 0));

	// Constructors.
	static Ref<TransformND> from_basis_columns(const Vector<VectorN> &p_columns);
	static Ref<TransformND> from_position(const VectorN &p_position);
	static Ref<TransformND> from_position_rotation(const VectorN &p_position, const int p_rot_from, const int p_rot_to, const double p_rot_angle);
	static Ref<TransformND> from_position_rotation_scale(const VectorN &p_position, const int p_rot_from, const int p_rot_to, const double p_rot_angle, const VectorN &p_scale);
	static Ref<TransformND> from_position_scale(const VectorN &p_position, const VectorN &p_scale);
	static Ref<TransformND> from_rotation(const int p_rot_from, const int p_rot_to, const double p_rot_angle);
	static Ref<TransformND> from_rotation_scale(const int p_rot_from, const int p_rot_to, const double p_rot_angle, const VectorN &p_scale);
	static Ref<TransformND> from_scale(const VectorN &p_scale);
	static Ref<TransformND> from_swap_rotation(const int p_rot_from, const int p_rot_to);
	static Ref<TransformND> identity_basis(const int p_dimension);
	static Ref<TransformND> identity_transform(const int p_dimension);
	TransformND() {}
};
