#include "vector_nd.h"

#include <algorithm>

VectorN VectorND::abs(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	VectorN abs_vector;
	abs_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		abs_vector.set(i, Math::abs(p_vector[i]));
	}
	return abs_vector;
}

VectorN VectorND::add(const VectorN &p_a, const VectorN &p_b) {
	const int dimension = MAX(p_a.size(), p_b.size());
	VectorN sum;
	sum.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		sum.set(i, a + b);
	}
	return sum;
}

void VectorND::add_in_place(const VectorN &p_a, VectorN &r_result) {
	const int dimension = p_a.size();
	if (r_result.size() < dimension) {
		r_result.resize(dimension);
	}
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < dimension) ? p_a[i] : 0.0;
		r_result.set(i, r_result[i] + a);
	}
}

VectorN VectorND::add_scalar(const VectorN &p_vector, const double p_scalar) {
	const int dimension = p_vector.size();
	VectorN sum;
	sum.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		sum.set(i, p_vector[i] + p_scalar);
	}
	return sum;
}

double VectorND::angle_to(const VectorN &p_from, const VectorN &p_to) {
	return Math::acos(VectorND::dot(p_from, p_to) / (VectorND::length(p_from) * VectorND::length(p_to)));
}

VectorN VectorND::bounce(const VectorN &p_vector, const VectorN &p_normal) {
	const int dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN bounce_vector;
	bounce_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		bounce_vector.set(i, a + (-2.0f) * dot_product * b);
	}
	return bounce_vector;
}

VectorN VectorND::bounce_ratio(const VectorN &p_vector, const VectorN &p_normal, const double p_bounce_ratio) {
	const int dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN bounce_vector;
	bounce_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		bounce_vector.set(i, a + (-1.0f - p_bounce_ratio) * dot_product * b);
	}
	return bounce_vector;
}

VectorN VectorND::ceil(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	VectorN ceil_vector;
	ceil_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		ceil_vector.set(i, Math::ceil(p_vector[i]));
	}
	return ceil_vector;
}

VectorN VectorND::clamp(const VectorN &p_vector, const VectorN &p_min, const VectorN &p_max) {
	const int dimension = p_vector.size();
	VectorN clamped_vector;
	clamped_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double value = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double min = likely(i < p_min.size()) ? p_min[i] : 0.0;
		const double max = likely(i < p_max.size()) ? p_max[i] : 0.0;
		clamped_vector.set(i, CLAMP(value, min, max));
	}
	return clamped_vector;
}

VectorN VectorND::clampf(const VectorN &p_vector, const double p_min, const double p_max) {
	const int dimension = p_vector.size();
	VectorN clamped_vector;
	clamped_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		clamped_vector.set(i, CLAMP(p_vector[i], p_min, p_max));
	}
	return clamped_vector;
}

double VectorND::cross(const VectorN &p_a, const VectorN &p_b) {
	const double diagonal = VectorND::length_squared(p_a) * VectorND::length_squared(p_b);
	const double non_diagonal = VectorND::dot(p_a, p_b);
	return Math::sqrt(diagonal - non_diagonal * non_diagonal);
}

VectorN VectorND::direction_to(const VectorN &p_from, const VectorN &p_to) {
	const int dimension = MAX(p_from.size(), p_to.size());
	VectorN direction;
	direction.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_from.size()) ? p_from[i] : 0.0;
		const double b = likely(i < p_to.size()) ? p_to[i] : 0.0;
		direction.set(i, b - a);
	}
	return VectorND::normalized(direction);
}

double VectorND::distance_to(const VectorN &p_from, const VectorN &p_to) {
	return Math::sqrt(VectorND::distance_squared_to(p_from, p_to));
}

double VectorND::distance_squared_to(const VectorN &p_from, const VectorN &p_to) {
	const int dimension = MAX(p_from.size(), p_to.size());
	double distance = 0.0;
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_from.size()) ? p_from[i] : 0.0;
		const double b = likely(i < p_to.size()) ? p_to[i] : 0.0;
		const double diff = a - b;
		distance += diff * diff;
	}
	return distance;
}

