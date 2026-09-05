#pragma once

#include "../../math/vector_nd.h"

#include "tests/test_macros.h"

namespace TestVectorND {
TEST_CASE("[VectorND] Drop first dimensions") {
	const VectorN test = { 1, 2, 3, 4, 5 };
	const VectorN dropped = VectorND::drop_first_dimensions(test, 2);
	const VectorN expected = { 3, 4, 5 };
	CHECK_MESSAGE(VectorND::is_equal_exact(dropped, expected), "VectorND drop_first_dimensions with 2 should drop the first two dimensions.");
}

TEST_CASE("[VectorND] Limit length taxicab") {
	const VectorN unchanged = VectorND::limit_length_taxicab(VectorN{ 1, -2, 0.5 }, 4.0);
	CHECK_MESSAGE(VectorND::is_equal_exact(unchanged, VectorN{ 1, -2, 0.5 }), "VectorND limit_length_taxicab should not change a vector within the limit.");
	const VectorN limited_2d = VectorND::limit_length_taxicab(VectorN{ 3, -3 }, 4.0);
	CHECK_MESSAGE(VectorND::is_equal_approx(limited_2d, VectorN{ 2, -2 }), "VectorND limit_length_taxicab should take away length from each axis as equally as possible, preserving signs.");
	const VectorN limited_3d = VectorND::limit_length_taxicab(VectorN{ 5, 1, 0 }, 3.0);
	CHECK_MESSAGE(VectorND::is_equal_approx(limited_3d, VectorN{ 3, 0, 0 }), "VectorND limit_length_taxicab should zero the shortest axes and take away more from the longest.");
	// This case matches the test of the specialized Vector4D function.
	const VectorN limited_4d = VectorND::limit_length_taxicab(VectorN{ 4, -2, 1, 0 }, 4.0);
	CHECK_MESSAGE(VectorND::is_equal_approx(limited_4d, VectorN{ 3, -1, 0, 0 }), "VectorND limit_length_taxicab should give the same result as the specialized Vector4D function.");
	const VectorN limited_5d = VectorND::limit_length_taxicab(VectorN{ 2, 2, 2, 2, 2 });
	CHECK_MESSAGE(VectorND::is_equal_approx(limited_5d, VectorN{ 0.2, 0.2, 0.2, 0.2, 0.2 }), "VectorND limit_length_taxicab should default to a taxicab length of 1.0.");
	const VectorN limited_empty = VectorND::limit_length_taxicab(VectorN());
	CHECK_MESSAGE(limited_empty.size() == 0, "VectorND limit_length_taxicab of an empty vector should be an empty vector.");
}

TEST_CASE("[VectorND] Is uniform") {
	CHECK_MESSAGE(VectorND::is_uniform(VectorN{ 3, 3, 3, 3 }), "VectorND is_uniform should be true when every component is equal.");
	CHECK_MESSAGE(!VectorND::is_uniform(VectorN{ 3, 3, 3, 4 }), "VectorND is_uniform should be false when any component differs.");
	CHECK_MESSAGE(VectorND::is_uniform(VectorN{ 5 }), "VectorND is_uniform should be true for a single-component vector.");
	CHECK_MESSAGE(VectorND::is_uniform(VectorN()), "VectorND is_uniform should be true for an empty vector.");
}

TEST_CASE("[VectorND] Move toward") {
	const VectorN reached = VectorND::move_toward(VectorN{ 0, 0 }, VectorN{ 1, 0 }, 5.0);
	CHECK_MESSAGE(VectorND::is_equal_exact(reached, VectorN{ 1, 0 }), "VectorND move_toward should not overshoot the target.");
	const VectorN partial = VectorND::move_toward(VectorN{ 0, 0 }, VectorN{ 10, 0 }, 4.0);
	CHECK_MESSAGE(VectorND::is_equal_approx(partial, VectorN{ 4, 0 }), "VectorND move_toward should move exactly the given delta towards the target.");
	const VectorN already_there = VectorND::move_toward(VectorN{ 1, 1 }, VectorN{ 1, 1 }, 1.0);
	CHECK_MESSAGE(VectorND::is_equal_exact(already_there, VectorN{ 1, 1 }), "VectorND move_toward should return the target when already there.");
	// Mismatched dimensions should be treated as zero in the missing axes.
	const VectorN mixed_dimension = VectorND::move_toward(VectorN{ 0, 0, 0 }, VectorN{ 3, 4 }, 2.5);
	CHECK_MESSAGE(VectorND::is_equal_approx(mixed_dimension, VectorN{ 1.5, 2.0, 0 }), "VectorND move_toward should treat a missing target element as zero.");
}

TEST_CASE("[VectorND] Random in radius") {
	for (int trial = 0; trial < 100; trial++) {
		const VectorN point = VectorND::random_in_radius(5, 3.0);
		CHECK_MESSAGE(point.size() == 5, "VectorND random_in_radius should return a vector of the requested dimension.");
		CHECK_MESSAGE(VectorND::length(point) <= 3.0 + CMP_EPSILON, "VectorND random_in_radius should never exceed the given radius.");
	}
	// The default radius should be 1.0.
	for (int trial = 0; trial < 100; trial++) {
		const VectorN point = VectorND::random_in_radius(3);
		CHECK_MESSAGE(VectorND::length(point) <= 1.0 + CMP_EPSILON, "VectorND random_in_radius should default to a radius of 1.0.");
	}
	// A high dimension must stay just as cheap as a low one. A rejection sampler
	// would hang forever here, because the fraction of a 100-dimensional hypercube
	// that lies inside its inscribed hypersphere is roughly 1e-70.
	const VectorN high_dimension = VectorND::random_in_radius(100, 2.0);
	CHECK_MESSAGE(high_dimension.size() == 100, "VectorND random_in_radius should return a vector of the requested dimension at high dimensions.");
	CHECK_MESSAGE(VectorND::length(high_dimension) <= 2.0 + CMP_EPSILON, "VectorND random_in_radius should never exceed the given radius at high dimensions.");
	const VectorN zero_dimension = VectorND::random_in_radius(0);
	CHECK_MESSAGE(zero_dimension.size() == 0, "VectorND random_in_radius of dimension zero should be an empty vector.");
	ERR_PRINT_OFF; // A negative dimension prints an error, which is expected here.
	const VectorN negative_dimension = VectorND::random_in_radius(-1);
	ERR_PRINT_ON;
	CHECK_MESSAGE(negative_dimension.size() == 0, "VectorND random_in_radius of a negative dimension should be an empty vector.");
}

TEST_CASE("[VectorND] Random in radius distribution") {
	// The test runner reseeds the global random number generator before every test
	// case, but seed explicitly so this stays reproducible no matter what else runs.
	Math::seed(0x5eed4ba11);
	constexpr int DIMENSION = 3;
	constexpr int SAMPLE_COUNT = 4000;
	int within_half_radius = 0;
	double axis_sums[DIMENSION] = { 0.0, 0.0, 0.0 };
	for (int trial = 0; trial < SAMPLE_COUNT; trial++) {
		const VectorN point = VectorND::random_in_radius(DIMENSION);
		if (VectorND::length(point) <= 0.5) {
			within_half_radius++;
		}
		for (int i = 0; i < DIMENSION; i++) {
			axis_sums[i] += point[i];
		}
	}
	// Uniformly distributed points satisfy `length <= t` with probability `t ^ N`,
	// so an eighth of them land within half of the radius in three dimensions. A
	// direction-only sampler that forgot to scale the radius would score near zero
	// here, and one that scaled linearly instead would score near a half.
	const double half_radius_fraction = (double)within_half_radius / (double)SAMPLE_COUNT;
	CHECK_MESSAGE(Math::abs(half_radius_fraction - 0.125) < 0.03, "VectorND random_in_radius should fill the volume uniformly rather than favoring the center or the surface.");
	// No axis should be favored over any other, nor either sign of an axis.
	for (int i = 0; i < DIMENSION; i++) {
		CHECK_MESSAGE(Math::abs(axis_sums[i] / (double)SAMPLE_COUNT) < 0.05, "VectorND random_in_radius should be centered on the origin in every axis.");
	}
}

TEST_CASE("[VectorND] Random in range") {
	const VectorN from = VectorN{ -1, 0, 5 };
	const VectorN to = VectorN{ 1, 10, 5 };
	for (int trial = 0; trial < 100; trial++) {
		const VectorN point = VectorND::random_in_range(from, to);
		CHECK_MESSAGE(point.size() == 3, "VectorND random_in_range should return a vector matching the dimension of the range.");
		CHECK_MESSAGE((point[0] >= -1.0 && point[0] <= 1.0), "VectorND random_in_range should keep each component within its own range.");
		CHECK_MESSAGE((point[1] >= 0.0 && point[1] <= 10.0), "VectorND random_in_range should keep each component within its own range.");
		CHECK_MESSAGE(point[2] == doctest::Approx(5.0), "VectorND random_in_range should return a fixed value when from equals to.");
	}
}

TEST_CASE("[VectorND] Rotate in plane") {
	// A 90-degree rotation in the XY-plane should behave like a standard 2D rotation.
	const VectorN rotated_x = VectorND::rotate_in_plane(VectorN{ 1, 0, 0 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, Math_PI * 0.5);
	CHECK_MESSAGE(VectorND::is_equal_approx(rotated_x, VectorN{ 0, 1, 0 }), "VectorND rotate_in_plane by 90 degrees should rotate the from-axis onto the to-axis.");
	// A vector entirely outside of the rotation plane should be unaffected.
	const VectorN unaffected = VectorND::rotate_in_plane(VectorN{ 0, 0, 1 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, Math_PI * 0.5);
	CHECK_MESSAGE(VectorND::is_equal_approx(unaffected, VectorN{ 0, 0, 1 }), "VectorND rotate_in_plane should not affect components perpendicular to the rotation plane.");
	// A vector with both in-plane and out-of-plane components should only have its in-plane part rotated.
	const VectorN mixed = VectorND::rotate_in_plane(VectorN{ 1, 0, 1 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, Math_PI * 0.5);
	CHECK_MESSAGE(VectorND::is_equal_approx(mixed, VectorN{ 0, 1, 1 }), "VectorND rotate_in_plane should rotate only the in-plane component of the vector.");
	// A full rotation should return the original vector.
	const VectorN full_turn = VectorND::rotate_in_plane(VectorN{ 2, 3, 4 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, Math_TAU);
	CHECK_MESSAGE(VectorND::is_equal_approx(full_turn, VectorN{ 2, 3, 4 }), "VectorND rotate_in_plane by a full turn should return the original vector.");
}

TEST_CASE("[VectorND] Perpendicular") {
	for (int64_t vector_amount = 1; vector_amount < 20; vector_amount++) {
		const int64_t dimension = vector_amount + 1;
		Vector<VectorN> vectors;
		vectors.resize(vector_amount);
		for (int64_t vector_index = 0; vector_index < vector_amount; vector_index++) {
			const VectorN vector = VectorND::value_on_axis_with_dimension(1.0, vector_index, dimension);
			vectors.set(vector_index, vector);
		}
		const VectorN perpendicular = VectorND::perpendicular(vectors);
		const VectorN expected = VectorND::value_on_axis_with_dimension(1.0, vector_amount, dimension);
		CHECK_MESSAGE(VectorND::is_equal_exact(perpendicular, expected), "VectorND perpendicular in N dimensions should return the correct perpendicular vector.");
		for (int64_t vector_index = 0; vector_index < vector_amount; vector_index++) {
			vectors.ptrw()[vector_index].resize(vector_index + 1);
		}
		const VectorN compact_perpendicular = VectorND::perpendicular(vectors);
		// Not operator==, which is bitwise and fails on the -0.0 that perpendicular
		// emits for a sign-flipped zero cofactor.
		CHECK(VectorND::is_equal_exact(compact_perpendicular, expected));
		CHECK(vectors[0].size() == 1); // Computing the result must not expand the input arrays.
	}
	for (int dimension = 2; dimension <= 5; dimension++) {
		Vector<VectorN> vectors;
		for (int axis = 0; axis < dimension - 1; axis++) {
			vectors.append(VectorND::value_on_axis_with_dimension(axis == 0 ? -1.0 : 1.0, axis, axis + 1));
		}
		const VectorN negative_perpendicular = VectorND::perpendicular(vectors);
		const VectorN expected = VectorND::value_on_axis_with_dimension(-1.0, dimension - 1, dimension);
		CHECK(VectorND::is_equal_exact(negative_perpendicular, expected));
		vectors.set(0, VectorN());
		const VectorN zero_perpendicular = VectorND::perpendicular(vectors);
		CHECK(VectorND::is_equal_exact(zero_perpendicular, VectorND::zero(dimension)));
		for (const int invalid_index : { 0, dimension - 2 }) {
			Vector<VectorN> invalid = vectors;
			invalid.set(invalid_index, VectorND::zero(dimension + 1));
			ERR_PRINT_OFF;
			const VectorN invalid_perpendicular = VectorND::perpendicular(invalid);
			ERR_PRINT_ON;
			CHECK(invalid_perpendicular.is_empty());
		}
	}
	ERR_PRINT_OFF;
	const VectorN empty_perpendicular = VectorND::perpendicular(Vector<VectorN>());
	ERR_PRINT_ON;
	CHECK(empty_perpendicular.is_empty());
}
} // namespace TestVectorND
