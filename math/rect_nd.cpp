#include "rect_nd.h"

#include "vector_nd.h"

// Trivial getters and setters.

VectorN RectND::get_position() const {
	return _position;
}

void RectND::set_position(const VectorN &p_position) {
	_position = p_position;
}

VectorN RectND::get_size() const {
	return _size;
}

void RectND::set_size(const VectorN &p_size) {
	_size = p_size;
}

VectorN RectND::get_end() const {
	return VectorND::add(_position, _size);
}

void RectND::set_end(const VectorN &p_end) {
	_size = VectorND::subtract(p_end, _position);
}

VectorN RectND::get_center() const {
	return VectorND::add(_position, VectorND::multiply_scalar(_size, 0.5));
}

void RectND::set_center(const VectorN &p_center) {
	_position = VectorND::subtract(p_center, VectorND::multiply_scalar(_size, 0.5));
}

int RectND::get_dimension() const {
	return MAX(_position.size(), _size.size());
}

void RectND::set_dimension(const int p_dimension) {
	_position.resize(p_dimension);
	_size.resize(p_dimension);
}

// Basic math functions.

Ref<RectND> RectND::abs() const {
	const int dimension = MIN(_position.size(), _size.size());
	VectorN abs_position = _position;
	VectorN abs_size = _size;
	for (int i = 0; i < dimension; ++i) {
		if (_size[i] < 0.0) {
			abs_position.set(i, _position[i] + _size[i]);
			abs_size.set(i, -_size[i]);
		}
	}
	Ref<RectND> abs_rect;
	abs_rect.instantiate();
	abs_rect->set_position(abs_position);
	abs_rect->set_size(abs_size);
	return abs_rect;
}

Ref<RectND> RectND::duplicate() const {
	Ref<RectND> dup;
	dup.instantiate();
	dup->set_position(_position);
	dup->set_size(_size);
	return dup;
}

double RectND::get_space() const {
	const int dimension = MIN(_position.size(), _size.size());
	double hypervolume = 1.0;
	for (int i = 0; i < dimension; ++i) {
		hypervolume *= _size[i];
	}
	return hypervolume;
}

double RectND::get_surface() const {
	const int dimension = MIN(_position.size(), _size.size());
	double surface_total = 0.0;
	for (int i = 0; i < dimension; ++i) {
		double surface = 1.0;
		for (int j = 0; j < dimension; ++j) {
			if (i != j) {
				surface *= _size[j];
			}
		}
		surface_total += surface;
	}
	return surface_total * 2.0;
}

// Point math functions.

void RectND::expand_self_to_point(const VectorN &p_vector) {
	if (_position.size() < p_vector.size()) {
		_position.resize(p_vector.size());
	}
	if (_size.size() < p_vector.size()) {
		_size.resize(p_vector.size());
	}
	const VectorN end = get_end();
	for (int i = 0; i < _position.size(); ++i) {
		if (p_vector[i] < _position[i]) {
			_position.set(i, p_vector[i]);
			_size.set(i, end[i] - p_vector[i]);
		} else if (p_vector[i] > end[i]) {
			_size.set(i, p_vector[i] - _position[i]);
		}
	}
}

Ref<RectND> RectND::expand_to_point(const VectorN &p_vector) const {
	Ref<RectND> new_rect = duplicate();
	new_rect->expand_self_to_point(p_vector);
	return new_rect;
}

VectorN RectND::get_nearest_point(const VectorN &p_point) const {
	const VectorN end = get_end();
	VectorN closest = p_point;
	for (int i = 0; i < _position.size(); ++i) {
		if (p_point[i] < _position[i]) {
			closest.set(i, _position[i]);
		} else if (p_point[i] > end[i]) {
			closest.set(i, end[i]);
		}
	}
	return closest;
}

VectorN RectND::get_support_point(const VectorN &p_direction) const {
	VectorN support = _position;
	for (int i = 0; i < _position.size(); ++i) {
		if (p_direction[i] > 0.0) {
			support.set(i, _position[i] + _size[i]);
		}
	}
	return support;
}

bool RectND::has_point(const VectorN &p_point) const {
	const VectorN end = get_end();
	for (int i = 0; i < _position.size(); ++i) {
		if (p_point[i] < _position[i] || p_point[i] > end[i]) {
			return false;
		}
	}
	return true;
}

