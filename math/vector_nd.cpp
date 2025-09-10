#include "vector_nd.h"

#include <algorithm>

#if GDEXTENSION
#include <godot_cpp/variant/typed_array.hpp>
#elif GODOT_MODULE
#include "core/variant/typed_array.h"
#endif

// These are a superset of the directions found in Godot's Vector3 type.
// These align with the G4MF specification: https://github.com/godot-dimensions/g4mf/blob/main/specification/parts/coordinate_system.md

const VectorN VectorND::DIR_RIGHT = VectorN{ 1 };
const VectorN VectorND::DIR_LEFT = VectorN{ -1 };
const VectorN VectorND::DIR_UP = VectorN{ 0, 1 };
const VectorN VectorND::DIR_DOWN = VectorN{ 0, -1 };
const VectorN VectorND::DIR_BACK = VectorN{ 0, 0, 1 };
const VectorN VectorND::DIR_FORWARD = VectorN{ 0, 0, -1 };
const VectorN VectorND::DIR_ANA = VectorN{ 0, 0, 0, 1 };
const VectorN VectorND::DIR_KATA = VectorN{ 0, 0, 0, -1 };
const VectorN VectorND::DIR_SURSUM = VectorN{ 0, 0, 0, 0, 1 };
const VectorN VectorND::DIR_DEORSUM = VectorN{ 0, 0, 0, 0, -1 };

const VectorN VectorND::MODEL_LEFT_SIDE = VectorN{ 1 };
const VectorN VectorND::MODEL_RIGHT_SIDE = VectorN{ -1 };
const VectorN VectorND::MODEL_TOP_SIDE = VectorN{ 0, 1 };
const VectorN VectorND::MODEL_BOTTOM_SIDE = VectorN{ 0, -1 };
const VectorN VectorND::MODEL_FRONT_SIDE = VectorN{ 0, 0, 1 };
const VectorN VectorND::MODEL_REAR_SIDE = VectorN{ 0, 0, -1 };
const VectorN VectorND::MODEL_ANA_SIDE = VectorN{ 0, 0, 0, 1 };
const VectorN VectorND::MODEL_KATA_SIDE = VectorN{ 0, 0, 0, -1 };
const VectorN VectorND::MODEL_SURSUM_SIDE = VectorN{ 0, 0, 0, 0, 1 };
const VectorN VectorND::MODEL_DEORSUM_SIDE = VectorN{ 0, 0, 0, 0, -1 };

const VectorN VectorND::CARDINAL_EAST = VectorN{ 1 };
const VectorN VectorND::CARDINAL_WEST = VectorN{ -1 };
const VectorN VectorND::CARDINAL_ZENITH = VectorN{ 0, 1 };
const VectorN VectorND::CARDINAL_NADIR = VectorN{ 0, -1 };
const VectorN VectorND::CARDINAL_SOUTH = VectorN{ 0, 0, 1 };
const VectorN VectorND::CARDINAL_NORTH = VectorN{ 0, 0, -1 };
const VectorN VectorND::CARDINAL_ANTH = VectorN{ 0, 0, 0, 1 };
const VectorN VectorND::CARDINAL_KENTH = VectorN{ 0, 0, 0, -1 };
const VectorN VectorND::CARDINAL_SURTH = VectorN{ 0, 0, 0, 0, 1 };
const VectorN VectorND::CARDINAL_DEORTH = VectorN{ 0, 0, 0, 0, -1 };

// Cosmetic functions.

