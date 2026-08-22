#pragma once

#include "../../math/plane_nd.h"

#include "tests/test_macros.h"

namespace TestPlaneND {
TEST_CASE("[PlaneND] Is Finite") {
	const Ref<PlaneND> finite_plane = PlaneND::from_normal_distance(VectorN{ 0, 1, 0, 0 }, 5.0);
	CHECK_MESSAGE(finite_plane->is_finite(), "PlaneND is_finite should be true for a plane with finite normal and distance.");
	const Ref<PlaneND> infinite_normal = PlaneND::from_normal_distance(VectorN{ 0, Math_INF, 0, 0 }, 5.0);
	CHECK_MESSAGE(!infinite_normal->is_finite(), "PlaneND is_finite should be false when the normal has an infinite component.");
	const Ref<PlaneND> infinite_distance = PlaneND::from_normal_distance(VectorN{ 0, 1, 0, 0 }, Math_INF);
	CHECK_MESSAGE(!infinite_distance->is_finite(), "PlaneND is_finite should be false when the distance is infinite.");
}
} // namespace TestPlaneND
