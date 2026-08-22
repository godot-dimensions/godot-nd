#pragma once

#include "../../math/rect_nd.h"
#include "../../math/vector_nd.h"

#include "tests/test_macros.h"

namespace TestRectND {
TEST_CASE("[RectND] Basic math functions") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	CHECK_MESSAGE(unit_rect->get_space() == doctest::Approx(1.0), "RectND get_space of a unit rect should be 1.0.");
	CHECK_MESSAGE(unit_rect->has_space(), "RectND has_space should be true when every size element is positive.");
	CHECK_MESSAGE(unit_rect->get_surface() == doctest::Approx(8.0), "RectND get_surface of a 4D unit rect should be 8.0.");
	CHECK_MESSAGE(unit_rect->has_surface(), "RectND has_surface should be true when any size element is positive.");
	CHECK_MESSAGE(unit_rect->has_any_size(), "RectND has_any_size should be true when any size element is non-zero.");

	const Ref<RectND> flat_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 0 });
	CHECK_MESSAGE(flat_rect->get_space() == doctest::Approx(0.0), "RectND get_space of a flat rect should be 0.0.");
	CHECK_MESSAGE(!flat_rect->has_space(), "RectND has_space should be false when any size element is zero.");
	CHECK_MESSAGE(flat_rect->has_surface(), "RectND has_surface should be true when any size element is positive.");
	CHECK_MESSAGE(flat_rect->has_any_size(), "RectND has_any_size should be true when any size element is non-zero.");

	const Ref<RectND> empty_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 0, 0, 0, 0 });
	CHECK_MESSAGE(!empty_rect->has_space(), "RectND has_space should be false for a zero-size rect.");
	CHECK_MESSAGE(!empty_rect->has_surface(), "RectND has_surface should be false for a zero-size rect.");
	CHECK_MESSAGE(!empty_rect->has_any_size(), "RectND has_any_size should be false for a zero-size rect.");

	const Ref<RectND> negative_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ -1, -1, -1, -1 });
	CHECK_MESSAGE(!negative_rect->has_space(), "RectND has_space should be false for a negative-size rect.");
	CHECK_MESSAGE(!negative_rect->has_surface(), "RectND has_surface should be false for a negative-size rect.");
	CHECK_MESSAGE(negative_rect->has_any_size(), "RectND has_any_size should be true for a negative-size rect.");
	const Ref<RectND> absolute_rect = negative_rect->abs();
	CHECK_MESSAGE(VectorND::is_equal_exact(absolute_rect->get_position(), VectorN{ -1, -1, -1, -1 }), "RectND abs should move the position to the negative end.");
	CHECK_MESSAGE(VectorND::is_equal_exact(absolute_rect->get_size(), VectorN{ 1, 1, 1, 1 }), "RectND abs should make the size positive.");

	// A rect with more position elements than size elements has zero size in the extra axes.
	const Ref<RectND> jagged_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, -1 });
	CHECK_MESSAGE(jagged_rect->get_dimension() == 4, "RectND get_dimension should be the maximum of the position and size dimensions.");
	CHECK_MESSAGE(jagged_rect->get_space() == doctest::Approx(0.0), "RectND get_space should treat missing size elements as zero.");
	const Ref<RectND> jagged_abs = jagged_rect->abs();
	CHECK_MESSAGE(VectorND::is_equal_exact(jagged_abs->get_position(), VectorN{ 0, -1, 0, 0 }), "RectND abs should handle a rect with a shorter size than position.");
	CHECK_MESSAGE(VectorND::is_equal_exact(jagged_abs->get_size(), VectorN{ 1, 1, 0, 0 }), "RectND abs should never leave a negative size element.");
}

