#pragma once

#include "../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../model/mesh/poly/orthoplex_poly_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestCellMeshND {
TEST_CASE("[CellMeshND] Decompose Box Polytope Cell into Simplexes") {
	// 0D case.
	Vector<VectorN> vertices;
	PackedInt32Array cell_indices;
	Vector<PackedInt32Array> decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 0, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 0);
	// 1D case.
	vertices.append(VectorN{ 0 });
	vertices.append(VectorN{ 1 });
	cell_indices = { 0, 1 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 1, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 1);
	CHECK(decomposed[0] == PackedInt32Array{ 0, 1 });
	// 2D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0 });
	vertices.append(VectorN{ 1, 0 });
	vertices.append(VectorN{ 0, 1 });
	vertices.append(VectorN{ 1, 1 });
	cell_indices = { 0, 1, 2, 3 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 2, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 2);
	// The exact values it returns are not important, so long as they do not overlap and cover the cell.
	CHECK(decomposed[0] == PackedInt32Array{ 0, 1, 3 });
	CHECK(decomposed[1] == PackedInt32Array{ 0, 2, 3 });
	// 3D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0, 0 });
	vertices.append(VectorN{ 1, 0, 0 });
	vertices.append(VectorN{ 0, 1, 0 });
	vertices.append(VectorN{ 1, 1, 0 });
	vertices.append(VectorN{ 0, 0, 1 });
	vertices.append(VectorN{ 1, 0, 1 });
	vertices.append(VectorN{ 0, 1, 1 });
	vertices.append(VectorN{ 1, 1, 1 });
	cell_indices = { 0, 1, 2, 3, 4, 5, 6, 7 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 3, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 6);
	// 4D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0, 0, 0 });
	vertices.append(VectorN{ 1, 0, 0, 0 });
	vertices.append(VectorN{ 0, 1, 0, 0 });
	vertices.append(VectorN{ 1, 1, 0, 0 });
	vertices.append(VectorN{ 0, 0, 1, 0 });
	vertices.append(VectorN{ 1, 0, 1, 0 });
	vertices.append(VectorN{ 0, 1, 1, 0 });
	vertices.append(VectorN{ 1, 1, 1, 0 });
	vertices.append(VectorN{ 0, 0, 0, 1 });
	vertices.append(VectorN{ 1, 0, 0, 1 });
	vertices.append(VectorN{ 0, 1, 0, 1 });
	vertices.append(VectorN{ 1, 1, 0, 1 });
	vertices.append(VectorN{ 0, 0, 1, 1 });
	vertices.append(VectorN{ 1, 0, 1, 1 });
	vertices.append(VectorN{ 0, 1, 1, 1 });
	vertices.append(VectorN{ 1, 1, 1, 1 });
	cell_indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 4, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 24);
	// 5D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0, 0, 0, 0 });
	vertices.append(VectorN{ 1, 0, 0, 0, 0 });
	vertices.append(VectorN{ 0, 1, 0, 0, 0 });
	vertices.append(VectorN{ 1, 1, 0, 0, 0 });
	vertices.append(VectorN{ 0, 0, 1, 0, 0 });
	vertices.append(VectorN{ 1, 0, 1, 0, 0 });
	vertices.append(VectorN{ 0, 1, 1, 0, 0 });
	vertices.append(VectorN{ 1, 1, 1, 0, 0 });
	vertices.append(VectorN{ 0, 0, 0, 1, 0 });
	vertices.append(VectorN{ 1, 0, 0, 1, 0 });
	vertices.append(VectorN{ 0, 1, 0, 1, 0 });
	vertices.append(VectorN{ 1, 1, 0, 1, 0 });
	vertices.append(VectorN{ 0, 0, 1, 1, 0 });
	vertices.append(VectorN{ 1, 0, 1, 1, 0 });
	vertices.append(VectorN{ 0, 1, 1, 1, 0 });
	vertices.append(VectorN{ 1, 1, 1, 1, 0 });
	vertices.append(VectorN{ 0, 0, 0, 0, 1 });
	vertices.append(VectorN{ 1, 0, 0, 0, 1 });
	vertices.append(VectorN{ 0, 1, 0, 0, 1 });
	vertices.append(VectorN{ 1, 1, 0, 0, 1 });
	vertices.append(VectorN{ 0, 0, 1, 0, 1 });
	vertices.append(VectorN{ 1, 0, 1, 0, 1 });
	vertices.append(VectorN{ 0, 1, 1, 0, 1 });
	vertices.append(VectorN{ 1, 1, 1, 0, 1 });
	vertices.append(VectorN{ 0, 0, 0, 1, 1 });
	vertices.append(VectorN{ 1, 0, 0, 1, 1 });
	vertices.append(VectorN{ 0, 1, 0, 1, 1 });
	vertices.append(VectorN{ 1, 1, 0, 1, 1 });
	vertices.append(VectorN{ 0, 0, 1, 1, 1 });
	vertices.append(VectorN{ 1, 0, 1, 1, 1 });
	vertices.append(VectorN{ 0, 1, 1, 1, 1 });
	vertices.append(VectorN{ 1, 1, 1, 1, 1 });
	cell_indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 5, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 120);
	// Note: Due to the nature of polytopes, this algorithm has O(2^n) complexity.
	// A test with 6D would take many minutes to run... so is omitted for sanity.
	// I am sure a better algorithm exists, but this technically works for now.
}

