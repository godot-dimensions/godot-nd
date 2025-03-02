#pragma once

#include "../../math/transform_nd.h"
#include "../../math/vector_nd.h"
#include "tests/test_macros.h"

namespace TestTransformND {
TEST_CASE("[TransformND] Trivial getters/setters") {
	VectorN empty;
	VectorN zero4d = VectorN{ 0, 0, 0, 0 };
	Ref<TransformND> test;
	test.instantiate();

	Vector<VectorN> columns = { VectorN{ 1, 0, 0, 0 }, empty, empty, empty };
	test->set_all_basis_columns(columns);
	VectorN column3 = VectorN{ 0, 0, 0, 1 };
	CHECK_MESSAGE(test->get_basis_column(3) == column3, "TransformND column getter should fill in missing values with identity.");
	CHECK_MESSAGE(test->get_basis_column(10) == zero4d, "TransformND column getter should not crash when out of bounds and fill missing values with zero.");
	VectorN row2 = VectorN{ 0, 0, 1, 0 };
	CHECK_MESSAGE(test->get_basis_row(2) == row2, "TransformND row getter should fill in missing values with identity.");
}

TEST_CASE("[TransformND] Determinant") {
	Ref<TransformND> test;
	test.instantiate();

	Vector<VectorN> identity = { VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 0, 1 } };
	test->set_all_basis_columns(identity);
	CHECK_MESSAGE(test->determinant() == 1.0, "TransformND determinant of identity matrix should be 1.0.");

	Vector<VectorN> zero_y = { VectorN{ 1, 0 }, VectorN{ 0, 0 } };
	test->set_all_basis_columns(zero_y);
	CHECK_MESSAGE(test->determinant() == 0.0, "TransformND determinant of zero-y matrix should be 0.0.");

	Vector<VectorN> non_square = { VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 } };
	test->set_all_basis_columns(non_square);
	CHECK_MESSAGE(test->determinant() == 0.0, "TransformND determinant of non-square matrix should be 0.0.");

	Vector<VectorN> scaled = { VectorN{ 2, 0, 0, 0 }, VectorN{ 0, 3, 0, 0 }, VectorN{ 0, 0, 4, 0 }, VectorN{ 0, 0, 0, 5 } };
	test->set_all_basis_columns(scaled);
	CHECK_MESSAGE(test->determinant() == 2 * 3 * 4 * 5, "TransformND determinant of scaled matrix should be those scales multiplied.");

	Vector<VectorN> dependent = { VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 1, 0 } };
	test->set_all_basis_columns(dependent);
	CHECK_MESSAGE(test->determinant() == 0.0, "TransformND determinant of linearly dependent matrix should be 0.0.");

	Vector<VectorN> negative_2d = { VectorN{ -1, 0 }, VectorN{ 0, -1 } };
	test->set_all_basis_columns(negative_2d);
	CHECK_MESSAGE(test->determinant() == 1.0, "TransformND determinant of negative 2D matrix should be 1.0.");

	Vector<VectorN> negative_3d = { VectorN{ -1, 0, 0 }, VectorN{ 0, -1, 0 }, VectorN{ 0, 0, -1 } };
	test->set_all_basis_columns(negative_3d);
	CHECK_MESSAGE(test->determinant() == -1.0, "TransformND determinant of negative 3D matrix should be -1.0.");

	// Rotation in the XY plane.
	Ref<TransformND> from_rot = TransformND::from_rotation(0, 1, 0.5);
	CHECK_MESSAGE(Math::is_equal_approx(from_rot->determinant(), 1.0), "TransformND determinant of XY rotation should be 1.0 except for floating-point error.");
}

