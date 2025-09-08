#include "node_nd.h"

// Transform getters and setters.

Ref<TransformND> NodeND::get_transform() const {
	return _transform;
}

void NodeND::set_transform(const Ref<TransformND> &p_transform) {
	_transform = p_transform;
}

Ref<BasisND> NodeND::get_basis() const {
	return _transform->get_basis();
}

void NodeND::set_basis(const Ref<BasisND> &p_basis) {
	_transform->set_basis(p_basis);
}

Vector<VectorN> NodeND::get_all_basis_columns() const {
	return _transform->get_all_basis_columns();
}

void NodeND::set_all_basis_columns(const Vector<VectorN> &p_columns) {
	_transform->set_all_basis_columns(p_columns);
}

TypedArray<VectorN> NodeND::get_all_basis_columns_bind() const {
	return _transform->get_all_basis_columns_bind();
}

void NodeND::set_all_basis_columns_bind(const TypedArray<VectorN> &p_columns) {
	_transform->set_all_basis_columns_bind(p_columns);
}

VectorN NodeND::get_position() const {
	return _transform->get_origin();
}

void NodeND::set_position(const VectorN &p_position) {
	_transform->set_origin(p_position);
}

VectorN NodeND::get_scale_abs() const {
	return _transform->get_scale_abs();
}

void NodeND::set_scale_abs(const VectorN &p_scale) {
	_transform->set_scale_abs(p_scale);
}

// Global transform getters and setters.

Ref<TransformND> NodeND::get_global_transform() const {
	NodeND *node_nd_parent = Object::cast_to<NodeND>(get_parent());
	if (node_nd_parent) {
		return node_nd_parent->get_global_transform()->compose_square(_transform);
	} else {
		return _transform;
	}
}

void NodeND::set_global_transform(const Ref<TransformND> &p_transform) {
	NodeND *node_nd_parent = Object::cast_to<NodeND>(get_parent());
	if (node_nd_parent) {
		set_transform(node_nd_parent->get_global_transform()->inverse()->compose_square(p_transform));
	} else {
		set_transform(p_transform);
	}
}

Ref<TransformND> NodeND::get_global_transform_expand() const {
	NodeND *node_nd_parent = Object::cast_to<NodeND>(get_parent());
	if (node_nd_parent) {
		return node_nd_parent->get_global_transform_expand()->compose_expand(_transform);
	} else {
		return _transform;
	}
}

Ref<TransformND> NodeND::get_global_transform_shrink() const {
	NodeND *node_nd_parent = Object::cast_to<NodeND>(get_parent());
	if (node_nd_parent) {
		return node_nd_parent->get_global_transform_shrink()->compose_shrink(_transform);
	} else {
		return _transform;
	}
}

VectorN NodeND::get_global_position() const {
	return get_global_transform()->get_origin();
}

void NodeND::set_global_position(const VectorN &p_global_position) {
	Ref<TransformND> global_xform = get_global_transform();
	global_xform->set_origin(p_global_position);
	set_global_transform(global_xform);
}

// Dimension functions.

NodeND::DimensionMode NodeND::get_dimension_mode() const {
	return _dimension_mode;
}

void NodeND::set_dimension_mode(const DimensionMode p_dimension_mode) {
	_dimension_mode = p_dimension_mode;
	if (_dimension_mode == DIMENSION_MODE_SQUARE) {
		set_dimension(get_dimension());
	}
	notify_property_list_changed();
}

int NodeND::get_dimension() const {
	return _transform->get_dimension();
}

void NodeND::set_dimension(const int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "NodeND: Dimension cannot be negative.");
	_transform->set_dimension(p_dimension);
	emit_signal("dimension_changed");
}

int NodeND::get_input_dimension() const {
	return _transform->get_basis_column_count();
}

void NodeND::set_input_dimension(const int p_input_dimension) {
	_transform->set_basis_column_count(p_input_dimension);
	emit_signal("dimension_changed");
}

int NodeND::get_output_dimension() const {
	return _transform->get_origin_dimension();
}

void NodeND::set_output_dimension(const int p_output_dimension) {
	_transform->set_origin_dimension(p_output_dimension);
	emit_signal("dimension_changed");
}

// Visibility.

bool NodeND::is_visible() const {
	return _is_visible;
}

bool NodeND::is_visible_in_tree() const {
	if (!_is_visible) {
		return false;
	}
	NodeND *node_nd_parent = Object::cast_to<NodeND>(get_parent());
	if (node_nd_parent) {
		return node_nd_parent->is_visible_in_tree();
	}
	return true;
}

void NodeND::set_visible(const bool p_visible) {
	_is_visible = p_visible;
}

// Rect bounds.

Ref<RectND> NodeND::get_rect_bounds(const Ref<TransformND> &p_inv_relative_to) const {
	Ref<RectND> result;
	GDVIRTUAL_CALL(_get_rect_bounds, p_inv_relative_to, result);
	return result;
}