float _get_axis_color_hue_nd(int64_t p_index) {
	// Main hue.
	int group = (p_index / 3) % 4;
	int parity = p_index % 3;
	float hue_base = 0.0;
	if (group == 0) {
		hue_base = 0.0;
		// Though our iteration direction is generally negative,
		// we want to keep Y as green and Z as blue.
		if (parity == 1) {
			parity = 2;
		} else if (parity == 2) {
			parity = 1;
		}
	} else if (group == 1) {
		hue_base = 1.0 / 6.0;
	} else if (group == 2) {
		hue_base = 1.0 / 12.0;
	} else { // if (group == 3)
		hue_base = 3.0 / 12.0;
	}
	float hue = hue_base - (parity * 0.33333333f + 0.04f);
	// Further refinement.
	if (((p_index / 48) % 2) == 1) {
		hue += 1.0 / 24.0;
	}
	if (((p_index / 96) % 2) == 1) {
		hue += 1.0 / 48.0;
	}
	if (hue < 0.0) {
		hue += 1.0;
	}
	return hue;
}

Color VectorND::axis_color(int64_t p_axis) {
	float hue = _get_axis_color_hue_nd(p_axis);
	float value = ((p_axis / 12) % 2) == 0 ? 1.0f : 0.5f;
	float sat = ((p_axis / 24) % 2) == 0 ? 0.8f : 0.4f;
	return Color::from_hsv(hue, sat, value);
}

int _get_axis_unicode_number_nd(int64_t p_axis) {
	if (p_axis < 3) {
		return p_axis + 88;
	} else if (p_axis < 26) {
		return 90 - p_axis;
	} else if (p_axis < 32) {
		return p_axis + 919;
	} else if (p_axis < 34) {
		return p_axis + 920;
	} else if (p_axis < 36) {
		return p_axis + 921;
	} else if (p_axis == 36) {
		return p_axis + 922;
	} else if (p_axis < 38) {
		return p_axis + 923;
	} else if (p_axis < 41) {
		return p_axis + 924;
	} else if (p_axis == 41) {
		return p_axis + 925;
	} else if (p_axis < 44) {
		return p_axis + 926;
	} else if (p_axis < 71) {
		return p_axis + 4260;
	} else if (p_axis < 73) {
		return p_axis + 4261;
	} else if (p_axis < 85) {
		return p_axis + 4262;
	} else if (p_axis < 110) {
		return p_axis * 2 + 12184;
	} else if (p_axis < 118) {
		return p_axis * 2 + 12194;
	} else if (p_axis < 124) {
		return p_axis + 12313;
	} else if (p_axis < 149) {
		return p_axis * 2 + 12202;
	} else if (p_axis < 163) {
		return p_axis * 2 + 12210;
	} else if (p_axis < 197) {
		return p_axis + 12386;
	} else if (p_axis < 21092) {
		return p_axis + 19777;
	} else {
		return 63; // ?
	}
}

String VectorND::axis_letter(int64_t p_axis) {
	if (p_axis < 0) {
		return String("*");
	}
	return String::chr(_get_axis_unicode_number_nd(p_axis));
}

// VectorN operations.

VectorN VectorND::abs(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	VectorN abs_vector;
	abs_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		abs_vector.set(i, Math::abs(p_vector[i]));
	}
	return abs_vector;
}

VectorN VectorND::add(const VectorN &p_a, const VectorN &p_b) {
	const int64_t dimension = MAX(p_a.size(), p_b.size());
	VectorN sum;
	sum.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		sum.set(i, a + b);
	}
	return sum;
}

void VectorND::add_in_place(const VectorN &p_a, VectorN &r_result) {
	const int64_t dimension = p_a.size();
	const int64_t old_size = r_result.size();
	if (old_size < dimension) {
		r_result.resize(dimension);
	}
	for (int64_t i = 0; i < dimension; i++) {
		if (i < old_size) {
			r_result.set(i, r_result[i] + p_a[i]);
		} else {
			r_result.set(i, p_a[i]);
		}
	}
}

VectorN VectorND::add_scalar(const VectorN &p_vector, const double p_scalar) {
	const int64_t dimension = p_vector.size();
	VectorN sum;
	sum.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		sum.set(i, p_vector[i] + p_scalar);
	}
	return sum;
}

double VectorND::angle_to(const VectorN &p_from, const VectorN &p_to) {
	return Math::acos(VectorND::dot(p_from, p_to) / (VectorND::length(p_from) * VectorND::length(p_to)));
}