TEST_CASE("[RectND] Intersection and merge") {
	// An intersection where only the position is clamped, and the size must shrink to match.
	const Ref<RectND> big_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 10, 10, 10, 10 });
	const Ref<RectND> offset_rect = RectND::from_position_size(VectorN{ 5, 5, 5, 5 }, VectorN{ 10, 10, 10, 10 });
	Ref<RectND> intersected = big_rect->intersection(offset_rect);
	CHECK_MESSAGE(VectorND::is_equal_exact(intersected->get_position(), VectorN{ 5, 5, 5, 5 }), "RectND intersection should clamp the position to the larger of the two.");
	CHECK_MESSAGE(VectorND::is_equal_exact(intersected->get_end(), VectorN{ 10, 10, 10, 10 }), "RectND intersection should clamp the end to the smaller of the two.");
	CHECK_MESSAGE(VectorND::is_equal_exact(intersected->get_size(), VectorN{ 5, 5, 5, 5 }), "RectND intersection size should follow the clamped position and end.");
	// An intersection fully contained within this rect.
	const Ref<RectND> inner_rect = RectND::from_position_size(VectorN{ 2, 2, 2, 2 }, VectorN{ 3, 3, 3, 3 });
	intersected = big_rect->intersection(inner_rect);
	CHECK_MESSAGE(VectorND::is_equal_exact(intersected->get_position(), VectorN{ 2, 2, 2, 2 }), "RectND intersection with an enclosed rect should give the enclosed position.");
	CHECK_MESSAGE(VectorND::is_equal_exact(intersected->get_size(), VectorN{ 3, 3, 3, 3 }), "RectND intersection with an enclosed rect should give the enclosed size.");
	// No intersection at all.
	const Ref<RectND> far_rect = RectND::from_position_size(VectorN{ 100, 100, 100, 100 }, VectorN{ 1, 1, 1, 1 });
	intersected = big_rect->intersection(far_rect);
	CHECK_MESSAGE(!intersected->has_any_size(), "RectND intersection of non-overlapping rects should have no size.");
	// Mixed dimensions: the 2D rect is a degenerate slab at Z=0 and W=0, which the 4D rect contains.
	const Ref<RectND> rect_2d = RectND::from_position_size(VectorN{ 2, 2 }, VectorN{ 3, 3 });
	intersected = big_rect->intersection(rect_2d);
	CHECK_MESSAGE(VectorND::is_equal_exact(intersected->get_position(), VectorN{ 2, 2, 0, 0 }), "RectND intersection should treat missing elements as zero.");
	CHECK_MESSAGE(VectorND::is_equal_exact(intersected->get_size(), VectorN{ 3, 3, 0, 0 }), "RectND intersection with a lower-dimensional rect should be degenerate in the extra axes.");
	// Merging across dimensions expands to the highest dimension.
	const Ref<RectND> merged = rect_2d->merge(far_rect);
	CHECK_MESSAGE(VectorND::is_equal_exact(merged->get_position(), VectorN{ 2, 2, 0, 0 }), "RectND merge should expand to the highest dimension.");
	CHECK_MESSAGE(VectorND::is_equal_exact(merged->get_end(), VectorN{ 101, 101, 101, 101 }), "RectND merge should enclose both rects.");
}

