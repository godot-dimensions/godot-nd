#pragma once

#include "../../math/basis_nd.h"
#include "../../math/vector_nd.h"

#include "tests/test_macros.h"

namespace TestBasisND {
TEST_CASE("[BasisND] Set uniform scale abs") {
	Ref<BasisND> basis = BasisND::from_scale(VectorN{ 2, 3, 4, 5 });
	basis->set_uniform_scale_abs(7.0);
	CHECK_MESSAGE(basis->get_uniform_scale_abs() == doctest::Approx(7.0), "BasisND set_uniform_scale_abs should set the uniform scale.");
	CHECK_MESSAGE(VectorND::is_equal_approx(basis->get_scale_abs(), VectorN{ 7, 7, 7, 7 }), "BasisND set_uniform_scale_abs should normalize each column before scaling.");
	// A negative input should still result in a positive scale, since this function has no sign convention.
	basis->set_uniform_scale_abs(-3.0);
	CHECK_MESSAGE(basis->get_uniform_scale_abs() == doctest::Approx(3.0), "BasisND set_uniform_scale_abs should treat a negative scale as its absolute value.");
	CHECK_MESSAGE(VectorND::is_equal_approx(basis->get_scale_abs(), VectorN{ 3, 3, 3, 3 }), "BasisND set_uniform_scale_abs should treat a negative scale as its absolute value.");
}
} // namespace TestBasisND