// Rect math functions.

Ref<RectND> RectND::grow(const double p_by) const {
	Ref<RectND> grown;
	grown.instantiate();
	grown->set_position(VectorND::add_scalar(_position, -p_by));
	grown->set_size(VectorND::add_scalar(_size, p_by * 2.0));
	return grown;
}

Ref<RectND> RectND::intersection(const Ref<RectND> &p_other) const {
	const VectorN end = get_end();
	const VectorN other_end = p_other->get_end();
	const VectorN other_position = p_other->get_position();
	const int dimension = MIN(MIN(_position.size(), end.size()), MIN(other_position.size(), other_end.size()));
	Ref<RectND> intersected;
	intersected.instantiate();
	VectorN intersect_position = _position;
	VectorN intersect_size = _size;
	for (int i = 0; i < dimension; ++i) {
		if (_position[i] > other_end[i] || end[i] < other_position[i]) {
			return intersected;
		}
		if (other_position[i] > _position[i]) {
			intersect_position.set(i, other_position[i]);
		}
		if (other_end[i] < end[i]) {
			intersect_size.set(i, other_end[i] - intersect_position[i]);
		}
	}
	intersected->set_position(intersect_position);
	intersected->set_size(intersect_size);
	return intersected;
}

Ref<RectND> RectND::merge(const Ref<RectND> &p_other) const {
	const int dimension = MAX(get_dimension(), p_other->get_dimension());
	VectorN other_start = VectorND::with_dimension(p_other->get_position(), dimension);
	VectorN other_end = VectorND::with_dimension(p_other->get_end(), dimension);
	VectorN merged_start = VectorND::with_dimension(_position, dimension);
	VectorN merged_end = VectorND::with_dimension(get_end(), dimension);
	for (int i = 0; i < dimension; ++i) {
		if (other_start[i] < merged_start[i]) {
			merged_start.set(i, other_start[i]);
		}
		if (other_end[i] > merged_end[i]) {
			merged_end.set(i, other_end[i]);
		}
	}
	Ref<RectND> merged;
	merged.instantiate();
	merged->set_position(merged_start);
	merged->set_end(merged_end);
	return merged;
}

