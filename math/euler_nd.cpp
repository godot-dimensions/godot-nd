#include "euler_nd.h"

// Getters and setters.
PackedFloat64Array EulerND::get_all_rotation_data() const {
	PackedFloat64Array data;
	data.resize(_rotations.size() * 3);
	for (int i = 0; i < _rotations.size(); i++) {
		data.set(i * 3 + 0, _rotations[i].rot_from);
		data.set(i * 3 + 1, _rotations[i].rot_to);
		data.set(i * 3 + 2, _rotations[i].angle);
	}
	return data;
}

void EulerND::set_all_rotation_data(const PackedFloat64Array &p_data) {
	ERR_FAIL_COND(p_data.size() % 3 != 0);
	const int rotation_count = p_data.size() / 3;
	set_rotation_count_no_signal(rotation_count);
	for (int i = 0; i < rotation_count; i++) {
		EulerRotationND rotation;
		rotation.rot_from = (int)p_data[i * 3 + 0];
		rotation.rot_to = (int)p_data[i * 3 + 1];
		rotation.angle = p_data[i * 3 + 2];
		_rotations.set(i, rotation);
	}
	notify_property_list_changed();
	emit_signal("rotation_changed");
}

EulerRotationND EulerND::get_rotation(const int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _rotations.size(), EulerRotationND());
	return _rotations[p_index];
}

void EulerND::set_rotation(const int p_index, const EulerRotationND &p_rotation) {
	ERR_FAIL_INDEX(p_index, _rotations.size());
	_rotations.set(p_index, p_rotation);
	emit_signal("rotation_changed");
}

double EulerND::get_rotation_angle(const int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _rotations.size(), 0.0);
	return _rotations[p_index].angle;
}

int EulerND::get_rotation_from(const int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _rotations.size(), 0);
	return _rotations[p_index].rot_from;
}

int EulerND::get_rotation_to(const int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _rotations.size(), 0);
	return _rotations[p_index].rot_to;
}

void EulerND::set_rotation_angle(const int p_index, const double p_angle) {
	ERR_FAIL_INDEX(p_index, _rotations.size());
	EulerRotationND rotation = _rotations[p_index];
	rotation.angle = p_angle;
	_rotations.set(p_index, rotation);
	emit_signal("rotation_changed");
}

void EulerND::set_rotation_from(const int p_index, const int p_rot_from) {
	ERR_FAIL_INDEX(p_index, _rotations.size());
	EulerRotationND rotation = _rotations[p_index];
	rotation.rot_from = p_rot_from;
	_rotations.set(p_index, rotation);
	emit_signal("rotation_changed");
}

void EulerND::set_rotation_to(const int p_index, const int p_rot_to) {
	ERR_FAIL_INDEX(p_index, _rotations.size());
	EulerRotationND rotation = _rotations[p_index];
	rotation.rot_to = p_rot_to;
	_rotations.set(p_index, rotation);
	emit_signal("rotation_changed");
}

PackedFloat64Array EulerND::get_rotation_angle_array() const {
	PackedFloat64Array angles;
	angles.resize(_rotations.size());
	for (int i = 0; i < _rotations.size(); i++) {
		angles.set(i, _rotations[i].angle);
	}
	return angles;
}

PackedInt32Array EulerND::get_rotation_from_array() const {
	PackedInt32Array rot_from;
	rot_from.resize(_rotations.size());
	for (int i = 0; i < _rotations.size(); i++) {
		rot_from.set(i, _rotations[i].rot_from);
	}
	return rot_from;
}

PackedInt32Array EulerND::get_rotation_to_array() const {
	PackedInt32Array rot_to;
	rot_to.resize(_rotations.size());
	for (int i = 0; i < _rotations.size(); i++) {
		rot_to.set(i, _rotations[i].rot_to);
	}
	return rot_to;
}

void EulerND::set_rotation_angle_array(const PackedFloat64Array &p_angle) {
	if (p_angle.size() > _rotations.size()) {
		set_rotation_count_no_signal(p_angle.size());
	}
	for (int i = 0; i < _rotations.size(); i++) {
		EulerRotationND rotation = _rotations[i];
		rotation.angle = p_angle[i];
		_rotations.set(i, rotation);
	}
	notify_property_list_changed();
	emit_signal("rotation_changed");
}

void EulerND::set_rotation_from_array(const PackedInt32Array &p_rot_from) {
	if (p_rot_from.size() > _rotations.size()) {
		set_rotation_count_no_signal(p_rot_from.size());
	}
	for (int i = 0; i < _rotations.size(); i++) {
		EulerRotationND rotation = _rotations[i];
		rotation.rot_from = p_rot_from[i];
		_rotations.set(i, rotation);
	}
	notify_property_list_changed();
	emit_signal("rotation_changed");
}