VectorN VectorND::bounce(const VectorN &p_vector, const VectorN &p_normal) {
	const int64_t dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN bounce_vector;
	bounce_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		bounce_vector.set(i, a + (-2.0f) * dot_product * b);
	}
	return bounce_vector;
}

VectorN VectorND::bounce_ratio(const VectorN &p_vector, const VectorN &p_normal, const double p_bounce_ratio) {
	const int64_t dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN bounce_vector;
	bounce_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		bounce_vector.set(i, a + (-1.0f - p_bounce_ratio) * dot_product * b);
	}
	return bounce_vector;
}

VectorN VectorND::ceil(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	VectorN ceil_vector;
	ceil_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		ceil_vector.set(i, Math::ceil(p_vector[i]));
	}
	return ceil_vector;
}

VectorN VectorND::clamp(const VectorN &p_vector, const VectorN &p_min, const VectorN &p_max) {
	const int64_t dimension = p_vector.size();
	VectorN clamped_vector;
	clamped_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double value = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double min = likely(i < p_min.size()) ? p_min[i] : 0.0;
		const double max = likely(i < p_max.size()) ? p_max[i] : 0.0;
		clamped_vector.set(i, CLAMP(value, min, max));
	}
	return clamped_vector;
}

VectorN VectorND::clampf(const VectorN &p_vector, const double p_min, const double p_max) {
	const int64_t dimension = p_vector.size();
	VectorN clamped_vector;
	clamped_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
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
	const int64_t dimension = MAX(p_from.size(), p_to.size());
	VectorN direction;
	direction.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
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
	const int64_t dimension = MAX(p_from.size(), p_to.size());
	double distance = 0.0;
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_from.size()) ? p_from[i] : 0.0;
		const double b = likely(i < p_to.size()) ? p_to[i] : 0.0;
		const double diff = a - b;
		distance += diff * diff;
	}
	return distance;
}

VectorN VectorND::divide_vector(const VectorN &p_a, const VectorN &p_b, const bool p_expand) {
	const int64_t dimension = p_expand ? MAX(p_a.size(), p_b.size()) : MIN(p_a.size(), p_b.size());
	VectorN quotient;
	quotient.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		quotient.set(i, a / b);
	}
	return quotient;
}

VectorN VectorND::divide_scalar(const VectorN &p_vector, const double p_scalar) {
	const int64_t dimension = p_vector.size();
	VectorN quotient;
	quotient.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		quotient.set(i, p_vector[i] / p_scalar);
	}
	return quotient;
}

double VectorND::dot(const VectorN &p_a, const VectorN &p_b) {
	const int64_t dimension = MIN(p_a.size(), p_b.size());
	double sum = 0.0;
	for (int64_t i = 0; i < dimension; i++) {
		sum += p_a[i] * p_b[i];
	}
	return sum;
}

VectorN VectorND::drop_first_dimensions(const VectorN &p_vector, const int64_t p_dimensions) {
	const int64_t dimension = p_vector.size();
	VectorN dropped_vector;
	if (dimension <= p_dimensions) {
		return dropped_vector;
	}
	const int64_t new_dimension = dimension - p_dimensions;
	dropped_vector.resize(new_dimension);
	for (int64_t i = 0; i < new_dimension; i++) {
		dropped_vector.set(i, p_vector[i + p_dimensions]);
	}
	return dropped_vector;
}

VectorN VectorND::duplicate(const VectorN &p_vector) {
	VectorN duplicate_vector = p_vector;
	duplicate_vector = duplicate_vector.duplicate();
	return duplicate_vector;
}

VectorN VectorND::fill(const int64_t p_dimension, const double p_value) {
	VectorN filled_vector;
	filled_vector.resize(p_dimension);
	for (int64_t i = 0; i < p_dimension; i++) {
		filled_vector.set(i, p_value);
	}
	return filled_vector;
}