bool RectND::encloses_exclusive(const Ref<RectND> &p_other) const {
	const VectorN end = get_end();
	const VectorN other_end = p_other->get_end();
	const VectorN other_position = p_other->get_position();
	const int dimension = MIN(MIN(_position.size(), end.size()), MIN(other_position.size(), other_end.size()));
	for (int i = 0; i < dimension; ++i) {
		if (_position[i] >= other_position[i] || end[i] <= other_end[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::encloses_inclusive(const Ref<RectND> &p_other) const {
	const VectorN end = get_end();
	const VectorN other_end = p_other->get_end();
	const VectorN other_position = p_other->get_position();
	const int dimension = MIN(MIN(_position.size(), end.size()), MIN(other_position.size(), other_end.size()));
	for (int i = 0; i < dimension; ++i) {
		if (_position[i] > other_position[i] || end[i] < other_end[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::intersects_exclusive(const Ref<RectND> &p_other) const {
	const VectorN end = get_end();
	const VectorN other_end = p_other->get_end();
	const VectorN other_position = p_other->get_position();
	const int dimension = MIN(MIN(_position.size(), end.size()), MIN(other_position.size(), other_end.size()));
	for (int i = 0; i < dimension; ++i) {
		if (_position[i] >= other_end[i] || end[i] <= other_position[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::intersects_inclusive(const Ref<RectND> &p_other) const {
	const VectorN end = get_end();
	const VectorN other_end = p_other->get_end();
	const VectorN other_position = p_other->get_position();
	const int dimension = MIN(MIN(_position.size(), end.size()), MIN(other_position.size(), other_end.size()));
	for (int i = 0; i < dimension; ++i) {
		if (_position[i] > other_end[i] || end[i] < other_position[i]) {
			return false;
		}
	}
	return true;
}

bool RectND::is_equal_approx(const Ref<RectND> &p_other) const {
	return VectorND::is_equal_approx(_position, p_other->get_position()) && VectorND::is_equal_approx(_size, p_other->get_size());
}

bool RectND::is_finite() const {
	return VectorND::is_finite(_position) && VectorND::is_finite(_size);
}

// Constructors.

Ref<RectND> RectND::from_center_size(const VectorN &p_center, const VectorN &p_size) {
	Ref<RectND> rect;
	rect.instantiate();
	rect->set_size(p_size);
	rect->set_center(p_center);
	return rect;
}

Ref<RectND> RectND::from_position_size(const VectorN &p_position, const VectorN &p_size) {
	Ref<RectND> rect;
	rect.instantiate();
	rect->set_position(p_position);
	rect->set_size(p_size);
	return rect;
}

Ref<RectND> RectND::from_position_end(const VectorN &p_position, const VectorN &p_end) {
	Ref<RectND> rect;
	rect.instantiate();
	rect->set_position(p_position);
	rect->set_end(p_end);
	return rect;
}

void RectND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_position"), &RectND::get_position);
	ClassDB::bind_method(D_METHOD("set_position", "position"), &RectND::set_position);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "position", PROPERTY_HINT_NONE, "suffix:m"), "set_position", "get_position");

	ClassDB::bind_method(D_METHOD("get_size"), &RectND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &RectND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");

	ClassDB::bind_method(D_METHOD("get_end"), &RectND::get_end);
	ClassDB::bind_method(D_METHOD("set_end", "end"), &RectND::set_end);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "end", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_end", "get_end");

	ClassDB::bind_method(D_METHOD("get_center"), &RectND::get_center);
	ClassDB::bind_method(D_METHOD("set_center", "center"), &RectND::set_center);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "center", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_center", "get_center");

	ClassDB::bind_method(D_METHOD("get_dimension"), &RectND::get_dimension);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &RectND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_dimension", "get_dimension");
	// Basic math functions.
	ClassDB::bind_method(D_METHOD("abs"), &RectND::abs);
	ClassDB::bind_method(D_METHOD("duplicate"), &RectND::duplicate);
	ClassDB::bind_method(D_METHOD("get_space"), &RectND::get_space);
	ClassDB::bind_method(D_METHOD("get_surface"), &RectND::get_surface);
	// Point math functions.
	ClassDB::bind_method(D_METHOD("expand_self_to_point", "vector"), &RectND::expand_self_to_point);
	ClassDB::bind_method(D_METHOD("expand_to_point", "vector"), &RectND::expand_to_point);
	ClassDB::bind_method(D_METHOD("get_nearest_point", "point"), &RectND::get_nearest_point);
	ClassDB::bind_method(D_METHOD("get_support_point", "direction"), &RectND::get_support_point);
	ClassDB::bind_method(D_METHOD("has_point", "point"), &RectND::has_point);
	// Rect math functions.
	ClassDB::bind_method(D_METHOD("grow", "by"), &RectND::grow);
	ClassDB::bind_method(D_METHOD("intersection", "other"), &RectND::intersection);
	ClassDB::bind_method(D_METHOD("merge", "other"), &RectND::merge);
	// Rect comparison functions.
	ClassDB::bind_method(D_METHOD("encloses_exclusive", "other"), &RectND::encloses_exclusive);
	ClassDB::bind_method(D_METHOD("encloses_inclusive", "other"), &RectND::encloses_inclusive);
	ClassDB::bind_method(D_METHOD("intersects_exclusive", "other"), &RectND::intersects_exclusive);
	ClassDB::bind_method(D_METHOD("intersects_inclusive", "other"), &RectND::intersects_inclusive);
	ClassDB::bind_method(D_METHOD("is_equal_approx", "other"), &RectND::is_equal_approx);
	ClassDB::bind_method(D_METHOD("is_finite"), &RectND::is_finite);
	// Constructors.
	ClassDB::bind_static_method("RectND", D_METHOD("from_center_size", "center", "size"), &RectND::from_center_size);
	ClassDB::bind_static_method("RectND", D_METHOD("from_position_size", "position", "size"), &RectND::from_position_size);
	ClassDB::bind_static_method("RectND", D_METHOD("from_position_end", "position", "end"), &RectND::from_position_end);
}
