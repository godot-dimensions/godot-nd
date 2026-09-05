#pragma once

#include "../../../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../../../model/mesh/poly/orthoplex_poly_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestCellMeshND {
static Ref<ArrayCellMeshND> make_single_simplex_cell_mesh(const int p_dimension, const bool p_with_attributes) {
	Ref<ArrayCellMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertex_positions;
	vertex_positions.append(VectorND::zero(p_dimension));
	for (int axis = 0; axis < p_dimension - 1; axis++) {
		vertex_positions.append(VectorND::value_on_axis_with_dimension(1.0, axis, p_dimension));
	}
	mesh->set_vertex_positions(vertex_positions);
	PackedInt32Array vertex_indices;
	for (int index = 0; index < p_dimension; index++) {
		vertex_indices.append(index);
	}
	mesh->set_simplex_cell_vertex_indices(vertex_indices);
	if (p_with_attributes) {
		mesh->set_simplex_cell_boundary_normals(Vector<VectorN>{ VectorND::value_on_axis_with_dimension(1.0, p_dimension - 1, p_dimension) });
		Vector<VectorN> normals;
		Vector<VectorM> texture_map;
		for (int index = 0; index < p_dimension; index++) {
			normals.append(VectorND::value_on_axis_with_dimension(1.0, index, p_dimension));
			texture_map.append(VectorND::fill(p_dimension - 1, (index + 1) * 0.125));
		}
		mesh->set_simplex_cell_vertex_normals(normals);
		mesh->set_simplex_cell_texture_map(texture_map);
	}
	return mesh;
}

TEST_CASE("[ArrayCellMeshND] Validation handles empty meshes and dense attribute counts") {
	Ref<ArrayCellMeshND> empty_mesh;
	empty_mesh.instantiate();
	CHECK(empty_mesh->is_mesh_data_valid());
	for (int attribute = 0; attribute < 4; attribute++) {
		Ref<ArrayCellMeshND> mesh;
		mesh.instantiate();
		mesh->set_vertex_positions(Vector<VectorN>{ VectorN() });
		switch (attribute) {
			case 0:
				mesh->set_simplex_cell_vertex_indices(PackedInt32Array{ 0 });
				break;
			case 1:
				mesh->set_simplex_cell_boundary_normals(Vector<VectorN>{ VectorN() });
				break;
			case 2:
				mesh->set_simplex_cell_vertex_normals(Vector<VectorN>{ VectorN() });
				break;
			case 3:
				mesh->set_simplex_cell_texture_map(Vector<VectorM>{ VectorM() });
				break;
		}
		ERR_PRINT_OFF;
		CHECK_FALSE(mesh->is_mesh_data_valid());
		ERR_PRINT_ON;
	}
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<ArrayCellMeshND> mesh = make_single_simplex_cell_mesh(dimension, true);
		CHECK(mesh->is_mesh_data_valid());
		Vector<VectorM> texture_map = mesh->get_simplex_cell_texture_map();
		texture_map.remove_at(texture_map.size() - 1);
		mesh->set_simplex_cell_texture_map(texture_map);
		ERR_PRINT_OFF;
		CHECK_FALSE(mesh->is_mesh_data_valid());
		ERR_PRINT_ON;
		mesh->set_simplex_cell_texture_map(Vector<VectorM>());
		CHECK(mesh->is_mesh_data_valid());
	}
}