void EulerND::set_rotation_to_array(const PackedInt32Array &p_rot_to) {
	if (p_rot_to.size() > _rotations.size()) {
		set_rotation_count_no_signal(p_rot_to.size());
	}
	for (int i = 0; i < _rotations.size(); i++) {
		EulerRotationND rotation = _rotations[i];
		rotation.rot_to = p_rot_to[i];
		_rotations.set(i, rotation);
	}
	notify_property_list_changed();
	emit_signal("rotation_changed");
}

// Count methods.
int EulerND::get_dimension() const {
	int max_rot_from_to = -1;
	for (int i = 0; i < _rotations.size(); i++) {
		max_rot_from_to = MAX(max_rot_from_to, MAX(_rotations[i].rot_from, _rotations[i].rot_to));
	}
	return max_rot_from_to + 1;
}

void EulerND::set_rotation_count(const int p_rotation_count) {
	ERR_FAIL_COND(p_rotation_count < 0);
	set_rotation_count_no_signal(p_rotation_count);
	notify_property_list_changed();
	emit_signal("rotation_changed");
}

void EulerND::set_rotation_count_no_signal(const int p_rotation_count) {
	ERR_FAIL_COND(p_rotation_count < 0);
	const int old_rotation_count = _rotations.size();
	_rotations.resize(p_rotation_count);
	for (int i = old_rotation_count; i < p_rotation_count; i++) {
		_rotations.set(i, EulerRotationND());
	}
}

// Misc methods.
void EulerND::append_rotation(const EulerRotationND &p_rotation) {
	_rotations.append(p_rotation);
	notify_property_list_changed();
	emit_signal("rotation_changed");
}

bool EulerND::is_equal_approx(const Ref<EulerND> &p_other) const {
	if (_rotations.size() != p_other->get_rotation_count()) {
		return false;
	}
	for (int i = 0; i < _rotations.size(); i++) {
		const EulerRotationND &a = _rotations[i];
		const EulerRotationND &b = p_other->get_rotation(i);
		if (a.rot_from == b.rot_from && a.rot_to == b.rot_to) {
			if (!Math::is_equal_approx(a.angle, b.angle)) {
				return false;
			}
		} else if (a.rot_from == b.rot_to && a.rot_to == b.rot_from) {
			if (!Math::is_equal_approx(a.angle, -b.angle)) {
				return false;
			}
		} else {
			return false;
		}
	}
	return true;
}

bool EulerND::is_equal_exact(const Ref<EulerND> &p_other) const {
	if (_rotations.size() != p_other->get_rotation_count()) {
		return false;
	}
	for (int i = 0; i < _rotations.size(); i++) {
		const EulerRotationND &a = _rotations[i];
		const EulerRotationND &b = p_other->get_rotation(i);
		if (a.rot_from != b.rot_from || a.rot_to != b.rot_to || a.angle != b.angle) {
			return false;
		}
	}
	return true;
}

Ref<BasisND> EulerND::rotate_basis(const Ref<BasisND> &p_basis) const {
	return to_rotation_basis()->compose_square(p_basis);
}

VectorN EulerND::rotate_point(const VectorN &p_point) const {
	return to_rotation_basis()->xform(p_point);
}

Ref<EulerND> EulerND::snapped(const double p_step) const {
	Ref<EulerND> euler = duplicate();
	for (int i = 0; i < euler->get_rotation_count(); i++) {
		euler->set_rotation_angle(i, Math::snapped(euler->get_rotation_angle(i), p_step));
	}
	return euler;
}

Ref<EulerND> EulerND::wrapped() const {
	Ref<EulerND> euler = duplicate();
	for (int i = 0; i < euler->get_rotation_count(); i++) {
		euler->set_rotation_angle(i, Math::wrapf(euler->get_rotation_angle(i), -Math_PI, Math_PI));
	}
	return euler;
}

// Radians/degrees.
Ref<EulerND> EulerND::deg_to_rad() const {
	Ref<EulerND> euler = duplicate();
	for (EulerRotationND &rotation : euler->get_all_rotations()) {
		rotation.angle = Math::deg_to_rad(rotation.angle);
	}
	return euler;
}

Ref<EulerND> EulerND::rad_to_deg() const {
	Ref<EulerND> euler = duplicate();
	for (EulerRotationND &rotation : euler->get_all_rotations()) {
		rotation.angle = Math::rad_to_deg(rotation.angle);
	}
	return euler;
}