Vector<VectorN> VectorND::fill_array(const int64_t p_dimension, const int64_t p_vector_amount, const double p_value) {
	Vector<VectorN> filled_array;
	filled_array.resize(p_vector_amount);
	const VectorN filled_vector = fill(p_value, p_dimension);
	for (int64_t i = 0; i < p_vector_amount; i++) {
		filled_array.set(i, filled_vector);
	}
	return filled_array;
}

TypedArray<VectorN> VectorND::fill_array_bind(const int64_t p_dimension, const int64_t p_vector_amount, const double p_value) {
	Vector<VectorN> filled_array = VectorND::fill_array(p_dimension, p_vector_amount, p_value);
	TypedArray<VectorN> filled_array_variant;
	filled_array_variant.resize(p_vector_amount);
	for (int64_t i = 0; i < p_vector_amount; i++) {
		filled_array_variant[i] = filled_array[i];
	}
	return filled_array_variant;
}

VectorN VectorND::floor(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	VectorN floor_vector;
	floor_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		floor_vector.set(i, Math::floor(p_vector[i]));
	}
	return floor_vector;
}

VectorN VectorND::inverse(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	VectorN inverse_vector;
	inverse_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		inverse_vector.set(i, 1.0 / p_vector[i]);
	}
	return inverse_vector;
}

bool VectorND::is_equal_approx(const VectorN &p_a, const VectorN &p_b) {
	const int64_t dimension = MAX(p_a.size(), p_b.size());
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		if (!Math::is_equal_approx(a, b)) {
			return false;
		}
	}
	return true;
}

bool VectorND::is_equal_exact(const VectorN &p_a, const VectorN &p_b) {
	const int64_t dimension = MAX(p_a.size(), p_b.size());
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		if (a != b) {
			return false;
		}
	}
	return true;
}

bool VectorND::is_finite(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	for (int64_t i = 0; i < dimension; i++) {
		if (!Math::is_finite(p_vector[i])) {
			return false;
		}
	}
	return true;
}

bool VectorND::is_zero_approx(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	for (int64_t i = 0; i < dimension; i++) {
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
	for (int64_t i = 0; i < p_vector.size(); i++) {
		sum += p_vector[i] * p_vector[i];
	}
	return sum;
}

VectorN VectorND::lerp(const VectorN &p_from, const VectorN &p_to, const double p_weight) {
	const int64_t dimension = MAX(p_from.size(), p_to.size());
	VectorN lerp_vector;
	lerp_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
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
		for (int64_t i = 0; i < p_vector.size(); i++) {
			limited.set(i, limited[i] * scale);
		}
	}
	return limited;
}

int64_t VectorND::max_absolute_axis_index(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	ERR_FAIL_COND_V(dimension == 0, -1);
	int64_t max_index = 0;
	double max_value = Math::abs(p_vector[0]);
	for (int64_t i = 1; i < dimension; i++) {
		double abs_value = Math::abs(p_vector[i]);
		if (abs_value > max_value) {
			max_value = abs_value;
			max_index = i;
		}
	}
	return max_index;
}

int64_t VectorND::max_axis_index(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	ERR_FAIL_COND_V(dimension == 0, -1);
	int64_t max_index = 0;
	double max_value = p_vector[0];
	for (int64_t i = 1; i < dimension; i++) {
		if (p_vector[i] > max_value) {
			max_value = p_vector[i];
			max_index = i;
		}
	}
	return max_index;
}

int64_t VectorND::min_axis_index(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	ERR_FAIL_COND_V(dimension == 0, -1);
	int64_t min_index = 0;
	double min_value = p_vector[0];
	for (int64_t i = 1; i < dimension; i++) {
		if (p_vector[i] < min_value) {
			min_value = p_vector[i];
			min_index = i;
		}
	}
	return min_index;
}

double VectorND::multiply_components_together(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	double product = 1.0;
	for (int64_t i = 0; i < dimension; i++) {
		product *= p_vector[i];
	}
	return product;
}