VectorN VectorND::divide_vector(const VectorN &p_a, const VectorN &p_b, const bool p_expand) {
	const int dimension = p_expand ? MAX(p_a.size(), p_b.size()) : MIN(p_a.size(), p_b.size());
	VectorN quotient;
	quotient.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		quotient.set(i, a / b);
	}
	return quotient;
}

VectorN VectorND::divide_scalar(const VectorN &p_vector, const double p_scalar) {
	const int dimension = p_vector.size();
	VectorN quotient;
	quotient.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		quotient.set(i, p_vector[i] / p_scalar);
	}
	return quotient;
}

double VectorND::dot(const VectorN &p_a, const VectorN &p_b) {
	const int dimension = MIN(p_a.size(), p_b.size());
	double sum = 0.0;
	for (int i = 0; i < dimension; i++) {
		sum += p_a[i] * p_b[i];
	}
	return sum;
}

VectorN VectorND::duplicate(const VectorN &p_vector) {
	VectorN duplicate_vector = p_vector;
	duplicate_vector = duplicate_vector.duplicate();
	return duplicate_vector;
}

VectorN VectorND::floor(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	VectorN floor_vector;
	floor_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		floor_vector.set(i, Math::floor(p_vector[i]));
	}
	return floor_vector;
}

VectorN VectorND::inverse(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	VectorN inverse_vector;
	inverse_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		inverse_vector.set(i, 1.0 / p_vector[i]);
	}
	return inverse_vector;
}

bool VectorND::is_equal_approx(const VectorN &p_a, const VectorN &p_b) {
	const int dimension = MAX(p_a.size(), p_b.size());
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		if (!Math::is_equal_approx(a, b)) {
			return false;
		}
	}
	return true;
}

bool VectorND::is_equal_exact(const VectorN &p_a, const VectorN &p_b) {
	const int dimension = MAX(p_a.size(), p_b.size());
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		if (a != b) {
			return false;
		}
	}
	return true;
}

bool VectorND::is_finite(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	for (int i = 0; i < dimension; i++) {
		if (!Math::is_finite(p_vector[i])) {
			return false;
		}
	}
	return true;
}

bool VectorND::is_zero_approx(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	for (int i = 0; i < dimension; i++) {
		if (!Math::is_zero_approx(p_vector[i])) {
			return false;
		}
	}
	return true;
}

double VectorND::length(const VectorN &p_vector) {
	return Math::sqrt(VectorND::length_squared(p_vector));
}

double VectorND::length_squared(const VectorN &p_vector) {
	double sum = 0.0;
	for (int i = 0; i < p_vector.size(); i++) {
		sum += p_vector[i] * p_vector[i];
	}
	return sum;
}

VectorN VectorND::lerp(const VectorN &p_from, const VectorN &p_to, const double p_weight) {
	const int dimension = MAX(p_from.size(), p_to.size());
	VectorN lerp_vector;
	lerp_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_from.size()) ? p_from[i] : 0.0;
		const double b = likely(i < p_to.size()) ? p_to[i] : 0.0;
		lerp_vector.set(i, Math::lerp(a, b, p_weight));
	}
	return lerp_vector;
}

VectorN VectorND::limit_length(const VectorN &p_vector, const double p_len) {
	const double vector_length = length(p_vector);
	VectorN limited = duplicate(p_vector);
	if (vector_length > 0 && p_len < vector_length) {
		const double scale = p_len / vector_length;
		for (int i = 0; i < p_vector.size(); i++) {
			limited.set(i, limited[i] * scale);
		}
	}
	return limited;
}

VectorN VectorND::multiply_vector(const VectorN &p_a, const VectorN &p_b, const bool p_expand) {
	const int dimension = p_expand ? MAX(p_a.size(), p_b.size()) : MIN(p_a.size(), p_b.size());
	VectorN product;
	product.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		product.set(i, a * b);
	}
	return product;
}

VectorN VectorND::multiply_scalar(const VectorN &p_vector, const double p_scalar) {
	const int dimension = p_vector.size();
	VectorN product;
	product.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		product.set(i, p_vector[i] * p_scalar);
	}
	return product;
}