// Conversion methods.
Ref<BasisND> EulerND::to_rotation_basis() const {
	Ref<BasisND> basis;
	basis.instantiate();
	for (int i = 0; i < _rotations.size(); i++) {
		const EulerRotationND &rotation = _rotations[i];
		basis = basis->compose_square(BasisND::from_rotation(rotation.rot_from, rotation.rot_to, rotation.angle));
	}
	return basis;
}

Ref<TransformND> EulerND::to_rotation_transform() const {
	Ref<TransformND> transform;
	transform->set_basis(to_rotation_basis());
	return transform;
}

void EulerND::set_rotation_of_basis(const Ref<BasisND> &p_basis) {
	Ref<BasisND> new_basis = to_rotation_basis();
	const int input_dim = p_basis->get_dimension();
	if (input_dim > new_basis->get_dimension()) {
		new_basis->set_dimension(input_dim);
	}
	if (input_dim != 0) {
		new_basis->scale_local(p_basis->get_scale_abs());
	}
	p_basis->set_all_columns(new_basis->get_all_columns());
}

void EulerND::set_rotation_of_transform(const Ref<TransformND> &p_transform) {
	Ref<BasisND> new_basis = to_rotation_basis();
	const int input_dim = p_transform->get_basis_dimension();
	if (input_dim > new_basis->get_dimension()) {
		new_basis->set_dimension(input_dim);
	}
	if (input_dim != 0) {
		new_basis->scale_local(p_transform->get_scale_abs());
	}
	p_transform->set_basis(new_basis);
}

bool EulerND::_is_zero_except_indices(const VectorN &p_vector, const int p_index_a, const int p_index_b) {
	for (int i = 0; i < p_vector.size(); i++) {
		if (i != p_index_a && i != p_index_b) {
			if (!Math::is_zero_approx(p_vector[i])) {
				return false;
			}
		}
	}
	return true;
}

Ref<EulerND> EulerND::decompose_simple_rotations(const Vector<VectorN> &p_columns) {
	Ref<EulerND> euler;
	euler.instantiate();
	for (int64_t col_index = 0; col_index < p_columns.size(); col_index++) {
		const VectorN &column = p_columns[col_index];
		const int64_t max_row_iter = MIN(p_columns.size(), column.size());
		for (int64_t row_index = col_index + 1; row_index < max_row_iter; row_index++) {
			const double element = column[row_index];
			if (!Math::is_zero_approx(element)) {
				const VectorN &other_column = p_columns[row_index];
				const bool is_self_simple = _is_zero_except_indices(column, col_index, row_index);
				const bool is_other_simple = _is_zero_except_indices(other_column, col_index, row_index);
				if (is_self_simple && is_other_simple) {
					EulerRotationND rotation;
					if ((row_index - col_index) % 2 == 0) {
						// Even permutation, swap and use negative angle. Example: ZX rotation.
						rotation.rot_from = row_index;
						rotation.rot_to = col_index;
						rotation.angle = -Math::atan2(element, column[col_index]);
					} else {
						// Odd permutation, keep and use positive angle. Example: XY rotation.
						rotation.rot_from = col_index;
						rotation.rot_to = row_index;
						rotation.angle = Math::atan2(element, column[col_index]);
					}
					euler->append_rotation(rotation);
				}
				// Either we added a simple rotation and so we are done here,
				// or we found a complex rotation and skip it.
				break;
			}
		}
	}
	return euler;
}

Ref<EulerND> EulerND::decompose_simple_rotations_from_basis(const Ref<BasisND> &p_basis) {
	return decompose_simple_rotations(p_basis->get_all_columns());
}

Ref<EulerND> EulerND::decompose_simple_rotations_from_transform(const Ref<TransformND> &p_transform) {
	return decompose_simple_rotations(p_transform->get_all_basis_columns());
}

