#pragma once

#include "../../math/vector_nd.h"
#include "../../model/mesh/poly/array_poly_mesh_nd.h"
#include "../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../model/mesh/poly/orthoplex_poly_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestPolyMeshND {
// Builds a poly mesh with a single tetrahedral 3D boundary cell in the w=0 hyperplane of 4D
// space, the same test mesh as the 4D module uses, so that conventions can be cross-checked.
// Edges: 0:(0,1) 1:(0,2) 2:(0,3) 3:(1,2) 4:(1,3) 5:(2,3)
// Faces: 0: verts 0-1-2, 1: verts 0-1-3, 2: verts 0-2-3, 3: verts 1-2-3.
inline Ref<ArrayPolyMeshND> make_tetrahedron_cell_mesh(const PackedInt32Array &p_cell_faces = PackedInt32Array{ 0, 1, 2, 3 }) {
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0, 0.0 });
	mesh->append_vertex(VectorN{ 1.0, 0.0, 0.0, 0.0 });
	mesh->append_vertex(VectorN{ 0.0, 1.0, 0.0, 0.0 });
	mesh->append_vertex(VectorN{ 0.0, 0.0, 1.0, 0.0 });
	mesh->append_edge_indices(0, 1);
	mesh->append_edge_indices(0, 2);
	mesh->append_edge_indices(0, 3);
	mesh->append_edge_indices(1, 2);
	mesh->append_edge_indices(1, 3);
	mesh->append_edge_indices(2, 3);
	mesh->append_poly_cell(2, PackedInt32Array{ 0, 3, 1 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 0, 4, 2 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 1, 5, 2 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 3, 5, 4 }, false);
	mesh->append_poly_cell(3, p_cell_faces, false);
	return mesh;
}

inline Ref<BoxPolyMeshND> make_box_poly_mesh(const int p_dimension) {
	Ref<BoxPolyMeshND> box;
	box.instantiate();
	box->set_size(VectorND::fill(p_dimension, 1.0));
	return box;
}

inline Ref<OrthoplexPolyMeshND> make_orthoplex_poly_mesh(const int p_dimension) {
	Ref<OrthoplexPolyMeshND> orthoplex;
	orthoplex.instantiate();
	orthoplex->set_size(VectorND::fill(p_dimension, 1.0));
	return orthoplex;
}

// Packs a sorted list of vertex indices into a single int64 key, for counting facets.
inline int64_t facet_key(const PackedInt32Array &p_sorted_vertex_indices) {
	int64_t key = 0;
	for (int64_t i = 0; i < p_sorted_vertex_indices.size(); i++) {
		key = (key << 16) | int64_t(p_sorted_vertex_indices[i] + 1);
	}
	return key;
}

TEST_CASE("[PolyMeshND] Validate poly mesh data") {
	SUBCASE("A single tetrahedral cell is valid") {
		Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh();
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "A hand-built tetrahedral cell should be valid poly mesh data.");
		CHECK_MESSAGE(mesh->is_mesh_data_valid(), "A hand-built tetrahedral cell should also pass full mesh validation.");
	}

	SUBCASE("An empty mesh is trivially valid") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "An empty mesh has nothing invalid in it.");
	}

	SUBCASE("Mismatched vertex dimensions are invalid") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 1.0, 0.0, 0.0 }, false);
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "All vertices must have the same number of dimensions.");
		ERR_PRINT_ON;
	}

	SUBCASE("Odd edge index count is invalid") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 1.0, 0.0, 0.0, 0.0 });
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 0 });
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "Edge indices must come in pairs of vertices.");
		ERR_PRINT_ON;
	}

	SUBCASE("Edge referencing a non-existent vertex is invalid") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 1.0, 0.0, 0.0, 0.0 });
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 5 });
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "Edges must reference vertices that exist.");
		ERR_PRINT_ON;
	}

	SUBCASE("Face with fewer than 3 edges is invalid") {
		Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh();
		Vector<Vector<PackedInt32Array>> poly_cell_indices;
		Vector<PackedInt32Array> faces;
		faces.push_back(PackedInt32Array{ 0, 3 });
		poly_cell_indices.push_back(faces);
		mesh->set_poly_cell_indices(poly_cell_indices);
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "A 2D face requires at least 3 edges.");
		ERR_PRINT_ON;
	}

	SUBCASE("Face whose first two edges do not share a vertex is invalid") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 1.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 1.0, 1.0, 0.0 });
		mesh->append_vertex(VectorN{ 0.0, 1.0, 0.0 });
		mesh->append_edge_indices(0, 1); // Edge 0.
		mesh->append_edge_indices(1, 2); // Edge 1.
		mesh->append_edge_indices(2, 3); // Edge 2.
		mesh->append_edge_indices(0, 3); // Edge 3.
		// Edges 0 and 2 are opposite sides of the square, so orientation is not determinable.
		mesh->append_poly_cell(2, PackedInt32Array{ 0, 2, 1, 3 }, false);
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "The first two edges of a face must share a vertex.");
		ERR_PRINT_ON;
	}

	SUBCASE("Cell whose first two faces do not share an edge is invalid") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		// Two disconnected triangles.
		mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 1.0, 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 0.0, 1.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 5.0, 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 6.0, 0.0, 0.0, 0.0 });
		mesh->append_vertex(VectorN{ 5.0, 1.0, 0.0, 0.0 });
		mesh->append_edge_indices(0, 1); // Edge 0.
		mesh->append_edge_indices(0, 2); // Edge 1.
		mesh->append_edge_indices(1, 2); // Edge 2.
		mesh->append_edge_indices(3, 4); // Edge 3.
		mesh->append_edge_indices(3, 5); // Edge 4.
		mesh->append_edge_indices(4, 5); // Edge 5.
		mesh->append_poly_cell(2, PackedInt32Array{ 0, 2, 1 }, false);
		mesh->append_poly_cell(2, PackedInt32Array{ 3, 5, 4 }, false);
		mesh->append_poly_cell(3, PackedInt32Array{ 0, 1, 0, 1 }, false);
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "The first two faces of a 3D cell must share an edge.");
		ERR_PRINT_ON;
	}

	SUBCASE("Boundary normals count must match boundary cell count") {
		Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh();
		Vector<VectorN> two_normals;
		two_normals.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		two_normals.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		mesh->set_poly_cell_boundary_normals(two_normals);
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "Two boundary normals for one cell is invalid.");
		ERR_PRINT_ON;
		Vector<VectorN> one_normal;
		one_normal.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		mesh->set_poly_cell_boundary_normals(one_normal);
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "One boundary normal for one cell is valid.");
	}

	SUBCASE("Boundary pivot overrides must reference valid vertices") {
		Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh();
		mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 99 });
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "Pivot override referencing a non-existent vertex is invalid.");
		ERR_PRINT_ON;
		mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ -1 });
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Pivot override of -1 means no override, which is valid.");
		mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 0 });
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Pivot override referencing an existing vertex is valid.");
	}

	SUBCASE("Vertex normals count must match cell vertex count") {
		Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh();
		Vector<Vector<VectorN>> vertex_normals;
		Vector<VectorN> cell_normals;
		for (int i = 0; i < 3; i++) {
			cell_normals.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		}
		vertex_normals.push_back(cell_normals);
		mesh->set_poly_cell_vertex_normals(vertex_normals);
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "Three vertex normals for a cell with four vertices is invalid.");
		ERR_PRINT_ON;
		cell_normals.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		vertex_normals.set(0, cell_normals);
		mesh->set_poly_cell_vertex_normals(vertex_normals);
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Four vertex normals for a cell with four vertices is valid.");
		vertex_normals.set(0, Vector<VectorN>());
		mesh->set_poly_cell_vertex_normals(vertex_normals);
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Cells are allowed to be missing vertex normals.");
	}

	SUBCASE("Texture map count must match cell vertex count") {
		Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh();
		Vector<Vector<VectorM>> texture_map;
		Vector<VectorM> cell_map;
		cell_map.push_back(VectorM{ 0.0, 0.0, 0.0 });
		cell_map.push_back(VectorM{ 1.0, 0.0, 0.0 });
		texture_map.push_back(cell_map);
		mesh->set_poly_cell_texture_map(texture_map);
		ERR_PRINT_OFF;
		CHECK_FALSE_MESSAGE(mesh->is_poly_mesh_data_valid(), "Two texture map entries for a cell with four vertices is invalid.");
		ERR_PRINT_ON;
		cell_map.push_back(VectorM{ 0.0, 1.0, 0.0 });
		cell_map.push_back(VectorM{ 0.0, 0.0, 1.0 });
		texture_map.set(0, cell_map);
		mesh->set_poly_cell_texture_map(texture_map);
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Four texture map entries for a cell with four vertices is valid.");
		texture_map.set(0, Vector<VectorM>());
		mesh->set_poly_cell_texture_map(texture_map);
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Cells are allowed to be missing texture map data.");
	}
}