TEST_CASE("[ArrayCellMeshND] Merge preserves dense attributes and fills missing data") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		for (const bool start_has_attributes : { false, true }) {
			for (const bool other_has_attributes : { false, true }) {
				Ref<ArrayCellMeshND> mesh = make_single_simplex_cell_mesh(dimension, start_has_attributes);
				Ref<ArrayCellMeshND> other = make_single_simplex_cell_mesh(dimension, other_has_attributes);
				const Vector<VectorN> start_positions = mesh->get_vertex_positions();
				const Vector<VectorN> start_boundary_normals = mesh->get_simplex_cell_boundary_normals();
				const Vector<VectorN> start_normals = mesh->get_simplex_cell_vertex_normals();
				const Vector<VectorM> start_texture_map = mesh->get_simplex_cell_texture_map();
				const Vector<VectorN> other_boundary_normals = other->get_simplex_cell_boundary_normals();
				const Vector<VectorN> other_normals = other->get_simplex_cell_vertex_normals();
				const Vector<VectorM> other_texture_map = other->get_simplex_cell_texture_map();
				Ref<TransformND> transform = TransformND::from_scale(VectorND::fill(dimension, -1.0));
				transform->set_origin(VectorND::value_on_axis_with_dimension(10.0, 0, dimension));
				REQUIRE(mesh->get_simplex_cell_positions().size() == dimension);
				mesh->merge_with(other, transform);
				REQUIRE(mesh->is_mesh_data_valid());
				const Vector<VectorN> positions = mesh->get_vertex_positions();
				const PackedInt32Array indices = mesh->get_simplex_cell_vertex_indices();
				REQUIRE(positions.size() == dimension * 2);
				REQUIRE(indices.size() == dimension * 2);
				CHECK(mesh->get_simplex_cell_positions() == positions);
				for (int i = 0; i < dimension; i++) {
					CHECK(positions[i] == start_positions[i]);
					CHECK(positions[dimension + i] == transform->xform(start_positions[i]));
					CHECK(indices[i] == i);
					CHECK(indices[dimension + i] == dimension + i);
				}
				const Vector<VectorN> boundary_normals = mesh->get_simplex_cell_boundary_normals();
				const Vector<VectorN> normals = mesh->get_simplex_cell_vertex_normals();
				const Vector<VectorM> texture_map = mesh->get_simplex_cell_texture_map();
				const bool has_attributes = start_has_attributes || other_has_attributes;
				REQUIRE(boundary_normals.size() == (has_attributes ? 2 : 0));
				REQUIRE(normals.size() == (has_attributes ? dimension * 2 : 0));
				REQUIRE(texture_map.size() == normals.size());
				if (has_attributes) {
					CHECK(boundary_normals[0] == (start_has_attributes ? start_boundary_normals[0] : VectorN()));
					CHECK(boundary_normals[1] == (other_has_attributes ? VectorND::negate(other_boundary_normals[0]) : VectorN()));
					for (int i = 0; i < dimension; i++) {
						CHECK(normals[i] == (start_has_attributes ? start_normals[i] : VectorN()));
						CHECK(normals[dimension + i] == (other_has_attributes ? VectorND::negate(other_normals[i]) : VectorN()));
						CHECK(texture_map[i] == (start_has_attributes ? start_texture_map[i] : VectorM()));
						CHECK(texture_map[dimension + i] == (other_has_attributes ? other_texture_map[i] : VectorM()));
					}
				}
				CHECK(other->get_vertex_positions() == start_positions);
				CHECK(other->get_simplex_cell_boundary_normals() == other_boundary_normals);
				CHECK(other->get_simplex_cell_vertex_normals() == other_normals);
				CHECK(other->get_simplex_cell_texture_map() == other_texture_map);
			}
		}
	}
}

TEST_CASE("[ArrayCellMeshND] Empty merges preserve the incoming dimension and data") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		for (const bool with_attributes : { false, true }) {
			Ref<ArrayCellMeshND> mesh;
			mesh.instantiate();
			Ref<ArrayCellMeshND> other = make_single_simplex_cell_mesh(dimension, with_attributes);
			const Ref<TransformND> transform = TransformND::identity_transform(dimension);
			mesh->merge_with(other, transform);
			REQUIRE(mesh->is_mesh_data_valid());
			CHECK(mesh->get_dimension() == dimension);
			Ref<ArrayCellMeshND> empty_mesh;
			empty_mesh.instantiate();
			mesh->merge_with(empty_mesh, transform);
			REQUIRE(mesh->is_mesh_data_valid());
			CHECK(mesh->get_vertex_positions() == other->get_vertex_positions());
			CHECK(mesh->get_simplex_cell_vertex_indices() == other->get_simplex_cell_vertex_indices());
			CHECK(mesh->get_simplex_cell_boundary_normals() == other->get_simplex_cell_boundary_normals());
			CHECK(mesh->get_simplex_cell_vertex_normals() == other->get_simplex_cell_vertex_normals());
			CHECK(mesh->get_simplex_cell_texture_map() == other->get_simplex_cell_texture_map());
		}
	}
	Ref<ArrayCellMeshND> mesh;
	mesh.instantiate();
	Ref<ArrayCellMeshND> other;
	other.instantiate();
	mesh->merge_with(other, TransformND::identity_transform(0));
	CHECK(mesh->is_mesh_data_valid());
	CHECK(mesh->get_vertex_positions().is_empty());
}