TEST_CASE("[RectND] Point functions with mismatched dimensions") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	// A lower-dimensional point has zeros in the missing axes, which are inside the unit rect.
	CHECK_MESSAGE(unit_rect->has_point(VectorN{ 0.5, 0.5 }), "RectND has_point should treat missing point elements as zero.");
	CHECK_MESSAGE(unit_rect->has_point(VectorN{}), "RectND has_point should accept an empty point when the rect contains the origin.");
	CHECK_MESSAGE(!unit_rect->has_point(VectorN{ 0.5, 0.5, 0.5, 0.5, 5 }), "RectND has_point should treat missing rect elements as zero.");
	const Ref<RectND> offset_w_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 2 }, VectorN{ 1, 1, 1, 1 });
	CHECK_MESSAGE(!offset_w_rect->has_point(VectorN{ 0.5, 0.5 }), "RectND has_point should not contain a point that is outside in an axis the point does not define.");

	CHECK_MESSAGE(VectorND::is_equal_exact(unit_rect->get_nearest_point(VectorN{ 5, 0.5 }), VectorN{ 1, 0.5, 0, 0 }), "RectND get_nearest_point should handle a lower-dimensional point.");
	CHECK_MESSAGE(VectorND::is_equal_exact(unit_rect->get_nearest_point(VectorN{ 0.5, 0.5, 0.5, 0.5, 5 }), VectorN{ 0.5, 0.5, 0.5, 0.5, 0 }), "RectND get_nearest_point should handle a higher-dimensional point.");
	CHECK_MESSAGE(VectorND::is_equal_exact(unit_rect->get_support_point(VectorN{ 1, -1 }), VectorN{ 1, 0, 0, 0 }), "RectND get_support_point should handle a lower-dimensional direction.");

	Ref<RectND> expanded = unit_rect->expand_to_point(VectorN{ 2, 0.5 });
	CHECK_MESSAGE(VectorND::is_equal_exact(expanded->get_position(), VectorN{ 0, 0, 0, 0 }), "RectND expand_to_point should not move the position when the point is past the end.");
	CHECK_MESSAGE(VectorND::is_equal_exact(expanded->get_size(), VectorN{ 2, 1, 1, 1 }), "RectND expand_to_point should handle a lower-dimensional point.");
	expanded = unit_rect->expand_to_point(VectorN{ 0.5, 0.5, 0.5, 0.5, 5 });
	CHECK_MESSAGE(VectorND::is_equal_exact(expanded->get_size(), VectorN{ 1, 1, 1, 1, 5 }), "RectND expand_to_point should handle a higher-dimensional point.");

	// A rect with a shorter size than position must not read out of bounds either.
	const Ref<RectND> jagged_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1 });
	CHECK_MESSAGE(jagged_rect->has_point(VectorN{ 0.5, 0.5 }), "RectND has_point should handle a rect with a shorter size than position.");
	CHECK_MESSAGE(!jagged_rect->has_point(VectorN{ 0.5, 0.5, 0.5, 0.5 }), "RectND has_point should treat missing size elements as zero size.");
	CHECK_MESSAGE(VectorND::is_equal_exact(jagged_rect->get_support_point(VectorN{ 1, 1, 1, 1 }), VectorN{ 1, 1, 0, 0 }), "RectND get_support_point should handle a rect with a shorter size than position.");
	const Ref<RectND> grown = jagged_rect->grow(1.0);
	CHECK_MESSAGE(VectorND::is_equal_exact(grown->get_position(), VectorN{ -1, -1, -1, -1 }), "RectND grow should grow every axis of the rect.");
	CHECK_MESSAGE(VectorND::is_equal_exact(grown->get_size(), VectorN{ 3, 3, 2, 2 }), "RectND grow should grow the axes with a missing size element too.");
}

TEST_CASE("[RectND] Never crashes on degenerate input") {
	// An entirely empty rect, and a rect with a negative dimension requested, must not crash.
	Ref<RectND> empty_rect;
	empty_rect.instantiate();
	CHECK(empty_rect->get_dimension() == 0);
	ERR_PRINT_OFF;
	empty_rect->set_dimension(-1);
	ERR_PRINT_ON;
	CHECK_MESSAGE(empty_rect->get_dimension() == 0, "RectND set_dimension should reject a negative dimension.");
	CHECK(empty_rect->has_point(VectorN{}));
	CHECK(empty_rect->has_point(VectorN{ 0, 0, 0, 0 }));
	CHECK(!empty_rect->has_point(VectorN{ 1, 2, 3, 4 }));
	CHECK(!empty_rect->has_any_size());
	CHECK(VectorND::is_equal_exact(empty_rect->get_nearest_point(VectorN{ 1, 2 }), VectorN{ 0, 0 }));
	CHECK(VectorND::is_equal_exact(empty_rect->get_support_point(VectorN{ 1, 2 }), VectorN{ 0, 0 }));
	CHECK(empty_rect->abs()->get_dimension() == 0);
	CHECK(empty_rect->intersects_inclusive(empty_rect));
	double distance;
	VectorN normal;
	// A zero-dimension raycast has nothing to miss, so it trivially hits, but must not read out of bounds.
	ERR_PRINT_OFF;
	empty_rect->raycast_intersects(VectorN{}, VectorN{}, false, &distance, &normal);
	ERR_PRINT_ON;
	CHECK(empty_rect->continuous_collision_depth(VectorN{}, empty_rect, &normal) == 1.0);
	CHECK(!empty_rect->continuous_collision_overlaps(VectorN{ 1, 1 }, empty_rect));

	// Every method taking another RectND must reject null instead of dereferencing it.
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	const Ref<RectND> null_rect;
	ERR_PRINT_OFF;
	CHECK(!unit_rect->intersection(null_rect)->has_any_size());
	CHECK(!unit_rect->merge(null_rect)->has_any_size());
	CHECK(!unit_rect->encloses_exclusive(null_rect));
	CHECK(!unit_rect->encloses_inclusive(null_rect));
	CHECK(!unit_rect->intersects_exclusive(null_rect));
	CHECK(!unit_rect->intersects_inclusive(null_rect));
	CHECK(!unit_rect->is_equal_approx(null_rect));
	CHECK_MESSAGE(unit_rect->continuous_collision_depth(VectorN{ 1, 0, 0, 0 }, null_rect) == 1.0, "RectND continuous_collision_depth with a null obstacle should allow all of the motion.");
	CHECK(!unit_rect->continuous_collision_overlaps(VectorN{ 1, 0, 0, 0 }, null_rect));
	ERR_PRINT_ON;
}