TEST_CASE("[PolyMeshND] Cell orientation determines boundary normals for all face permutations") {
	// The same exhaustive test as the 4D module: the test tetrahedron is flat in the w=0
	// hyperplane, so every orientation-derived normal must be exactly +W or -W, the order of
	// faces after the first two must not matter, and swapping the first two faces must flip
	// the normal. This cross-validates that the ND canonical span matches the 4D conventions.
	VectorN pair_normals[4][4];
	bool pair_seen[4][4] = {};
	const VectorN pos_w = VectorN{ 0.0, 0.0, 0.0, 1.0 };
	for (int32_t first = 0; first < 4; first++) {
		for (int32_t second = 0; second < 4; second++) {
			if (second == first) {
				continue;
			}
			for (int32_t third = 0; third < 4; third++) {
				if (third == first || third == second) {
					continue;
				}
				const int32_t fourth = 6 - first - second - third;
				Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh(PackedInt32Array{ first, second, third, fourth });
				CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Any permutation of a tetrahedron's faces is a valid cell.");
				mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
				const Vector<VectorN> normals = mesh->get_poly_cell_boundary_normals();
				REQUIRE(normals.size() == 1);
				const VectorN normal = normals[0];
				REQUIRE(normal.size() == 4);
				CHECK_MESSAGE(Math::is_equal_approx(Math::abs(normal[3]), 1.0), "The normal of a cell flat in the w=0 hyperplane must be +W or -W.");
				if (pair_seen[first][second]) {
					CHECK_MESSAGE(VectorND::is_equal_approx(normal, pair_normals[first][second]), "The order of faces after the first two must not affect the normal.");
				} else {
					pair_normals[first][second] = normal;
					pair_seen[first][second] = true;
				}
			}
		}
	}
	// Verify the anchor orientation, matching the 4D module's convention for the same data.
	CHECK_MESSAGE(VectorND::is_equal_approx(pair_normals[0][1], pos_w), "The face order {0, 1, 2, 3} must produce a +W normal, matching the 4D module.");
	// Verify that swapping the first two faces flips the normal, for every possible pair.
	for (int32_t first = 0; first < 4; first++) {
		for (int32_t second = first + 1; second < 4; second++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(pair_normals[first][second], VectorND::negate(pair_normals[second][first])), "Swapping the first two faces of a cell must flip its normal vector.");
		}
	}
}

TEST_CASE("[PolyMeshND] Box generator produces valid oriented data") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<BoxPolyMeshND> box = make_box_poly_mesh(dimension);
		CHECK_MESSAGE(box->get_vertex_positions().size() == (int64_t(1) << dimension), "An N-box has 2^N vertices.");
		CHECK_MESSAGE(box->get_edge_indices().size() == dimension * (int64_t(1) << (dimension - 1)) * 2, "An N-box has N*2^(N-1) edges.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = box->get_poly_cell_indices();
		REQUIRE_MESSAGE(poly_cell_indices.size() == dimension - 1, "An N-box has poly cells from 2D faces up to the N-dimensional volume.");
		CHECK_MESSAGE(poly_cell_indices[dimension - 3].size() == 2 * dimension, "An N-box has 2N boundary cells.");
		REQUIRE(poly_cell_indices[dimension - 2].size() == 1);
		const PackedInt32Array volumetric_cell = poly_cell_indices[dimension - 2][0];
		CHECK_MESSAGE(volumetric_cell.size() == 2 * dimension, "The N-box volume contains all boundary cells.");
		const Vector<PackedInt32Array> boundary_cell_vertex_indices = box->get_all_boundary_cell_vertex_indices(false);
		int64_t shared_vertex_count = 0;
		for (const int32_t vertex_index : boundary_cell_vertex_indices[volumetric_cell[0]]) {
			if (boundary_cell_vertex_indices[volumetric_cell[1]].has(vertex_index)) {
				shared_vertex_count++;
			}
		}
		CHECK_MESSAGE(shared_vertex_count == (int64_t(1) << (dimension - 2)), "The first two boundary cells listed in the volumetric cell must be connected, sharing a common ridge.");
		const Vector<VectorN> generated_normals = box->get_poly_cell_boundary_normals();
		REQUIRE(generated_normals.size() == 2 * dimension);
		// The generated boundary cell orientations must reproduce the generated outward normals.
		Ref<ArrayPolyMeshND> array_mesh = box->to_array_poly_mesh();
		CHECK(array_mesh->is_poly_mesh_data_valid());
		array_mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> computed_normals = array_mesh->get_poly_cell_boundary_normals();
		REQUIRE(computed_normals.size() == generated_normals.size());
		for (int64_t cell_index = 0; cell_index < generated_normals.size(); cell_index++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(computed_normals[cell_index], generated_normals[cell_index]), "The box cell orientations must reproduce the generated outward normals.");
		}
	}
}