TEST_CASE("[ArrayCellMeshND] Invalid merges leave the destination unchanged") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		for (int invalid_case = 0; invalid_case < 6; invalid_case++) {
			Ref<ArrayCellMeshND> mesh = make_single_simplex_cell_mesh(dimension, true);
			Ref<ArrayCellMeshND> other = make_single_simplex_cell_mesh(dimension, true);
			Ref<TransformND> transform = TransformND::identity_transform(dimension);
			switch (invalid_case) {
				case 0:
					other.unref();
					break;
				case 1:
					transform.unref();
					break;
				case 2:
					other->set_simplex_cell_texture_map(Vector<VectorM>{ VectorM() });
					break;
				case 3:
					mesh->set_simplex_cell_texture_map(Vector<VectorM>{ VectorM() });
					break;
				case 4:
					other = make_single_simplex_cell_mesh(dimension + 1, true);
					break;
				case 5:
					mesh.instantiate();
					mesh->set_vertex_positions(Vector<VectorN>{ VectorN() });
					break;
			}
			const Vector<VectorN> positions = mesh->get_vertex_positions();
			const PackedInt32Array indices = mesh->get_simplex_cell_vertex_indices();
			const Vector<VectorN> boundary_normals = mesh->get_simplex_cell_boundary_normals();
			const Vector<VectorN> normals = mesh->get_simplex_cell_vertex_normals();
			const Vector<VectorM> texture_map = mesh->get_simplex_cell_texture_map();
			ERR_PRINT_OFF;
			mesh->merge_with(other, transform);
			ERR_PRINT_ON;
			CHECK(mesh->get_vertex_positions() == positions);
			CHECK(mesh->get_simplex_cell_vertex_indices() == indices);
			CHECK(mesh->get_simplex_cell_boundary_normals() == boundary_normals);
			CHECK(mesh->get_simplex_cell_vertex_normals() == normals);
			CHECK(mesh->get_simplex_cell_texture_map() == texture_map);
		}
	}
}

TEST_CASE("[CellMeshND] Array conversion preserves dense attributes") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<ArrayCellMeshND> mesh = make_single_simplex_cell_mesh(dimension, true);
		Ref<ArrayCellMeshND> converted = mesh->to_array_cell_mesh();
		REQUIRE(converted->is_mesh_data_valid());
		CHECK(converted->get_vertex_positions() == mesh->get_vertex_positions());
		CHECK(converted->get_simplex_cell_vertex_indices() == mesh->get_simplex_cell_vertex_indices());
		CHECK(converted->get_simplex_cell_boundary_normals() == mesh->get_simplex_cell_boundary_normals());
		CHECK(converted->get_simplex_cell_vertex_normals() == mesh->get_simplex_cell_vertex_normals());
		CHECK(converted->get_simplex_cell_texture_map() == mesh->get_simplex_cell_texture_map());
	}
}

TEST_CASE("[ArrayCellMeshND] Self merge preserves the source prefixes") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<ArrayCellMeshND> mesh = make_single_simplex_cell_mesh(dimension, true);
		const Vector<VectorN> positions = mesh->get_vertex_positions();
		const Vector<VectorN> boundary_normals = mesh->get_simplex_cell_boundary_normals();
		const Vector<VectorN> normals = mesh->get_simplex_cell_vertex_normals();
		const Vector<VectorM> texture_map = mesh->get_simplex_cell_texture_map();
		Ref<TransformND> transform = TransformND::from_scale(VectorND::fill(dimension, -1.0));
		transform->set_origin(VectorND::fill(dimension, 10.0));
		mesh->merge_with(mesh, transform);
		REQUIRE(mesh->is_mesh_data_valid());
		CHECK(mesh->get_simplex_cell_boundary_normals()[0] == boundary_normals[0]);
		CHECK(mesh->get_simplex_cell_boundary_normals()[1] == VectorND::negate(boundary_normals[0]));
		for (int i = 0; i < dimension; i++) {
			CHECK(mesh->get_vertex_positions()[i] == positions[i]);
			CHECK(mesh->get_vertex_positions()[dimension + i] == transform->xform(positions[i]));
			CHECK(mesh->get_simplex_cell_vertex_normals()[i] == normals[i]);
			CHECK(mesh->get_simplex_cell_vertex_normals()[dimension + i] == VectorND::negate(normals[i]));
			CHECK(mesh->get_simplex_cell_texture_map()[i] == texture_map[i]);
			CHECK(mesh->get_simplex_cell_texture_map()[dimension + i] == texture_map[i]);
			CHECK(mesh->get_simplex_cell_vertex_indices()[dimension + i] == dimension + i);
		}
	}
}

