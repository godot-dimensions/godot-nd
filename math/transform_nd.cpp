#include "transform_nd.h"

#include "vector_nd.h"

#if GDEXTENSION
#include <godot_cpp/variant/typed_array.hpp>
#elif GODOT_MODULE
#include "core/variant/typed_array.h"
#endif

void make_basis_square_in_place(Vector<VectorN> &p_basis) {
	const int64_t column_count = p_basis.size();
	for (int64_t i = 0; i < column_count; i++) {
		const int64_t current_size = p_basis[i].size();
		// Force each column to have the same amount of rows as there are columns (square matrix).
		if (current_size < column_count) {
			// Grow the column and fill missing values with the identity matrix (1.0 on the diagonal).
			p_basis.write[i].resize(column_count);
			for (int j = current_size; j < column_count; j++) {
				p_basis.write[i].set(j, i == j ? 1.0 : 0.0);
			}
		} else {
			// Shrink the column, just resize, no need to write new values.
			p_basis.write[i].resize(column_count);
		}
	}
}

// Trivial getters and setters.

Vector<VectorN> TransformND::get_all_basis_columns() const {
	return _columns;
}

void TransformND::set_all_basis_columns(const Vector<VectorN> &p_columns) {
	_columns = p_columns;
}

TypedArray<VectorN> TransformND::get_all_basis_columns_bind() const {
	TypedArray<VectorN> ret;
	ret.resize(_columns.size());
	for (int i = 0; i < _columns.size(); i++) {
		ret[i] = _columns[i];
	}
	return ret;
}

void TransformND::set_all_basis_columns_bind(const TypedArray<VectorN> &p_columns) {
	_columns.resize(p_columns.size());
	for (int i = 0; i < p_columns.size(); i++) {
		_columns.set(i, p_columns[i]);
	}
}

VectorN TransformND::get_basis_column_raw(const int p_index) const {
	if (p_index >= _columns.size()) {
		return VectorN();
	}
	return _columns[p_index];
}

VectorN TransformND::get_basis_column(const int p_index) const {
	const int row_count = get_basis_row_count();
	const VectorN raw_column = p_index < _columns.size() ? _columns[p_index] : VectorN();
	if (raw_column.size() == row_count) {
		return raw_column;
	}
	VectorN filled_column;
	filled_column.resize(row_count);
	for (int i = 0; i < row_count; i++) {
		filled_column.set(i, i < raw_column.size() ? raw_column[i] : (i == p_index ? 1.0 : 0.0));
	}
	return filled_column;
}

void TransformND::set_basis_column(const int p_index, const VectorN &p_column) {
	if (p_index >= _columns.size()) {
		_columns.resize(p_index + 1);
	}
	_columns.set(p_index, p_column);
}

VectorN TransformND::get_basis_row(const int p_index) const {
	const int column_count = _columns.size();
	VectorN row;
	row.resize(column_count);
	for (int i = 0; i < column_count; i++) {
		const VectorN &column = _columns[i];
		if (p_index < column.size()) {
			row.set(i, column[p_index]);
		} else if (i == p_index) {
			row.set(i, 1.0);
		} else {
			row.set(i, 0.0);
		}
	}
	return row;
}

void TransformND::set_basis_row(const int p_index, const VectorN &p_row) {
	const int column_count = _columns.size();
	for (int i = 0; i < column_count; i++) {
		VectorN column = _columns[i];
		if (p_index >= column.size()) {
			column.resize(p_index + 1);
		}
		column.set(p_index, p_row[i]);
		_columns.set(i, column);
	}
}

double TransformND::get_basis_element(const int p_column, const int p_row) const {
	if (p_column < _columns.size()) {
		const VectorN &column = _columns[p_column];
		if (p_row < column.size()) {
			return column[p_row];
		}
	}
	if (p_column == p_row) {
		return 1.0;
	}
	return 0.0;
}

void TransformND::set_basis_element(const int p_column, const int p_row, const double p_value) {
	if (p_column >= _columns.size()) {
		_columns.resize(p_column + 1);
	}
	VectorN column = _columns[p_column];
	if (p_row >= column.size()) {
		column.resize(p_row + 1);
	}
	column.set(p_row, p_value);
	_columns.set(p_column, column);
}

VectorN TransformND::get_origin() const {
	return _origin;
}

void TransformND::set_origin(const VectorN &p_origin) {
	_origin = p_origin;
}

double TransformND::get_origin_element(const int p_index) const {
	if (p_index < _origin.size()) {
		return _origin[p_index];
	}
	return 0.0;
}

void TransformND::set_origin_element(const int p_index, const double p_value) {
	if (p_index >= _origin.size()) {
		_origin.resize(p_index + 1);
	}
	_origin.set(p_index, p_value);
}

// Dimension methods.

int TransformND::get_basis_column_count() const {
	return _columns.size();
}

void TransformND::set_basis_column_count(const int p_column_count) {
	_columns.resize(p_column_count);
}

int TransformND::get_basis_dimension() const {
	const int column_count = _columns.size();
	int dimension = column_count;
	for (int i = 0; i < column_count; i++) {
		const int column_size = _columns[i].size();
		if (column_size > dimension) {
			dimension = column_size;
		}
	}
	return dimension;
}

void TransformND::set_basis_dimension(const int p_basis_dimension) {
	_columns.resize(p_basis_dimension);
	make_basis_square_in_place(_columns);
}

int TransformND::get_basis_row_count() const {
	const int column_count = _columns.size();
	int row_count = 0;
	for (int i = 0; i < column_count; i++) {
		const int column_size = _columns[i].size();
		if (column_size > row_count) {
			row_count = column_size;
		}
	}
	return row_count;
}

void TransformND::set_basis_row_count(const int p_row_count) {
	const int column_count = _columns.size();
	for (int i = 0; i < column_count; i++) {
		VectorN column = _columns[i];
		if (column.size() < i + 1) {
			column.resize(p_row_count);
			column.set(i, 1.0);
		} else {
			column.resize(p_row_count);
		}
		_columns.set(i, column);
	}
}