Ref<RectND> NodeND::get_rect_bounds_recursive(const Ref<TransformND> &p_inv_relative_to) const {
	Ref<RectND> bounds = get_rect_bounds(p_inv_relative_to);
	if (bounds.is_null()) {
		bounds.instantiate();
	}
	const int child_count = get_child_count();
	for (int i = 0; i < child_count; i++) {
		NodeND *child_nd = Object::cast_to<NodeND>(get_child(i));
		if (child_nd != nullptr) {
			bounds = bounds->merge(child_nd->get_rect_bounds_recursive(p_inv_relative_to));
		}
	}
	return bounds;
}

void NodeND::_bind_methods() {
	// Transform getters and setters.
	ClassDB::bind_method(D_METHOD("get_transform"), &NodeND::get_transform);
	ClassDB::bind_method(D_METHOD("set_transform", "transform"), &NodeND::set_transform);
	ClassDB::bind_method(D_METHOD("get_basis"), &NodeND::get_basis);
	ClassDB::bind_method(D_METHOD("set_basis", "basis"), &NodeND::set_basis);
	ClassDB::bind_method(D_METHOD("get_all_basis_columns"), &NodeND::get_all_basis_columns_bind);
	ClassDB::bind_method(D_METHOD("set_all_basis_columns", "columns"), &NodeND::set_all_basis_columns_bind);
	ClassDB::bind_method(D_METHOD("get_position"), &NodeND::get_position);
	ClassDB::bind_method(D_METHOD("set_position", "position"), &NodeND::set_position);
	ClassDB::bind_method(D_METHOD("get_scale_abs"), &NodeND::get_scale_abs);
	ClassDB::bind_method(D_METHOD("set_scale_abs", "scale"), &NodeND::set_scale_abs);
	// Global transform getters and setters.
	ClassDB::bind_method(D_METHOD("get_global_transform"), &NodeND::get_global_transform);
	ClassDB::bind_method(D_METHOD("set_global_transform", "global_transform"), &NodeND::set_global_transform);
	ClassDB::bind_method(D_METHOD("get_global_transform_expand"), &NodeND::get_global_transform_expand);
	ClassDB::bind_method(D_METHOD("get_global_transform_shrink"), &NodeND::get_global_transform_shrink);
	ClassDB::bind_method(D_METHOD("get_global_position"), &NodeND::get_global_position);
	ClassDB::bind_method(D_METHOD("set_global_position", "global_position"), &NodeND::set_global_position);
	// Dimension functions.
	ClassDB::bind_method(D_METHOD("get_dimension_mode"), &NodeND::get_dimension_mode);
	ClassDB::bind_method(D_METHOD("set_dimension_mode", "dimension_mode"), &NodeND::set_dimension_mode);
	ClassDB::bind_method(D_METHOD("get_dimension"), &NodeND::get_dimension);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &NodeND::set_dimension);
	ClassDB::bind_method(D_METHOD("get_input_dimension"), &NodeND::get_input_dimension);
	ClassDB::bind_method(D_METHOD("set_input_dimension", "input_dimension"), &NodeND::set_input_dimension);
	ClassDB::bind_method(D_METHOD("get_output_dimension"), &NodeND::get_output_dimension);
	ClassDB::bind_method(D_METHOD("set_output_dimension", "output_dimension"), &NodeND::set_output_dimension);
	// Visibility.
	ClassDB::bind_method(D_METHOD("is_visible"), &NodeND::is_visible);
	ClassDB::bind_method(D_METHOD("is_visible_in_tree"), &NodeND::is_visible_in_tree);
	ClassDB::bind_method(D_METHOD("set_visible", "visible"), &NodeND::set_visible);
	// Properties.
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension_mode", PROPERTY_HINT_ENUM, "Square,Non-Square"), "set_dimension_mode", "get_dimension_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "input_dimension", PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_EDITOR), "set_input_dimension", "get_input_dimension");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output_dimension", PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_EDITOR), "set_output_dimension", "get_output_dimension");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "transform", PROPERTY_HINT_RESOURCE_TYPE, "TransformND", PROPERTY_USAGE_NONE), "set_transform", "get_transform");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "position", PROPERTY_HINT_NONE, "suffix:m"), "set_position", "get_position");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "scale_abs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_scale_abs", "get_scale_abs");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "basis", PROPERTY_HINT_RESOURCE_TYPE, "BasisND", PROPERTY_USAGE_NONE), "set_basis", "get_basis");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "basis_columns", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat64Array", PROPERTY_USAGE_STORAGE), "set_all_basis_columns", "get_all_basis_columns");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "global_transform", PROPERTY_HINT_RESOURCE_TYPE, "TransformND", PROPERTY_USAGE_NONE), "set_global_transform", "get_global_transform");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "global_position", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_global_position", "get_global_position");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "visible"), "set_visible", "is_visible");

	BIND_ENUM_CONSTANT(DIMENSION_MODE_SQUARE);
	BIND_ENUM_CONSTANT(DIMENSION_MODE_NON_SQUARE);

	ADD_SIGNAL(MethodInfo("dimension_changed"));
}

void NodeND::_validate_property(PropertyInfo &p_property) const {
	if (_dimension_mode == DIMENSION_MODE_SQUARE) {
		if (p_property.name == StringName("input_dimension")) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
		if (p_property.name == StringName("output_dimension")) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else {
		if (p_property.name == StringName("dimension")) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	}
}

NodeND::NodeND() {
	_transform.instantiate();
}