void EulerND::set_from_decomposed_simple_rotations(const Vector<VectorN> &p_columns) {
	Ref<EulerND> temp_euler = decompose_simple_rotations(p_columns);
	Vector<EulerRotationND> temp_rotations = temp_euler->get_all_rotations();
	// Try to reuse existing rotations as much as possible.
	const int64_t rotation_count = temp_rotations.size();
	set_rotation_count_no_signal(rotation_count);
	PackedInt32Array emplaced_self_indices;
	PackedInt32Array emplaced_temp_indices;
	// If any existing rotations have the same from/to as a new one, reuse it.
	for (int64_t temp_index = 0; temp_index < rotation_count; temp_index++) {
		const EulerRotationND &temp_rotation = temp_rotations[temp_index];
		for (int64_t existing_index = 0; existing_index < rotation_count; existing_index++) {
			if (emplaced_temp_indices.has(existing_index)) {
				continue;
			}
			const EulerRotationND &existing_rotation = _rotations[existing_index];
			if (temp_rotation.rot_from == existing_rotation.rot_from && temp_rotation.rot_to == existing_rotation.rot_to) {
				_rotations.set(existing_index, temp_rotation);
				emplaced_self_indices.append(existing_index);
				emplaced_temp_indices.append(temp_index);
				break;
			}
		}
	}
	// Add any remaining new rotations to non-emplaced slots.
	for (int64_t temp_index = 0; temp_index < rotation_count; temp_index++) {
		if (emplaced_temp_indices.has(temp_index)) {
			continue;
		}
		const EulerRotationND &temp_rotation = temp_rotations[temp_index];
		for (int64_t existing_index = 0; existing_index < rotation_count; existing_index++) {
			if (emplaced_self_indices.has(existing_index)) {
				continue;
			}
			_rotations.set(existing_index, temp_rotation);
			emplaced_self_indices.append(existing_index);
			emplaced_temp_indices.append(temp_index);
			break;
		}
	}
	notify_property_list_changed();
	emit_signal("rotation_changed");
}

void EulerND::set_from_decomposed_simple_rotations_from_basis(const Ref<BasisND> &p_basis) {
	set_from_decomposed_simple_rotations(p_basis->get_all_columns());
}

void EulerND::set_from_decomposed_simple_rotations_from_transform(const Ref<TransformND> &p_transform) {
	set_from_decomposed_simple_rotations(p_transform->get_all_basis_columns());
}

bool EulerND::does_name_look_like_numbered_rotation_property(const StringName &p_name, int &r_index, String &r_property) {
	String name_str = p_name;
	if (!name_str.begins_with("rotation_") || !name_str.contains("/")) {
		return false;
	}
	name_str = name_str.substr(9, name_str.length() - 9); // "rotation_"
	const PackedStringArray split = name_str.split("/");
	if (split.size() != 2) {
		return false;
	}
	r_index = split[0].to_int();
	r_property = split[1];
	return true;
}

void EulerND::get_rotation_property_list(List<PropertyInfo> *p_list) const {
	for (int i = 0; i < _rotations.size(); i++) {
		const String rotation_prefix = String("rotation_") + itos(i) + String("/");
		p_list->push_back(PropertyInfo(Variant::FLOAT, rotation_prefix + String("angle"), PROPERTY_HINT_RANGE, "-360,360,0.001,radians_as_degrees", PROPERTY_USAGE_EDITOR));
		p_list->push_back(PropertyInfo(Variant::INT, rotation_prefix + String("from"), PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_EDITOR));
		p_list->push_back(PropertyInfo(Variant::INT, rotation_prefix + String("to"), PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_EDITOR));
	}
}