TEST_CASE("[PolyMeshND] Orthoplex generator produces valid oriented data") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<OrthoplexPolyMeshND> orthoplex = make_orthoplex_poly_mesh(dimension);
		CHECK_MESSAGE(orthoplex->get_vertex_positions().size() == 2 * dimension, "An N-orthoplex has 2N vertices.");
		CHECK_MESSAGE(orthoplex->get_edge_indices().size() == dimension * (dimension - 1) * 2 * 2, "An N-orthoplex has C(N,2)*4 edges.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = orthoplex->get_poly_cell_indices();
		REQUIRE_MESSAGE(poly_cell_indices.size() == dimension - 1, "An N-orthoplex has poly cells from 2D faces up to the N-dimensional volume.");
		CHECK_MESSAGE(poly_cell_indices[dimension - 3].size() == (int64_t(1) << dimension), "An N-orthoplex has 2^N boundary cells.");
		REQUIRE(poly_cell_indices[dimension - 2].size() == 1);
		const PackedInt32Array volumetric_cell = poly_cell_indices[dimension - 2][0];
		CHECK_MESSAGE(volumetric_cell.size() == (int64_t(1) << dimension), "The N-orthoplex volume contains all boundary cells.");
		const Vector<PackedInt32Array> boundary_cell_vertex_indices = orthoplex->get_all_boundary_cell_vertex_indices(false);
		int64_t shared_vertex_count = 0;
		for (const int32_t vertex_index : boundary_cell_vertex_indices[volumetric_cell[0]]) {
			if (boundary_cell_vertex_indices[volumetric_cell[1]].has(vertex_index)) {
				shared_vertex_count++;
			}
		}
		CHECK_MESSAGE(shared_vertex_count == dimension - 1, "The first two boundary cells listed in the volumetric cell must be connected, sharing a common ridge.");
		const Vector<VectorN> generated_normals = orthoplex->get_poly_cell_boundary_normals();
		REQUIRE(generated_normals.size() == (int64_t(1) << dimension));
		Ref<ArrayPolyMeshND> array_mesh = orthoplex->to_array_poly_mesh();
		CHECK(array_mesh->is_poly_mesh_data_valid());
		array_mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> computed_normals = array_mesh->get_poly_cell_boundary_normals();
		REQUIRE(computed_normals.size() == generated_normals.size());
		for (int64_t cell_index = 0; cell_index < generated_normals.size(); cell_index++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(computed_normals[cell_index], generated_normals[cell_index]), "The orthoplex cell orientations must reproduce the generated outward normals.");
		}
	}
}

TEST_CASE("[PolyMeshND] Simplex decomposition") {
	SUBCASE("A single tetrahedral cell decomposes into one simplex") {
		Ref<ArrayPolyMeshND> mesh = make_tetrahedron_cell_mesh();
		const PackedInt32Array simplex_indices = mesh->get_simplex_cell_vertex_indices();
		REQUIRE_MESSAGE(simplex_indices.size() == 4, "A tetrahedral cell in 4D must decompose into exactly one simplex of 4 vertices.");
		for (int32_t vertex_index = 0; vertex_index < 4; vertex_index++) {
			CHECK_MESSAGE(simplex_indices.has(vertex_index), "The single simplex must use every vertex of the tetrahedral cell.");
		}
		CHECK_MESSAGE(mesh->get_source_poly_cell_for_simplex_cell(0) == 0, "The simplex must map back to the boundary cell it came from.");
		CHECK_MESSAGE(mesh->get_source_poly_cell_for_simplex_cell(1) == -1, "Out of range simplex indices must map to -1.");
		const Vector<VectorN> simplex_normals = mesh->get_simplex_cell_boundary_normals();
		REQUIRE(simplex_normals.size() == 1);
		CHECK_MESSAGE(VectorND::is_equal_approx(simplex_normals[0], VectorN{ 0.0, 0.0, 0.0, 1.0 }), "The simplex normal must match the cell orientation.");
	}

	SUBCASE("A 3D box decomposes into 12 triangles") {
		Ref<BoxPolyMeshND> box = make_box_poly_mesh(3);
		const PackedInt32Array simplex_indices = box->get_simplex_cell_vertex_indices();
		CHECK_MESSAGE(simplex_indices.size() == 12 * 3, "A cube's 6 quad faces each decompose into 2 triangles.");
	}

	SUBCASE("A 4D orthoplex decomposes into 16 tetrahedra") {
		Ref<OrthoplexPolyMeshND> orthoplex = make_orthoplex_poly_mesh(4);
		const PackedInt32Array simplex_indices = orthoplex->get_simplex_cell_vertex_indices();
		REQUIRE_MESSAGE(simplex_indices.size() == 16 * 4, "Each of the 16 tetrahedral cells decomposes into exactly one simplex.");
		CHECK_MESSAGE(orthoplex->get_vertex_positions().size() == 8, "Decomposing an orthoplex should not add any vertices.");
	}

	SUBCASE("Simplex normals match the source cell normals") {
		for (int dimension = 3; dimension <= 5; dimension++) {
			Ref<BoxPolyMeshND> box = make_box_poly_mesh(dimension);
			const PackedInt32Array simplex_indices = box->get_simplex_cell_vertex_indices();
			REQUIRE(simplex_indices.size() % dimension == 0);
			const int64_t simplex_count = simplex_indices.size() / dimension;
			REQUIRE(simplex_count > 0);
			const Vector<VectorN> cell_normals = box->get_poly_cell_boundary_normals();
			const Vector<VectorN> simplex_normals = box->get_simplex_cell_boundary_normals();
			REQUIRE(simplex_normals.size() == simplex_count);
			for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
				const int32_t source_cell = box->get_source_poly_cell_for_simplex_cell(simplex_index);
				REQUIRE(source_cell >= 0);
				REQUIRE(source_cell < cell_normals.size());
				CHECK_MESSAGE(VectorND::is_equal_approx(simplex_normals[simplex_index], cell_normals[source_cell]), "Each simplex normal must match the normal of its source cell.");
			}
		}
	}

	SUBCASE("Simplex vertex normals come from the source cells") {
		Ref<BoxPolyMeshND> box = make_box_poly_mesh(4);
		const PackedInt32Array simplex_indices = box->get_simplex_cell_vertex_indices();
		const int64_t simplex_count = simplex_indices.size() / 4;
		const Vector<VectorN> cell_normals = box->get_poly_cell_boundary_normals();
		const Vector<VectorN> simplex_vertex_normals = box->get_simplex_cell_vertex_normals();
		REQUIRE(simplex_vertex_normals.size() == simplex_count * 4);
		for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
			const int32_t source_cell = box->get_source_poly_cell_for_simplex_cell(simplex_index);
			for (int64_t vertex_in_simplex = 0; vertex_in_simplex < 4; vertex_in_simplex++) {
				CHECK(VectorND::is_equal_approx(simplex_vertex_normals[simplex_index * 4 + vertex_in_simplex], cell_normals[source_cell]));
			}
		}
	}

	SUBCASE("Boundary pivot overrides are used by the simplex decomposition") {
		Ref<BoxPolyMeshND> box = make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		// Force a corner of cell 0 as its pivot: every simplex of cell 0 must contain it.
		const Vector<PackedInt32Array> cell_vertex_indices = mesh->get_all_boundary_cell_vertex_indices(false);
		const int32_t override_vertex = cell_vertex_indices[0][cell_vertex_indices[0].size() - 1];
		PackedInt32Array pivot_overrides;
		pivot_overrides.resize(8);
		for (int64_t i = 0; i < 8; i++) {
			pivot_overrides.set(i, -1);
		}
		pivot_overrides.set(0, override_vertex);
		mesh->set_poly_cell_boundary_pivot_overrides(pivot_overrides);
		const PackedInt32Array simplex_indices = mesh->get_simplex_cell_vertex_indices();
		REQUIRE(simplex_indices.size() % 4 == 0);
		const int64_t simplex_count = simplex_indices.size() / 4;
		int64_t cell_0_simplex_count = 0;
		for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
			if (mesh->get_source_poly_cell_for_simplex_cell(simplex_index) != 0) {
				continue;
			}
			cell_0_simplex_count++;
			bool has_pivot = false;
			for (int64_t i = 0; i < 4; i++) {
				if (simplex_indices[simplex_index * 4 + i] == override_vertex) {
					has_pivot = true;
				}
			}
			CHECK_MESSAGE(has_pivot, "Every simplex of a cell with a pivot override must contain the override vertex.");
		}
		CHECK_MESSAGE(cell_0_simplex_count > 0, "Cell 0 must produce simplexes.");
	}
}