void VectorND::multiply_scalar_and_add_in_place(const VectorN &p_vector, const double p_scalar, VectorN &r_result) {
	const int dimension = p_vector.size();
	if (unlikely(r_result.size() < dimension)) {
		r_result.resize(dimension);
	}
	for (int i = 0; i < dimension; i++) {
		r_result.set(i, r_result[i] + p_vector[i] * p_scalar);
	}
}

VectorN VectorND::negate(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	VectorN negated;
	negated.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		negated.set(i, -p_vector[i]);
	}
	return negated;
}

VectorN VectorND::normalized(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	const double vector_length = VectorND::length(p_vector);
	if (vector_length == 0) {
		return VectorND::duplicate(p_vector);
	}
	const double scale = 1.0 / vector_length;
	VectorN norm;
	norm.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		norm.set(i, p_vector[i] * scale);
	}
	return norm;
}

VectorN VectorND::posmod(const VectorN &p_vector, const double p_mod) {
	const int dimension = p_vector.size();
	VectorN posmod_vector;
	posmod_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		posmod_vector.set(i, Math::fposmod(p_vector[i], p_mod));
	}
	return posmod_vector;
}

VectorN VectorND::posmodv(const VectorN &p_vector, const VectorN &p_modv) {
	const int dimension = MAX(p_vector.size(), p_modv.size());
	VectorN posmod_vector;
	posmod_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double value = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double mod = likely(i < p_modv.size()) ? p_modv[i] : 0.0;
		posmod_vector.set(i, Math::fposmod(value, mod));
	}
	return posmod_vector;
}

VectorN VectorND::project(const VectorN &p_vector, const VectorN &p_on_normal) {
	const int dimension = p_on_normal.size();
	const double dot_product = VectorND::dot(p_vector, p_on_normal);
	const double normal_length_squared = VectorND::length_squared(p_on_normal);
	VectorN projected;
	projected.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		projected.set(i, p_on_normal[i] * dot_product / normal_length_squared);
	}
	return projected;
}

VectorN VectorND::reflect(const VectorN &p_vector, const VectorN &p_normal) {
	const int dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN reflected;
	reflected.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		reflected.set(i, 2.0f * dot_product * b - a);
	}
	return reflected;
}

VectorN VectorND::round(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	VectorN round_vector;
	round_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		round_vector.set(i, Math::round(p_vector[i]));
	}
	return round_vector;
}

VectorN VectorND::sign(const VectorN &p_vector) {
	const int dimension = p_vector.size();
	VectorN sign_vector;
	sign_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		sign_vector.set(i, SIGN(p_vector[i]));
	}
	return sign_vector;
}

VectorN VectorND::with_length(const VectorN &p_vector, const double p_length) {
	const int dimension = p_vector.size();
	const double vector_length = VectorND::length(p_vector);
	if (vector_length == 0) {
		return VectorND::duplicate(p_vector);
	}
	const double scale = p_length / vector_length;
	VectorN norm;
	norm.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		norm.set(i, p_vector[i] * scale);
	}
	return norm;
}

// Slide returns the component of the vector along the given plane, specified by its normal vector.
VectorN VectorND::slide(const VectorN &p_vector, const VectorN &p_normal) {
	const int dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN slide_vector;
	slide_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		slide_vector.set(i, a - dot_product * b);
	}
	return slide_vector;
}

VectorN VectorND::snapped(const VectorN &p_vector, const VectorN &p_by) {
	const int dimension = p_vector.size();
	VectorN snapped_vector;
	snapped_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double value = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double by = likely(i < p_by.size()) ? p_by[i] : 0.0;
		snapped_vector.set(i, Math::snapped(value, by));
	}
	return snapped_vector;
}

VectorN VectorND::snappedf(const VectorN &p_vector, const double p_by) {
	const int dimension = p_vector.size();
	VectorN snapped_vector;
	snapped_vector.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		snapped_vector.set(i, Math::snapped(p_vector[i], p_by));
	}
	return snapped_vector;
}

VectorN VectorND::subtract(const VectorN &p_a, const VectorN &p_b) {
	const int dimension = MAX(p_a.size(), p_b.size());
	VectorN difference;
	difference.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		difference.set(i, a - b);
	}
	return difference;
}

// Conversion.