TEST_CASE("[CellMeshND] Decompose Box Polytope Cell into Simplexes") {
	// 0D case.
	Vector<VectorN> vertex_positions;
	PackedInt32Array cell_vertex_indices;
	Vector<PackedInt32Array> decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertex_positions, cell_vertex_indices, 0, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 0);
	// 1D case.
	vertex_positions.append(VectorN{ 0 });
	vertex_positions.append(VectorN{ 1 });
	cell_vertex_indices = { 0, 1 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertex_positions, cell_vertex_indices, 1, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 1);
	CHECK(decomposed[0] == PackedInt32Array{ 0, 1 });
	// 2D case.
	vertex_positions.clear();
	vertex_positions.append(VectorN{ 0, 0 });
	vertex_positions.append(VectorN{ 1, 0 });
	vertex_positions.append(VectorN{ 0, 1 });
	vertex_positions.append(VectorN{ 1, 1 });
	cell_vertex_indices = { 0, 1, 2, 3 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertex_positions, cell_vertex_indices, 2, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 2);
	// The exact values it returns are not important, so long as they do not overlap and cover the cell.
	CHECK(decomposed[0] == PackedInt32Array{ 0, 1, 3 });
	CHECK(decomposed[1] == PackedInt32Array{ 0, 2, 3 });
	// 3D case.
	vertex_positions.clear();
	vertex_positions.append(VectorN{ 0, 0, 0 });
	vertex_positions.append(VectorN{ 1, 0, 0 });
	vertex_positions.append(VectorN{ 0, 1, 0 });
	vertex_positions.append(VectorN{ 1, 1, 0 });
	vertex_positions.append(VectorN{ 0, 0, 1 });
	vertex_positions.append(VectorN{ 1, 0, 1 });
	vertex_positions.append(VectorN{ 0, 1, 1 });
	vertex_positions.append(VectorN{ 1, 1, 1 });
	cell_vertex_indices = { 0, 1, 2, 3, 4, 5, 6, 7 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertex_positions, cell_vertex_indices, 3, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 6);
	// 4D case.
	vertex_positions.clear();
	vertex_positions.append(VectorN{ 0, 0, 0, 0 });
	vertex_positions.append(VectorN{ 1, 0, 0, 0 });
	vertex_positions.append(VectorN{ 0, 1, 0, 0 });
	vertex_positions.append(VectorN{ 1, 1, 0, 0 });
	vertex_positions.append(VectorN{ 0, 0, 1, 0 });
	vertex_positions.append(VectorN{ 1, 0, 1, 0 });
	vertex_positions.append(VectorN{ 0, 1, 1, 0 });
	vertex_positions.append(VectorN{ 1, 1, 1, 0 });
	vertex_positions.append(VectorN{ 0, 0, 0, 1 });
	vertex_positions.append(VectorN{ 1, 0, 0, 1 });
	vertex_positions.append(VectorN{ 0, 1, 0, 1 });
	vertex_positions.append(VectorN{ 1, 1, 0, 1 });
	vertex_positions.append(VectorN{ 0, 0, 1, 1 });
	vertex_positions.append(VectorN{ 1, 0, 1, 1 });
	vertex_positions.append(VectorN{ 0, 1, 1, 1 });
	vertex_positions.append(VectorN{ 1, 1, 1, 1 });
	cell_vertex_indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertex_positions, cell_vertex_indices, 4, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 24);
	// 5D case.
	vertex_positions.clear();
	vertex_positions.append(VectorN{ 0, 0, 0, 0, 0 });
	vertex_positions.append(VectorN{ 1, 0, 0, 0, 0 });
	vertex_positions.append(VectorN{ 0, 1, 0, 0, 0 });
	vertex_positions.append(VectorN{ 1, 1, 0, 0, 0 });
	vertex_positions.append(VectorN{ 0, 0, 1, 0, 0 });
	vertex_positions.append(VectorN{ 1, 0, 1, 0, 0 });
	vertex_positions.append(VectorN{ 0, 1, 1, 0, 0 });
	vertex_positions.append(VectorN{ 1, 1, 1, 0, 0 });
	vertex_positions.append(VectorN{ 0, 0, 0, 1, 0 });
	vertex_positions.append(VectorN{ 1, 0, 0, 1, 0 });
	vertex_positions.append(VectorN{ 0, 1, 0, 1, 0 });
	vertex_positions.append(VectorN{ 1, 1, 0, 1, 0 });
	vertex_positions.append(VectorN{ 0, 0, 1, 1, 0 });
	vertex_positions.append(VectorN{ 1, 0, 1, 1, 0 });
	vertex_positions.append(VectorN{ 0, 1, 1, 1, 0 });
	vertex_positions.append(VectorN{ 1, 1, 1, 1, 0 });
	vertex_positions.append(VectorN{ 0, 0, 0, 0, 1 });
	vertex_positions.append(VectorN{ 1, 0, 0, 0, 1 });
	vertex_positions.append(VectorN{ 0, 1, 0, 0, 1 });
	vertex_positions.append(VectorN{ 1, 1, 0, 0, 1 });
	vertex_positions.append(VectorN{ 0, 0, 1, 0, 1 });
	vertex_positions.append(VectorN{ 1, 0, 1, 0, 1 });
	vertex_positions.append(VectorN{ 0, 1, 1, 0, 1 });
	vertex_positions.append(VectorN{ 1, 1, 1, 0, 1 });
	vertex_positions.append(VectorN{ 0, 0, 0, 1, 1 });
	vertex_positions.append(VectorN{ 1, 0, 0, 1, 1 });
	vertex_positions.append(VectorN{ 0, 1, 0, 1, 1 });
	vertex_positions.append(VectorN{ 1, 1, 0, 1, 1 });
	vertex_positions.append(VectorN{ 0, 0, 1, 1, 1 });
	vertex_positions.append(VectorN{ 1, 0, 1, 1, 1 });
	vertex_positions.append(VectorN{ 0, 1, 1, 1, 1 });
	vertex_positions.append(VectorN{ 1, 1, 1, 1, 1 });
	cell_vertex_indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertex_positions, cell_vertex_indices, 5, -1, Vector<VectorN>());
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
	SUBCASE("Raycasts hit boxes with exact distances and normals") {
		for (int dimension = 3; dimension <= 5; dimension++) {
			Ref<BoxPolyMeshND> box;
			box.instantiate();
			box->set_size(VectorND::fill(dimension, 2.0));
			box->populate_inverse_metric_cache();
			const VectorN outside_point = VectorND::value_on_axis_with_dimension(2.0, 0, dimension);
			const VectorN toward_box = VectorND::value_on_axis_with_dimension(-1.0, 0, dimension);
			const VectorN away_from_box = VectorND::value_on_axis_with_dimension(1.0, 0, dimension);
			CHECK_MESSAGE(box->raycast_intersects_fast(outside_point, toward_box), "A ray pointing at the box must intersect it.");
			CHECK_MESSAGE(!box->raycast_intersects_fast(outside_point, away_from_box), "A ray pointing away from the box must not intersect it.");
			CHECK_MESSAGE(!box->raycast_intersects_fast(outside_point, toward_box, 0.5), "A ray must not intersect the box beyond the maximum distance.");
			const Dictionary hit_result = box->raycast_intersects(outside_point, toward_box);
			REQUIRE_MESSAGE(bool(hit_result["hit"]), "A ray pointing at the box must intersect it.");
			CHECK_MESSAGE(double(hit_result["distance"]) == doctest::Approx(1.0), "The ray must hit the box one unit away from its start.");
			CHECK_MESSAGE(VectorND::is_equal_approx(VectorN(hit_result["normal"]), VectorND::value_on_axis_with_dimension(1.0, 0, dimension)), "The ray must hit the side facing the positive first axis.");
			CHECK_MESSAGE(int32_t(hit_result["cell_index"]) >= 0, "The hit result must include the simplex cell index.");
			const Dictionary miss_result = box->raycast_intersects(outside_point, away_from_box);
			CHECK_MESSAGE(!bool(miss_result["hit"]), "A ray pointing away from the box must not intersect it.");
			CHECK_MESSAGE(!miss_result.has("distance"), "A missed raycast must not include a distance.");
			// A ray from inside the box must hit the far side from the inside.
			const Dictionary inside_result = box->raycast_intersects(VectorND::zero(dimension), away_from_box);
			REQUIRE_MESSAGE(bool(inside_result["hit"]), "A ray from inside the box must hit a side from the inside.");
			CHECK_MESSAGE(double(inside_result["distance"]) == doctest::Approx(1.0), "The ray from the center must hit the side one unit away.");
		}
	}
	SUBCASE("Raycasts require the inverse metric cache to be populated in advance") {
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(3, 2.0));
		ERR_PRINT_OFF;
		CHECK_MESSAGE(!box->raycast_intersects_fast(VectorN{ 2.0, 0.0, 0.0 }, VectorN{ -1.0, 0.0, 0.0 }), "Raycasts must fail safely when the closest-point cache is not populated.");
		const Dictionary result = box->raycast_intersects(VectorN{ 2.0, 0.0, 0.0 }, VectorN{ -1.0, 0.0, 0.0 });
		CHECK_MESSAGE(!bool(result["hit"]), "Raycasts must fail safely when the closest-point cache is not populated.");
		ERR_PRINT_ON;
		box->populate_inverse_metric_cache();
		CHECK_MESSAGE(box->raycast_intersects_fast(VectorN{ 2.0, 0.0, 0.0 }, VectorN{ -1.0, 0.0, 0.0 }), "Raycasts must succeed after the closest-point cache is populated.");
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