TEST_CASE("[PolyMeshND] Watertight simplex decomposition") {
	// The critical acceptance test for the constraint-propagation decomposition: for a closed
	// mesh, the boundary simplexes must form a crack-free simplicial complex, which holds if
	// and only if every (N-2)-dimensional facet is shared by exactly 2 simplexes. Any
	// T-junction (two adjacent cells decomposing a shared cell differently) breaks this count,
	// so passing this proves that deformation (skinning, blend shapes) cannot open cracks.
	for (int dimension = 3; dimension <= 5; dimension++) {
		for (int shape = 0; shape < 2; shape++) {
			Ref<PolyMeshND> mesh;
			if (shape == 0) {
				mesh = make_box_poly_mesh(dimension);
			} else {
				mesh = make_orthoplex_poly_mesh(dimension);
			}
			const PackedInt32Array simplex_indices = mesh->get_simplex_cell_vertex_indices();
			REQUIRE(simplex_indices.size() % dimension == 0);
			const int64_t simplex_count = simplex_indices.size() / dimension;
			REQUIRE(simplex_count > 0);
			HashMap<int64_t, int32_t> facet_counts;
			for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
				for (int64_t drop_index = 0; drop_index < dimension; drop_index++) {
					PackedInt32Array facet;
					for (int64_t i = 0; i < dimension; i++) {
						if (i == drop_index) {
							continue;
						}
						facet.append(simplex_indices[simplex_index * dimension + i]);
					}
					facet.sort();
					const int64_t key = facet_key(facet);
					if (facet_counts.has(key)) {
						facet_counts[key] = facet_counts[key] + 1;
					} else {
						facet_counts[key] = 1;
					}
				}
			}
			bool all_facets_shared_exactly_twice = true;
			for (const KeyValue<int64_t, int32_t> &facet_kv : facet_counts) {
				if (facet_kv.value != 2) {
					all_facets_shared_exactly_twice = false;
					break;
				}
			}
			CHECK_MESSAGE(all_facets_shared_exactly_twice, "Every facet of a closed mesh's decomposition must be shared by exactly 2 simplexes (no cracks or T-junctions).");
		}
	}
}

TEST_CASE("[PolyMeshND] Box texture maps") {
	SUBCASE("Cross tiles align with the center cell in texture and world space") {
		// For each cross-tiled cell, the vertices shared with the center cell must have the
		// same texture coordinates in both cells, so that the unfolding is seamless. Shared
		// vertices are the same vertex indices, so the world coordinates match trivially.
		for (int dimension = 3; dimension <= 5; dimension++) {
			for (int mode = 0; mode < 2; mode++) {
				Ref<BoxPolyMeshND> box = make_box_poly_mesh(dimension);
				box->set_poly_texture_map(mode == 0 ? BoxPolyMeshND::BOX_POLY_TEXTURE_MAP_CROSS_ISLAND : BoxPolyMeshND::BOX_POLY_TEXTURE_MAP_LONG_CROSS);
				const Vector<PackedInt32Array> cell_vertex_indices = box->get_all_boundary_cell_vertex_indices(false);
				const Vector<Vector<VectorM>> texture_map = box->get_poly_cell_texture_map();
				const Vector<VectorN> normals = box->get_poly_cell_boundary_normals();
				REQUIRE(texture_map.size() == cell_vertex_indices.size());
				// Find the center cell, facing the positive last axis, and map its texcoords.
				const int64_t last_axis = dimension - 1;
				int64_t center_cell = -1;
				for (int64_t cell_index = 0; cell_index < normals.size(); cell_index++) {
					if (normals[cell_index][last_axis] > 0.0) {
						center_cell = cell_index;
						break;
					}
				}
				REQUIRE(center_cell != -1);
				HashMap<int32_t, VectorM> center_texcoords;
				for (int64_t i = 0; i < cell_vertex_indices[center_cell].size(); i++) {
					center_texcoords.insert(cell_vertex_indices[center_cell][i], texture_map[center_cell][i]);
				}
				for (int64_t cell_index = 0; cell_index < normals.size(); cell_index++) {
					if (normals[cell_index][last_axis] != 0.0) {
						continue; // Only check the side cells of the cross, not the last axis cells.
					}
					int64_t shared_vertex_count = 0;
					for (int64_t i = 0; i < cell_vertex_indices[cell_index].size(); i++) {
						const int32_t vertex_index = cell_vertex_indices[cell_index][i];
						if (!center_texcoords.has(vertex_index)) {
							continue;
						}
						shared_vertex_count++;
						CHECK_MESSAGE(VectorND::is_equal_approx(texture_map[cell_index][i], center_texcoords[vertex_index]), "Vertices shared with the center cell must have the same texture coordinates in both cells.");
					}
					CHECK_MESSAGE(shared_vertex_count == (int64_t(1) << (dimension - 2)), "Each side cell shares half of its vertices with the center cell.");
				}
			}
		}
	}

	SUBCASE("The long cross negative cell connects to the positive first axis cell") {
		for (int dimension = 3; dimension <= 5; dimension++) {
			Ref<BoxPolyMeshND> box = make_box_poly_mesh(dimension);
			box->set_poly_texture_map(BoxPolyMeshND::BOX_POLY_TEXTURE_MAP_LONG_CROSS);
			const Vector<PackedInt32Array> cell_vertex_indices = box->get_all_boundary_cell_vertex_indices(false);
			const Vector<Vector<VectorM>> texture_map = box->get_poly_cell_texture_map();
			const Vector<VectorN> normals = box->get_poly_cell_boundary_normals();
			const int64_t last_axis = dimension - 1;
			int64_t negative_last_cell = -1;
			int64_t positive_first_cell = -1;
			for (int64_t cell_index = 0; cell_index < normals.size(); cell_index++) {
				if (normals[cell_index][last_axis] < 0.0) {
					negative_last_cell = cell_index;
				} else if (normals[cell_index][0] > 0.0) {
					positive_first_cell = cell_index;
				}
			}
			REQUIRE(negative_last_cell != -1);
			REQUIRE(positive_first_cell != -1);
			HashMap<int32_t, VectorM> positive_first_texcoords;
			for (int64_t i = 0; i < cell_vertex_indices[positive_first_cell].size(); i++) {
				positive_first_texcoords.insert(cell_vertex_indices[positive_first_cell][i], texture_map[positive_first_cell][i]);
			}
			int64_t shared_vertex_count = 0;
			for (int64_t i = 0; i < cell_vertex_indices[negative_last_cell].size(); i++) {
				const int32_t vertex_index = cell_vertex_indices[negative_last_cell][i];
				if (!positive_first_texcoords.has(vertex_index)) {
					continue;
				}
				shared_vertex_count++;
				CHECK_MESSAGE(VectorND::is_equal_approx(texture_map[negative_last_cell][i], positive_first_texcoords[vertex_index]), "The long cross's negative last cell must connect seamlessly to the positive first axis cell.");
			}
			CHECK(shared_vertex_count == (int64_t(1) << (dimension - 2)));
		}
	}

	SUBCASE("Fill each side spans the full texture space with the 4D orientation conventions") {
		Ref<BoxPolyMeshND> box = make_box_poly_mesh(4);
		box->set_poly_texture_map(BoxPolyMeshND::BOX_POLY_TEXTURE_MAP_FILL_EACH_SIDE);
		const Vector<PackedInt32Array> cell_vertex_indices = box->get_all_boundary_cell_vertex_indices(false);
		const Vector<Vector<VectorM>> texture_map = box->get_poly_cell_texture_map();
		const Vector<VectorN> normals = box->get_poly_cell_boundary_normals();
		for (int64_t cell_index = 0; cell_index < normals.size(); cell_index++) {
			for (int64_t i = 0; i < cell_vertex_indices[cell_index].size(); i++) {
				const int32_t vertex_index = cell_vertex_indices[cell_index][i];
				const VectorM &texcoord = texture_map[cell_index][i];
				if (normals[cell_index][3] > 0.0 && vertex_index == 0b1111) {
					// The positive last cell maps vertex bits directly, matching the 4D module.
					CHECK(VectorND::is_equal_approx(texcoord, VectorM{ 1.0, 1.0, 1.0 }));
				} else if (normals[cell_index][3] < 0.0 && vertex_index == 0b0111) {
					// The negative last cell is mirrored on every axis, matching the 4D module.
					CHECK(VectorND::is_equal_approx(texcoord, VectorM{ 0.0, 0.0, 0.0 }));
				}
				for (int64_t axis = 0; axis < 3; axis++) {
					CHECK_MESSAGE((texcoord[axis] == 0.0 || texcoord[axis] == 1.0), "Fill each side must map every vertex to a corner of the texture space.");
				}
			}
		}
	}

	SUBCASE("Texture maps are valid and within range for all modes") {
		for (int dimension = 3; dimension <= 5; dimension++) {
			for (int mode = 0; mode <= 2; mode++) {
				Ref<BoxPolyMeshND> box = make_box_poly_mesh(dimension);
				box->set_poly_texture_map((BoxPolyMeshND::BoxPolyTextureMap)mode);
				const Vector<Vector<VectorM>> texture_map = box->get_poly_cell_texture_map();
				REQUIRE(texture_map.size() == 2 * dimension);
				for (int64_t cell_index = 0; cell_index < texture_map.size(); cell_index++) {
					REQUIRE(texture_map[cell_index].size() == (int64_t(1) << (dimension - 1)));
					for (int64_t i = 0; i < texture_map[cell_index].size(); i++) {
						const VectorM &texcoord = texture_map[cell_index][i];
						REQUIRE(texcoord.size() == dimension - 1);
						for (int64_t axis = 0; axis < texcoord.size(); axis++) {
							CHECK(texcoord[axis] >= 0.0);
							CHECK(texcoord[axis] <= 1.0);
						}
					}
				}
				CHECK_MESSAGE(box->to_array_poly_mesh()->is_poly_mesh_data_valid(), "The generated texture map must be valid for the box's cells.");
			}
		}
		Ref<BoxPolyMeshND> flat_box = make_box_poly_mesh(2);
		CHECK_MESSAGE(flat_box->get_poly_cell_texture_map().is_empty(), "A 2D box has no boundary poly cells to texture map.");
	}
}