int TransformND::get_dimension() const {
	return MAX(get_basis_dimension(), get_origin_dimension());
}

void TransformND::set_dimension(const int p_dimension) {
	set_basis_dimension(p_dimension);
	set_origin_dimension(p_dimension);
}

int TransformND::get_origin_dimension() const {
	return _origin.size();
}

void TransformND::set_origin_dimension(const int p_origin_dimension) {
	const int current_size = _origin.size();
	_origin.resize(p_origin_dimension);
	for (int i = current_size; i < p_origin_dimension; i++) {
		_origin.set(i, 0.0);
	}
}

Ref<TransformND> TransformND::with_dimension(const int p_dimension) const {
	Ref<TransformND> ret = duplicate();
	ret->set_dimension(p_dimension);
	return ret;
}

// Misc methods.

// Compute the determinant of an NxN matrix using Gaussian elimination with partial pivoting.
double TransformND::determinant() const {
	// Only square matrices have a determinant.
	const int column_count = _columns.size();
	const int row_count = get_basis_row_count();
	if (column_count != row_count || column_count == 0) {
		return 0.0;
	}
	// This algorithm needs to swap row items, so we need to copy the columns.
	Vector<VectorN> local_columns = _columns;
	double det = 1.0;
	int num_swaps = 0;
	for (int pivot_column_index = 0; pivot_column_index < column_count; ++pivot_column_index) {
		// Find the biggest absolute value in the column to pivot on.
		int pivot_row = pivot_column_index;
		{
			const VectorN &pivot_col = local_columns[pivot_column_index];
			double abs_max_val = Math::abs(pivot_col[pivot_column_index]);
			for (int row_index = pivot_column_index + 1; row_index < row_count; row_index++) {
				const double abs_val = Math::abs(pivot_col[row_index]);
				if (abs_val > abs_max_val) {
					abs_max_val = abs_val;
					pivot_row = row_index;
				}
			}
			// If pivot is effectively zero, determinant is zero.
			if (Math::is_zero_approx(abs_max_val)) {
				return 0.0;
			}
		}
		// If needed, swap the pivot row with the current row.
		if (pivot_row != pivot_column_index) {
			for (int inner_column_index = 0; inner_column_index < column_count; ++inner_column_index) {
				VectorN column_vector = local_columns[inner_column_index];
				ERR_FAIL_COND_V_MSG(column_vector.size() < row_count, 0.0, "TransformND.determinant: This function requires a non-jagged square matrix.");
				const double column_index_number = column_vector[pivot_column_index];
				const double pivot_row_number = column_vector[pivot_row];
				// Swap the two row elements and save them back.
				column_vector.set(pivot_column_index, pivot_row_number);
				column_vector.set(pivot_row, column_index_number);
				local_columns.set(inner_column_index, column_vector);
			}
			num_swaps++;
		}
		// Eliminate values in this column below the pivot (gaussian elimination).
		{
			VectorN pivot_column = local_columns[pivot_column_index];
			const double pivot_val = pivot_column[pivot_column_index];
			for (int row_index = pivot_column_index + 1; row_index < column_count; row_index++) {
				// factor = A[r, i] / A[i, i]
				const double row_val = pivot_column[row_index];
				const double factor = row_val / pivot_val;
				// Now subtract factor * pivot-row from each subsequent column entry
				for (int inner_column_index = pivot_column_index; inner_column_index < column_count; inner_column_index++) {
					// local_columns[c][r] -= factor * local_columns[c][i]
					VectorN inner_column_vector = local_columns[inner_column_index];
					ERR_FAIL_COND_V_MSG(inner_column_vector.size() < row_count, 0.0, "TransformND.determinant: This function requires a non-jagged square matrix.");
					const double old_val = inner_column_vector[row_index];
					const double pivot_c_val = inner_column_vector[pivot_column_index];
					const double new_val = old_val - factor * pivot_c_val;
					inner_column_vector.set(row_index, new_val);
					local_columns.set(inner_column_index, inner_column_vector);
				}
			}
		}
	}
	// Compute the product of the diagonal elements (upper triangular).
	for (int column_index = 0; column_index < column_count; ++column_index) {
		const VectorN &column = local_columns[column_index];
		det *= column[column_index];
	}
	// Each swap flips the sign, so if we had an odd number of swaps, flip the sign.
	if ((num_swaps % 2) != 0) {
		det = -det;
	}
	return det;
}

Ref<TransformND> TransformND::duplicate() const {
	Ref<TransformND> ret;
	ret.instantiate();
	ret->set_all_basis_columns(_columns);
	ret->set_origin(_origin);
	return ret;
}

bool TransformND::is_equal_approx(const Ref<TransformND> &p_other) const {
	const int basis_dimension = MAX(get_basis_dimension(), p_other->get_basis_dimension());
	for (int i = 0; i < basis_dimension; i++) {
		const VectorN &a = get_basis_column(i);
		const VectorN &b = p_other->get_basis_column(i);
		if (!VectorND::is_equal_approx(a, b)) {
			return false;
		}
	}
	return VectorND::is_equal_approx(get_origin(), p_other->get_origin());
}

Ref<TransformND> TransformND::lerp(const Ref<TransformND> &p_to, const double p_weight) const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	for (int i = 0; i < get_basis_column_count(); i++) {
		ret_columns.set(i, VectorND::lerp(get_basis_column(i), p_to->get_basis_column(i), p_weight));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(VectorND::lerp(get_origin(), p_to->get_origin(), p_weight));
	return ret;
}

// Transformation methods.