VectorN VectorND::multiply_vector(const VectorN &p_a, const VectorN &p_b, const bool p_expand) {
	const int64_t dimension = p_expand ? MAX(p_a.size(), p_b.size()) : MIN(p_a.size(), p_b.size());
	VectorN product;
	product.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		product.set(i, a * b);
	}
	return product;
}

VectorN VectorND::multiply_scalar(const VectorN &p_vector, const double p_scalar) {
	const int64_t dimension = p_vector.size();
	VectorN product;
	product.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		product.set(i, p_vector[i] * p_scalar);
	}
	return product;
}

void VectorND::multiply_scalar_and_add_in_place(const VectorN &p_vector, const double p_scalar, VectorN &r_result) {
	const int64_t dimension = p_vector.size();
	const int64_t old_size = r_result.size();
	if (unlikely(old_size < dimension)) {
		r_result.resize(dimension);
		for (int64_t i = old_size; i < dimension; i++) {
			r_result.set(i, 0.0);
		}
	}
	for (int64_t i = 0; i < dimension; i++) {
		if (i >= old_size) {
			r_result.set(i, p_vector[i] * p_scalar);
		} else {
			r_result.set(i, r_result[i] + p_vector[i] * p_scalar);
		}
	}
}

VectorN VectorND::negate(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	VectorN negated;
	negated.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		negated.set(i, -p_vector[i]);
	}
	return negated;
}

VectorN VectorND::normalized(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	const double vector_length = VectorND::length(p_vector);
	if (vector_length == 0) {
		return VectorND::duplicate(p_vector);
	}
	const double scale = 1.0 / vector_length;
	VectorN norm;
	norm.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		norm.set(i, p_vector[i] * scale);
	}
	return norm;
}

VectorN VectorND::one(const int64_t p_dimension) {
	VectorN filled_vector;
	filled_vector.resize(p_dimension);
	for (int64_t i = 0; i < p_dimension; i++) {
		filled_vector.set(i, 1.0);
	}
	return filled_vector;
}