VectorN VectorND::from_2d(const Vector2 &p_vector) {
	return { p_vector.x, p_vector.y };
}

VectorN VectorND::from_3d(const Vector3 &p_vector) {
	return { p_vector.x, p_vector.y, p_vector.z };
}

VectorN VectorND::from_4d(const Vector4 &p_vector) {
	return { p_vector.x, p_vector.y, p_vector.z, p_vector.w };
}

Vector2 VectorND::to_2d(const VectorN &p_vector) {
	const int size = p_vector.size();
	if (likely(size > 1)) {
		return Vector2(p_vector[0], p_vector[1]);
	}
	if (size == 1) {
		return Vector2(p_vector[0], 0.0f);
	}
	return Vector2();
}

Vector3 VectorND::to_3d(const VectorN &p_vector) {
	const int size = p_vector.size();
	if (likely(size > 2)) {
		return Vector3(p_vector[0], p_vector[1], p_vector[2]);
	}
	if (size == 2) {
		return Vector3(p_vector[0], p_vector[1], 0.0f);
	}
	if (size == 1) {
		return Vector3(p_vector[0], 0.0f, 0.0f);
	}
	return Vector3();
}

Vector4 VectorND::to_4d(const VectorN &p_vector) {
	const int size = p_vector.size();
	if (likely(size > 3)) {
		return Vector4(p_vector[0], p_vector[1], p_vector[2], p_vector[3]);
	}
	if (size == 3) {
		return Vector4(p_vector[0], p_vector[1], p_vector[2], 0.0f);
	}
	if (size == 2) {
		return Vector4(p_vector[0], p_vector[1], 0.0f, 0.0f);
	}
	if (size == 1) {
		return Vector4(p_vector[0], 0.0f, 0.0f, 0.0f);
	}
	return Vector4();
}

String VectorND::to_string(const VectorN &p_vector) {
	String str = "(";
	for (int i = 0; i < p_vector.size(); i++) {
		str += String::num(p_vector[i]);
		if (i < p_vector.size() - 1) {
			str += ", ";
		}
	}
	str += ")";
	return str;
}

VectorND *VectorND::singleton = nullptr;