TEST_CASE("[RectND] Continuous Collision Depth") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	VectorN normal;
	double depth = unit_rect->continuous_collision_depth(VectorN{ 0, 0, 0, 0 }, unit_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with no motion should not be stopped.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 1, 0, 0, 0 }, unit_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with perfectly overlapping rects should depenetrate (in any direction, -1.0 would also be acceptable).");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
	depth = unit_rect->continuous_collision_depth(VectorN{ -1, 0, 0, 0 }, unit_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with perfectly overlapping rects should depenetrate (in any direction, -1.0 would also be acceptable).");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");

	const Ref<RectND> offset_3w_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 3 }, VectorN{ 1, 1, 1, 1 });
	depth = unit_rect->continuous_collision_depth(VectorN{ 0, 0, 0, 0 }, offset_3w_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with no motion should not be stopped.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 0, 0, 0, 5 }, offset_3w_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.4), "RectND continuous_collision_depth with high motion should be stopped by the obstacle.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, -1 }), "RectND continuous_collision_depth should give the correct normal.");
	depth = unit_rect->continuous_collision_depth(VectorN{ -0.0, -0.0, -0.0, 5 }, offset_3w_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.4), "RectND continuous_collision_depth with negative zero components should be stopped by the obstacle.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, -1 }), "RectND continuous_collision_depth with negative zero components should give the correct normal.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 0, 0, 0, -5 }, offset_3w_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with motion away from the obstacle should not be stopped.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 10, 0, 0, 0 }, offset_3w_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with motion perpendicular to the obstacle should not be stopped.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");

	const Ref<RectND> offset_minus_z_rect = RectND::from_position_size(VectorN{ 0, 0.5, -2, 0 }, VectorN{ 1, 1, 1, 1 });
	depth = unit_rect->continuous_collision_depth(VectorN{ 0, 0, -2, 0 }, offset_minus_z_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.5), "RectND continuous_collision_depth with motion towards the obstacle should be stopped.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 1, 0 }), "RectND continuous_collision_depth should give the correct normal.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 10, 0, 0, 0 }, offset_minus_z_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with motion perpendicular to the obstacle should not be stopped.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 0, 10, 0, 0 }, offset_minus_z_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with motion perpendicular to the obstacle should not be stopped.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");

	const Ref<RectND> overlap_pos_y_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 2, 3, 4, 5 });
	depth = unit_rect->continuous_collision_depth(VectorN{ 0, 2, 0, 0 }, overlap_pos_y_rect, &normal);
	CHECK_MESSAGE(depth == -0.5, "RectND continuous_collision_depth overlapping should depenetrate the obstacle.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, -1, 0, 0 }), "RectND continuous_collision_depth should give the correct normal.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 0, -2, 0, 0 }, overlap_pos_y_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth overlapping should be allowed to move out of the obstacle.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");

	const Ref<RectND> overlap_neg_x_rect = RectND::from_position_size(VectorN{ -1, 0, 0, 0 }, VectorN{ 2, 3, 4, 5 });
	depth = unit_rect->continuous_collision_depth(VectorN{ -2, 0, 0, 0 }, overlap_neg_x_rect, &normal);
	CHECK_MESSAGE(depth == -0.5, "RectND continuous_collision_depth overlapping should depenetrate the obstacle.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 1, 0, 0, 0 }), "RectND continuous_collision_depth should give the correct normal.");
	depth = unit_rect->continuous_collision_depth(VectorN{ 2, 0, 0, 0 }, overlap_neg_x_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth overlapping should be allowed to move out of the obstacle.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
}

TEST_CASE("[RectND] Continuous Collision Depth Higher Dimensions") {
	// The same setup as the 4D case, but offset in the 6th dimension instead of the 4th.
	const Ref<RectND> unit_rect_6d = RectND::from_position_size(VectorN{ 0, 0, 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1, 1, 1 });
	const Ref<RectND> offset_3rd_axis_5_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0, 0, 3 }, VectorN{ 1, 1, 1, 1, 1, 1 });
	VectorN normal;
	double depth = unit_rect_6d->continuous_collision_depth(VectorN{ 0, 0, 0, 0, 0, 5 }, offset_3rd_axis_5_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.4), "RectND continuous_collision_depth should work the same in 6D as it does in 4D.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0, 0, -1 }), "RectND continuous_collision_depth should give a 6D normal for a 6D collision.");
	depth = unit_rect_6d->continuous_collision_depth(VectorN{ 0, 0, 0, 0, 0, -5 }, offset_3rd_axis_5_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth with motion away from the obstacle should not be stopped in 6D.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
}

