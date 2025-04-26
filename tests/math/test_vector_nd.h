#pragma once

#include "../../math/vector_nd.h"
#include "tests/test_macros.h"

namespace TestVectorND {
TEST_CASE("[VectorND] Drop first dimensions") {
	const VectorN test = { 1, 2, 3, 4, 5 };
	const VectorN dropped = VectorND::drop_first_dimensions(test, 2);
	const VectorN expected = { 3, 4, 5 };
	CHECK_MESSAGE(dropped == expected, "VectorND drop_first_dimensions with 2 should drop the first two dimensions.");
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
		CHECK_MESSAGE(perpendicular == expected, "VectorND perpendicular in N dimensions should return the correct perpendicular vector.");
	}
}
} // namespace TestVectorND