TEST_CASE("[PolyMeshND] Orthoplex texture map") {
	SUBCASE("Texture coordinates match the reflection construction") {
		// A 3D orthoplex's +X +Y -Z cell maps -Z to the center reflected over the line
		// through (1.0, 0.5) and (0.5, 1.0), which is (1.0, 1.0).
		Ref<OrthoplexPolyMeshND> octahedron = make_orthoplex_poly_mesh(3);
		const Vector<PackedInt32Array> cell_vertex_indices_3d = octahedron->get_all_boundary_cell_vertex_indices(false);
		const Vector<Vector<VectorM>> texture_map_3d = octahedron->get_poly_cell_texture_map();
		const Vector<VectorN> normals_3d = octahedron->get_poly_cell_boundary_normals();
		for (int64_t cell_index = 0; cell_index < normals_3d.size(); cell_index++) {
			if (normals_3d[cell_index][0] > 0.0 && normals_3d[cell_index][1] > 0.0 && normals_3d[cell_index][2] < 0.0) {
				for (int64_t i = 0; i < cell_vertex_indices_3d[cell_index].size(); i++) {
					const int32_t vertex_index = cell_vertex_indices_3d[cell_index][i];
					if (vertex_index == 1) { // +X vertex.
						CHECK(VectorND::is_equal_approx(texture_map_3d[cell_index][i], VectorM{ 1.0, 0.5 }));
					} else if (vertex_index == 3) { // +Y vertex.
						CHECK(VectorND::is_equal_approx(texture_map_3d[cell_index][i], VectorM{ 0.5, 1.0 }));
					} else if (vertex_index == 4) { // -Z vertex.
						CHECK(VectorND::is_equal_approx(texture_map_3d[cell_index][i], VectorM{ 1.0, 1.0 }));
					}
				}
			}
		}
		// A 4D orthoplex's +X +Y +Z -W cell maps -W to the center reflected over the plane
		// through (1.0, 0.5, 0.5), (0.5, 1.0, 0.5), and (0.5, 0.5, 1.0), which is 5/6 on each axis.
		Ref<OrthoplexPolyMeshND> orthoplex = make_orthoplex_poly_mesh(4);
		const Vector<PackedInt32Array> cell_vertex_indices_4d = orthoplex->get_all_boundary_cell_vertex_indices(false);
		const Vector<Vector<VectorM>> texture_map_4d = orthoplex->get_poly_cell_texture_map();
		const Vector<VectorN> normals_4d = orthoplex->get_poly_cell_boundary_normals();
		for (int64_t cell_index = 0; cell_index < normals_4d.size(); cell_index++) {
			const bool all_others_positive = normals_4d[cell_index][0] > 0.0 && normals_4d[cell_index][1] > 0.0 && normals_4d[cell_index][2] > 0.0;
			if (!all_others_positive) {
				continue;
			}
			for (int64_t i = 0; i < cell_vertex_indices_4d[cell_index].size(); i++) {
				const int32_t vertex_index = cell_vertex_indices_4d[cell_index][i];
				if (vertex_index == 6 && normals_4d[cell_index][3] < 0.0) { // -W vertex.
					CHECK(VectorND::is_equal_approx(texture_map_4d[cell_index][i], VectorM{ 5.0 / 6.0, 5.0 / 6.0, 5.0 / 6.0 }));
				} else if (vertex_index == 7 && normals_4d[cell_index][3] > 0.0) { // +W vertex.
					CHECK(VectorND::is_equal_approx(texture_map_4d[cell_index][i], VectorM{ 0.5, 0.5, 0.5 }));
				} else if (vertex_index == 1) { // +X vertex, same in both cells.
					CHECK(VectorND::is_equal_approx(texture_map_4d[cell_index][i], VectorM{ 1.0, 0.5, 0.5 }));
				}
			}
		}
	}

	SUBCASE("Cells sharing the same non-last signs align on their shared vertices") {
		for (int dimension = 3; dimension <= 5; dimension++) {
			Ref<OrthoplexPolyMeshND> orthoplex = make_orthoplex_poly_mesh(dimension);
			const Vector<PackedInt32Array> cell_vertex_indices = orthoplex->get_all_boundary_cell_vertex_indices(false);
			const Vector<Vector<VectorM>> texture_map = orthoplex->get_poly_cell_texture_map();
			const Vector<VectorN> normals = orthoplex->get_poly_cell_boundary_normals();
			const int64_t last_axis = dimension - 1;
			for (int64_t cell_index = 0; cell_index < normals.size(); cell_index++) {
				if (normals[cell_index][last_axis] < 0.0) {
					continue;
				}
				// Find the partner cell with the same signs except for the negative last axis.
				int64_t partner_cell = -1;
				for (int64_t other_index = 0; other_index < normals.size(); other_index++) {
					if (normals[other_index][last_axis] > 0.0) {
						continue;
					}
					bool signs_match = true;
					for (int64_t axis = 0; axis < last_axis; axis++) {
						if ((normals[cell_index][axis] > 0.0) != (normals[other_index][axis] > 0.0)) {
							signs_match = false;
							break;
						}
					}
					if (signs_match) {
						partner_cell = other_index;
						break;
					}
				}
				REQUIRE(partner_cell != -1);
				HashMap<int32_t, VectorM> cell_texcoords;
				for (int64_t i = 0; i < cell_vertex_indices[cell_index].size(); i++) {
					cell_texcoords.insert(cell_vertex_indices[cell_index][i], texture_map[cell_index][i]);
				}
				int64_t shared_vertex_count = 0;
				for (int64_t i = 0; i < cell_vertex_indices[partner_cell].size(); i++) {
					const int32_t vertex_index = cell_vertex_indices[partner_cell][i];
					if (!cell_texcoords.has(vertex_index)) {
						continue;
					}
					shared_vertex_count++;
					CHECK_MESSAGE(VectorND::is_equal_approx(texture_map[partner_cell][i], cell_texcoords[vertex_index]), "The shared vertices of the positive and negative last axis cells must have the same texture coordinates.");
				}
				CHECK_MESSAGE(shared_vertex_count == dimension - 1, "The partner cells share every vertex except their last axis vertices.");
			}
		}
	}

	SUBCASE("Texture maps are valid and within range") {
		for (int dimension = 3; dimension <= 5; dimension++) {
			Ref<OrthoplexPolyMeshND> orthoplex = make_orthoplex_poly_mesh(dimension);
			const Vector<Vector<VectorM>> texture_map = orthoplex->get_poly_cell_texture_map();
			REQUIRE(texture_map.size() == (int64_t(1) << dimension));
			for (int64_t cell_index = 0; cell_index < texture_map.size(); cell_index++) {
				REQUIRE(texture_map[cell_index].size() == dimension);
				for (int64_t i = 0; i < texture_map[cell_index].size(); i++) {
					const VectorM &texcoord = texture_map[cell_index][i];
					REQUIRE(texcoord.size() == dimension - 1);
					for (int64_t axis = 0; axis < texcoord.size(); axis++) {
						CHECK(texcoord[axis] >= 0.0);
						CHECK(texcoord[axis] <= 1.0);
					}
				}
			}
			CHECK_MESSAGE(orthoplex->to_array_poly_mesh()->is_poly_mesh_data_valid(), "The generated texture map must be valid for the orthoplex's cells.");
			CHECK_MESSAGE(!orthoplex->get_simplex_cell_texture_map().is_empty(), "The texture map must carry through to the simplex decomposition.");
		}
		Ref<OrthoplexPolyMeshND> diamond = make_orthoplex_poly_mesh(2);
		CHECK_MESSAGE(diamond->get_poly_cell_texture_map().is_empty(), "A 2D orthoplex has no boundary poly cells to texture map.");
	}
}