TEST_CASE("[RectND] Continuous Collision Depth Mixed Dimensions") {
	// A 2D rect is a degenerate slab in higher dimensions: it has zero size in the axes it doesn't define.
	const Ref<RectND> unit_rect_2d = RectND::from_position_size(VectorN{ 0, 0 }, VectorN{ 1, 1 });
	const Ref<RectND> offset_3w_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 3 }, VectorN{ 1, 1, 1, 1 });
	VectorN normal;
	// The 2D rect's W end is 0 rather than 1, so it must travel further than the 4D unit rect would.
	double depth = unit_rect_2d->continuous_collision_depth(VectorN{ 0, 0, 0, 5 }, offset_3w_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.6), "RectND continuous_collision_depth should treat missing elements of a lower-dimensional rect as zero.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, -1 }), "RectND continuous_collision_depth should give a normal in the dimension the calculation was performed in.");
	// Moving a 4D rect against a 2D obstacle, which is a degenerate slab at Z=0 and W=0.
	const Ref<RectND> unit_rect_4d = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	const Ref<RectND> offset_2d_rect = RectND::from_position_size(VectorN{ 3, 0 }, VectorN{ 1, 1 });
	depth = unit_rect_4d->continuous_collision_depth(VectorN{ 5, 0, 0, 0 }, offset_2d_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.4), "RectND continuous_collision_depth should treat missing elements of a lower-dimensional obstacle as zero.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ -1, 0, 0, 0 }), "RectND continuous_collision_depth should give the correct normal for a lower-dimensional obstacle.");
	// The same obstacle, but the mover is offset past the obstacle's zero-size W slab, so it can never collide.
	const Ref<RectND> offset_w_rect_4d = RectND::from_position_size(VectorN{ 0, 0, 0, 2 }, VectorN{ 1, 1, 1, 1 });
	depth = offset_w_rect_4d->continuous_collision_depth(VectorN{ 5, 0, 0, 0 }, offset_2d_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth should not collide when separated in an axis the obstacle does not define.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
}