TEST_CASE("[TransformND] Compose Square") {
	// Test a transform multiplication.
	const double angle = 0.5;
	const VectorN scale = VectorN{ 2, 3 };
	const VectorN origin = VectorN{ 4, 5 };
	// Pre-compute the expected result of the composition.
	const double cos_angle = Math::cos(angle);
	const double sin_angle = Math::sin(angle);
	Vector<VectorN> precomputed_columns = { VectorN{ cos_angle * scale[0], sin_angle * scale[0] }, VectorN{ -sin_angle * scale[1], cos_angle * scale[1] } };
	Ref<TransformND> precomputed_transform;
	precomputed_transform.instantiate();
	precomputed_transform->set_all_basis_columns(precomputed_columns);
	VectorN precomputed_origin = VectorND::add(
			VectorND::multiply_scalar(precomputed_columns[0], origin[0]),
			VectorND::multiply_scalar(precomputed_columns[1], origin[1]));
	precomputed_transform->set_origin(precomputed_origin);
	// Test that the compose_square function gives the expected result.
	Ref<TransformND> rotated_transform = TransformND::from_rotation(0, 1, angle);
	Ref<TransformND> scaled_transform = TransformND::from_scale(scale);
	Ref<TransformND> position_transform = TransformND::from_position(origin);
	Ref<TransformND> composed_transform = rotated_transform->compose_square(scaled_transform)->compose_square(position_transform);
	CHECK_MESSAGE(composed_transform->is_equal_approx(precomputed_transform), "TransformND compose_square should match precomputed result.");
}

TEST_CASE("[TransformND] Compose Shrink") {
	// Test a general matrix multiplication. Set up two matrices.
	Ref<TransformND> mn; // 2x3 matrix.
	mn.instantiate();
	Ref<TransformND> no; // 3x4 matrix.
	no.instantiate();
	Vector<VectorN> mn_columns = { VectorN{ 1, 2 }, VectorN{ 3, 4 }, VectorN{ 5, 6 } };
	Vector<VectorN> no_columns = { VectorN{ 7, 8, 9 }, VectorN{ 10, 11, 12 }, VectorN{ 13, 14, 15 }, VectorN{ 16, 17, 18 } };
	mn->set_all_basis_columns(mn_columns);
	no->set_all_basis_columns(no_columns);
	// Compose then: MxN * NxO = MxO
	Ref<TransformND> composed_matrices = mn->compose_shrink(no);
	CHECK_MESSAGE(composed_matrices->get_basis_row_count() == 2, "TransformND compose_shrink should have the same row count as the first matrix.");
	CHECK_MESSAGE(composed_matrices->get_basis_column_count() == 4, "TransformND compose_shrink should have the same column count as the second matrix.");
	// Check that the data is correct.
	Ref<TransformND> mo;
	mo.instantiate();
	Vector<VectorN> mo_columns = { VectorN{ 76, 100 }, VectorN{ 103, 136 }, VectorN{ 130, 172 }, VectorN{ 157, 208 } };
	mo->set_all_basis_columns(mo_columns);
	CHECK_MESSAGE(composed_matrices->is_equal_approx(mo), "TransformND compose_shrink should match precomputed result.");
}

TEST_CASE("[TransformND] Inverse Basis") {
	Ref<TransformND> test;
	test.instantiate();
	Ref<TransformND> precomputed_inverse;
	precomputed_inverse.instantiate();

	Vector<VectorN> identity = { VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 0, 1 } };
	test->set_all_basis_columns(identity);
	CHECK_MESSAGE(test->inverse_basis()->is_equal_approx(test), "TransformND inverse_basis of identity matrix should be identity.");

	identity = { VectorN{ 1 }, VectorN{ 0, 1 }, VectorN{ 0, 0, 1 } };
	test->set_all_basis_columns(identity);
	CHECK_MESSAGE(test->inverse_basis()->is_equal_approx(test), "TransformND inverse_basis should not crash with a jagged matrix.");

	test->set_origin(VectorN{ 1, 2, 3 });
	CHECK_MESSAGE(VectorND::is_equal_approx(test->inverse()->get_origin(), VectorN{ -1, -2, -3 }), "TransformND inverse of translated matrix should match precomputed inverse.");
	CHECK_MESSAGE(!test->inverse()->is_equal_approx(test), "TransformND inverse of translated matrix should not be equal to original.");

	Vector<VectorN> scaled = { VectorN{ 2, 0, 0, 0 }, VectorN{ 0, 3, 0, 0 }, VectorN{ 0, 0, 4, 0 }, VectorN{ 0, 0, 0, 5 } };
	test->set_all_basis_columns(scaled);
	Vector<VectorN> precomputed_inv_scaled = { VectorN{ 0.5, 0, 0, 0 }, VectorN{ 0, 1.0 / 3.0, 0, 0 }, VectorN{ 0, 0, 0.25, 0 }, VectorN{ 0, 0, 0, 0.2 } };
	precomputed_inverse->set_all_basis_columns(precomputed_inv_scaled);
	CHECK_MESSAGE(test->inverse_basis()->is_equal_approx(precomputed_inverse), "TransformND inverse_basis of scaled matrix should match precomputed inverse.");

	const double angle = 0.5;
	test = TransformND::from_rotation(0, 1, angle);
	const double cos_neg_angle = Math::cos(-angle);
	const double sin_neg_angle = Math::sin(-angle);
	Vector<VectorN> precomputed_inv_rot = { VectorN{ cos_neg_angle, sin_neg_angle }, VectorN{ -sin_neg_angle, cos_neg_angle } };
	precomputed_inverse->set_all_basis_columns(precomputed_inv_rot);
	CHECK_MESSAGE(test->inverse_basis()->is_equal_approx(precomputed_inverse), "TransformND inverse_basis of rotated matrix should match precomputed inverse.");
}