TEST_CASE("[PolyMeshND] Poly cell vertex indices and poly indices") {
	Ref<BoxPolyMeshND> box = make_box_poly_mesh(4);
	SUBCASE("Vertex indices by dimension") {
		CHECK(box->get_all_poly_cell_vertex_indices(0, false).size() == 16);
		CHECK(box->get_all_poly_cell_vertex_indices(1, false).size() == 32);
		const Vector<PackedInt32Array> face_vertices = box->get_all_poly_cell_vertex_indices(2, false);
		REQUIRE(face_vertices.size() == 24);
		for (int64_t face_index = 0; face_index < 24; face_index++) {
			CHECK(face_vertices[face_index].size() == 4);
		}
		const Vector<PackedInt32Array> cell_vertices = box->get_all_poly_cell_vertex_indices(3, false);
		REQUIRE(cell_vertices.size() == 8);
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			CHECK(cell_vertices[cell_index].size() == 8);
		}
		const Vector<PackedInt32Array> hyper_vertices = box->get_all_poly_cell_vertex_indices(4, true);
		REQUIRE(hyper_vertices.size() == 1);
		CHECK(hyper_vertices[0].size() == 16);
	}
	SUBCASE("Poly indices decompositions") {
		const Vector<PackedInt32Array> cells_to_edges = box->get_all_poly_cell_poly_indices(3, 1);
		REQUIRE(cells_to_edges.size() == 8);
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			CHECK_MESSAGE(cells_to_edges[cell_index].size() == 12, "Each cube cell of a 4D box has 12 edges.");
		}
		const Vector<PackedInt32Array> hyper_to_faces = box->get_all_poly_cell_poly_indices(4, 2);
		REQUIRE(hyper_to_faces.size() == 1);
		CHECK(hyper_to_faces[0].size() == 24);
		ERR_PRINT_OFF;
		CHECK(box->get_all_poly_cell_poly_indices(2, 3).is_empty());
		CHECK(box->get_all_poly_cell_poly_indices(3, -1).is_empty());
		ERR_PRINT_ON;
	}
}

TEST_CASE("[PolyMeshND] To array poly mesh") {
	Ref<BoxPolyMeshND> box = make_box_poly_mesh(4);
	Ref<ArrayPolyMeshND> array_mesh = box->to_array_poly_mesh();
	REQUIRE(array_mesh.is_valid());
	CHECK(VectorND::array_is_equal_exact(array_mesh->get_poly_cell_vertex_positions(), box->get_poly_cell_vertex_positions()));
	CHECK((array_mesh->get_edge_indices() == box->get_edge_indices()));
	const Vector<Vector<PackedInt32Array>> array_indices = array_mesh->get_poly_cell_indices();
	const Vector<Vector<PackedInt32Array>> box_indices = box->get_poly_cell_indices();
	REQUIRE(array_indices.size() == box_indices.size());
	for (int64_t dim_index = 0; dim_index < array_indices.size(); dim_index++) {
		CHECK((array_indices[dim_index] == box_indices[dim_index]));
	}
	CHECK(VectorND::array_is_equal_exact(array_mesh->get_poly_cell_boundary_normals(), box->get_poly_cell_boundary_normals()));
	CHECK_MESSAGE(array_mesh->is_poly_mesh_data_valid(), "The converted array mesh must be valid.");
	CHECK_MESSAGE((array_mesh->get_simplex_cell_vertex_indices() == box->get_simplex_cell_vertex_indices()), "The converted array mesh must decompose identically.");
}