VectorN VectorND::perpendicular(const Vector<VectorN> &p_input_vectors) {
	// Handle edge cases and determine if the input is valid.
	ERR_FAIL_COND_V_MSG(p_input_vectors.is_empty(), VectorN(), "VectorND.perpendicular: Cannot compute a vector perpendicular to nothing.");
	const int64_t count = p_input_vectors.size();
	const int64_t dimension = p_input_vectors[0].size();
	ERR_FAIL_COND_V_MSG(count != dimension - 1, VectorN(), "VectorND.perpendicular: Expected exactly N-1 vectors for N-dimensional space.");
	for (int64_t input_vec_index = 1; input_vec_index < count; input_vec_index++) {
		const VectorN input_vector = p_input_vectors[input_vec_index];
		ERR_FAIL_COND_V_MSG(input_vector.size() != dimension, VectorN(), "VectorND.perpendicular: All input vectors must have the same dimension.");
	}
	if (dimension > 100) {
		WARN_PRINT("VectorND.perpendicular: Calculating a perpendicular vector in " + itos(dimension) + "-dimensional space will be very slow.");
	}
	// Allocate the result vector and a matrix to perform the intermediate calculations.
	VectorN result = VectorND::zero(dimension);
	const int64_t sub_size = count; // == dimension - 1
	Vector<VectorN> sub_matrix = VectorND::fill_array(sub_size, sub_size, 0.0);
	// Flip the entire result if dimension is even.
	const bool global_parity = (dimension % 2 == 0);
	// This algorithm was found by ChatGPT and manually tweaked. It is likely suboptimal.
	for (int64_t dimension_index = 0; dimension_index < dimension; dimension_index++) {
		// Build the (N-1)x(N-1) submatrix omitting column `dimension_index`.
		// The naming convention of rows vs columns is for matching the typical
		// Gaussian elimination algorithm, but the actual math works either way.
		for (int64_t row_index = 0; row_index < sub_size; row_index++) {
			int64_t row_col_index = 0;
			VectorN row = sub_matrix[row_index];
			for (int64_t column_index = 0; column_index < dimension; column_index++) {
				if (column_index == dimension_index) {
					continue;
				}
				row.set(row_col_index++, p_input_vectors[row_index][column_index]);
			}
			sub_matrix.set(row_index, row);
		}

		// Compute det(sub_matrix) via Gaussian elimination.
		double det = 1.0;
		bool pivot_parity = dimension_index % 2;
		for (int64_t pivot_index = 0; pivot_index < sub_size; pivot_index++) {
			// Find the pivot row.
			int64_t pivot = pivot_index;
			while (pivot < sub_size && sub_matrix[pivot][pivot_index] == 0.0) {
				pivot++;
			}
			if (pivot == sub_size) {
				det = 0.0;
				break;
			}
			if (pivot != pivot_index) {
				VectorN tmp = sub_matrix[pivot_index];
				sub_matrix.set(pivot_index, sub_matrix[pivot]);
				sub_matrix.set(pivot, tmp);
				pivot_parity = !pivot_parity;
			}
			const double pivot_val = sub_matrix[pivot_index][pivot_index];
			if (pivot_val == 0.0) {
				det = 0.0;
				break;
			}
			for (int64_t row_index = pivot_index + 1; row_index < sub_size; row_index++) {
				const double factor = sub_matrix[row_index][pivot_index] / pivot_val;
				VectorN mat_row = sub_matrix[row_index];
				for (int64_t column_index = pivot_index; column_index < sub_size; column_index++) {
					mat_row.set(column_index, mat_row[column_index] - factor * sub_matrix[pivot_index][column_index]);
				}
				sub_matrix.set(row_index, mat_row);
			}
		}
		if (det != 0.0) {
			for (int64_t diagonal_index = 0; diagonal_index < sub_size; diagonal_index++) {
				det *= sub_matrix[diagonal_index][diagonal_index];
			}
		}
		// Cofactor sign and global flip.
		const bool parity_sign_flip = global_parity ^ pivot_parity;
		const double cofactor = parity_sign_flip ? -det : det;
		result.set(dimension_index, cofactor);
	}
	return result;
}

VectorN VectorND::perpendicular_bind(const TypedArray<VectorN> &p_input_vectors) {
	Vector<VectorN> vectors;
	vectors.resize(p_input_vectors.size());
	for (int64_t i = 0; i < p_input_vectors.size(); i++) {
		vectors.set(i, p_input_vectors[i]);
	}
	return VectorND::perpendicular(vectors);
}

VectorN VectorND::posmod(const VectorN &p_vector, const double p_mod) {
	const int64_t dimension = p_vector.size();
	VectorN posmod_vector;
	posmod_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		posmod_vector.set(i, Math::fposmod(p_vector[i], p_mod));
	}
	return posmod_vector;
}

VectorN VectorND::posmodv(const VectorN &p_vector, const VectorN &p_modv) {
	const int64_t dimension = MAX(p_vector.size(), p_modv.size());
	VectorN posmod_vector;
	posmod_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double value = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double mod = likely(i < p_modv.size()) ? p_modv[i] : 0.0;
		posmod_vector.set(i, Math::fposmod(value, mod));
	}
	return posmod_vector;
}

VectorN VectorND::project(const VectorN &p_vector, const VectorN &p_on_normal) {
	const int64_t dimension = p_on_normal.size();
	const double dot_product = VectorND::dot(p_vector, p_on_normal);
	const double normal_length_squared = VectorND::length_squared(p_on_normal);
	const double scale = dot_product / normal_length_squared;
	VectorN projected;
	projected.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		projected.set(i, p_on_normal[i] * scale);
	}
	return projected;
}

VectorN VectorND::reflect(const VectorN &p_vector, const VectorN &p_normal) {
	const int64_t dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN reflected;
	reflected.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		reflected.set(i, 2.0f * dot_product * b - a);
	}
	return reflected;
}

VectorN VectorND::round(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	VectorN round_vector;
	round_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		round_vector.set(i, Math::round(p_vector[i]));
	}
	return round_vector;
}