TEST_CASE("[TransformND] From Rotation") {
	const double angle = 0.5;
	const double cos_angle = Math::cos(angle);
	const double sin_angle = Math::sin(angle);
	Ref<TransformND> precomputed;
	precomputed.instantiate();
	Ref<TransformND> from_rot;
	// Rotation in the XY plane.
	from_rot = TransformND::from_rotation(0, 1, angle);
	Vector<VectorN> rot_xy = { VectorN{ cos_angle, sin_angle }, VectorN{ -sin_angle, cos_angle } };
	precomputed->set_all_basis_columns(rot_xy);
	CHECK_MESSAGE(from_rot->is_equal_approx(precomputed), "TransformND from_rotation(0, 1, angle) should match precomputed XY rotation.");
	// Rotation in the YX plane (reversed, not recommended but should still work).
	from_rot = TransformND::from_rotation(1, 0, angle);
	Vector<VectorN> rot_yx = { VectorN{ cos_angle, -sin_angle }, VectorN{ sin_angle, cos_angle } };
	precomputed->set_all_basis_columns(rot_yx);
	CHECK_MESSAGE(from_rot->is_equal_approx(precomputed), "TransformND from_rotation(1, 0, angle) should match precomputed YX rotation.");
	// Rotation in the YZ plane.
	from_rot = TransformND::from_rotation(1, 2, angle);
	Vector<VectorN> rot_yz = { VectorN{}, VectorN{ 0, cos_angle, sin_angle }, VectorN{ 0, -sin_angle, cos_angle } };
	precomputed->set_all_basis_columns(rot_yz);
	CHECK_MESSAGE(from_rot->is_equal_approx(precomputed), "TransformND from_rotation(1, 2, angle) should match precomputed YZ rotation.");
	// Rotation in the ZX plane.
	from_rot = TransformND::from_rotation(2, 0, angle);
	Vector<VectorN> rot_zx = { VectorN{ cos_angle, 0, -sin_angle }, VectorN{}, VectorN{ sin_angle, 0, cos_angle } };
	precomputed->set_all_basis_columns(rot_zx);
	CHECK_MESSAGE(from_rot->is_equal_approx(precomputed), "TransformND from_rotation(2, 0, angle) should match precomputed ZX rotation.");
	// Rotation in the ZW plane.
	from_rot = TransformND::from_rotation(2, 3, angle);
	Vector<VectorN> rot_zw = { VectorN{}, VectorN{}, VectorN{ 0, 0, cos_angle, sin_angle }, VectorN{ 0, 0, -sin_angle, cos_angle } };
	precomputed->set_all_basis_columns(rot_zw);
	CHECK_MESSAGE(from_rot->is_equal_approx(precomputed), "TransformND from_rotation(2, 3, angle) should match precomputed ZW rotation.");
}

TEST_CASE("[TransformND] From Scale") {
	Ref<TransformND> precomputed;
	precomputed.instantiate();
	Ref<TransformND> from_scale;
	// Scale of [2, 3, 4, 5, 6].
	VectorN scale = VectorN{ 2, 3, 4, 5, 6 };
	from_scale = TransformND::from_scale(scale);
	Vector<VectorN> scaled23456 = { VectorN{ 2 }, VectorN{ 0, 3 }, VectorN{ 0, 0, 4 }, VectorN{ 0, 0, 0, 5 }, VectorN{ 0, 0, 0, 0, 6 } };
	precomputed->set_all_basis_columns(scaled23456);
	CHECK_MESSAGE(from_scale->is_equal_approx(precomputed), "TransformND from_scale should match precomputed scale matrix.");
}
} // namespace TestTransformND