void EulerND::_bind_methods() {
	// Getters and setters.
	ClassDB::bind_method(D_METHOD("get_all_rotation_data"), &EulerND::get_all_rotation_data);
	ClassDB::bind_method(D_METHOD("set_all_rotation_data", "data"), &EulerND::set_all_rotation_data);
	ClassDB::bind_method(D_METHOD("get_rotation_angle", "index"), &EulerND::get_rotation_angle);
	ClassDB::bind_method(D_METHOD("get_rotation_from", "index"), &EulerND::get_rotation_from);
	ClassDB::bind_method(D_METHOD("get_rotation_to", "index"), &EulerND::get_rotation_to);
	ClassDB::bind_method(D_METHOD("set_rotation_angle", "index", "angle"), &EulerND::set_rotation_angle);
	ClassDB::bind_method(D_METHOD("set_rotation_from", "index", "rot_from"), &EulerND::set_rotation_from);
	ClassDB::bind_method(D_METHOD("set_rotation_to", "index", "rot_to"), &EulerND::set_rotation_to);
	ClassDB::bind_method(D_METHOD("get_rotation_angle_array"), &EulerND::get_rotation_angle_array);
	ClassDB::bind_method(D_METHOD("get_rotation_from_array"), &EulerND::get_rotation_from_array);
	ClassDB::bind_method(D_METHOD("get_rotation_to_array"), &EulerND::get_rotation_to_array);
	ClassDB::bind_method(D_METHOD("set_rotation_angle_array", "angle"), &EulerND::set_rotation_angle_array);
	ClassDB::bind_method(D_METHOD("set_rotation_from_array", "rot_from"), &EulerND::set_rotation_from_array);
	ClassDB::bind_method(D_METHOD("set_rotation_to_array", "rot_to"), &EulerND::set_rotation_to_array);
	// Count methods.
	ClassDB::bind_method(D_METHOD("get_dimension"), &EulerND::get_dimension);
	ClassDB::bind_method(D_METHOD("get_rotation_count"), &EulerND::get_rotation_count);
	ClassDB::bind_method(D_METHOD("set_rotation_count", "rotation_count"), &EulerND::set_rotation_count);
	// Misc methods.
	ClassDB::bind_method(D_METHOD("is_equal_approx", "other"), &EulerND::is_equal_approx);
	ClassDB::bind_method(D_METHOD("is_equal_exact", "other"), &EulerND::is_equal_exact);
	ClassDB::bind_method(D_METHOD("rotate_basis", "basis"), &EulerND::rotate_basis);
	ClassDB::bind_method(D_METHOD("rotate_point", "point"), &EulerND::rotate_point);
	ClassDB::bind_method(D_METHOD("snapped", "step"), &EulerND::snapped);
	ClassDB::bind_method(D_METHOD("wrapped"), &EulerND::wrapped);
	// Radians/degrees.
	ClassDB::bind_method(D_METHOD("deg_to_rad"), &EulerND::deg_to_rad);
	ClassDB::bind_method(D_METHOD("rad_to_deg"), &EulerND::rad_to_deg);
	// Conversion methods.
	ClassDB::bind_method(D_METHOD("to_rotation_basis"), &EulerND::to_rotation_basis);
	ClassDB::bind_method(D_METHOD("to_rotation_transform"), &EulerND::to_rotation_transform);
	ClassDB::bind_method(D_METHOD("set_rotation_of_basis", "basis"), &EulerND::set_rotation_of_basis);
	ClassDB::bind_method(D_METHOD("set_rotation_of_transform", "transform"), &EulerND::set_rotation_of_transform);
	ClassDB::bind_static_method("EulerND", D_METHOD("decompose_simple_rotations_from_basis", "basis"), &EulerND::decompose_simple_rotations_from_basis);
	ClassDB::bind_static_method("EulerND", D_METHOD("decompose_simple_rotations_from_transform", "transform"), &EulerND::decompose_simple_rotations_from_transform);
	// Properties.
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "all_rotation_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_all_rotation_data", "get_all_rotation_data");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rotation_count", PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_EDITOR), "set_rotation_count", "get_rotation_count");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "rotation_angle_array", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_rotation_angle_array", "get_rotation_angle_array");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "rotation_from_array", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_rotation_from_array", "get_rotation_from_array");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "rotation_to_array", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_rotation_to_array", "get_rotation_to_array");
	ADD_SIGNAL(MethodInfo("rotation_changed"));
}

bool EulerND::_set(const StringName &p_name, const Variant &p_value) {
	int index;
	String property;
	bool looks_like_rotation_property = does_name_look_like_numbered_rotation_property(p_name, index, property);
	if (!looks_like_rotation_property) {
		return false;
	}
	ERR_FAIL_INDEX_V(index, _rotations.size(), false);
	if (property == "angle") {
		ERR_FAIL_COND_V(p_value.get_type() != Variant::FLOAT && p_value.get_type() != Variant::INT, false);
		set_rotation_angle(index, p_value);
		return true;
	} else if (property == "from") {
		ERR_FAIL_COND_V(p_value.get_type() != Variant::INT, false);
		set_rotation_from(index, p_value);
		return true;
	} else if (property == "to") {
		ERR_FAIL_COND_V(p_value.get_type() != Variant::INT, false);
		set_rotation_to(index, p_value);
		return true;
	}
	return false;
}

bool EulerND::_get(const StringName &p_name, Variant &r_ret) const {
	int index;
	String property;
	bool looks_like_rotation_property = does_name_look_like_numbered_rotation_property(p_name, index, property);
	if (!looks_like_rotation_property) {
		return false;
	}
	ERR_FAIL_INDEX_V(index, _rotations.size(), false);
	if (property == "angle") {
		r_ret = get_rotation_angle(index);
		return true;
	} else if (property == "from") {
		r_ret = get_rotation_from(index);
		return true;
	} else if (property == "to") {
		r_ret = get_rotation_to(index);
		return true;
	}
	return false;
}

void EulerND::_get_property_list(List<PropertyInfo> *p_list) const {
	get_rotation_property_list(p_list);
}