VectorN VectorND::sign(const VectorN &p_vector) {
	const int64_t dimension = p_vector.size();
	VectorN sign_vector;
	sign_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		sign_vector.set(i, SIGN(p_vector[i]));
	}
	return sign_vector;
}

// Slide returns the component of the vector along the given plane, specified by its normal vector.
VectorN VectorND::slide(const VectorN &p_vector, const VectorN &p_normal) {
	const int64_t dimension = MAX(p_vector.size(), p_normal.size());
	const double dot_product = VectorND::dot(p_vector, p_normal);
	VectorN slide_vector;
	slide_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double b = likely(i < p_normal.size()) ? p_normal[i] : 0.0;
		slide_vector.set(i, a - dot_product * b);
	}
	return slide_vector;
}

VectorN VectorND::snapped(const VectorN &p_vector, const VectorN &p_by) {
	const int64_t dimension = p_vector.size();
	VectorN snapped_vector;
	snapped_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double value = likely(i < p_vector.size()) ? p_vector[i] : 0.0;
		const double by = likely(i < p_by.size()) ? p_by[i] : 0.0;
		snapped_vector.set(i, Math::snapped(value, by));
	}
	return snapped_vector;
}

VectorN VectorND::snappedf(const VectorN &p_vector, const double p_by) {
	const int64_t dimension = p_vector.size();
	VectorN snapped_vector;
	snapped_vector.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		snapped_vector.set(i, Math::snapped(p_vector[i], p_by));
	}
	return snapped_vector;
}

VectorN VectorND::subtract(const VectorN &p_a, const VectorN &p_b) {
	const int64_t dimension = MAX(p_a.size(), p_b.size());
	VectorN difference;
	difference.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		const double a = likely(i < p_a.size()) ? p_a[i] : 0.0;
		const double b = likely(i < p_b.size()) ? p_b[i] : 0.0;
		difference.set(i, a - b);
	}
	return difference;
}

VectorN VectorND::value_on_axis(const double p_value, const int64_t p_axis) {
	VectorN vector;
	vector.resize(p_axis + 1);
	for (int64_t i = 0; i < p_axis; i++) {
		vector.set(i, 0.0);
	}
	vector.set(p_axis, p_value);
	return vector;
}

VectorN VectorND::value_on_axis_with_dimension(const double p_value, const int64_t p_axis, const int64_t p_dimension) {
	VectorN vector;
	vector.resize(p_dimension);
	for (int64_t i = 0; i < p_dimension; i++) {
		vector.set(i, 0.0);
	}
	if (likely(p_axis < p_dimension)) {
		vector.set(p_axis, p_value);
	}
	return vector;
}

VectorN VectorND::with_dimension(const VectorN &p_vector, const int64_t p_dimension) {
	const int64_t old_dimension = p_vector.size();
	VectorN new_vector;
	new_vector.resize(p_dimension);
	for (int64_t i = 0; i < p_dimension; i++) {
		if (i < old_dimension) {
			new_vector.set(i, p_vector[i]);
		} else {
			new_vector.set(i, 0.0);
		}
	}
	return new_vector;
}

VectorN VectorND::with_length(const VectorN &p_vector, const double p_length) {
	const int64_t dimension = p_vector.size();
	const double vector_length = VectorND::length(p_vector);
	if (vector_length == 0) {
		return VectorND::duplicate(p_vector);
	}
	const double scale = p_length / vector_length;
	VectorN norm;
	norm.resize(dimension);
	for (int64_t i = 0; i < dimension; i++) {
		norm.set(i, p_vector[i] * scale);
	}
	return norm;
}

VectorN VectorND::zero(const int64_t p_dimension) {
	VectorN filled_vector;
	filled_vector.resize(p_dimension);
	for (int64_t i = 0; i < p_dimension; i++) {
		filled_vector.set(i, 0.0);
	}
	return filled_vector;
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
	const int64_t size = p_vector.size();
	if (likely(size > 1)) {
		return Vector2(p_vector[0], p_vector[1]);
	}
	if (size == 1) {
		return Vector2(p_vector[0], 0.0f);
	}
	return Vector2();
}

