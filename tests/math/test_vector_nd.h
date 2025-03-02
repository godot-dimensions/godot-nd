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
} // namespace TestVectorND