TEST_CASE("[PolyMeshND] Meshes without boundary cells") {
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	mesh->append_edge_points(VectorN{ 0.0, 0.0, 0.0, 0.0 }, VectorN{ 1.0, 0.0, 0.0, 0.0 });
	mesh->append_edge_points(VectorN{ 1.0, 0.0, 0.0, 0.0 }, VectorN{ 1.0, 1.0, 0.0, 0.0 });
	CHECK(mesh->is_poly_mesh_data_valid());
	CHECK_MESSAGE(VectorND::array_is_equal_exact(mesh->get_vertex_positions(), mesh->get_poly_cell_vertex_positions()), "Without boundary cells, the simplex vertices are just the poly vertices.");
	CHECK_MESSAGE(mesh->get_simplex_cell_vertex_indices().is_empty(), "Without boundary cells, there are no simplexes.");
}

TEST_CASE("[PolyMeshND] Faces with unordered edges are read in walk order") {
	// A pentagon face listing its edges in a valid but non-connected order: the first two
	// edges share a vertex, as validation requires, but the rest are not in loop order.
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertices = {
		VectorN{ 0.0, 0.0, 0.0 },
		VectorN{ 2.0, 0.0, 0.0 },
		VectorN{ 3.0, 2.0, 0.0 },
		VectorN{ 1.0, 4.0, 0.0 },
		VectorN{ -1.0, 2.0, 0.0 },
	};
	mesh->set_poly_cell_vertex_positions(vertices);
	mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 4, 0, 4 });
	Vector<PackedInt32Array> faces;
	faces.append(PackedInt32Array{ 0, 4, 1, 2, 3 }); // The scrambled pentagon: AB, AE, BC, CD, DE.
	mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
	ERR_PRINT_OFF; // The unordered edge list prints a warning, which is expected here.
	CHECK(mesh->is_poly_mesh_data_valid());
	const PackedInt32Array simplex_indices = mesh->get_simplex_cell_vertex_indices();
	const Vector<PackedInt32Array> face_vertex_indices = mesh->get_all_face_vertex_indices();
	ERR_PRINT_ON;
	// The triangulated area must match the pentagon's area of 10.
	const Vector<VectorN> simplex_vertex_positions = mesh->get_vertex_positions();
	double total_area = 0.0;
	for (int64_t simplex_start = 0; simplex_start < simplex_indices.size(); simplex_start += 3) {
		Vector<VectorN> edges = {
			VectorND::subtract(simplex_vertex_positions[simplex_indices[simplex_start + 1]], simplex_vertex_positions[simplex_indices[simplex_start]]),
			VectorND::subtract(simplex_vertex_positions[simplex_indices[simplex_start + 2]], simplex_vertex_positions[simplex_indices[simplex_start]]),
		};
		total_area += VectorND::length(VectorND::perpendicular(edges)) / 2.0;
	}
	CHECK_MESSAGE(total_area == doctest::Approx(10.0), "The triangulated area must match the pentagon's area even with an unordered edge list.");
	// The face's vertex sequence must be in boundary walk order: every cyclically
	// consecutive pair of vertices in the sequence must be connected by an edge.
	REQUIRE(face_vertex_indices.size() == 1);
	const PackedInt32Array &sequence = face_vertex_indices[0];
	REQUIRE(sequence.size() == 5);
	const PackedInt32Array all_edges = mesh->get_edge_indices();
	for (int64_t i = 0; i < sequence.size(); i++) {
		const int32_t vertex_a = sequence[i];
		const int32_t vertex_b = sequence[(i + 1) % sequence.size()];
		bool edge_exists = false;
		for (int64_t edge = 0; edge < all_edges.size(); edge += 2) {
			if ((all_edges[edge] == vertex_a && all_edges[edge + 1] == vertex_b) || (all_edges[edge] == vertex_b && all_edges[edge + 1] == vertex_a)) {
				edge_exists = true;
				break;
			}
		}
		CHECK_MESSAGE(edge_exists, "Every consecutive pair in the face's vertex sequence must be connected by an edge.");
	}
}