void VectorND::_bind_methods() {
	ClassDB::bind_static_method("VectorND", D_METHOD("abs", "vector"), &VectorND::abs);
	ClassDB::bind_static_method("VectorND", D_METHOD("add", "a", "b"), &VectorND::add);
	ClassDB::bind_static_method("VectorND", D_METHOD("add_scalar", "vector", "scalar"), &VectorND::add_scalar);
	ClassDB::bind_static_method("VectorND", D_METHOD("angle_to", "from", "to"), &VectorND::angle_to);
	ClassDB::bind_static_method("VectorND", D_METHOD("bounce", "vector", "normal"), &VectorND::bounce);
	ClassDB::bind_static_method("VectorND", D_METHOD("bounce_ratio", "vector", "normal", "bounce_ratio"), &VectorND::bounce_ratio);
	ClassDB::bind_static_method("VectorND", D_METHOD("ceil", "vector"), &VectorND::ceil);
	ClassDB::bind_static_method("VectorND", D_METHOD("clamp", "vector", "min", "max"), &VectorND::clamp);
	ClassDB::bind_static_method("VectorND", D_METHOD("clampf", "vector", "min", "max"), &VectorND::clampf);
	ClassDB::bind_static_method("VectorND", D_METHOD("cross", "a", "b"), &VectorND::cross);
	ClassDB::bind_static_method("VectorND", D_METHOD("direction_to", "from", "to"), &VectorND::direction_to);
	ClassDB::bind_static_method("VectorND", D_METHOD("distance_to", "from", "to"), &VectorND::distance_to);
	ClassDB::bind_static_method("VectorND", D_METHOD("distance_squared_to", "from", "to"), &VectorND::distance_squared_to);
	ClassDB::bind_static_method("VectorND", D_METHOD("divide_vector", "a", "b", "expand"), &VectorND::divide_vector, DEFVAL(false));
	ClassDB::bind_static_method("VectorND", D_METHOD("divide_scalar", "vector", "scalar"), &VectorND::divide_scalar);
	ClassDB::bind_static_method("VectorND", D_METHOD("dot", "a", "b"), &VectorND::dot);
	ClassDB::bind_static_method("VectorND", D_METHOD("duplicate", "vector"), &VectorND::duplicate);
	ClassDB::bind_static_method("VectorND", D_METHOD("floor", "vector"), &VectorND::floor);
	ClassDB::bind_static_method("VectorND", D_METHOD("inverse", "vector"), &VectorND::inverse);
	ClassDB::bind_static_method("VectorND", D_METHOD("is_equal_approx", "a", "b"), &VectorND::is_equal_approx);
	ClassDB::bind_static_method("VectorND", D_METHOD("is_equal_exact", "a", "b"), &VectorND::is_equal_exact);
	ClassDB::bind_static_method("VectorND", D_METHOD("is_finite", "vector"), &VectorND::is_finite);
	ClassDB::bind_static_method("VectorND", D_METHOD("is_zero_approx", "vector"), &VectorND::is_zero_approx);
	ClassDB::bind_static_method("VectorND", D_METHOD("length", "vector"), &VectorND::length);
	ClassDB::bind_static_method("VectorND", D_METHOD("length_squared", "vector"), &VectorND::length_squared);
	ClassDB::bind_static_method("VectorND", D_METHOD("lerp", "from", "to", "weight"), &VectorND::lerp);
	ClassDB::bind_static_method("VectorND", D_METHOD("limit_length", "vector", "length"), &VectorND::limit_length, DEFVAL(1.0));
	ClassDB::bind_static_method("VectorND", D_METHOD("multiply_vector", "a", "b", "expand"), &VectorND::multiply_vector, DEFVAL(false));
	ClassDB::bind_static_method("VectorND", D_METHOD("multiply_scalar", "vector", "scalar"), &VectorND::multiply_scalar);
	ClassDB::bind_static_method("VectorND", D_METHOD("negate", "vector"), &VectorND::negate);
	ClassDB::bind_static_method("VectorND", D_METHOD("normalized", "vector"), &VectorND::normalized);
	ClassDB::bind_static_method("VectorND", D_METHOD("posmod", "vector", "mod"), &VectorND::posmod);
	ClassDB::bind_static_method("VectorND", D_METHOD("posmodv", "vector", "modv"), &VectorND::posmodv);
	ClassDB::bind_static_method("VectorND", D_METHOD("project", "vector", "on_normal"), &VectorND::project);
	ClassDB::bind_static_method("VectorND", D_METHOD("reflect", "vector", "normal"), &VectorND::reflect);
	ClassDB::bind_static_method("VectorND", D_METHOD("round", "vector"), &VectorND::round);
	ClassDB::bind_static_method("VectorND", D_METHOD("sign", "vector"), &VectorND::sign);
	ClassDB::bind_static_method("VectorND", D_METHOD("slide", "vector", "normal"), &VectorND::slide);
	ClassDB::bind_static_method("VectorND", D_METHOD("snapped", "vector", "by"), &VectorND::snapped);
	ClassDB::bind_static_method("VectorND", D_METHOD("snappedf", "vector", "by"), &VectorND::snappedf);
	ClassDB::bind_static_method("VectorND", D_METHOD("subtract", "a", "b"), &VectorND::subtract);
	ClassDB::bind_static_method("VectorND", D_METHOD("with_length", "vector", "length"), &VectorND::with_length, DEFVAL(1.0));
	// Conversion.
	ClassDB::bind_static_method("VectorND", D_METHOD("from_2d", "vector"), &VectorND::from_2d);
	ClassDB::bind_static_method("VectorND", D_METHOD("from_3d", "vector"), &VectorND::from_3d);
	ClassDB::bind_static_method("VectorND", D_METHOD("from_4d", "vector"), &VectorND::from_4d);
	ClassDB::bind_static_method("VectorND", D_METHOD("to_2d", "vector"), &VectorND::to_2d);
	ClassDB::bind_static_method("VectorND", D_METHOD("to_3d", "vector"), &VectorND::to_3d);
	ClassDB::bind_static_method("VectorND", D_METHOD("to_4d", "vector"), &VectorND::to_4d);
}
