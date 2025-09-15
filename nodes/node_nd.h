#pragma once

#include "../math/euler_nd.h"
#include "../math/rect_nd.h"
#include "../math/transform_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#elif GODOT_MODULE
#include "scene/main/node.h"
#endif

class NodeND : public Node {
	GDCLASS(NodeND, Node);

public:
	enum DimensionMode {
		DIMENSION_MODE_SQUARE,
		DIMENSION_MODE_NON_SQUARE,
	};

private:
	Ref<EulerND> _rotation_euler;
	Ref<TransformND> _transform;
	DimensionMode _dimension_mode = DIMENSION_MODE_SQUARE;
	bool _is_visible = true;

	void _update_transform_from_euler();

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _validate_property(PropertyInfo &p_property) const;

public:
	// Transform getters and setters.
	Ref<TransformND> get_transform() const;
	void set_transform(const Ref<TransformND> &p_transform);
	Ref<BasisND> get_basis() const;
	void set_basis(const Ref<BasisND> &p_basis);
	Vector<VectorN> get_all_basis_columns() const;
	void set_all_basis_columns(const Vector<VectorN> &p_columns);
	TypedArray<VectorN> get_all_basis_columns_bind() const;
	void set_all_basis_columns_bind(const TypedArray<VectorN> &p_columns);
	VectorN get_basis_flat_array() const;
	void set_basis_flat_array(const VectorN &p_array);

	VectorN get_position() const;
	void set_position(const VectorN &p_position);
	VectorN get_scale_abs() const;
	void set_scale_abs(const VectorN &p_scale);

	int get_euler_rotation_count() const;
	void set_euler_rotation_count(const int p_rotation_count);
	PackedFloat64Array get_euler_rotation_data() const;
	void set_euler_rotation_data(const PackedFloat64Array &p_data);
	Ref<EulerND> get_rotation_euler() const;
	void set_rotation_euler(const Ref<EulerND> &p_euler);

	// Global transform getters and setters.
	Ref<TransformND> get_global_transform() const;
	void set_global_transform(const Ref<TransformND> &p_transform);
	Ref<BasisND> get_global_basis() const;
	void set_global_basis(const Ref<BasisND> &p_basis);

	Ref<TransformND> get_global_transform_expand() const;
	Ref<TransformND> get_global_transform_shrink() const;

	VectorN get_global_position() const;
	void set_global_position(const VectorN &p_global_position);

	// Dimension functions.
	DimensionMode get_dimension_mode() const;
	void set_dimension_mode(const DimensionMode p_dimension_mode);
	int get_dimension() const;
	void set_dimension(const int p_dimension);
	int get_input_dimension() const;
	void set_input_dimension(const int p_input_dimension);
	int get_output_dimension() const;
	void set_output_dimension(const int p_output_dimension);

	// Visibility.
	bool is_visible() const;
	bool is_visible_in_tree() const;
	void set_visible(const bool p_visible);

	// Rect bounds.
	virtual Ref<RectND> get_rect_bounds(const Ref<TransformND> &p_inv_relative_to) const;
	Ref<RectND> get_rect_bounds_recursive(const Ref<TransformND> &p_inv_relative_to) const;
	GDVIRTUAL1RC(Ref<RectND>, _get_rect_bounds, const Ref<TransformND> &);

	NodeND();
};

VARIANT_ENUM_CAST(NodeND::DimensionMode);