TEST_CASE("[RectND] Continuous Collision Touching Faces") {
	// A player resting exactly on a surface must be able to move along it. These rects share only
	// the plane Y=0, so they have zero-space contact and can never overlap for any amount of motion.
	const Ref<RectND> player_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	const Ref<RectND> floor_ahead_rect = RectND::from_position_size(VectorN{ 1, -1, 0, 0 }, VectorN{ 2, 1, 1, 1 });
	VectorN normal;
	double depth = player_rect->continuous_collision_depth(VectorN{ 1, 0, 0, 0 }, floor_ahead_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth should not be stopped by a rect it only shares a face with.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0 }), "RectND continuous_collision_depth should give a zero normal when there is no collision.");
	CHECK_MESSAGE(!player_rect->continuous_collision_overlaps(VectorN{ 1, 0, 0, 0 }, floor_ahead_rect), "RectND continuous_collision_overlaps should not overlap a rect it only shares a face with.");
	// The same is true for motion parallel to the shared face, in which case the rects stay touching the whole time.
	const Ref<RectND> touching_x_rect = RectND::from_position_size(VectorN{ 1, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	depth = player_rect->continuous_collision_depth(VectorN{ 0, 1, 0, 0 }, touching_x_rect, &normal);
	CHECK_MESSAGE(depth == 1.0, "RectND continuous_collision_depth should allow sliding along a shared face.");
	CHECK_MESSAGE(!player_rect->continuous_collision_overlaps(VectorN{ 0, 1, 0, 0 }, touching_x_rect), "RectND continuous_collision_overlaps should agree with continuous_collision_depth when sliding along a shared face.");
	// Moving into the shared face rather than along it must still collide.
	depth = player_rect->continuous_collision_depth(VectorN{ 1, 0, 0, 0 }, touching_x_rect, &normal);
	CHECK_MESSAGE(depth == 0.0, "RectND continuous_collision_depth should be stopped when moving directly into a touching rect.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ -1, 0, 0, 0 }), "RectND continuous_collision_depth should give the correct normal when moving into a touching rect.");

	// A zero-thickness rect must still collide with what it touches, so touching is inclusive when either rect is degenerate.
	const Ref<RectND> flat_z_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 0, 1 });
	const Ref<RectND> solid_rect = RectND::from_position_size(VectorN{ 3, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	depth = flat_z_rect->continuous_collision_depth(VectorN{ 4, 0, 0, 0 }, solid_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.5), "RectND continuous_collision_depth should let a zero-thickness rect collide with a solid rect.");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ -1, 0, 0, 0 }), "RectND continuous_collision_depth should give the correct normal for a zero-thickness rect.");
	// A rect with a missing size element is zero-thickness on that axis, so the same inclusive rule applies.
	const Ref<RectND> rect_2d = RectND::from_position_size(VectorN{ 0, 0 }, VectorN{ 1, 1 });
	const Ref<RectND> solid_ahead_rect = RectND::from_position_size(VectorN{ 3, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	depth = rect_2d->continuous_collision_depth(VectorN{ 4, 0, 0, 0 }, solid_ahead_rect, &normal);
	CHECK_MESSAGE(depth == doctest::Approx(0.5), "RectND continuous_collision_depth should let a lower-dimensional rect collide in the axes it does not define.");
}

TEST_CASE("[RectND] Continuous Collision Overlaps") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	const Ref<RectND> offset_3w_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 3 }, VectorN{ 1, 1, 1, 1 });
	bool overlaps = unit_rect->continuous_collision_overlaps(VectorN{ -0.0, -0.0, -0.0, 5 }, offset_3w_rect);
	CHECK_MESSAGE(overlaps, "RectND continuous_collision_overlaps with negative zero components should overlap the obstacle.");
	overlaps = unit_rect->continuous_collision_overlaps(VectorN{ -0.0, -0.0, -0.0, -5 }, offset_3w_rect);
	CHECK_MESSAGE(!overlaps, "RectND continuous_collision_overlaps with negative zero components and motion away should not overlap the obstacle.");
	// A 2D motion vector only defines motion in X and Y, so the W axis is treated as having no motion.
	overlaps = unit_rect->continuous_collision_overlaps(VectorN{ 5, 5 }, offset_3w_rect);
	CHECK_MESSAGE(!overlaps, "RectND continuous_collision_overlaps should treat missing motion elements as zero motion.");
	// In 6D, with an obstacle only reachable by moving along the 6th axis.
	const Ref<RectND> unit_rect_6d = RectND::from_position_size(VectorN{ 0, 0, 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1, 1, 1 });
	const Ref<RectND> offset_6d_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0, 0, 3 }, VectorN{ 1, 1, 1, 1, 1, 1 });
	overlaps = unit_rect_6d->continuous_collision_overlaps(VectorN{ 0, 0, 0, 0, 0, 5 }, offset_6d_rect);
	CHECK_MESSAGE(overlaps, "RectND continuous_collision_overlaps should overlap the obstacle in 6D.");
	overlaps = unit_rect_6d->continuous_collision_overlaps(VectorN{ 0, 0, 0, 0, 0, 1 }, offset_6d_rect);
	CHECK_MESSAGE(!overlaps, "RectND continuous_collision_overlaps with motion too small to reach the obstacle should not overlap.");
}