TEST_CASE("[PolyMeshND] Conformed and interior cells") {
	SUBCASE("A face with a collinear vertex chain decomposes without zero-measure simplexes") {
		// A square face whose bottom edge is split at its midpoint, as if conformed to
		// subdivided neighbors, making a 5-edge loop with a collinear vertex chain.
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0, 0.0 },
			VectorN{ 1.0, 0.0, 0.0 },
			VectorN{ 2.0, 0.0, 0.0 },
			VectorN{ 2.0, 2.0, 0.0 },
			VectorN{ 0.0, 2.0, 0.0 },
		};
		mesh->set_poly_cell_vertex_positions(vertices);
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 4, 0, 4 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3, 4 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
		CHECK(mesh->is_poly_mesh_data_valid());
		CHECK_MESSAGE(mesh->get_simplex_cell_vertex_indices().size() == 2 * 3, "The face must decompose into 2 simplexes, with the zero-measure one skipped.");
		const Vector<VectorN> simplex_normals = mesh->get_simplex_cell_boundary_normals();
		for (int64_t simplex_index = 0; simplex_index < simplex_normals.size(); simplex_index++) {
			CHECK_MESSAGE(!VectorND::is_zero_approx(simplex_normals[simplex_index]), "Every emitted simplex must have a nonzero normal.");
		}
		mesh->populate_inverse_metric_cache();
		const double sdf = mesh->get_signed_distance_to_mesh(VectorN{ 1.0, 1.0, 1.0 }, nullptr, nullptr);
		CHECK_MESSAGE(Math::abs(sdf) == doctest::Approx(1.0), "The signed distance must work on a mesh with a conformed face.");
	}
	SUBCASE("A fully conformed face with all edges split decomposes without zero-measure simplexes") {
		// A square face with all 4 edges split at their midpoints, making an 8-edge loop
		// where every corner vertex sits on a collinear chain.
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0, 0.0 },
			VectorN{ 1.0, 0.0, 0.0 },
			VectorN{ 2.0, 0.0, 0.0 },
			VectorN{ 2.0, 1.0, 0.0 },
			VectorN{ 2.0, 2.0, 0.0 },
			VectorN{ 1.0, 2.0, 0.0 },
			VectorN{ 0.0, 2.0, 0.0 },
			VectorN{ 0.0, 1.0, 0.0 },
		};
		mesh->set_poly_cell_vertex_positions(vertices);
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 0, 7 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3, 4, 5, 6, 7 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
		CHECK(mesh->is_poly_mesh_data_valid());
		const PackedInt32Array simplex_indices = mesh->get_simplex_cell_vertex_indices();
		CHECK_MESSAGE(simplex_indices.size() >= 4 * 3, "The face must decompose into at least 4 simplexes to cover its area.");
		const Vector<VectorN> simplex_normals = mesh->get_simplex_cell_boundary_normals();
		for (int64_t simplex_index = 0; simplex_index < simplex_normals.size(); simplex_index++) {
			CHECK_MESSAGE(!VectorND::is_zero_approx(simplex_normals[simplex_index]), "Every emitted simplex must have a nonzero normal.");
		}
		mesh->populate_inverse_metric_cache();
		const double sdf = mesh->get_signed_distance_to_mesh(VectorN{ 1.0, 1.0, 1.0 }, nullptr, nullptr);
		CHECK_MESSAGE(Math::abs(sdf) == doctest::Approx(1.0), "The signed distance must work on a mesh with a fully conformed face.");
	}
	SUBCASE("A box conformed to a subdivided neighbor face stays fully functional") {
		// A cube whose top face is subdivided into 4 sub-quads, so its 4 side faces are
		// conformed into 5-edge loops, and its volumetric cell references 9 faces.
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ -1.0, -1.0, -1.0 },
			VectorN{ 1.0, -1.0, -1.0 },
			VectorN{ 1.0, 1.0, -1.0 },
			VectorN{ -1.0, 1.0, -1.0 },
			VectorN{ -1.0, -1.0, 1.0 },
			VectorN{ 1.0, -1.0, 1.0 },
			VectorN{ 1.0, 1.0, 1.0 },
			VectorN{ -1.0, 1.0, 1.0 },
			VectorN{ 0.0, -1.0, 1.0 },
			VectorN{ 1.0, 0.0, 1.0 },
			VectorN{ 0.0, 1.0, 1.0 },
			VectorN{ -1.0, 0.0, 1.0 },
			VectorN{ 0.0, 0.0, 1.0 },
		};
		mesh->set_poly_cell_vertex_positions(vertices);
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 0, 3, 0, 4, 1, 5, 2, 6, 3, 7, 4, 8, 5, 8, 5, 9, 6, 9, 6, 10, 7, 10, 4, 11, 7, 11, 8, 12, 9, 12, 10, 12, 11, 12 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3 });
		faces.append(PackedInt32Array{ 0, 5, 9, 8, 4 });
		faces.append(PackedInt32Array{ 1, 6, 11, 10, 5 });
		faces.append(PackedInt32Array{ 2, 7, 13, 12, 6 });
		faces.append(PackedInt32Array{ 3, 4, 14, 15, 7 });
		faces.append(PackedInt32Array{ 8, 16, 19, 14 });
		faces.append(PackedInt32Array{ 9, 10, 17, 16 });
		faces.append(PackedInt32Array{ 17, 11, 12, 18 });
		faces.append(PackedInt32Array{ 19, 18, 13, 15 });
		Vector<PackedInt32Array> volumes;
		volumes.append(PackedInt32Array{ 0, 1, 2, 3, 4, 5, 6, 7, 8 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces, volumes });
		CHECK(mesh->is_poly_mesh_data_valid());
		mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);
		const Vector<VectorN> boundary_normals = mesh->get_poly_cell_boundary_normals();
		REQUIRE(boundary_normals.size() == 9);
		const VectorN pos_z = VectorN{ 0.0, 0.0, 1.0 };
		for (int64_t face_index = 5; face_index < 9; face_index++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(boundary_normals[face_index], pos_z), "The coplanar sub-faces of the subdivided top side must all have +Z normals.");
		}
		const Vector<VectorN> simplex_normals = mesh->get_simplex_cell_boundary_normals();
		for (int64_t simplex_index = 0; simplex_index < simplex_normals.size(); simplex_index++) {
			CHECK_MESSAGE(!VectorND::is_zero_approx(simplex_normals[simplex_index]), "Every emitted simplex must have a nonzero normal.");
		}
		mesh->populate_inverse_metric_cache();
		CHECK_MESSAGE(mesh->get_signed_distance_to_mesh(VectorN{ 2.0, 0.0, 0.0 }, nullptr, nullptr) == doctest::Approx(1.0), "The signed distance must be exact outside the conformed box.");
		CHECK_MESSAGE(mesh->get_signed_distance_to_mesh(VectorN{ 0.0, 0.0, 0.0 }, nullptr, nullptr) == doctest::Approx(-1.0), "The signed distance must be exact inside the conformed box.");
	}
	SUBCASE("Interior boundary cells shared by two volumetric cells generate no simplexes") {
		// Two unit cubes side by side sharing one square face, with two volumetric cells.
		// The shared face is an interior boundary cell, which must be excluded from rendering.
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0, 0.0 },
			VectorN{ 0.0, 1.0, 0.0 },
			VectorN{ 0.0, 0.0, 1.0 },
			VectorN{ 0.0, 1.0, 1.0 },
			VectorN{ 1.0, 0.0, 0.0 },
			VectorN{ 1.0, 1.0, 0.0 },
			VectorN{ 1.0, 0.0, 1.0 },
			VectorN{ 1.0, 1.0, 1.0 },
			VectorN{ 2.0, 0.0, 0.0 },
			VectorN{ 2.0, 1.0, 0.0 },
			VectorN{ 2.0, 0.0, 1.0 },
			VectorN{ 2.0, 1.0, 1.0 },
		};
		mesh->set_poly_cell_vertex_positions(vertices);
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 3, 2, 3, 0, 2, 4, 5, 5, 7, 6, 7, 4, 6, 8, 9, 9, 11, 10, 11, 8, 10, 0, 4, 1, 5, 2, 6, 3, 7, 4, 8, 5, 9, 6, 10, 7, 11 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3 }); // Left square at X = 0.
		faces.append(PackedInt32Array{ 4, 5, 6, 7 }); // The shared middle square at X = 1.
		faces.append(PackedInt32Array{ 8, 9, 10, 11 }); // Right square at X = 2.
		faces.append(PackedInt32Array{ 12, 7, 14, 3 });
		faces.append(PackedInt32Array{ 13, 5, 15, 1 });
		faces.append(PackedInt32Array{ 12, 4, 13, 0 });
		faces.append(PackedInt32Array{ 14, 6, 15, 2 });
		faces.append(PackedInt32Array{ 16, 11, 18, 7 });
		faces.append(PackedInt32Array{ 17, 9, 19, 5 });
		faces.append(PackedInt32Array{ 16, 8, 17, 4 });
		faces.append(PackedInt32Array{ 18, 10, 19, 6 });
		Vector<PackedInt32Array> volumes;
		volumes.append(PackedInt32Array{ 0, 3, 1, 4, 5, 6 });
		volumes.append(PackedInt32Array{ 2, 7, 1, 8, 9, 10 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces, volumes });
		CHECK(mesh->is_poly_mesh_data_valid());
		const PackedInt32Array simplex_indices = mesh->get_simplex_cell_vertex_indices();
		CHECK_MESSAGE(simplex_indices.size() == 20 * 3, "Only the 10 outer faces must decompose, into 2 simplexes each.");
		for (int32_t simplex_index = 0; simplex_index < (int32_t)(simplex_indices.size() / 3); simplex_index++) {
			CHECK_MESSAGE(mesh->get_source_poly_cell_for_simplex_cell(simplex_index) != 1, "The interior boundary cell shared by both volumetric cells must generate no simplexes.");
		}
	}
}
} // namespace TestPolyMeshND