TEST_CASE("[CellMeshND] Signed distance to mesh") {
	SUBCASE("Signed distance to a box is exact for faces, corners, and interior points") {
		for (int dimension = 3; dimension <= 5; dimension++) {
			Ref<BoxPolyMeshND> box;
			box.instantiate();
			box->set_size(VectorND::fill(dimension, 2.0));
			// A point outside the box, one unit away from the middle of the positive first axis side.
			VectorN nearest_point;
			int simplex_index = -1;
			const VectorN outside_point = VectorND::value_on_axis_with_dimension(2.0, 0, dimension);
			const double outside_distance = box->get_signed_distance_to_mesh(outside_point, &nearest_point, &simplex_index);
			CHECK_MESSAGE(outside_distance == doctest::Approx(1.0), "A point one unit outside the box must have a signed distance of 1.");
			CHECK_MESSAGE(VectorND::is_equal_approx(nearest_point, VectorND::value_on_axis_with_dimension(1.0, 0, dimension)), "The nearest point must be in the middle of the positive first axis side.");
			CHECK_MESSAGE(simplex_index >= 0, "The nearest simplex cell index must be written to the output parameter.");
			// The center of the box is one unit inside every side.
			const double center_distance = box->get_signed_distance_to_mesh(VectorND::zero(dimension), nullptr, nullptr);
			CHECK_MESSAGE(center_distance == doctest::Approx(-1.0), "The center of the box must have a signed distance of -1.");
			// A point inside the box, half a unit away from the positive first axis side.
			const double inside_distance = box->get_signed_distance_to_mesh(VectorND::value_on_axis_with_dimension(0.5, 0, dimension), nullptr, nullptr);
			CHECK_MESSAGE(inside_distance == doctest::Approx(-0.5), "A point half a unit inside the box must have a signed distance of -0.5.");
			// A point outside the box diagonally past a corner, exercising the candidate disambiguation.
			const double corner_distance = box->get_signed_distance_to_mesh(VectorND::fill(dimension, 2.0), &nearest_point, nullptr);
			CHECK_MESSAGE(corner_distance == doctest::Approx(Math::sqrt((double)dimension)), "A point diagonally past a corner must have the distance to the corner.");
			CHECK_MESSAGE(VectorND::is_equal_approx(nearest_point, VectorND::fill(dimension, 1.0)), "The nearest point to a point diagonally past a corner must be the corner.");
		}
	}
	SUBCASE("Signed distance to an orthoplex is exact for vertices and interior points") {
		Ref<OrthoplexPolyMeshND> orthoplex;
		orthoplex.instantiate();
		orthoplex->set_size(VectorND::fill(3, 2.0));
		// A point outside the orthoplex, one unit away from the positive first axis vertex.
		VectorN nearest_point;
		const double outside_distance = orthoplex->get_signed_distance_to_mesh(VectorN{ 2.0, 0.0, 0.0 }, &nearest_point, nullptr);
		CHECK_MESSAGE(outside_distance == doctest::Approx(1.0), "A point one unit outside the orthoplex must have a signed distance of 1.");
		CHECK_MESSAGE(VectorND::is_equal_approx(nearest_point, VectorN{ 1.0, 0.0, 0.0 }), "The nearest point must be the positive first axis vertex.");
		// The center of the orthoplex is 1/sqrt(3) away from each of the 8 triangular sides.
		const double center_distance = orthoplex->get_signed_distance_to_mesh(VectorND::zero(3), nullptr, nullptr);
		CHECK_MESSAGE(center_distance == doctest::Approx(-1.0 / Math::sqrt(3.0)), "The center of the orthoplex must have a signed distance of -1/sqrt(3).");
	}
	SUBCASE("The inverse metric cache is invalidated when the mesh data changes") {
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(3, 2.0));
		box->populate_inverse_metric_cache();
		CHECK(box->get_signed_distance_to_mesh(VectorN{ 2.0, 0.0, 0.0 }, nullptr, nullptr) == doctest::Approx(1.0));
		// Resizing the box must invalidate the cache and give distances for the new size.
		box->set_size(VectorND::fill(3, 4.0));
		CHECK_MESSAGE(box->get_signed_distance_to_mesh(VectorN{ 3.0, 0.0, 0.0 }, nullptr, nullptr) == doctest::Approx(1.0), "The closest-point cache must be invalidated when the mesh data changes.");
	}
}
} // namespace TestCellMeshND
