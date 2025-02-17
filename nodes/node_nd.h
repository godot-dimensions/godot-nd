#pragma once

#include "../math/transform_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/node.hpp>
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
	Ref<TransformND> _transform;
	DimensionMode _dimension_mode = DIMENSION_MODE_SQUARE;
	bool _is_visible = true;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	// Transform getters and setters.
	Ref<TransformND> get_transform() const;
	void set_transform(const Ref<TransformND> &p_transform);
	Vector<VectorN> get_all_basis_columns() const;
	void set_all_basis_columns(const Vector<VectorN> &p_columns);
	TypedArray<VectorN> get_all_basis_columns_bind() const;
	void set_all_basis_columns_bind(const TypedArray<VectorN> &p_columns);

	VectorN get_position() const;
	void set_position(const VectorN &p_position);
	VectorN get_scale_abs() const;
	void set_scale_abs(const VectorN &p_scale);

	// Global transform getters and setters.
	Ref<TransformND> get_global_transform() const;
	void set_global_transform(const Ref<TransformND> &p_transform);

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

	NodeND();
};

VARIANT_ENUM_CAST(NodeND::DimensionMode);