Vector3 VectorND::to_3d(const VectorN &p_vector) {
	const int64_t size = p_vector.size();
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
	const int64_t size = p_vector.size();
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
	for (int64_t i = 0; i < p_vector.size(); i++) {
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
	// Cosmetic functions.
	ClassDB::bind_static_method("VectorND", D_METHOD("axis_color", "axis"), &VectorND::axis_color);
	ClassDB::bind_static_method("VectorND", D_METHOD("axis_letter", "axis"), &VectorND::axis_letter);
	// VectorN operations.
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
	ClassDB::bind_static_method("VectorND", D_METHOD("drop_first_dimensions", "vector", "dimensions"), &VectorND::drop_first_dimensions);
	ClassDB::bind_static_method("VectorND", D_METHOD("duplicate", "vector"), &VectorND::duplicate);
	ClassDB::bind_static_method("VectorND", D_METHOD("fill", "dimension", "value"), &VectorND::fill);
	ClassDB::bind_static_method("VectorND", D_METHOD("fill_array", "dimension", "vector_amount", "value"), &VectorND::fill_array_bind);
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
	ClassDB::bind_static_method("VectorND", D_METHOD("max_absolute_axis_index", "vector"), &VectorND::max_absolute_axis_index);
	ClassDB::bind_static_method("VectorND", D_METHOD("max_axis_index", "vector"), &VectorND::max_axis_index);
	ClassDB::bind_static_method("VectorND", D_METHOD("min_axis_index", "vector"), &VectorND::min_axis_index);
	ClassDB::bind_static_method("VectorND", D_METHOD("multiply_components_together", "vector"), &VectorND::multiply_components_together);
	ClassDB::bind_static_method("VectorND", D_METHOD("multiply_vector", "a", "b", "expand"), &VectorND::multiply_vector, DEFVAL(false));
	ClassDB::bind_static_method("VectorND", D_METHOD("multiply_scalar", "vector", "scalar"), &VectorND::multiply_scalar);
	ClassDB::bind_static_method("VectorND", D_METHOD("negate", "vector"), &VectorND::negate);
	ClassDB::bind_static_method("VectorND", D_METHOD("normalized", "vector"), &VectorND::normalized);
	ClassDB::bind_static_method("VectorND", D_METHOD("one", "dimension"), &VectorND::one);
	ClassDB::bind_static_method("VectorND", D_METHOD("perpendicular", "input_vectors"), &VectorND::perpendicular_bind);
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
	ClassDB::bind_static_method("VectorND", D_METHOD("value_on_axis", "value", "axis"), &VectorND::value_on_axis);
	ClassDB::bind_static_method("VectorND", D_METHOD("with_dimension", "vector", "dimension"), &VectorND::with_dimension);
	ClassDB::bind_static_method("VectorND", D_METHOD("with_length", "vector", "length"), &VectorND::with_length, DEFVAL(1.0));
	ClassDB::bind_static_method("VectorND", D_METHOD("zero", "dimension"), &VectorND::zero);
	// Conversion.
	ClassDB::bind_static_method("VectorND", D_METHOD("from_2d", "vector"), &VectorND::from_2d);
	ClassDB::bind_static_method("VectorND", D_METHOD("from_3d", "vector"), &VectorND::from_3d);
	ClassDB::bind_static_method("VectorND", D_METHOD("from_4d", "vector"), &VectorND::from_4d);
	ClassDB::bind_static_method("VectorND", D_METHOD("to_2d", "vector"), &VectorND::to_2d);
	ClassDB::bind_static_method("VectorND", D_METHOD("to_3d", "vector"), &VectorND::to_3d);
	ClassDB::bind_static_method("VectorND", D_METHOD("to_4d", "vector"), &VectorND::to_4d);
}