Ref<TransformND> TransformND::compose_square(const Ref<TransformND> &p_child_transform) const {
	Ref<TransformND> ret;
	ret.instantiate();
	const int dimension = MAX(MAX(get_basis_column_count(), get_origin_dimension()), MAX(p_child_transform->get_basis_column_count(), p_child_transform->get_origin_dimension()));
	const Ref<TransformND> parent = with_dimension(dimension);
	const Ref<TransformND> child = p_child_transform->with_dimension(dimension);
	const Vector<VectorN> &child_columns = child->get_all_basis_columns();
	Vector<VectorN> ret_columns;
	ret_columns.resize(child_columns.size());
	for (int i = 0; i < child_columns.size(); i++) {
		ret_columns.set(i, parent->xform_basis_axis(child_columns[i], i));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(parent->xform(p_child_transform->get_origin()));
	return ret;
}

Ref<TransformND> TransformND::compose_expand(const Ref<TransformND> &p_child_transform) const {
	Ref<TransformND> ret;
	ret.instantiate();
	const int dimension = MAX(get_dimension(), p_child_transform->get_dimension());
	const Ref<TransformND> parent = with_dimension(dimension);
	const Ref<TransformND> child = p_child_transform->with_dimension(dimension);
	const Vector<VectorN> &child_columns = child->get_all_basis_columns();
	Vector<VectorN> ret_columns;
	ret_columns.resize(child_columns.size());
	for (int i = 0; i < child_columns.size(); i++) {
		ret_columns.set(i, xform_basis_axis(child_columns[i], i));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(xform(p_child_transform->get_origin()));
	return ret;
}

Ref<TransformND> TransformND::compose_shrink(const Ref<TransformND> &p_child_transform) const {
	Ref<TransformND> ret;
	ret.instantiate();
	const Vector<VectorN> &child_columns = p_child_transform->get_all_basis_columns();
	if (child_columns.is_empty()) {
		// If the child has no columns, we treat it like an infinite identity matrix.
		ret->set_all_basis_columns(_columns);
	} else {
		// If the child does have columns, we treat it with the dimensions it has.
		Vector<VectorN> ret_columns;
		ret_columns.resize(child_columns.size());
		for (int i = 0; i < child_columns.size(); i++) {
			ret_columns.set(i, xform_basis_axis(child_columns[i], i));
		}
		ret->set_all_basis_columns(ret_columns);
	}
	ret->set_origin(xform(p_child_transform->get_origin()));
	return ret;
}

Ref<TransformND> TransformND::transform_to(const Ref<TransformND> &p_to) const {
	return p_to->compose_square(inverse());
}

void TransformND::translate_global(const VectorN &p_translation) {
	_origin = VectorND::add(_origin, p_translation);
}

void TransformND::translate_local(const VectorN &p_translation) {
	_origin = VectorND::add(_origin, xform_basis(p_translation));
}

VectorN TransformND::xform(const VectorN &p_vector) const {
	const int dimension = MIN(p_vector.size(), _columns.size());
	VectorN ret = _origin;
	for (int i = 0; i < dimension; i++) {
		ret = VectorND::add(ret, VectorND::multiply_scalar(_columns[i], p_vector[i]));
	}
	return ret;
}

Vector<VectorN> TransformND::xform_many(const Vector<VectorN> &p_vectors) const {
	Vector<VectorN> ret;
	ret.resize(p_vectors.size());
	for (int i = 0; i < p_vectors.size(); i++) {
		ret.set(i, xform(p_vectors[i]));
	}
	return ret;
}

VectorN TransformND::xform_basis(const VectorN &p_vector) const {
	const int column_count = _columns.size();
	const int dimension = MIN(p_vector.size(), column_count);
	VectorN ret;
	for (int i = 0; i < dimension; i++) {
		VectorND::multiply_scalar_and_add_in_place(_columns[i], p_vector[i], ret);
	}
	return ret;
}

VectorN TransformND::xform_basis_axis(const VectorN &p_axis, const int p_axis_index) const {
	const int column_count = _columns.size();
	const int dimension = MIN(p_axis.size(), column_count);
	VectorN ret;
	for (int i = 0; i < dimension; i++) {
		VectorND::multiply_scalar_and_add_in_place(_columns[i], p_axis[i], ret);
	}
	// This is where the special case of this function comes in to handle axes.
	// We want to treat the vector as if it had identity values in the missing dimensions.
	// For a basis, the identity has 1.0 on the diagonal, so for an axis in that basis,
	// the identity has 1.0 in the same index as the axis. So for example, if the Z
	// axis only has 2 numbers, we assume the third is 1.0, so we add one of the third column.
	if (p_axis_index < column_count && p_axis_index >= p_axis.size()) {
		VectorND::add_in_place(_columns[p_axis_index], ret);
	}
	return ret;
}

VectorN TransformND::xform_transposed(const VectorN &p_vector) const {
	return xform_transposed_basis(VectorND::subtract(p_vector, _origin));
}

VectorN TransformND::xform_transposed_basis(const VectorN &p_vector) const {
	VectorN ret;
	ret.resize(_columns.size());
	for (int i = 0; i < _columns.size(); i++) {
		ret.set(i, VectorND::dot(_columns[i], p_vector));
	}
	return ret;
}

// Inversion methods.

static bool lup_decompose(Vector<VectorN> &p_columns, PackedInt32Array &p_permutations, int p_dimension) {
	p_permutations.resize(p_dimension);
	for (int i = 0; i < p_dimension; i++) {
		p_permutations.set(i, i);
	}
	for (int pivot_index = 0; pivot_index < p_dimension; pivot_index++) {
		double largest_pivot_value = 0.0;
		int best_row_index = -1;
		// Find the pivot row.
		for (int row = pivot_index; row < p_dimension; row++) {
			const double value = Math::abs(p_columns[pivot_index][p_permutations[row]]);
			if (value > largest_pivot_value) {
				largest_pivot_value = value;
				best_row_index = row;
			}
		}
		if (Math::is_zero_approx(largest_pivot_value)) {
			return false; // Singular matrix.
		}
		// Swap permutation indices.
		const int current_perm = p_permutations[pivot_index];
		if (best_row_index != pivot_index) {
			p_permutations.set(pivot_index, p_permutations[best_row_index]);
			p_permutations.set(best_row_index, current_perm);
		}
		// Perform elimination on rows below the pivot.
		const int pivot_row = p_permutations[pivot_index];
		for (int row = pivot_index + 1; row < p_dimension; row++) {
			const int permuted_row = p_permutations[row];
			const double alpha = p_columns[pivot_index][permuted_row] / p_columns[pivot_index][pivot_row];
			p_columns.write[pivot_index].set(permuted_row, alpha);
			for (int col = pivot_index + 1; col < p_dimension; col++) {
				double new_value = p_columns[col][permuted_row] - alpha * p_columns[col][pivot_row];
				p_columns.write[col].set(permuted_row, new_value);
			}
		}
	}
	return true;
}

static Vector<VectorN> lup_invert(const Vector<VectorN> &p_decomposed, const PackedInt32Array &p_permutations, int p_dimension) {
	Vector<VectorN> inverted;
	inverted.resize(p_dimension);
	// Solve for each column.
	for (int column_index = 0; column_index < p_dimension; column_index++) {
		// Build RHS inverted column.
		VectorN inv_column;
		inv_column.resize(p_dimension);
		for (int i = 0; i < p_dimension; i++) {
			inv_column.set(i, (p_permutations[i] == column_index) ? 1.0 : 0.0);
		}
		// Forward-substitution.
		for (int i = 0; i < p_dimension; i++) {
			double sum = inv_column[i];
			for (int k = 0; k < i; k++) {
				sum -= p_decomposed[k][p_permutations[i]] * inv_column[k];
			}
			inv_column.set(i, sum); // L's diagonal = 1.0
		}
		// Back-substitution.
		for (int i = p_dimension - 1; i >= 0; i--) {
			double sum = inv_column[i];
			for (int k = i + 1; k < p_dimension; k++) {
				sum -= p_decomposed[k][p_permutations[i]] * inv_column[k];
			}
			const double pivot = p_decomposed[i][p_permutations[i]]; // U diagonal
			inv_column.set(i, sum / pivot);
		}
		inverted.set(column_index, inv_column);
	}
	return inverted;
}

Ref<TransformND> TransformND::inverse() const {
	Ref<TransformND> inv = inverse_basis();
	inv->set_origin(inv->xform_basis(VectorND::negate(_origin)));
	return inv;
}

Ref<TransformND> TransformND::inverse_basis() const {
	const int dimension = _columns.size();
	if (dimension <= 0) {
		// Nothing to invert.
		return duplicate();
	}
	Ref<TransformND> inv;
	inv.instantiate();
	// Operate on a square copy of the columns (Vector<> is copy-on-write).
	Vector<VectorN> decomposed = _columns;
	make_basis_square_in_place(decomposed);
	// LUP decompose.
	PackedInt32Array permutations;
	const bool success = lup_decompose(decomposed, permutations, dimension);
	ERR_FAIL_COND_V_MSG(!success, inv, "Matrix is singular or nearly singular.");
	// LUP invert.
	Vector<VectorN> inverted = lup_invert(decomposed, permutations, dimension);
	inv->set_all_basis_columns(inverted);
	return inv;
}

Ref<TransformND> TransformND::inverse_basis_transposed() const {
	const int column_count = _columns.size();
	const int row_count = get_basis_row_count();
	Vector<VectorN> transposed_columns;
	transposed_columns.resize(row_count);
	for (int i = 0; i < row_count; i++) {
		VectorN transposed_column;
		transposed_column.resize(column_count);
		for (int j = 0; j < column_count; j++) {
			transposed_column.set(j, _columns[j][i]);
		}
		transposed_columns.set(i, transposed_column);
	}
	Ref<TransformND> transposed;
	transposed.instantiate();
	transposed->set_all_basis_columns(transposed_columns);
	return transposed;
}

Ref<TransformND> TransformND::inverse_transposed() const {
	Ref<TransformND> inv = inverse_basis_transposed();
	inv->set_origin(xform_basis(VectorND::negate(_origin)));
	return inv;
}

// Scale methods.

VectorN TransformND::get_scale_abs() const {
	const int column_count = _columns.size();
	VectorN scale;
	scale.resize(column_count);
	for (int i = 0; i < column_count; i++) {
		scale.set(i, VectorND::length(_columns[i]));
	}
	return scale;
}

void TransformND::set_scale_abs(const VectorN &p_scale) {
	const int column_count = _columns.size();
	for (int i = 0; i < column_count; i++) {
		const double scale = p_scale[i];
		_columns.set(i, VectorND::with_length(_columns[i], scale));
	}
}

double TransformND::get_uniform_scale() const {
	// Scale computed from the determinant only makes sense for square matrices.
	const int column_count = _columns.size();
	const int row_count = get_basis_row_count();
	if (column_count != row_count) {
		return get_uniform_scale_abs();
	}
	const double det = determinant();
	// Take the nth root of the determinant, while handling negative
	// determinants as a special case that gives a negative scale.
	if (det < 0.0) {
		return -Math::pow(-det, 1.0 / column_count);
	} else {
		return Math::pow(det, 1.0 / column_count);
	}
}

double TransformND::get_uniform_scale_abs() const {
	const double column_count = _columns.size();
	double all_scales = 1.0;
	for (int i = 0; i < column_count; i++) {
		const double column_scale = VectorND::length(_columns[i]);
		if (column_scale == 0.0) {
			return 0.0;
		}
		all_scales *= column_scale;
	}
	return Math::pow(all_scales, 1.0 / column_count);
}

void TransformND::scale_global(const VectorN &p_scale) {
	for (int i = 0; i < _columns.size(); i++) {
		_columns.set(i, VectorND::multiply_vector(_columns[i], p_scale));
	}
	_origin = VectorND::multiply_vector(_origin, p_scale);
}

Ref<TransformND> TransformND::scaled_global(const VectorN &p_scale) const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	ret_columns.resize(_columns.size());
	for (int i = 0; i < ret_columns.size(); i++) {
		ret_columns.set(i, VectorND::multiply_vector(_columns[i], p_scale));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(VectorND::multiply_vector(_origin, p_scale));
	return ret;
}

void TransformND::scale_local(const VectorN &p_scale) {
	ERR_FAIL_COND_MSG(p_scale.is_empty(), "TransformND.scale_local: Cannot scale with nothing.");
	for (int i = 0; i < _columns.size(); i++) {
		const double scale = i < p_scale.size() ? p_scale[i] : 1.0;
		_columns.set(i, VectorND::multiply_scalar(_columns[i], scale));
	}
}

Ref<TransformND> TransformND::scaled_local(const VectorN &p_scale) const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	ret_columns.resize(_columns.size());
	for (int i = 0; i < ret_columns.size(); i++) {
		const double scale = p_scale[i];
		ret_columns.set(i, VectorND::multiply_scalar(_columns[i], scale));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(_origin);
	return ret;
}

void TransformND::scale_uniform(const double p_scale) {
	for (int i = 0; i < _columns.size(); i++) {
		_columns.set(i, VectorND::multiply_scalar(_columns[i], p_scale));
	}
}

Ref<TransformND> TransformND::scaled_uniform(const double p_scale) const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	ret_columns.resize(_columns.size());
	for (int i = 0; i < ret_columns.size(); i++) {
		ret_columns.set(i, VectorND::multiply_scalar(_columns[i], p_scale));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(_origin);
	return ret;
}

// Validation methods.

Ref<TransformND> TransformND::conformalized() const {
	// Conformalize.
	const double uniform_scale = get_uniform_scale();
	Ref<TransformND> ret = orthonormalized();
	return ret->scaled_uniform(uniform_scale);
}

Ref<TransformND> TransformND::normalized() const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	const int column_count = _columns.size();
	ret_columns.resize(column_count);
	// Normalize.
	for (int i = 0; i < column_count; i++) {
		ret_columns.set(i, VectorND::normalized(_columns[i]));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(_origin);
	return ret;
}

Ref<TransformND> TransformND::orthonormalized() const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	const int column_count = _columns.size();
	ret_columns.resize(column_count);
	// Gram-Schmidt Process, now in ND.
	// https://en.wikipedia.org/wiki/Gram-Schmidt_process
	for (int i = 0; i < column_count; i++) {
		VectorN column = _columns[i];
		for (int j = 0; j < i; j++) {
			const VectorN &other_column = ret_columns[j];
			const double dot = VectorND::dot(column, other_column);
			column = VectorND::subtract(column, VectorND::multiply_scalar(other_column, dot));
		}
		ret_columns.set(i, VectorND::normalized(column));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(_origin);
	return ret;
}

Ref<TransformND> TransformND::orthonormalized_axis_aligned() const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	const int column_count = _columns.size();
	ret_columns.resize(column_count);
	// Gram-Schmidt Process, now in ND.
	// https://en.wikipedia.org/wiki/Gram-Schmidt_process
	for (int i = 0; i < column_count; i++) {
		VectorN column = _columns[i];
		for (int j = 0; j < i; j++) {
			const VectorN &other_column = ret_columns[j];
			const double dot = VectorND::dot(column, other_column);
			column = VectorND::subtract(column, VectorND::multiply_scalar(other_column, dot));
		}
		const int64_t max_abs_index = VectorND::max_absolute_axis_index(column);
		if (max_abs_index != -1) {
			column = VectorND::value_on_axis_with_dimension(SIGN(column[max_abs_index]), max_abs_index, column_count);
		}
		ret_columns.set(i, VectorND::normalized(column));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(_origin);
	return ret;
}

Ref<TransformND> TransformND::orthogonalized() const {
	const VectorN scale_abs = get_scale_abs();
	Ref<TransformND> ret = orthonormalized();
	ret->scale_local(scale_abs);
	return ret;
}

// Returns true if the basis is conformal (orthogonal, uniform scale, preserves angles and distance ratios).
// See https://en.wikipedia.org/wiki/Conformal_linear_transformation
bool TransformND::is_conformal() const {
	return is_uniform_scale() && is_orthogonal();
}

bool TransformND::is_diagonal() const {
	const int column_count = _columns.size();
	for (int i = 0; i < column_count; i++) {
		const VectorN &column = _columns[i];
		for (int j = i + 1; j < column_count; j++) {
			if (i != j && !Math::is_zero_approx(column[j])) {
				return false;
			}
		}
	}
	return true;
}

bool TransformND::is_normalized() const {
	const int column_count = _columns.size();
	for (int i = 0; i < column_count; i++) {
		const VectorN &column = _columns[i];
		if (!Math::is_equal_approx(VectorND::length_squared(column), 1.0)) {
			return false;
		}
	}
	return true;
}

// Returns true if the basis vectors are orthogonal (perpendicular), so it has no skew or shear, and can be decomposed into rotation and scale.
// See https://en.wikipedia.org/wiki/Orthogonal_basis
bool TransformND::is_orthogonal() const {
	const int column_count = _columns.size();
	for (int i = 0; i < column_count; i++) {
		const VectorN &column = _columns[i];
		for (int j = i + 1; j < column_count; j++) {
			const VectorN &other_column = _columns[j];
			if (!Math::is_zero_approx(VectorND::dot(column, other_column))) {
				return false;
			}
		}
	}
	return true;
}

// Returns true if the basis vectors are orthonormal (orthogonal and normalized), so it has no scale, skew, or shear.
// See https://en.wikipedia.org/wiki/Orthonormal_basis
bool TransformND::is_orthonormal() const {
	return is_normalized() && is_orthogonal();
}

bool TransformND::is_rotation() const {
	const double det = determinant();
	return is_orthonormal() && Math::is_equal_approx(det, 1.0);
}

bool TransformND::is_uniform_scale() const {
	const VectorN scale_abs = get_scale_abs();
	for (int i = 1; i < scale_abs.size(); i++) {
		if (!Math::is_equal_approx(scale_abs[0], scale_abs[i])) {
			return false;
		}
	}
	return true;
}

// Trivial math. Not useful by itself, but can be a part of a larger expression.

Ref<TransformND> TransformND::add(const Ref<TransformND> &p_other) const {
	Ref<TransformND> ret;
	ret.instantiate();
	const int column_count = MAX(get_basis_column_count(), p_other->get_basis_column_count());
	Vector<VectorN> ret_columns;
	ret_columns.resize(column_count);
	for (int i = 0; i < column_count; i++) {
		const VectorN &a = get_basis_column_raw(i);
		const VectorN &b = p_other->get_basis_column_raw(i);
		ret_columns.set(i, VectorND::add(a, b));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(VectorND::add(get_origin(), p_other->get_origin()));
	return ret;
}

Ref<TransformND> TransformND::divide_scalar(const double p_scalar) const {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	ret_columns.resize(_columns.size());
	for (int i = 0; i < ret_columns.size(); i++) {
		ret_columns.set(i, VectorND::divide_scalar(_columns[i], p_scalar));
	}
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(VectorND::divide_scalar(_origin, p_scalar));
	return ret;
}

// Conversion.

Transform2D TransformND::to_2d() {
	return Transform2D(
			get_basis_element(0, 0),
			get_basis_element(0, 1),
			get_basis_element(1, 0),
			get_basis_element(1, 1),
			get_origin_element(0),
			get_origin_element(1));
}

Transform3D TransformND::to_3d() {
	const Basis basis = Basis(
			Vector3(get_basis_element(0, 0), get_basis_element(0, 1), get_basis_element(0, 2)),
			Vector3(get_basis_element(1, 0), get_basis_element(1, 1), get_basis_element(1, 2)),
			Vector3(get_basis_element(2, 0), get_basis_element(2, 1), get_basis_element(2, 2)));
	const Vector3 origin = Vector3(get_origin_element(0), get_origin_element(1), get_origin_element(2));
	return Transform3D(basis, origin);
}

Projection TransformND::to_4d() {
	return Projection(
			Vector4(get_basis_element(0, 0), get_basis_element(0, 1), get_basis_element(0, 2), get_basis_element(0, 3)),
			Vector4(get_basis_element(1, 0), get_basis_element(1, 1), get_basis_element(1, 2), get_basis_element(1, 3)),
			Vector4(get_basis_element(2, 0), get_basis_element(2, 1), get_basis_element(2, 2), get_basis_element(2, 3)),
			Vector4(get_basis_element(3, 0), get_basis_element(3, 1), get_basis_element(3, 2), get_basis_element(3, 3)));
}

String TransformND::to_string() {
	String ret = "TransformND(B[";
	const int column_count = _columns.size();
	for (int i = 0; i < column_count; i++) {
		const VectorN &column = _columns[i];
		ret += String::num_int64(i) + ":" + VectorND::to_string(column);
		if (i < column_count - 1) {
			ret += ", ";
		}
	}
	ret += "], O" + VectorND::to_string(_origin) + ")";
	return ret;
}

Ref<TransformND> TransformND::from_2d(const Transform2D &p_transform) {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	ret_columns.resize(2);
	ret_columns.set(0, VectorND::from_2d(p_transform[0]));
	ret_columns.set(1, VectorND::from_2d(p_transform[1]));
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(VectorND::from_2d(p_transform[2]));
	return ret;
}

Ref<TransformND> TransformND::from_3d(const Transform3D &p_transform) {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	ret_columns.resize(3);
	ret_columns.set(0, VectorND::from_3d(p_transform.basis.get_column(0)));
	ret_columns.set(1, VectorND::from_3d(p_transform.basis.get_column(1)));
	ret_columns.set(2, VectorND::from_3d(p_transform.basis.get_column(2)));
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(VectorND::from_3d(p_transform.origin));
	return ret;
}

Ref<TransformND> TransformND::from_4d(const Projection &p_basis, const Vector4 &p_origin) {
	Ref<TransformND> ret;
	ret.instantiate();
	Vector<VectorN> ret_columns;
	ret_columns.resize(4);
	ret_columns.set(0, VectorND::from_4d(p_basis[0]));
	ret_columns.set(1, VectorND::from_4d(p_basis[1]));
	ret_columns.set(2, VectorND::from_4d(p_basis[2]));
	ret_columns.set(3, VectorND::from_4d(p_basis[3]));
	ret->set_all_basis_columns(ret_columns);
	ret->set_origin(VectorND::from_4d(p_origin));
	return ret;
}

// Constructors.

Ref<TransformND> TransformND::from_basis_columns(const Vector<VectorN> &p_columns) {
	Ref<TransformND> ret;
	ret.instantiate();
	ret->set_all_basis_columns(p_columns);
	return ret;
}

Ref<TransformND> TransformND::from_position(const VectorN &p_position) {
	Ref<TransformND> ret;
	ret.instantiate();
	ret->set_origin(p_position);
	return ret;
}

Ref<TransformND> TransformND::from_position_rotation(const VectorN &p_position, const int p_rot_from, const int p_rot_to, const double p_rot_angle) {
	Ref<TransformND> ret = from_rotation(p_rot_from, p_rot_to, p_rot_angle);
	ret->set_origin(p_position);
	return ret;
}

Ref<TransformND> TransformND::from_position_rotation_scale(const VectorN &p_position, const int p_rot_from, const int p_rot_to, const double p_rot_angle, const VectorN &p_scale) {
	Ref<TransformND> ret = from_rotation(p_rot_from, p_rot_to, p_rot_angle);
	ret->scale_local(p_scale);
	ret->set_origin(p_position);
	return ret;
}

Ref<TransformND> TransformND::from_position_scale(const VectorN &p_position, const VectorN &p_scale) {
	Ref<TransformND> ret = from_scale(p_scale);
	ret->set_origin(p_position);
	return ret;
}

Ref<TransformND> TransformND::from_rotation(const int p_rot_from, const int p_rot_to, const double p_rot_angle) {
	Ref<TransformND> ret;
	ret.instantiate();
	ERR_FAIL_COND_V_MSG(p_rot_from < 0 || p_rot_to < 0 || p_rot_from == p_rot_to, ret, "Invalid rotation dimension indices.");
	const int array_size = MAX(p_rot_from, p_rot_to) + 1;
	// Allocate the columns.
	Vector<VectorN> ret_columns;
	ret_columns.resize(array_size);
	VectorN from_column;
	from_column.resize(array_size);
	VectorN to_column;
	to_column.resize(array_size);
	for (int i = 0; i < array_size - 1; i++) {
		from_column.set(i, 0.0);
		to_column.set(i, 0.0);
	}
	// Set the rotation values.
	const double cos_angle = Math::cos(p_rot_angle);
	const double sin_angle = Math::sin(p_rot_angle);
	from_column.set(p_rot_from, cos_angle);
	from_column.set(p_rot_to, sin_angle);
	to_column.set(p_rot_from, -sin_angle);
	to_column.set(p_rot_to, cos_angle);
	// Fill the transform.
	ret_columns.set(p_rot_from, from_column);
	ret_columns.set(p_rot_to, to_column);
	ret->set_all_basis_columns(ret_columns);
	return ret;
}

Ref<TransformND> TransformND::from_scale(const VectorN &p_scale) {
	Ref<TransformND> ret;
	ret.instantiate();
	const int dimension = p_scale.size();
	Vector<VectorN> ret_columns;
	ret_columns.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		VectorN column;
		column.resize(i + 1);
		for (int j = 0; j < i; j++) {
			column.set(j, 0.0);
		}
		column.set(i, p_scale[i]);
		ret_columns.set(i, column);
	}
	ret->set_all_basis_columns(ret_columns);
	return ret;
}

Ref<TransformND> TransformND::from_swap_rotation(const int p_rot_from, const int p_rot_to) {
	Ref<TransformND> ret;
	ret.instantiate();
	ERR_FAIL_COND_V_MSG(p_rot_from < 0 || p_rot_to < 0 || p_rot_from == p_rot_to, ret, "Invalid rotation dimension indices.");
	const int array_size = MAX(p_rot_from, p_rot_to) + 1;
	// Allocate the columns.
	Vector<VectorN> ret_columns;
	ret_columns.resize(array_size);
	VectorN from_column;
	from_column.resize(array_size);
	VectorN to_column;
	to_column.resize(array_size);
	for (int i = 0; i < array_size - 1; i++) {
		from_column.set(i, 0.0);
		to_column.set(i, 0.0);
	}
	// Set the rotation values as if the rotation was 90 degrees.
	from_column.set(p_rot_to, 1.0);
	to_column.set(p_rot_from, -1.0);
	ret_columns.set(p_rot_from, from_column);
	ret_columns.set(p_rot_to, to_column);
	ret->set_all_basis_columns(ret_columns);
	return ret;
}

Ref<TransformND> TransformND::identity_basis(const int p_dimension) {
	Ref<TransformND> ret;
	ret.instantiate();
	// Allocate the columns.
	Vector<VectorN> ret_columns;
	ret_columns.resize(p_dimension);
	for (int i = 0; i < p_dimension; i++) {
		VectorN column;
		column.resize(p_dimension);
		for (int j = 0; j < p_dimension; j++) {
			if (i == j) {
				column.set(j, 1.0);
			} else {
				column.set(j, 0.0);
			}
		}
		ret_columns.set(i, column);
	}
	ret->set_all_basis_columns(ret_columns);
	return ret;
}

Ref<TransformND> TransformND::identity_transform(const int p_dimension) {
	Ref<TransformND> ret = identity_basis(p_dimension);
	ret->set_origin(VectorND::fill(0.0, p_dimension));
	return ret;
}

void TransformND::_bind_methods() {
	// Trivial getters and setters.
	ClassDB::bind_method(D_METHOD("get_all_basis_columns"), &TransformND::get_all_basis_columns_bind);
	ClassDB::bind_method(D_METHOD("set_all_basis_columns", "columns"), &TransformND::set_all_basis_columns_bind);
	ClassDB::bind_method(D_METHOD("get_basis_column_raw", "index"), &TransformND::get_basis_column_raw);
	ClassDB::bind_method(D_METHOD("get_basis_column", "index"), &TransformND::get_basis_column);
	ClassDB::bind_method(D_METHOD("set_basis_column", "index", "column"), &TransformND::set_basis_column);
	ClassDB::bind_method(D_METHOD("get_basis_row", "index"), &TransformND::get_basis_row);
	ClassDB::bind_method(D_METHOD("set_basis_row", "index", "row"), &TransformND::set_basis_row);
	ClassDB::bind_method(D_METHOD("get_basis_element", "column", "row"), &TransformND::get_basis_element);
	ClassDB::bind_method(D_METHOD("set_basis_element", "column", "row", "value"), &TransformND::set_basis_element);
	ClassDB::bind_method(D_METHOD("get_origin"), &TransformND::get_origin);
	ClassDB::bind_method(D_METHOD("set_origin", "origin"), &TransformND::set_origin);
	ClassDB::bind_method(D_METHOD("get_origin_element", "index"), &TransformND::get_origin_element);
	ClassDB::bind_method(D_METHOD("set_origin_element", "index", "value"), &TransformND::set_origin_element);
	// Dimension methods.
	ClassDB::bind_method(D_METHOD("get_basis_column_count"), &TransformND::get_basis_column_count);
	ClassDB::bind_method(D_METHOD("set_basis_column_count", "count"), &TransformND::set_basis_column_count);
	ClassDB::bind_method(D_METHOD("get_basis_dimension"), &TransformND::get_basis_dimension);
	ClassDB::bind_method(D_METHOD("set_basis_dimension", "dimension"), &TransformND::set_basis_dimension);
	ClassDB::bind_method(D_METHOD("get_basis_row_count"), &TransformND::get_basis_row_count);
	ClassDB::bind_method(D_METHOD("set_basis_row_count", "count"), &TransformND::set_basis_row_count);
	ClassDB::bind_method(D_METHOD("get_dimension"), &TransformND::get_dimension);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &TransformND::set_dimension);
	ClassDB::bind_method(D_METHOD("get_origin_dimension"), &TransformND::get_origin_dimension);
	ClassDB::bind_method(D_METHOD("set_origin_dimension", "dimension"), &TransformND::set_origin_dimension);
	ClassDB::bind_method(D_METHOD("with_dimension", "dimension"), &TransformND::with_dimension);
	// Misc methods.
	ClassDB::bind_method(D_METHOD("determinant"), &TransformND::determinant);
	ClassDB::bind_method(D_METHOD("duplicate"), &TransformND::duplicate);
	ClassDB::bind_method(D_METHOD("is_equal_approx", "other"), &TransformND::is_equal_approx);
	ClassDB::bind_method(D_METHOD("lerp", "to", "weight"), &TransformND::lerp);
	// Transformation methods.
	ClassDB::bind_method(D_METHOD("compose_square", "child_transform"), &TransformND::compose_square);
	ClassDB::bind_method(D_METHOD("compose_expand", "child_transform"), &TransformND::compose_expand);
	ClassDB::bind_method(D_METHOD("compose_shrink", "child_transform"), &TransformND::compose_shrink);
	ClassDB::bind_method(D_METHOD("transform_to", "to"), &TransformND::transform_to);
	ClassDB::bind_method(D_METHOD("translate_global", "translation"), &TransformND::translate_global);
	ClassDB::bind_method(D_METHOD("translate_local", "translation"), &TransformND::translate_local);
	ClassDB::bind_method(D_METHOD("xform", "vector"), &TransformND::xform);
	ClassDB::bind_method(D_METHOD("xform_basis", "vector"), &TransformND::xform_basis);
	ClassDB::bind_method(D_METHOD("xform_basis_axis", "axis", "axis_index"), &TransformND::xform_basis_axis);
	ClassDB::bind_method(D_METHOD("xform_transposed", "vector"), &TransformND::xform_transposed);
	ClassDB::bind_method(D_METHOD("xform_transposed_basis", "vector"), &TransformND::xform_transposed_basis);
	// Inversion methods.
	ClassDB::bind_method(D_METHOD("inverse"), &TransformND::inverse);
	ClassDB::bind_method(D_METHOD("inverse_basis"), &TransformND::inverse_basis);
	ClassDB::bind_method(D_METHOD("inverse_basis_transposed"), &TransformND::inverse_basis_transposed);
	ClassDB::bind_method(D_METHOD("inverse_transposed"), &TransformND::inverse_transposed);
	// Scale methods.
	ClassDB::bind_method(D_METHOD("get_scale_abs"), &TransformND::get_scale_abs);
	ClassDB::bind_method(D_METHOD("set_scale_abs", "scale"), &TransformND::set_scale_abs);
	ClassDB::bind_method(D_METHOD("get_uniform_scale"), &TransformND::get_uniform_scale);
	ClassDB::bind_method(D_METHOD("get_uniform_scale_abs"), &TransformND::get_uniform_scale_abs);
	ClassDB::bind_method(D_METHOD("scaled_global", "scale"), &TransformND::scaled_global);
	ClassDB::bind_method(D_METHOD("scaled_local", "scale"), &TransformND::scaled_local);
	ClassDB::bind_method(D_METHOD("scaled_uniform", "scale"), &TransformND::scaled_uniform);
	// Validation methods.
	ClassDB::bind_method(D_METHOD("conformalized"), &TransformND::conformalized);
	ClassDB::bind_method(D_METHOD("normalized"), &TransformND::normalized);
	ClassDB::bind_method(D_METHOD("orthonormalized"), &TransformND::orthonormalized);
	ClassDB::bind_method(D_METHOD("orthonormalized_axis_aligned"), &TransformND::orthonormalized_axis_aligned);
	ClassDB::bind_method(D_METHOD("orthogonalized"), &TransformND::orthogonalized);
	ClassDB::bind_method(D_METHOD("is_conformal"), &TransformND::is_conformal);
	ClassDB::bind_method(D_METHOD("is_diagonal"), &TransformND::is_diagonal);
	ClassDB::bind_method(D_METHOD("is_normalized"), &TransformND::is_normalized);
	ClassDB::bind_method(D_METHOD("is_orthogonal"), &TransformND::is_orthogonal);
	ClassDB::bind_method(D_METHOD("is_orthonormal"), &TransformND::is_orthonormal);
	ClassDB::bind_method(D_METHOD("is_rotation"), &TransformND::is_rotation);
	ClassDB::bind_method(D_METHOD("is_uniform_scale"), &TransformND::is_uniform_scale);
	// Trivial math. Not useful by itself, but can be a part of a larger expression.
	ClassDB::bind_method(D_METHOD("add", "other"), &TransformND::add);
	ClassDB::bind_method(D_METHOD("divide_scalar", "scalar"), &TransformND::divide_scalar);
	// Conversion.
	ClassDB::bind_method(D_METHOD("to_2d"), &TransformND::to_2d);
	ClassDB::bind_method(D_METHOD("to_3d"), &TransformND::to_3d);
	ClassDB::bind_method(D_METHOD("to_4d"), &TransformND::to_4d);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_2d", "transform"), &TransformND::from_2d);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_3d", "transform"), &TransformND::from_3d);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_4d", "basis", "origin"), &TransformND::from_4d);
	// Constructors.
	ClassDB::bind_static_method("TransformND", D_METHOD("from_position", "position"), &TransformND::from_position);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_position_rotation", "position", "rot_from", "rot_to", "rot_angle"), &TransformND::from_position_rotation);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_position_rotation_scale", "position", "rot_from", "rot_to", "rot_angle", "scale"), &TransformND::from_position_rotation_scale);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_position_scale", "position", "scale"), &TransformND::from_position_scale);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_rotation", "rot_from", "rot_to", "rot_angle"), &TransformND::from_rotation);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_scale", "scale"), &TransformND::from_scale);
	ClassDB::bind_static_method("TransformND", D_METHOD("from_swap_rotation", "rot_from", "rot_to"), &TransformND::from_swap_rotation);
	ClassDB::bind_static_method("TransformND", D_METHOD("identity_basis", "dimension"), &TransformND::identity_basis);
	ClassDB::bind_static_method("TransformND", D_METHOD("identity_transform", "dimension"), &TransformND::identity_transform);
	// Properties.
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "origin", PROPERTY_HINT_NONE, "suffix:m"), "set_origin", "get_origin");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "scale_abs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_scale_abs", "get_scale_abs");
}