TEST_CASE("[RectND] Raycast from outside") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	double distance;
	VectorN normal;
	// Ray from outside, pointing at center.
	bool hit = unit_rect->raycast_intersects(VectorN{ -2, 0.5, 0.5, 0.5 }, VectorN{ 1, 0, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == true, "Raycast from outside should hit the box");
	CHECK_MESSAGE(distance == doctest::Approx(2.0), "Raycast distance should be 2.0");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ -1, 0, 0, 0 }), "Normal should point backwards along X");
	// Ray from outside, pointing at corner.
	hit = unit_rect->raycast_intersects(VectorN{ -1, -1, -1, -1 }, VectorND::normalized(VectorN{ 1, 1, 1, 1 }), false, &distance, &normal);
	CHECK_MESSAGE(hit == true, "Raycast at corner should hit");
	// Ray from outside, missing the box.
	hit = unit_rect->raycast_intersects(VectorN{ -1, 2, 0.5, 0.5 }, VectorN{ 1, 0, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == false, "Raycast missing the box should not hit");
	// Ray parallel to box, pointing away.
	hit = unit_rect->raycast_intersects(VectorN{ 2, 0.5, 0.5, 0.5 }, VectorN{ 1, 0, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == false, "Raycast pointing away should not hit");
}

TEST_CASE("[RectND] Raycast from inside") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	double distance;
	VectorN normal;
	// When inside_is_zero=true, should return distance 0.
	bool hit = unit_rect->raycast_intersects(VectorN{ 0.5, 0.5, 0.5, 0.5 }, VectorN{ 1, 0, 0, 0 }, true, &distance, &normal);
	CHECK_MESSAGE(hit == true, "Raycast from inside should hit with inside_is_zero=true");
	CHECK_MESSAGE(distance == doctest::Approx(0.0), "Raycast from inside with inside_is_zero should have distance 0");
	// When inside_is_zero=false, the ray should hit the forward exit surface, not the entry surface behind the origin.
	hit = unit_rect->raycast_intersects(VectorN{ 0.5, 0.5, 0.5, 0.5 }, VectorN{ 1, 0, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == true, "Raycast from inside should hit with inside_is_zero=false");
	CHECK_MESSAGE(distance == doctest::Approx(0.5), "Raycast from inside should return the distance to the forward exit surface");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 1, 0, 0, 0 }), "Raycast from inside should return the outward normal of the exit surface");
}

TEST_CASE("[RectND] Raycast max distance is exclusive") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	const VectorN origin = VectorN{ -2, 0.5, 0.5, 0.5 };
	const VectorN direction = VectorN{ 1, 0, 0, 0 };
	Dictionary result = unit_rect->raycast_intersects_dict(origin, direction, 1.999, false);
	CHECK_FALSE((bool)result["hit"]);
	result = unit_rect->raycast_intersects_dict(origin, direction, 2.0, false);
	CHECK_FALSE((bool)result["hit"]);
	result = unit_rect->raycast_intersects_dict(origin, direction, 2.001, false);
	REQUIRE((bool)result["hit"]);
	CHECK((double)result["distance"] == doctest::Approx(2.0));
	CHECK(VectorND::is_equal_approx((VectorN)result["point"], VectorN{ 0, 0.5, 0.5, 0.5 }));
}

TEST_CASE("[RectND] Raycast parallel to axes") {
	const Ref<RectND> unit_rect = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	double distance;
	VectorN normal;
	// Ray parallel to box, pointing along Y through center.
	bool hit = unit_rect->raycast_intersects(VectorN{ 0.5, -1, 0.5, 0.5 }, VectorN{ 0, 1, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == true, "Ray through center should hit");
	CHECK_MESSAGE(distance == doctest::Approx(1.0), "Distance should be 1.0");
	// Ray parallel to box, missing on one axis.
	hit = unit_rect->raycast_intersects(VectorN{ 0.5, -1, 2, 0.5 }, VectorN{ 0, 1, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == false, "Ray missing on Z axis should not hit");
}

TEST_CASE("[RectND] Raycast Higher Dimensions") {
	const Ref<RectND> unit_rect_6d = RectND::from_position_size(VectorN{ 0, 0, 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1, 1, 1 });
	double distance;
	VectorN normal;
	bool hit = unit_rect_6d->raycast_intersects(VectorN{ 0.5, 0.5, 0.5, 0.5, 0.5, -2 }, VectorN{ 0, 0, 0, 0, 0, 1 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == true, "Raycast along the 6th axis should hit the 6D box");
	CHECK_MESSAGE(distance == doctest::Approx(2.0), "Raycast distance should be 2.0 in 6D");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ 0, 0, 0, 0, 0, -1 }), "Raycast normal should point backwards along the 6th axis");
	// Missing the box in an axis the ray is parallel to.
	hit = unit_rect_6d->raycast_intersects(VectorN{ 0.5, 0.5, 0.5, 0.5, 2, -2 }, VectorN{ 0, 0, 0, 0, 0, 1 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == false, "Raycast missing on the 5th axis should not hit the 6D box");
}

TEST_CASE("[RectND] Raycast Mixed Dimensions") {
	const Ref<RectND> unit_rect_4d = RectND::from_position_size(VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 });
	double distance;
	VectorN normal;
	// A 2D ray still hits a 4D box, because the missing ray elements are zero, which is inside the box in those axes.
	bool hit = unit_rect_4d->raycast_intersects(VectorN{ -2, 0.5 }, VectorN{ 1, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == true, "A 2D raycast should hit a 4D box when the missing elements are within the box");
	CHECK_MESSAGE(distance == doctest::Approx(2.0), "A 2D raycast against a 4D box should give the correct distance");
	CHECK_MESSAGE(VectorND::is_equal_exact(normal, VectorN{ -1, 0, 0, 0 }), "A 2D raycast against a 4D box should give a 4D normal");
	// The same 2D ray misses a 4D box that does not contain zero in the axes the ray does not define.
	const Ref<RectND> offset_w_rect_4d = RectND::from_position_size(VectorN{ 0, 0, 0, 2 }, VectorN{ 1, 1, 1, 1 });
	hit = offset_w_rect_4d->raycast_intersects(VectorN{ -2, 0.5 }, VectorN{ 1, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == false, "A 2D raycast should miss a 4D box that is offset in an axis the ray does not define");
	// A 4D ray against a 2D box, which is a degenerate slab at Z=0 and W=0.
	const Ref<RectND> unit_rect_2d = RectND::from_position_size(VectorN{ 0, 0 }, VectorN{ 1, 1 });
	hit = unit_rect_2d->raycast_intersects(VectorN{ -2, 0.5, 0, 0 }, VectorN{ 1, 0, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == true, "A 4D raycast should hit a 2D box when it is aligned with the box's degenerate axes");
	CHECK_MESSAGE(distance == doctest::Approx(2.0), "A 4D raycast against a 2D box should give the correct distance");
	hit = unit_rect_2d->raycast_intersects(VectorN{ -2, 0.5, 0, 0.5 }, VectorN{ 1, 0, 0, 0 }, false, &distance, &normal);
	CHECK_MESSAGE(hit == false, "A 4D raycast should miss a 2D box when it is outside of the box's degenerate axes");
}
} // namespace TestRectND
