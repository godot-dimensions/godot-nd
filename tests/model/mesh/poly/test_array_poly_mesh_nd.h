#pragma once

#include "../../../../math/math_nd.h"
#include "../../../../math/transform_nd.h"
#include "../../../../math/vector_nd.h"
#include "../../../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../../../model/mesh/poly/array_poly_mesh_nd.h"
#include "test_poly_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestArrayPolyMeshND {
// Two tetrahedral cells sharing face 3 (verts 1-2-3), both flat in the w=0 hyperplane.
// Edges: 0:(0,1) 1:(0,2) 2:(0,3) 3:(1,2) 4:(1,3) 5:(2,3) 6:(1,4) 7:(2,4) 8:(3,4)
// Faces: 0: 0-1-2, 1: 0-1-3, 2: 0-2-3, 3: 1-2-3 (shared), 4: 1-2-4, 5: 1-3-4, 6: 2-3-4.
inline Ref<ArrayPolyMeshND> make_two_tetrahedra_cells_mesh() {
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0, 0.0 });
	mesh->append_vertex(VectorN{ 1.0, 0.0, 0.0, 0.0 });
	mesh->append_vertex(VectorN{ 0.0, 1.0, 0.0, 0.0 });
	mesh->append_vertex(VectorN{ 0.0, 0.0, 1.0, 0.0 });
	mesh->append_vertex(VectorN{ 1.0, 1.0, 1.0, 0.0 });
	mesh->append_edge_indices(0, 1);
	mesh->append_edge_indices(0, 2);
	mesh->append_edge_indices(0, 3);
	mesh->append_edge_indices(1, 2);
	mesh->append_edge_indices(1, 3);
	mesh->append_edge_indices(2, 3);
	mesh->append_edge_indices(1, 4);
	mesh->append_edge_indices(2, 4);
	mesh->append_edge_indices(3, 4);
	mesh->append_poly_cell(2, PackedInt32Array{ 0, 3, 1 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 0, 4, 2 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 1, 5, 2 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 3, 5, 4 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 3, 7, 6 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 4, 8, 6 }, false);
	mesh->append_poly_cell(2, PackedInt32Array{ 5, 8, 7 }, false);
	mesh->append_poly_cell(3, PackedInt32Array{ 0, 1, 2, 3 }, false);
	mesh->append_poly_cell(3, PackedInt32Array{ 3, 4, 5, 6 }, false);
	return mesh;
}

TEST_CASE("[ArrayPolyMeshND] Append vertices, edges, and poly cells") {
	SUBCASE("Appending vertices and edges deduplicates by default") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		CHECK(mesh->append_vertex(VectorN{ 1.0, 2.0, 3.0, 4.0 }) == 0);
		CHECK(mesh->append_vertex(VectorN{ 5.0, 6.0, 7.0, 8.0 }) == 1);
		CHECK_MESSAGE(mesh->append_vertex(VectorN{ 1.0, 2.0, 3.0, 4.0 }) == 0, "Appending a duplicate vertex should return the existing index.");
		CHECK(mesh->get_poly_cell_vertex_positions().size() == 2);
		CHECK_MESSAGE(mesh->append_vertex(VectorN{ 1.0, 2.0, 3.0, 4.0 }, false) == 2, "Appending without deduplication should append a new vertex.");
		CHECK(mesh->append_edge_indices(1, 0) == 0);
		CHECK_MESSAGE((mesh->get_edge_indices() == PackedInt32Array{ 0, 1 }), "Edges should be stored with sorted vertex indices.");
		CHECK_MESSAGE(mesh->append_edge_indices(0, 1) == 0, "The same edge in either vertex order should deduplicate.");
		CHECK_MESSAGE(mesh->append_edge_indices(0, 1, false) == 1, "Appending without deduplication should append a new edge.");
	}

	SUBCASE("Appending poly cells validates and deduplicates") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		ERR_PRINT_OFF;
		CHECK(mesh->append_poly_cell(1, PackedInt32Array{ 0, 1 }) == -1);
		CHECK(mesh->append_poly_cell(5, PackedInt32Array{ 0, 0, 0, 0, 0, 0 }) == -1);
		CHECK(mesh->append_poly_cell(2, PackedInt32Array{ 0, 99, 1 }) == -1);
		ERR_PRINT_ON;
		CHECK_MESSAGE(mesh->append_poly_cell(2, PackedInt32Array{ 1, 0, 3 }) == 0, "A face with the same edges in a different order should deduplicate.");
		CHECK_MESSAGE(mesh->append_poly_cell(3, PackedInt32Array{ 3, 2, 1, 0 }) == 0, "A cell with the same faces in a different order should deduplicate.");
		CHECK_MESSAGE(mesh->append_poly_cell(4, PackedInt32Array{ 0, 0, 0, 0, 0 }, false) == 0, "The first cell of a new dimension should be at index 0.");
		CHECK(mesh->get_poly_cell_indices().size() == 3);
	}
}

TEST_CASE("[ArrayPolyMeshND] Delete poly elements") {
	SUBCASE("Deleting a vertex cascades to edges, faces, and cells") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->delete_poly_element(0, 3);
		CHECK(mesh->get_poly_cell_vertex_positions().size() == 3);
		CHECK_MESSAGE((mesh->get_edge_indices() == PackedInt32Array{ 0, 1, 0, 2, 1, 2 }), "Edges referencing the deleted vertex should be deleted, and the rest reindexed.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		REQUIRE_MESSAGE(poly_cell_indices.size() == 1, "The cell dimension should be trimmed once it becomes empty.");
		REQUIRE(poly_cell_indices[0].size() == 1);
		CHECK_MESSAGE((poly_cell_indices[0][0] == PackedInt32Array{ 0, 2, 1 }), "The surviving face should have its edge references reindexed.");
		CHECK(mesh->is_poly_mesh_data_valid());
	}

	SUBCASE("Deleting a boundary cell from a box updates normals") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		const Vector<VectorN> original_normals = mesh->get_poly_cell_boundary_normals();
		mesh->delete_poly_element(3, 0);
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		REQUIRE_MESSAGE(poly_cell_indices.size() == 2, "The volumetric cell referencing the deleted cell should be deleted, trimming the top dimension.");
		CHECK(poly_cell_indices[0].size() == 24);
		CHECK(poly_cell_indices[1].size() == 7);
		const Vector<VectorN> adjusted_normals = mesh->get_poly_cell_boundary_normals();
		REQUIRE_MESSAGE(adjusted_normals.size() == 7, "Boundary normals should shrink with the deleted cell.");
		for (int64_t cell_index = 0; cell_index < 7; cell_index++) {
			CHECK(VectorND::is_equal_approx(adjusted_normals[cell_index], original_normals[cell_index + 1]));
		}
		CHECK(mesh->is_poly_mesh_data_valid());
	}
}

TEST_CASE("[ArrayPolyMeshND] Force outward normal modes") {
	Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
	const Vector<VectorN> outward_normals = box->get_poly_cell_boundary_normals();

	// Flip the orientation of two cells so their orientation-derived normals point inward.
	Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
	Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
	Vector<PackedInt32Array> cells = poly_cell_indices[1];
	for (int64_t swap_cell = 2; swap_cell <= 5; swap_cell += 3) {
		PackedInt32Array cell = cells[swap_cell];
		const int32_t temp = cell[0];
		cell.set(0, cell[1]);
		cell.set(1, temp);
		cells.set(swap_cell, cell);
	}
	poly_cell_indices.set(1, cells);
	mesh->set_poly_cell_indices(poly_cell_indices);

	SUBCASE("Cell orientation only preserves inward normals") {
		mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> normals = mesh->get_poly_cell_boundary_normals();
		CHECK(VectorND::is_equal_approx(normals[2], VectorND::negate(outward_normals[2])));
		CHECK(VectorND::is_equal_approx(normals[5], VectorND::negate(outward_normals[5])));
		CHECK(VectorND::is_equal_approx(normals[0], outward_normals[0]));
	}

	SUBCASE("Force outward override flips normals but not cell orientation") {
		mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_FORCE_OUTWARD_OVERRIDE_CELL_ORIENTATION);
		const Vector<VectorN> normals = mesh->get_poly_cell_boundary_normals();
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(normals[cell_index], outward_normals[cell_index]), "All normals must point outward.");
		}
		mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> orientation_normals = mesh->get_poly_cell_boundary_normals();
		CHECK_MESSAGE(VectorND::is_equal_approx(orientation_normals[2], VectorND::negate(outward_normals[2])), "Override mode must not alter the cell data.");
	}

	SUBCASE("Force outward fix flips normals and fixes cell orientation") {
		mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);
		mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> orientation_normals = mesh->get_poly_cell_boundary_normals();
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(orientation_normals[cell_index], outward_normals[cell_index]), "Fix mode must repair the cell orientation.");
		}
	}

	SUBCASE("Keep existing preserves non-zero normals") {
		Ref<ArrayPolyMeshND> tetra_mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Vector<VectorN> preset_normals;
		preset_normals.push_back(VectorN{ 0.0, 0.0, 0.0, -1.0 });
		tetra_mesh->set_poly_cell_boundary_normals(preset_normals);
		tetra_mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, true);
		CHECK_MESSAGE(VectorND::is_equal_approx(tetra_mesh->get_poly_cell_boundary_normals()[0], VectorN{ 0.0, 0.0, 0.0, -1.0 }), "Existing non-zero normals must be kept.");
		tetra_mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, false);
		CHECK_MESSAGE(VectorND::is_equal_approx(tetra_mesh->get_poly_cell_boundary_normals()[0], VectorN{ 0.0, 0.0, 0.0, 1.0 }), "Without keep existing, normals must be recomputed from orientation.");
	}
}

TEST_CASE("[ArrayPolyMeshND] Flat and smooth shading normals") {
	SUBCASE("Flat shading gives every vertex instance its cell's normal") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->set_flat_shading_normals();
		const Vector<VectorN> boundary_normals = mesh->get_poly_cell_boundary_normals();
		const Vector<Vector<VectorN>> vertex_normals = mesh->get_poly_cell_dense_normals(Vector2i(mesh->get_dimension() - 1, 0));
		REQUIRE(vertex_normals.size() == 8);
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			REQUIRE(vertex_normals[cell_index].size() == 8);
			for (int64_t vertex_in_cell = 0; vertex_in_cell < 8; vertex_in_cell++) {
				CHECK(VectorND::is_equal_approx(vertex_normals[cell_index][vertex_in_cell], boundary_normals[cell_index]));
			}
		}
	}

	SUBCASE("Smooth shading on a box gives corner-diagonal normals") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->set_smooth_shading_normals();
		const Vector<VectorN> vertex_positions = mesh->get_poly_cell_vertex_positions();
		const Vector<PackedInt32Array> cell_vertex_indices = mesh->get_all_boundary_cell_vertex_indices(false);
		const Vector<Vector<VectorN>> vertex_normals = mesh->get_poly_cell_dense_normals(Vector2i(mesh->get_dimension() - 1, 0));
		REQUIRE(vertex_normals.size() == 8);
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			REQUIRE(vertex_normals[cell_index].size() == cell_vertex_indices[cell_index].size());
			for (int64_t vertex_in_cell = 0; vertex_in_cell < cell_vertex_indices[cell_index].size(); vertex_in_cell++) {
				// Each box vertex is used by cells whose normals are the signed axes matching
				// the vertex's coordinate signs, so the average is the corner diagonal.
				const VectorN expected = VectorND::normalized(vertex_positions[cell_vertex_indices[cell_index][vertex_in_cell]]);
				CHECK_MESSAGE(VectorND::is_equal_approx(vertex_normals[cell_index][vertex_in_cell], expected), "Smooth box normals must point along the corner diagonals.");
			}
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Make double sided") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->make_double_sided();
	const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
	REQUIRE(poly_cell_indices[1].size() == 2);
	CHECK_MESSAGE((poly_cell_indices[1][1] == PackedInt32Array{ 1, 0, 2, 3 }), "The flipped cell must have its first two faces swapped.");
	const Vector<VectorN> boundary_normals = mesh->get_poly_cell_boundary_normals();
	REQUIRE(boundary_normals.size() == 2);
	CHECK(VectorND::is_equal_approx(boundary_normals[1], VectorND::negate(boundary_normals[0])));
	CHECK(mesh->is_poly_mesh_data_valid());
	mesh->make_double_sided(true);
	CHECK_MESSAGE(mesh->get_poly_cell_indices()[1].size() == 2, "Doubling twice with idempotence must not add more cells.");
	mesh->make_double_sided(false);
	CHECK_MESSAGE(mesh->get_poly_cell_indices()[1].size() == 4, "Doubling without idempotence must add more cells.");
}

TEST_CASE("[ArrayPolyMeshND] Single cell from all cells") {
	SUBCASE("Single 3D cell from the faces of a tetrahedron") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		const PackedInt32Array cell = mesh->make_single_cell_from_all_cells(3);
		CHECK_MESSAGE((cell == PackedInt32Array{ 0, 1, 2, 3 }), "Faces 0 and 1 already share an edge, so the order should be unchanged.");
	}

	SUBCASE("Single 4D volume from the cells of a box") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		const PackedInt32Array volume = mesh->make_single_cell_from_all_cells(4);
		REQUIRE(volume.size() == 8);
		// The first two cells must share a common face.
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		int64_t index_in_first;
		int64_t index_in_second;
		CHECK(MathND::find_common_int32(poly_cell_indices[1][volume[0]], poly_cell_indices[1][volume[1]], index_in_first, index_in_second) != INT32_MIN);
	}
}

TEST_CASE("[ArrayPolyMeshND] Seams and islands") {
	SUBCASE("All box faces are seams at the default threshold in 4D") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->calculate_seams();
		CHECK_MESSAGE(mesh->get_seam_indices_bind().size() == 24, "Every 4D box face borders two perpendicular cells, above the default threshold.");
		CHECK_MESSAGE(mesh->collect_all_islands().size() == 8, "With every face a seam, each cell is its own island.");
		mesh->calculate_seams(2.0);
		CHECK_MESSAGE(mesh->get_seam_indices_bind().is_empty(), "No seams above a high threshold.");
		CHECK(mesh->collect_all_islands().size() == 1);
	}

	SUBCASE("All cube edges are seams at the default threshold in 3D") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(3);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->calculate_seams();
		CHECK_MESSAGE(mesh->get_seam_indices_bind().size() == 12, "For 3D meshes the seams are edges, and every cube edge borders two perpendicular faces.");
		CHECK_MESSAGE(mesh->collect_all_islands().size() == 6, "With every edge a seam, each face is its own island.");
	}

	SUBCASE("Coplanar cells with matching normals produce no seams") {
		Ref<ArrayPolyMeshND> mesh = make_two_tetrahedra_cells_mesh();
		Vector<VectorN> normals;
		normals.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		normals.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		mesh->set_poly_cell_boundary_normals(normals);
		mesh->calculate_seams();
		CHECK(mesh->get_seam_indices_bind().is_empty());
		CHECK(mesh->collect_all_islands().size() == 1);
		// Opposite normals produce a seam on the shared face, splitting the islands.
		normals.set(1, VectorN{ 0.0, 0.0, 0.0, -1.0 });
		mesh->set_poly_cell_boundary_normals(normals);
		mesh->calculate_seams();
		CHECK((mesh->get_seam_indices_bind() == PackedInt32Array{ 3 }));
		CHECK(mesh->collect_all_islands().size() == 2);
	}
}

TEST_CASE("[ArrayPolyMeshND] Unwrap texture map") {
	SUBCASE("Tile cells mode gives each 4D box cell a half-size tile") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->unwrap_texture_map(ArrayPolyMeshND::UNWRAP_MODE_TILE_CELLS);
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0));
		REQUIRE(texture_map.size() == 8);
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			REQUIRE(texture_map[cell_index].size() == 8);
			VectorM minimum = VectorND::duplicate(texture_map[cell_index][0]);
			VectorM maximum = VectorND::duplicate(minimum);
			for (int64_t vertex_in_cell = 0; vertex_in_cell < 8; vertex_in_cell++) {
				const VectorM &texcoord = texture_map[cell_index][vertex_in_cell];
				REQUIRE(texcoord.size() == 3);
				for (int64_t axis = 0; axis < 3; axis++) {
					CHECK(texcoord[axis] >= -0.001);
					CHECK(texcoord[axis] <= 1.001);
					minimum.set(axis, MIN(minimum[axis], texcoord[axis]));
					maximum.set(axis, MAX(maximum[axis], texcoord[axis]));
				}
			}
			// 8 islands tile as a 2x2x2 grid, and an unwrapped cube fills its half-size tile.
			for (int64_t axis = 0; axis < 3; axis++) {
				CHECK_MESSAGE(Math::is_equal_approx(maximum[axis] - minimum[axis], 0.5), "Each cube cell must fill a half-size tile.");
			}
		}
	}

	SUBCASE("Each cell fills mode maps each cell to the full unit box, with optional padding") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->unwrap_texture_map(ArrayPolyMeshND::UNWRAP_MODE_EACH_CELL_FILLS, 1.0);
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0));
		REQUIRE(texture_map.size() == 8);
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			VectorM minimum = VectorND::duplicate(texture_map[cell_index][0]);
			VectorM maximum = VectorND::duplicate(minimum);
			for (int64_t vertex_in_cell = 0; vertex_in_cell < 8; vertex_in_cell++) {
				const VectorM &texcoord = texture_map[cell_index][vertex_in_cell];
				for (int64_t axis = 0; axis < 3; axis++) {
					minimum.set(axis, MIN(minimum[axis], texcoord[axis]));
					maximum.set(axis, MAX(maximum[axis], texcoord[axis]));
				}
			}
			// A padding of 1.0 means half the space is padding: 0.25 on each side.
			for (int64_t axis = 0; axis < 3; axis++) {
				CHECK(Math::is_equal_approx(minimum[axis], 0.25));
				CHECK(Math::is_equal_approx(maximum[axis], 0.75));
			}
		}
	}

	SUBCASE("Unwrapping works for 3D meshes with 2D texture space") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(3);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->unwrap_texture_map(ArrayPolyMeshND::UNWRAP_MODE_TILE_CELLS);
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0));
		REQUIRE(texture_map.size() == 6);
		for (int64_t cell_index = 0; cell_index < 6; cell_index++) {
			REQUIRE(texture_map[cell_index].size() == 4);
			for (int64_t vertex_in_cell = 0; vertex_in_cell < 4; vertex_in_cell++) {
				const VectorM &texcoord = texture_map[cell_index][vertex_in_cell];
				REQUIRE_MESSAGE(texcoord.size() == 2, "3D meshes have a 2D UV texture space.");
				for (int64_t axis = 0; axis < 2; axis++) {
					CHECK(texcoord[axis] >= -0.001);
					CHECK(texcoord[axis] <= 1.001);
				}
			}
		}
	}

	SUBCASE("Unwrapping a single island only fills that island") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		// Clear the box's built-in texture map, so only the unwrapped island is mapped.
		mesh->set_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0), Vector<Vector<VectorM>>());
		mesh->unwrap_texture_map_island(PackedInt32Array{ 0 });
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0));
		REQUIRE(texture_map.size() == 8);
		CHECK(texture_map[0].size() == 8);
		for (int64_t cell_index = 1; cell_index < 8; cell_index++) {
			CHECK_MESSAGE(texture_map[cell_index].is_empty(), "Cells outside the island must not be mapped.");
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Transform texture map and vertices") {
	SUBCASE("Transforming the texture map offsets all coordinates") {
		Ref<BoxPolyMeshND> box = TestPolyMeshND::make_box_poly_mesh(4);
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		mesh->unwrap_texture_map(ArrayPolyMeshND::UNWRAP_MODE_TILE_CELLS);
		const Vector<Vector<VectorM>> original = mesh->get_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0));
		mesh->transform_texture_map(TransformND::from_position(VectorN{ 10.0, 20.0, 30.0 }));
		const Vector<Vector<VectorM>> transformed = mesh->get_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0));
		REQUIRE(transformed.size() == original.size());
		for (int64_t cell_index = 0; cell_index < original.size(); cell_index++) {
			for (int64_t vertex_in_cell = 0; vertex_in_cell < original[cell_index].size(); vertex_in_cell++) {
				CHECK(VectorND::is_equal_approx(transformed[cell_index][vertex_in_cell], VectorND::add(original[cell_index][vertex_in_cell], VectorN{ 10.0, 20.0, 30.0 })));
			}
		}
	}

	SUBCASE("Transforming vertices applies the transform") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->transform_vertices(TransformND::from_position(VectorN{ 1.0, 2.0, 3.0, 4.0 }));
		const Vector<VectorN> vertex_positions = mesh->get_poly_cell_vertex_positions();
		REQUIRE(vertex_positions.size() == 4);
		CHECK(VectorND::is_equal_approx(vertex_positions[0], VectorN{ 1.0, 2.0, 3.0, 4.0 }));
		CHECK(VectorND::is_equal_approx(vertex_positions[1], VectorN{ 2.0, 2.0, 3.0, 4.0 }));
		CHECK(mesh->is_poly_mesh_data_valid());
	}
}

TEST_CASE("[ArrayPolyMeshND] Merge meshes") {
	SUBCASE("Merging two tetrahedra with an offset adjusts all indices") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->merge_with(other, TransformND::from_position(VectorN{ 10.0, 0.0, 0.0, 0.0 }));
		const Vector<VectorN> vertex_positions = mesh->get_poly_cell_vertex_positions();
		REQUIRE(vertex_positions.size() == 8);
		CHECK(VectorND::is_equal_approx(vertex_positions[4], VectorN{ 10.0, 0.0, 0.0, 0.0 }));
		const PackedInt32Array edge_indices = mesh->get_edge_indices();
		REQUIRE(edge_indices.size() == 24);
		CHECK_MESSAGE(edge_indices[12] == 4, "The merged edges must reference the offset vertices.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		REQUIRE(poly_cell_indices.size() == 2);
		REQUIRE(poly_cell_indices[0].size() == 8);
		CHECK_MESSAGE((poly_cell_indices[0][4] == PackedInt32Array{ 6, 9, 7 }), "The merged faces must reference the offset edges.");
		REQUIRE(poly_cell_indices[1].size() == 2);
		CHECK_MESSAGE((poly_cell_indices[1][1] == PackedInt32Array{ 4, 5, 6, 7 }), "The merged cells must reference the offset faces.");
		CHECK(mesh->is_poly_mesh_data_valid());
		CHECK_MESSAGE(mesh->collect_all_islands().size() == 2, "Two disconnected tetrahedra form two islands.");
	}

	SUBCASE("Merging generates missing boundary normals in both directions") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->calculate_boundary_normals();
		mesh->merge_with(other, TransformND::from_position(VectorN{ 10.0, 0.0, 0.0, 0.0 }));
		Vector<VectorN> normals = mesh->get_poly_cell_boundary_normals();
		REQUIRE_MESSAGE(normals.size() == 2, "The other mesh's missing normals must be generated during the merge.");
		CHECK(VectorND::is_equal_approx(normals[0], VectorN{ 0.0, 0.0, 0.0, 1.0 }));
		CHECK(VectorND::is_equal_approx(normals[1], VectorN{ 0.0, 0.0, 0.0, 1.0 }));
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Merging two valid meshes must produce a valid mesh.");
		// The reverse direction: the original mesh has no normals but the other does.
		Ref<ArrayPolyMeshND> plain = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Ref<ArrayPolyMeshND> with_normals = TestPolyMeshND::make_tetrahedron_cell_mesh();
		with_normals->calculate_boundary_normals();
		plain->merge_with(with_normals, TransformND::from_position(VectorN{ 10.0, 0.0, 0.0, 0.0 }));
		normals = plain->get_poly_cell_boundary_normals();
		REQUIRE(normals.size() == 2);
		CHECK(VectorND::is_equal_approx(normals[0], VectorN{ 0.0, 0.0, 0.0, 1.0 }));
		CHECK(plain->is_poly_mesh_data_valid());
	}

	SUBCASE("Merging a mesh with fewer dimensions does not crash") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Ref<ArrayPolyMeshND> other;
		other.instantiate();
		other->append_vertex(VectorN{ 10.0, 0.0, 0.0, 0.0 });
		other->append_vertex(VectorN{ 11.0, 0.0, 0.0, 0.0 });
		other->append_vertex(VectorN{ 10.0, 1.0, 0.0, 0.0 });
		other->append_edge_indices(0, 1);
		other->append_edge_indices(0, 2);
		other->append_edge_indices(1, 2);
		other->append_poly_cell(2, PackedInt32Array{ 0, 2, 1 }, false);
		mesh->merge_with(other);
		CHECK(mesh->get_poly_cell_vertex_positions().size() == 7);
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		REQUIRE(poly_cell_indices.size() == 2);
		CHECK(poly_cell_indices[0].size() == 5);
		CHECK(poly_cell_indices[1].size() == 1);
		CHECK(mesh->is_poly_mesh_data_valid());
	}

	SUBCASE("Merging offsets the other mesh's seam indices") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		other->set_seam_indices_bind(PackedInt32Array{ 0, 2 });
		mesh->merge_with(other, TransformND::from_position(VectorN{ 10.0, 0.0, 0.0, 0.0 }));
		CHECK((mesh->get_seam_indices_bind() == PackedInt32Array{ 4, 6 }));
	}
}

TEST_CASE("[ArrayPolyMeshND] Deduplicate all elements") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->append_vertex(VectorN{ 0.0, 0.0, 1.0, 0.0 }, false); // Duplicate of vertex 3.
	mesh->append_edge_indices(2, 4, false); // Becomes a duplicate of edge 5 (2, 3) after vertex dedup.
	mesh->append_poly_cell(2, PackedInt32Array{ 3, 5, 4 }, false); // Duplicate of face 3.
	CHECK(mesh->get_poly_cell_vertex_positions().size() == 5);
	CHECK(mesh->get_edge_indices().size() == 14);
	CHECK(mesh->get_poly_cell_indices()[0].size() == 5);
	mesh->deduplicate_all_elements();
	CHECK(mesh->get_poly_cell_vertex_positions().size() == 4);
	CHECK(mesh->get_edge_indices().size() == 12);
	const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
	CHECK(poly_cell_indices[0].size() == 4);
	CHECK(poly_cell_indices[1].size() == 1);
	CHECK(mesh->is_poly_mesh_data_valid());
	const Vector<VectorN> normals = mesh->get_poly_cell_boundary_normals();
	REQUIRE(normals.size() == 1);
	CHECK_MESSAGE(VectorND::is_equal_approx(normals[0], VectorN{ 0.0, 0.0, 0.0, 1.0 }), "Deduplication must preserve the boundary normal direction.");
}

TEST_CASE("[ArrayPolyMeshND] Getters and setters") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	SUBCASE("Boundary normals") {
		CHECK(mesh->get_poly_cell_boundary_normals().is_empty());
		Vector<VectorN> normals;
		normals.push_back(VectorN{ 0.0, 0.0, 0.0, 1.0 });
		mesh->set_poly_cell_boundary_normals(normals);
		CHECK(mesh->get_poly_cell_boundary_normals().size() == 1);
		mesh->set_poly_cell_boundary_normals(Vector<VectorN>());
		CHECK_MESSAGE(mesh->get_poly_cell_boundary_normals().is_empty(), "Setting empty boundary normals should erase them.");
	}
	SUBCASE("Seam indices round trip sorted") {
		mesh->set_seam_indices_bind(PackedInt32Array{ 9, 3, 5 });
		CHECK((mesh->get_seam_indices_bind() == PackedInt32Array{ 3, 5, 9 }));
	}
	SUBCASE("All poly cell normals by binding key") {
		mesh->calculate_boundary_normals();
		HashMap<Vector2i, Vector<PackedInt32Array>> all_normal_indices = mesh->get_all_poly_cell_normal_indices();
		REQUIRE(all_normal_indices.has(Vector2i(3, 3)));
		// Round trip through the setters. The value pool must be copied along with the indices.
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		other->set_poly_cell_normal_values(mesh->get_poly_cell_normal_values());
		other->set_all_poly_cell_normal_indices(all_normal_indices);
		CHECK(other->get_poly_cell_boundary_normals().size() == 1);
	}
#if GODOT_HAS_TYPED_DICTIONARY
	SUBCASE("Typed dictionary round trip for all normals and texture maps") {
		mesh->calculate_boundary_normals();
		Vector<Vector<VectorM>> texture_map;
		Vector<VectorM> cell_map;
		cell_map.push_back(VectorM{ 0.0, 0.0, 0.0 });
		cell_map.push_back(VectorM{ 1.0, 0.0, 0.0 });
		cell_map.push_back(VectorM{ 0.0, 1.0, 0.0 });
		cell_map.push_back(VectorM{ 0.0, 0.0, 1.0 });
		texture_map.push_back(cell_map);
		mesh->set_poly_cell_dense_texture_map(Vector2i(mesh->get_dimension() - 1, 0), texture_map);
		const TypedDictionary<Vector2i, Array> normal_indices_dict = mesh->get_all_poly_cell_normal_indices_bind();
		const TypedDictionary<Vector2i, Array> texture_map_indices_dict = mesh->get_all_poly_cell_texture_map_indices_bind();
		CHECK_MESSAGE(normal_indices_dict.has(Vector2i(3, 3)), "The boundary normals of a 4D mesh use the geometry dimension 3 key.");
		CHECK_MESSAGE(texture_map_indices_dict.has(Vector2i(3, 0)), "The texture map of a 4D mesh uses the cell-to-vertex decomposition key.");
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		other->set_poly_cell_normal_values(mesh->get_poly_cell_normal_values());
		other->set_poly_cell_texture_map_values(mesh->get_poly_cell_texture_map_values());
		other->set_all_poly_cell_normal_indices_bind(normal_indices_dict);
		other->set_all_poly_cell_texture_map_indices_bind(texture_map_indices_dict);
		const Vector<VectorN> round_trip_normals = other->get_poly_cell_boundary_normals();
		REQUIRE(round_trip_normals.size() == 1);
		CHECK(VectorND::is_equal_approx(round_trip_normals[0], mesh->get_poly_cell_boundary_normals()[0]));
		const Vector<Vector<VectorM>> round_trip_texture_map = other->get_poly_cell_dense_texture_map(Vector2i(other->get_dimension() - 1, 0));
		REQUIRE(round_trip_texture_map.size() == 1);
		REQUIRE(round_trip_texture_map[0].size() == 4);
		CHECK(VectorND::is_equal_approx(round_trip_texture_map[0][1], VectorM{ 1.0, 0.0, 0.0 }));
	}
#endif // GODOT_HAS_TYPED_DICTIONARY

	SUBCASE("Geometry round trip") {
		Ref<ArrayPolyMeshND> copy;
		copy.instantiate();
		copy->set_poly_cell_vertex_positions(mesh->get_poly_cell_vertex_positions());
		copy->set_edge_vertex_indices(mesh->get_edge_indices());
		copy->set_poly_cell_indices(mesh->get_poly_cell_indices());
		CHECK((copy->get_edge_indices() == mesh->get_edge_indices()));
		CHECK(copy->get_poly_cell_indices().size() == 2);
		CHECK(copy->is_poly_mesh_data_valid());
	}
}

TEST_CASE("[ArrayPolyMeshND] Make double sided preserves pentagon triangulation") {
	// A flat pentagon face with no three collinear vertices and an area of 10. Making it
	// double-sided adds a flipped copy, whose triangulation must still cover the pentagon.
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertex_positions = {
		VectorN{ 0.0, 0.0, 0.0 },
		VectorN{ 2.0, 0.0, 0.0 },
		VectorN{ 3.0, 2.0, 0.0 },
		VectorN{ 1.0, 4.0, 0.0 },
		VectorN{ -1.0, 2.0, 0.0 },
	};
	mesh->set_poly_cell_vertex_positions(vertex_positions);
	mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 4, 0, 4 });
	Vector<PackedInt32Array> faces;
	faces.append(PackedInt32Array{ 0, 1, 2, 3, 4 });
	mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
	mesh->set_poly_cell_boundary_normals(Vector<VectorN>{ VectorN{ 0.0, 0.0, 1.0 } });
	Vector<VectorN> vertex_normals;
	Vector<VectorM> texture_map;
	for (int64_t i = 0; i < vertex_positions.size(); i++) {
		vertex_normals.append(VectorN{ double(i + 1) });
		texture_map.append(VectorM{ vertex_positions[i][0], vertex_positions[i][1] });
	}
	TestMeshDataND::set_poly_normals(mesh, { vertex_normals });
	TestMeshDataND::set_poly_texture_map(mesh, { texture_map });
	REQUIRE(mesh->is_mesh_data_valid());
	mesh->make_double_sided(true);
	CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The double-sided mesh must have valid poly mesh data.");
	REQUIRE(mesh->get_poly_cell_indices()[0].size() == 2);
	const PackedInt32Array simplex_indices = mesh->get_simplex_cell_vertex_indices();
	const Vector<VectorN> simplex_vertex_positions = mesh->get_vertex_positions();
	const Vector<VectorN> simplex_boundary_normals = mesh->get_simplex_cell_boundary_normals();
	const Vector<VectorN> simplex_vertex_normals = TestMeshDataND::get_simplex_normals(mesh);
	const Vector<VectorM> simplex_texture_map = TestMeshDataND::get_simplex_texture_map(mesh);
	REQUIRE(simplex_boundary_normals.size() * 3 == simplex_indices.size());
	REQUIRE(simplex_vertex_normals.size() == simplex_indices.size());
	REQUIRE(simplex_texture_map.size() == simplex_indices.size());
	for (int64_t i = 0; i < simplex_indices.size(); i++) {
		const int32_t vertex = simplex_indices[i];
		REQUIRE(vertex >= 0);
		REQUIRE(vertex < vertex_positions.size());
		const bool flipped = simplex_boundary_normals[i / 3][2] < 0.0;
		const VectorN expected_normal = flipped ? VectorND::negate(vertex_normals[vertex]) : vertex_normals[vertex];
		CHECK(VectorND::is_equal_exact(simplex_vertex_normals[i], expected_normal));
		CHECK(simplex_texture_map[i] == texture_map[vertex]);
	}
	double total_area = 0.0;
	for (int64_t simplex_start = 0; simplex_start < simplex_indices.size(); simplex_start += 3) {
		Vector<VectorN> edges = {
			VectorND::subtract(simplex_vertex_positions[simplex_indices[simplex_start + 1]], simplex_vertex_positions[simplex_indices[simplex_start]]),
			VectorND::subtract(simplex_vertex_positions[simplex_indices[simplex_start + 2]], simplex_vertex_positions[simplex_indices[simplex_start]]),
		};
		total_area += VectorND::length(VectorND::perpendicular(edges)) / 2.0;
	}
	CHECK_MESSAGE(total_area == doctest::Approx(20.0), "Both sides of the pentagon must be triangulated with the full polygon area.");
}

TEST_CASE("[ArrayPolyMeshND] Duplicate preserves the normals and texture map dictionaries") {
	// A flat 4-dimensional mesh stores its normals under the per-face key, which is not
	// covered by the high-level boundary properties, so it only survives duplication if
	// the dictionaries are bound as properties on all Godot versions.
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertex_positions = {
		VectorN{ 0.0, 0.0, 0.0, 0.0 },
		VectorN{ 1.0, 0.0, 0.0, 0.0 },
		VectorN{ 0.0, 1.0, 0.0, 0.0 },
	};
	mesh->set_poly_cell_vertex_positions(vertex_positions);
	mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 0, 2 });
	Vector<PackedInt32Array> faces;
	faces.append(PackedInt32Array{ 0, 1, 2 });
	mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
	const VectorN pos_z = VectorN{ 0.0, 0.0, 1.0, 0.0 };
	const Vector2i per_face_key = Vector2i(2, 2);
	const Vector2i face_to_vert_key = Vector2i(2, 0);
	mesh->set_poly_cell_dense_normals(per_face_key, Vector<Vector<VectorN>>{ Vector<VectorN>{ pos_z } });
	Vector<VectorM> face_texture_map = { VectorM{ 0.0, 0.0, 0.0 }, VectorM{ 1.0, 0.0, 0.0 }, VectorM{ 0.0, 1.0, 0.0 } };
	mesh->set_poly_cell_dense_texture_map(face_to_vert_key, Vector<Vector<VectorM>>{ face_texture_map });
	Ref<ArrayPolyMeshND> duplicated = mesh->duplicate();
	REQUIRE(duplicated.is_valid());
	const Vector<Vector<VectorN>> duplicated_normals = duplicated->get_poly_cell_dense_normals(per_face_key);
	REQUIRE_MESSAGE(!duplicated_normals.is_empty(), "Duplicating a mesh must preserve the per-face normals.");
	REQUIRE(duplicated_normals.size() == 1);
	REQUIRE(duplicated_normals[0].size() == 1);
	CHECK(VectorND::is_equal_approx(duplicated_normals[0][0], pos_z));
	const Vector<Vector<VectorM>> duplicated_texture_maps = duplicated->get_poly_cell_dense_texture_map(face_to_vert_key);
	REQUIRE_MESSAGE(!duplicated_texture_maps.is_empty(), "Duplicating a mesh must preserve the face texture maps.");
	REQUIRE(duplicated_texture_maps.size() == 1);
	CHECK(duplicated_texture_maps[0].size() == 3);
}

TEST_CASE("[ArrayPolyMeshND] Indexed attributes validate every binding") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 0, 0, 0, 1 } });
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 1, 2, 3 } });
	for (const int32_t invalid_index : PackedInt32Array{ -1, 1 }) {
		for (const Vector2i key : { Vector2i(3, 3), Vector2i(3, 0), Vector2i(2, 2) }) {
			HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
			bindings.insert(key, Vector<PackedInt32Array>{ PackedInt32Array{ invalid_index } });
			mesh->set_all_poly_cell_normal_indices(bindings);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_poly_mesh_data_valid());
			ERR_PRINT_ON;
			mesh->set_all_poly_cell_normal_indices(HashMap<Vector2i, Vector<PackedInt32Array>>());
			mesh->set_all_poly_cell_texture_map_indices(bindings);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_poly_mesh_data_valid());
			ERR_PRINT_ON;
			mesh->set_all_poly_cell_texture_map_indices(HashMap<Vector2i, Vector<PackedInt32Array>>());
			CHECK(mesh->is_poly_mesh_data_valid());
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Pool compaction preserves other bindings") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	const VectorN normal_a = VectorN{ 1, 0, 0, 0 };
	const VectorN normal_b = VectorN{ 0, 0, 0, 1 };
	const VectorM texture_a = VectorM{ 1, 2, 3 };
	const VectorM texture_b = VectorM{ 4, 5, 6 };
	const Vector2i vertex_key(3, 0);
	const Vector2i boundary_key(3, 3);
	mesh->set_poly_cell_dense_normals(vertex_key, Vector<Vector<VectorN>>{ Vector<VectorN>{ normal_a, normal_b, normal_a, normal_b } });
	mesh->set_poly_cell_dense_normals(boundary_key, Vector<Vector<VectorN>>{ Vector<VectorN>{ normal_b } });
	mesh->set_poly_cell_dense_texture_map(vertex_key, Vector<Vector<VectorM>>{ Vector<VectorM>{ texture_a, texture_b, texture_a, texture_b } });
	mesh->set_poly_cell_dense_texture_map(boundary_key, Vector<Vector<VectorM>>{ Vector<VectorM>{ texture_b } });
	CHECK(mesh->get_poly_cell_normal_values().size() == 2);
	CHECK(mesh->get_poly_cell_texture_map_values().size() == 2);
	// Remove one binding. The now-unreferenced values stay in the pools until compaction,
	// which must remap the surviving binding when pool slot zero is removed.
	mesh->set_poly_cell_dense_normals(vertex_key, Vector<Vector<VectorN>>());
	mesh->set_poly_cell_dense_texture_map(vertex_key, Vector<Vector<VectorM>>());
	CHECK(mesh->get_poly_cell_normal_values().size() == 2);
	CHECK(mesh->get_poly_cell_texture_map_values().size() == 2);
	mesh->compact_normal_values();
	mesh->compact_texture_map_values();
	REQUIRE(mesh->get_poly_cell_normal_values().size() == 1);
	REQUIRE(mesh->get_poly_cell_texture_map_values().size() == 1);
	CHECK(mesh->get_poly_cell_normal_values()[0] == normal_b);
	CHECK(mesh->get_poly_cell_texture_map_values()[0] == texture_b);
	CHECK(mesh->get_all_poly_cell_normal_indices()[boundary_key][0][0] == 0);
	CHECK(mesh->get_all_poly_cell_texture_map_indices()[boundary_key][0][0] == 0);
	CHECK(mesh->get_poly_cell_boundary_normals()[0] == normal_b);
	mesh->set_poly_cell_dense_normals(boundary_key, Vector<Vector<VectorN>>());
	mesh->set_poly_cell_dense_texture_map(boundary_key, Vector<Vector<VectorM>>());
	mesh->compact_normal_values();
	mesh->compact_texture_map_values();
	CHECK(mesh->get_poly_cell_normal_values().is_empty());
	CHECK(mesh->get_poly_cell_texture_map_values().is_empty());
}

TEST_CASE("[ArrayPolyMeshND] Conversion and cache updates preserve indexed attributes") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	const VectorN normal_a = VectorN{ 1, 0, 0, 0 };
	const VectorN normal_b = VectorN{ 0, 1, 0, 0 };
	const VectorM texture_a = VectorM{ 1, 2, 3 };
	const VectorM texture_b = VectorM{ 4, 5, 6 };
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ normal_a, normal_b });
	mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 1, 0, 1, 0 } });
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ texture_a, texture_b });
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 1, 0, 1 } });
	mesh->set_poly_cell_boundary_normals(Vector<VectorN>{ VectorN{ 0, 0, 0, 1 } });
	Ref<ArrayPolyMeshND> converted = mesh->to_array_poly_mesh();
	CHECK(converted->get_poly_cell_dense_normals(Vector2i(3, 0)) == mesh->get_poly_cell_dense_normals(Vector2i(3, 0)));
	CHECK(converted->get_poly_cell_dense_texture_map(Vector2i(3, 0)) == mesh->get_poly_cell_dense_texture_map(Vector2i(3, 0)));
	CHECK(converted->get_poly_cell_boundary_normals() == mesh->get_poly_cell_boundary_normals());
	// Prime both caches, then change values while retaining their indices.
	REQUIRE(mesh->get_simplex_cell_normal_indices().size() == 4);
	REQUIRE(mesh->get_simplex_cell_texture_map_indices().size() == 4);
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ normal_b, normal_a, VectorN{ 0, 0, 0, 1 } });
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ texture_b, texture_a });
	const PackedInt32Array normals = mesh->get_simplex_cell_normal_indices();
	const PackedInt32Array textures = mesh->get_simplex_cell_texture_map_indices();
	CHECK(mesh->get_normal_values()[normals[0]] == normal_a);
	CHECK(mesh->get_texture_map_values()[textures[0]] == texture_b);
	mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>());
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>());
	CHECK(mesh->get_simplex_cell_normal_indices().is_empty());
	CHECK(mesh->get_simplex_cell_texture_map_indices().is_empty());
	CHECK(mesh->get_normal_values().is_empty());
	CHECK(mesh->get_texture_map_values().is_empty());
}

TEST_CASE("[ArrayPolyMeshND] Partial indexed attributes preserve mapped cells") {
	Ref<ArrayPolyMeshND> mesh = make_two_tetrahedra_cells_mesh();
	const VectorN normal = VectorN{ 0, 0, 0, 1 };
	const VectorM texture = VectorM{ 1, 2, 3 };
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ normal });
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ texture });
	const Vector<PackedInt32Array> indices = { PackedInt32Array{ 0, 0, 0, 0 }, PackedInt32Array() };
	mesh->set_poly_cell_normal_indices(indices);
	mesh->set_poly_cell_texture_map_indices(indices);
	REQUIRE(mesh->is_poly_mesh_data_valid());
	ERR_PRINT_OFF; // Missing attributes on the second cell intentionally produce warnings.
	const PackedInt32Array normals = mesh->get_simplex_cell_normal_indices();
	const PackedInt32Array textures = mesh->get_simplex_cell_texture_map_indices();
	ERR_PRINT_ON;
	REQUIRE(normals.size() == 8);
	REQUIRE(textures.size() == 8);
	CHECK(mesh->to_array_cell_mesh()->is_mesh_data_valid());
	for (int i = 0; i < 8; i++) {
		const bool mapped = mesh->get_source_poly_cell_for_simplex_cell(i / 4) == 0;
		CHECK(mesh->get_normal_values()[normals[i]] == (mapped ? normal : VectorN()));
		CHECK(mesh->get_texture_map_values()[textures[i]] == (mapped ? texture : VectorM()));
	}
	mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array(), PackedInt32Array() });
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array(), PackedInt32Array() });
	CHECK(mesh->get_simplex_cell_normal_indices().is_empty());
	CHECK(mesh->get_simplex_cell_texture_map_indices().is_empty());
}

TEST_CASE("[ArrayPolyMeshND] Double sided meshes reuse texture values and negate normal values") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	const VectorN normal = VectorN{ 0, 0, 0, 1 };
	const Vector<VectorM> texture_values = { VectorM{ 1, 2, 3 }, VectorM{ 4, 5, 6 } };
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ normal });
	mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
	mesh->set_poly_cell_boundary_normals(Vector<VectorN>{ normal });
	mesh->set_poly_cell_texture_map_values(texture_values);
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 1, 0, 1, 0 } });
	mesh->make_double_sided();
	REQUIRE(mesh->is_poly_mesh_data_valid());
	REQUIRE(mesh->get_poly_cell_normal_indices().size() == 2);
	CHECK(mesh->get_poly_cell_normal_values().size() == 2);
	CHECK(mesh->get_poly_cell_texture_map_values() == texture_values);
	// The flipped cell reuses the original texture values, with each value kept on its
	// vertex, which can be a different position in the flipped cell's derived vertex order.
	const Vector<PackedInt32Array> cell_vertices = mesh->get_all_poly_cell_vertex_indices(3, false);
	const Vector<PackedInt32Array> texture_map_indices = mesh->get_poly_cell_texture_map_indices();
	REQUIRE(cell_vertices.size() == 2);
	for (int64_t vertex_in_cell = 0; vertex_in_cell < cell_vertices[1].size(); vertex_in_cell++) {
		const int64_t original_position = cell_vertices[0].find(cell_vertices[1][vertex_in_cell]);
		REQUIRE(original_position >= 0);
		CHECK(texture_map_indices[1][vertex_in_cell] == texture_map_indices[0][original_position]);
	}
	const Vector<VectorN> values = mesh->get_poly_cell_normal_values();
	const Vector<PackedInt32Array> normal_indices = mesh->get_poly_cell_normal_indices();
	for (const int32_t index : normal_indices[1]) {
		CHECK(VectorND::is_equal_exact(values[index], VectorND::negate(normal)));
	}
	mesh->make_double_sided();
	CHECK(mesh->get_poly_cell_normal_indices().size() == 2);
	CHECK(mesh->get_poly_cell_normal_values().size() == 2);
	CHECK(mesh->get_poly_cell_texture_map_values() == texture_values);
}

TEST_CASE("[ArrayPolyMeshND] Merge remaps shared pools and preserves absent attributes") {
	for (const bool destination_has_attributes : { false, true }) {
		for (const bool source_has_attributes : { false, true }) {
			CAPTURE(destination_has_attributes);
			CAPTURE(source_has_attributes);
			Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
			Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
			const VectorN normal = VectorN{ 0, 0, 0, 1 };
			const VectorM texture_a = VectorM{ 1, 2, 3 };
			const VectorM texture_b = VectorM{ 4, 5, 6 };
			if (destination_has_attributes) {
				mesh->set_poly_cell_normal_values(Vector<VectorN>{ normal });
				mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
				mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ texture_a, texture_b });
				mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 1, 0, 1 } });
			}
			if (source_has_attributes) {
				// The same texture coordinates occupy opposite slots in the source pool.
				other->set_poly_cell_normal_values(Vector<VectorN>{ normal });
				other->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
				other->set_poly_cell_texture_map_values(Vector<VectorM>{ texture_b, texture_a });
				other->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 1, 0, 1, 0 } });
			}
			mesh->merge_with(other, TransformND::from_scale(VectorN{ -1, -1, -1, -1 }));
			REQUIRE(mesh->is_mesh_data_valid());
			if (!destination_has_attributes && !source_has_attributes) {
				CHECK(mesh->get_poly_cell_normal_indices().is_empty());
				CHECK(mesh->get_poly_cell_texture_map_indices().is_empty());
				continue;
			}
			const Vector<Vector<VectorN>> normals = mesh->get_poly_cell_dense_normals(Vector2i(3, 0));
			const Vector<Vector<VectorM>> textures = mesh->get_poly_cell_dense_texture_map(Vector2i(3, 0));
			REQUIRE(normals.size() == 2);
			REQUIRE(textures.size() == 2);
			for (int cell = 0; cell < 2; cell++) {
				const bool has_attributes = cell == 0 ? destination_has_attributes : source_has_attributes;
				if (!has_attributes) {
					CHECK(textures[cell].is_empty());
					continue; // Missing shading normals can be generated from the cell orientation.
				}
				REQUIRE(normals[cell].size() == 4);
				REQUIRE(textures[cell].size() == 4);
				for (int i = 0; i < 4; i++) {
					CHECK(VectorND::is_equal_exact(normals[cell][i], cell == 0 ? normal : VectorND::negate(normal)));
					CHECK(textures[cell][i] == (i % 2 == 0 ? texture_a : texture_b));
				}
			}
			CHECK(other->get_poly_cell_normal_values().size() == (source_has_attributes ? 1 : 0));
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Deduplication preserves empty bindings when flipping cell orientation") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->set_poly_cell_boundary_normals(Vector<VectorN>{ VectorN({ 0, 0, 0, -1 }) });
	// Per-face data follows the cell's member order, which will change to match the custom normal.
	const Vector2i key(3, 2);
	HashMap<Vector2i, Vector<PackedInt32Array>> normals = mesh->get_all_poly_cell_normal_indices();
	normals.insert(key, Vector<PackedInt32Array>{ PackedInt32Array() });
	mesh->set_all_poly_cell_normal_indices(normals);
	HashMap<Vector2i, Vector<PackedInt32Array>> textures;
	textures.insert(key, Vector<PackedInt32Array>{ PackedInt32Array() });
	mesh->set_all_poly_cell_texture_map_indices(textures);
	mesh->deduplicate_all_elements();
	CHECK(mesh->is_poly_mesh_data_valid());
	CHECK(mesh->get_all_poly_cell_normal_indices()[key][0].is_empty());
	CHECK(mesh->get_all_poly_cell_texture_map_indices()[key][0].is_empty());
	CHECK(VectorND::is_equal_exact(mesh->get_poly_cell_boundary_normals()[0], VectorN({ 0, 0, 0, -1 })));
}

TEST_CASE("[ArrayPolyMeshND] Texture transforms isolate values shared across bindings") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	const VectorM original = VectorM({ 1, 2, 3 });
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ original });
	HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
	bindings.insert(Vector2i(3, 0), Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
	bindings.insert(Vector2i(3, 3), Vector<PackedInt32Array>{ PackedInt32Array{ 0 } });
	mesh->set_all_poly_cell_texture_map_indices(bindings);
	mesh->transform_texture_map(TransformND::from_position(VectorN{ 1, 0, 0 }));
	CHECK(mesh->get_poly_cell_texture_map_values().size() == 2);
	CHECK(mesh->get_poly_cell_dense_texture_map(Vector2i(3, 3))[0][0] == original);
	const Vector<VectorM> transformed = mesh->get_poly_cell_dense_texture_map(Vector2i(3, 0))[0];
	REQUIRE(transformed.size() == 4);
	for (const VectorM &value : transformed) {
		CHECK(value == VectorM({ 2, 2, 3 }));
	}
}

TEST_CASE("[ArrayPolyMeshND] Self merge snapshots indexed attributes") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	const VectorN normal = VectorN({ 0, 0, 0, 1 });
	const VectorM texture = VectorM({ 1, 2, 3 });
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ normal });
	mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ texture });
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
	mesh->merge_with(mesh, TransformND::from_scale(VectorN{ 2, 2, 2, 2 }));
	REQUIRE(mesh->is_mesh_data_valid());
	CHECK(mesh->get_poly_cell_vertex_positions().size() == 8);
	const Vector<Vector<VectorN>> normals = mesh->get_poly_cell_dense_normals(Vector2i(3, 0));
	const Vector<Vector<VectorM>> textures = mesh->get_poly_cell_dense_texture_map(Vector2i(3, 0));
	REQUIRE(normals.size() == 2);
	REQUIRE(textures.size() == 2);
	for (int cell = 0; cell < 2; cell++) {
		REQUIRE(normals[cell].size() == 4);
		REQUIRE(textures[cell].size() == 4);
		for (int vertex = 0; vertex < 4; vertex++) {
			CHECK(normals[cell][vertex] == VectorND::multiply_scalar(normal, cell + 1));
			CHECK(textures[cell][vertex] == texture);
		}
	}
	CHECK(mesh->get_poly_cell_texture_map_values().size() == 1);
}

TEST_CASE("[ArrayPolyMeshND] Merge preserves absent pivot overrides") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 2 });
	// The other mesh's cell explicitly has no pivot override, which must stay -1 instead of
	// being offset by the merged vertex count into a valid-looking but bogus vertex index.
	other->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ -1 });
	mesh->merge_with(other, TransformND::from_position(VectorN{ 10, 0, 0, 0 }));
	REQUIRE(mesh->is_mesh_data_valid());
	CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array({ 2, -1 }));
	// Real overrides still need the vertex offset, and omitted overrides stay absent.
	other->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 1 });
	mesh->merge_with(other, TransformND::identity_transform(4));
	CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array({ 2, -1, 9 }));
	other->set_poly_cell_boundary_pivot_overrides(PackedInt32Array());
	mesh->merge_with(other, TransformND::identity_transform(4));
	CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array({ 2, -1, 9, -1 }));
	CHECK(mesh->is_mesh_data_valid());
}

TEST_CASE("[ArrayPolyMeshND] Double sided meshes keep per-vertex attributes on their vertices") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	const Vector<VectorN> normal_values = { VectorN{ 1, 0, 0, 0 }, VectorN{ 0, 1, 0, 0 }, VectorN{ 0, 0, 1, 0 }, VectorN{ 0, 0, 0, 1 } };
	const Vector<VectorM> texture_values = { VectorM{ 0, 0, 0 }, VectorM{ 1, 0, 0 }, VectorM{ 0, 1, 0 }, VectorM{ 0, 0, 1 } };
	mesh->set_poly_cell_normal_values(normal_values);
	mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 1, 2, 3 } });
	mesh->set_poly_cell_texture_map_values(texture_values);
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 1, 2, 3 } });
	mesh->set_poly_cell_boundary_normals(Vector<VectorN>{ VectorN{ 0, 0, 0, 1 } });
	mesh->make_double_sided();
	REQUIRE(mesh->is_mesh_data_valid());
	const Vector<PackedInt32Array> cell_vertices = mesh->get_all_poly_cell_vertex_indices(3, false);
	const Vector<Vector<VectorN>> normals = mesh->get_poly_cell_dense_normals(Vector2i(3, 0));
	const Vector<Vector<VectorM>> textures = mesh->get_poly_cell_dense_texture_map(Vector2i(3, 0));
	REQUIRE(cell_vertices.size() == 2);
	REQUIRE(normals.size() == 2);
	REQUIRE(textures.size() == 2);
	// Flipping the cell changes its derived vertex traversal order, which this test relies
	// on to check that the per-vertex data was re-aligned rather than copied positionally.
	CHECK(cell_vertices[0] != cell_vertices[1]);
	for (int64_t vertex_in_cell = 0; vertex_in_cell < cell_vertices[1].size(); vertex_in_cell++) {
		const int64_t original_position = cell_vertices[0].find(cell_vertices[1][vertex_in_cell]);
		REQUIRE(original_position >= 0);
		CHECK(VectorND::is_equal_exact(normals[1][vertex_in_cell], VectorND::negate(normals[0][original_position])));
		CHECK(textures[1][vertex_in_cell] == textures[0][original_position]);
	}
}

TEST_CASE("[ArrayPolyMeshND] Validation rejects malformed binding shapes") {
	{
		// An empty binding means no data for the key, which is valid and must not crash deletion.
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
		bindings.insert(Vector2i(3, 3), Vector<PackedInt32Array>());
		mesh->set_all_poly_cell_normal_indices(bindings);
		CHECK(mesh->is_mesh_data_valid());
		mesh->delete_poly_element(3, 0);
	}
	{
		// A flat binding's array must have one entry per element of the geometry dimension.
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 0, 0, 0, 1 } });
		HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
		bindings.insert(Vector2i(3, 3), Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0 } });
		mesh->set_all_poly_cell_normal_indices(bindings);
		ERR_PRINT_OFF;
		CHECK_FALSE(mesh->is_mesh_data_valid());
		ERR_PRINT_ON;
	}
	{
		// A decomposed binding must not have more arrays than the geometry dimension has elements.
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 1, 2, 3 } });
		HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
		Vector<PackedInt32Array> too_many_arrays;
		too_many_arrays.resize(5); // The tetrahedron cell mesh only has 4 faces.
		bindings.insert(Vector2i(2, 0), too_many_arrays);
		mesh->set_all_poly_cell_texture_map_indices(bindings);
		ERR_PRINT_OFF;
		CHECK_FALSE(mesh->is_mesh_data_valid());
		ERR_PRINT_ON;
	}
	{
		// Pool values may have fewer dimensions than the mesh (zero-extended), but not more.
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 0, 0, 1 } });
		mesh->set_poly_cell_normal_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
		CHECK(mesh->is_mesh_data_valid());
		mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 0, 0, 0, 0, 1 } });
		ERR_PRINT_OFF;
		CHECK_FALSE(mesh->is_mesh_data_valid());
		ERR_PRINT_ON;
	}
}

TEST_CASE("[ArrayPolyMeshND] Indexed attributes on fully covered cells have no simplex data") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(dimension, 2.0));
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		// Referencing every boundary cell from two volumes leaves no exposed surface.
		const PackedInt32Array volume = mesh->get_poly_cell_indices()[dimension - 2][0];
		mesh->append_poly_cell(dimension, volume, false);
		REQUIRE(mesh->is_poly_mesh_data_valid());
		CHECK(mesh->get_simplex_cell_normal_indices().is_empty());
		CHECK(mesh->get_normal_values().is_empty());
		CHECK(mesh->get_simplex_cell_texture_map_indices().is_empty());
		CHECK(mesh->get_texture_map_values().is_empty());
	}
}

TEST_CASE("[ArrayPolyMeshND] Simplex texture map can be requested before decomposition") {
	// Requesting the simplex texture map indices as the very first derived data
	// on a fresh mesh must trigger simplex decomposition instead of returning empty.
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 1, 2, 3 } });
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
	const PackedInt32Array texture_map_indices = mesh->get_simplex_cell_texture_map_indices();
	CHECK(texture_map_indices.size() == mesh->get_simplex_cell_vertex_indices().size());
	CHECK_FALSE(mesh->get_texture_map_values().is_empty());
}

TEST_CASE("[ArrayPolyMeshND] Double sided meshes preserve empty indexed bindings") {
	for (const bool empty_outer : { false, true }) {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Vector<PackedInt32Array> binding;
		if (!empty_outer) {
			binding.append(PackedInt32Array());
		}
		HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
		bindings.insert(Vector2i(3, 0), binding);
		mesh->set_all_poly_cell_normal_indices(bindings);
		mesh->set_all_poly_cell_texture_map_indices(bindings);
		REQUIRE(mesh->is_mesh_data_valid());
		mesh->make_double_sided();
		REQUIRE(mesh->get_poly_cell_indices()[1].size() == 2);
		REQUIRE(mesh->is_mesh_data_valid());
		const Vector<PackedInt32Array> normal_indices = mesh->get_poly_cell_normal_indices();
		const Vector<PackedInt32Array> texture_indices = mesh->get_poly_cell_texture_map_indices();
		REQUIRE(normal_indices.size() == (empty_outer ? 0 : 2));
		REQUIRE(texture_indices.size() == normal_indices.size());
		for (int64_t cell_index = 0; cell_index < normal_indices.size(); cell_index++) {
			CHECK(normal_indices[cell_index].is_empty());
			CHECK(texture_indices[cell_index].is_empty());
		}
		mesh->make_double_sided();
		CHECK(mesh->get_poly_cell_indices()[1].size() == 2);
	}
}

TEST_CASE("[ArrayPolyMeshND] Compaction after flat shading removes replaced values and preserves other bindings") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 1, 0, 0, 0 }, VectorN{ 0, 0, 0, 1 }, VectorN{ 0, 1, 0, 0 } });
	HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
	bindings.insert(Vector2i(3, 0), Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0 } });
	bindings.insert(Vector2i(3, 3), Vector<PackedInt32Array>{ PackedInt32Array{ 1 } });
	bindings.insert(Vector2i(0, 0), Vector<PackedInt32Array>{ PackedInt32Array{ 2, 2, 2, 2 } });
	mesh->set_all_poly_cell_normal_indices(bindings);
	REQUIRE(mesh->is_mesh_data_valid());
	const Vector<VectorN> boundary_normals = mesh->get_poly_cell_boundary_normals();
	const Vector<Vector<VectorN>> vertex_normals = mesh->get_poly_cell_dense_normals(Vector2i(0, 0));
	mesh->set_flat_shading_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, false);
	REQUIRE(mesh->is_mesh_data_valid());
	// The value replaced by flat shading stays in the pool until explicit compaction.
	CHECK(mesh->get_poly_cell_normal_values().size() == 3);
	mesh->compact_normal_values();
	CHECK(mesh->get_poly_cell_normal_values().size() == 2);
	CHECK(mesh->get_poly_cell_boundary_normals() == boundary_normals);
	CHECK(mesh->get_poly_cell_dense_normals(Vector2i(0, 0)) == vertex_normals);
	const Vector<Vector<VectorN>> flat_normals = mesh->get_poly_cell_dense_normals(Vector2i(3, 0));
	REQUIRE(flat_normals.size() == 1);
	REQUIRE(flat_normals[0].size() == 4);
	for (const VectorN &normal : flat_normals[0]) {
		CHECK(normal == boundary_normals[0]);
	}
}

TEST_CASE("[ArrayPolyMeshND] Double sided indexed attributes follow boundary vertex order across dimensions") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_box_poly_mesh(dimension)->to_array_poly_mesh();
		const Vector<PackedInt32Array> original_vertices = mesh->get_all_boundary_cell_vertex_indices(false);
		const int64_t cell_count = original_vertices.size();
		const int64_t vertex_count = mesh->get_poly_cell_vertex_positions().size();
		Vector<VectorN> normal_values;
		Vector<VectorM> texture_values;
		for (int64_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
			VectorN normal = VectorND::zero(dimension);
			normal.set(0, vertex_index + 1);
			normal.set(dimension - 1, 1);
			normal_values.append(VectorND::normalized(normal));
			VectorM texture = VectorND::zero(dimension - 1);
			texture.set(0, vertex_index + 1);
			texture_values.append(texture);
		}
		Vector<PackedInt32Array> indices = original_vertices;
		// Mix a missing cell with populated, asymmetric per-vertex attributes.
		indices.set(0, PackedInt32Array());
		HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
		bindings.insert(Vector2i(dimension - 1, 0), indices);
		mesh->set_poly_cell_normal_values(normal_values);
		mesh->set_poly_cell_texture_map_values(texture_values);
		mesh->set_all_poly_cell_normal_indices(bindings);
		mesh->set_all_poly_cell_texture_map_indices(bindings);
		REQUIRE(mesh->is_mesh_data_valid());
		mesh->make_double_sided();
		REQUIRE(mesh->is_mesh_data_valid());
		const Vector<PackedInt32Array> doubled_vertices = mesh->get_all_boundary_cell_vertex_indices(false);
		const Vector<Vector<VectorN>> normals = mesh->get_poly_cell_dense_normals(Vector2i(dimension - 1, 0));
		const Vector<Vector<VectorM>> textures = mesh->get_poly_cell_dense_texture_map(Vector2i(dimension - 1, 0));
		REQUIRE(doubled_vertices.size() == cell_count * 2);
		REQUIRE(normals.size() == cell_count * 2);
		REQUIRE(textures.size() == cell_count * 2);
		CHECK(normals[0].is_empty());
		CHECK(normals[cell_count].is_empty());
		CHECK(textures[0].is_empty());
		CHECK(textures[cell_count].is_empty());
		bool changed_vertex_order = false;
		for (int64_t cell_index = 1; cell_index < cell_count; cell_index++) {
			const int64_t flipped_cell_index = cell_index + cell_count;
			const PackedInt32Array &flipped_vertices = doubled_vertices[flipped_cell_index];
			changed_vertex_order |= flipped_vertices != original_vertices[cell_index];
			REQUIRE(normals[flipped_cell_index].size() == flipped_vertices.size());
			REQUIRE(textures[flipped_cell_index].size() == flipped_vertices.size());
			for (int64_t vertex_in_cell = 0; vertex_in_cell < flipped_vertices.size(); vertex_in_cell++) {
				const int32_t vertex_index = flipped_vertices[vertex_in_cell];
				CHECK(VectorND::is_equal_exact(normals[flipped_cell_index][vertex_in_cell], VectorND::negate(normal_values[vertex_index])));
				CHECK(textures[flipped_cell_index][vertex_in_cell] == texture_values[vertex_index]);
			}
		}
		CHECK(changed_vertex_order);
		mesh->make_double_sided();
		CHECK(mesh->get_all_boundary_cell_vertex_indices(false).size() == cell_count * 2);
	}
}

TEST_CASE("[ArrayPolyMeshND] Repeated merges do not accumulate unreferenced pool values") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(dimension, 2.0));
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		Ref<ArrayPolyMeshND> other = box->to_array_poly_mesh();
		const Vector<PackedInt32Array> cell_vertices = mesh->get_all_poly_cell_vertex_indices(dimension - 1, false);
		Vector<PackedInt32Array> attribute_indices;
		for (const PackedInt32Array &vertices : cell_vertices) {
			PackedInt32Array indices;
			indices.resize(vertices.size());
			indices.fill(0);
			attribute_indices.append(indices);
		}
		const VectorN normal = VectorND::value_on_axis_with_dimension(1.0, dimension - 1, dimension);
		const VectorM texture = VectorND::fill(dimension - 1, 0.25);
		for (const Ref<ArrayPolyMeshND> &part : { mesh, other }) {
			part->set_all_poly_cell_normal_indices(HashMap<Vector2i, Vector<PackedInt32Array>>());
			part->set_all_poly_cell_texture_map_indices(HashMap<Vector2i, Vector<PackedInt32Array>>());
			part->set_poly_cell_normal_values(Vector<VectorN>{ normal });
			part->set_poly_cell_texture_map_values(Vector<VectorM>{ texture });
			part->set_poly_cell_normal_indices(attribute_indices);
			part->set_poly_cell_texture_map_indices(attribute_indices);
			REQUIRE(part->is_mesh_data_valid());
		}
		for (int merge_index = 1; merge_index <= 3; merge_index++) {
			const VectorN offset = VectorND::value_on_axis_with_dimension(10.0 * merge_index, 0, dimension);
			mesh->merge_with(other, TransformND::from_position(offset));
			REQUIRE(mesh->is_mesh_data_valid());
			CHECK(mesh->get_poly_cell_normal_values() == Vector<VectorN>({ normal }));
			CHECK(mesh->get_poly_cell_texture_map_values() == Vector<VectorM>({ texture }));
			CHECK(mesh->get_poly_cell_normal_indices().size() == cell_vertices.size() * (merge_index + 1));
			CHECK(mesh->get_poly_cell_texture_map_indices().size() == cell_vertices.size() * (merge_index + 1));
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Auxiliary bindings validate sub-element shapes and keys") {
	for (const Vector2i key : { Vector2i(-1, 0), Vector2i(2, 3), Vector2i(4, 4), Vector2i(1, 0), Vector2i(2, 0), Vector2i(2, 1), Vector2i(3, 1) }) {
		for (const bool normals : { false, true }) {
			Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
			mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 0, 0, 0, 1 } });
			mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 1, 2, 3 } });
			HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
			bindings.insert(key, Vector<PackedInt32Array>{ PackedInt32Array{ 0 } });
			if (normals) {
				mesh->set_all_poly_cell_normal_indices(bindings);
			} else {
				mesh->set_all_poly_cell_texture_map_indices(bindings);
			}
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_mesh_data_valid());
			ERR_PRINT_ON;
		}
	}
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ -1, -1 });
	ERR_PRINT_OFF;
	CHECK_FALSE(mesh->is_mesh_data_valid());
	ERR_PRINT_ON;
}

TEST_CASE("[ArrayPolyMeshND] Deduplication preserves partial auxiliary bindings") {
	Ref<ArrayPolyMeshND> mesh = make_two_tetrahedra_cells_mesh();
	mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 0, 0, 0, 1 } });
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 1, 2, 3 } });
	HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
	bindings.insert(Vector2i(3, 1), Vector<PackedInt32Array>{ PackedInt32Array{ 0, 0, 0, 0, 0, 0 } });
	mesh->set_all_poly_cell_normal_indices(bindings);
	mesh->set_all_poly_cell_texture_map_indices(bindings);
	REQUIRE(mesh->is_mesh_data_valid());
	const auto normals = mesh->get_poly_cell_dense_normals(Vector2i(3, 1));
	const auto textures = mesh->get_poly_cell_dense_texture_map(Vector2i(3, 1));
	mesh->deduplicate_all_elements();
	REQUIRE(mesh->is_mesh_data_valid());
	CHECK(mesh->get_poly_cell_dense_normals(Vector2i(3, 1)) == normals);
	CHECK(mesh->get_poly_cell_dense_texture_map(Vector2i(3, 1)) == textures);
}

TEST_CASE("[ArrayPolyMeshND] Deleting geometry keeps indexed bindings aligned") {
	for (int dimension = 0; dimension <= 3; dimension++) {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 0, 0, 0, 1 } });
		mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 1, 2, 3 } });
		HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
		for (int geometry_dimension = 0; geometry_dimension <= 3; geometry_dimension++) {
			const int64_t element_count = mesh->get_all_poly_cell_poly_indices(geometry_dimension, geometry_dimension).size();
			PackedInt32Array flat_indices;
			flat_indices.resize(element_count);
			flat_indices.fill(0);
			bindings.insert(Vector2i(geometry_dimension, geometry_dimension), Vector<PackedInt32Array>{ flat_indices });
			if (geometry_dimension > 0) {
				Vector<PackedInt32Array> vertex_indices = mesh->get_all_poly_cell_vertex_indices(geometry_dimension, false);
				for (int64_t element = 0; element < vertex_indices.size(); element++) {
					vertex_indices.ptrw()[element].fill(0);
				}
				bindings.insert(Vector2i(geometry_dimension, 0), vertex_indices);
			}
		}
		mesh->set_all_poly_cell_normal_indices(bindings);
		mesh->set_all_poly_cell_texture_map_indices(bindings);
		mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 0 });
		REQUIRE(mesh->is_mesh_data_valid());
		mesh->delete_poly_element(dimension, 0);
		REQUIRE(mesh->is_mesh_data_valid());
		CHECK(mesh->get_poly_cell_boundary_pivot_overrides().is_empty());
		const auto normal_bindings = mesh->get_all_poly_cell_normal_indices();
		const auto texture_bindings = mesh->get_all_poly_cell_texture_map_indices();
		for (const auto &binding : normal_bindings) {
			CHECK(texture_bindings.has(binding.key));
			CHECK(texture_bindings[binding.key] == binding.value);
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Deleting external pivot vertices remaps overrides") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->append_vertex(VectorN{ 2, 2, 2, 2 });
	mesh->append_vertex(VectorN{ 3, 3, 3, 3 });
	mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 5 });
	REQUIRE(mesh->is_poly_mesh_data_valid());
	mesh->delete_poly_element(0, 4);
	CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array{ 4 });
	mesh->delete_poly_element(0, 4);
	CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array{ -1 });
	CHECK(mesh->is_poly_mesh_data_valid());
}

TEST_CASE("[ArrayPolyMeshND] Short indexed values remain valid after simplex conversion") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_box_poly_mesh(dimension)->to_array_poly_mesh();
		Vector<PackedInt32Array> indices = mesh->get_all_boundary_cell_vertex_indices(false);
		for (int64_t cell_index = 0; cell_index < indices.size(); cell_index++) {
			indices.ptrw()[cell_index].fill(0);
		}
		HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
		bindings.insert(Vector2i(dimension - 1, 0), indices);
		mesh->set_all_poly_cell_normal_indices(bindings);
		mesh->set_all_poly_cell_texture_map_indices(bindings);
		mesh->set_poly_cell_normal_values(Vector<VectorN>{ VectorN{ 1 } });
		mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 0.5 } });
		REQUIRE(mesh->is_mesh_data_valid());
		Ref<ArrayCellMeshND> simplex = mesh->to_array_cell_mesh();
		REQUIRE(simplex->is_mesh_data_valid());
		CHECK(simplex->get_normal_values()[0] == VectorN{ 1 });
		CHECK(simplex->get_texture_map_values()[0] == VectorM{ 0.5 });
	}
}

TEST_CASE("[ArrayPolyMeshND] Texture fitting zero-extends shorter indexed values") {
	Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
	mesh->set_poly_cell_texture_map_values(Vector<VectorM>{ VectorM{ 0 }, VectorM{ 2, 4 }, VectorM{ 0, 2, 6 }, VectorM() });
	mesh->set_poly_cell_texture_map_indices(Vector<PackedInt32Array>{ PackedInt32Array{ 0, 1, 2, 3 } });
	REQUIRE(mesh->is_mesh_data_valid());
	mesh->unwrap_texture_map(ArrayPolyMeshND::UNWRAP_MODE_EACH_CELL_FILLS, 0.0, false, true);
	REQUIRE(mesh->is_mesh_data_valid());
	const Vector<Vector<VectorM>> textures = mesh->get_poly_cell_dense_texture_map(Vector2i(3, 0));
	REQUIRE(textures.size() == 1);
	REQUIRE(textures[0].size() == 4);
	CHECK(textures[0][0] == VectorM({ 0, 0, 0 }));
	CHECK(textures[0][1] == VectorM({ 1, 1, 0 }));
	CHECK(textures[0][2] == VectorM({ 0, 0.5, 1 }));
	CHECK(textures[0][3] == VectorM({ 0, 0, 0 }));
}

TEST_CASE("[ArrayPolyMeshND] Boundary pivot overrides follow mesh edits") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(dimension, 2.0));
		Ref<ArrayPolyMeshND> base = box->to_array_poly_mesh();
		// Keep these geometry tests independent of normal and texture binding maintenance.
		TestMeshDataND::set_all_poly_normals(base, HashMap<Vector2i, Vector<Vector<VectorN>>>());
		TestMeshDataND::set_all_poly_texture_maps(base, HashMap<Vector2i, Vector<Vector<VectorM>>>());
		const int32_t vertex_count = (int32_t)base->get_poly_cell_vertex_positions().size();
		const int64_t cell_count = base->get_poly_cell_indices()[dimension - 3].size();
		for (const bool source_has_overrides : { false, true }) {
			Ref<ArrayPolyMeshND> mesh = base->duplicate();
			Ref<ArrayPolyMeshND> other = base->duplicate();
			mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 2 });
			if (source_has_overrides) {
				other->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ -1, vertex_count - 1 });
			}
			mesh->merge_with(other, TransformND::from_position(VectorND::value_on_axis_with_dimension(10.0, 0, dimension)));
			PackedInt32Array expected;
			expected.resize(cell_count * 2);
			expected.fill(-1);
			expected.set(0, 2);
			if (source_has_overrides) {
				expected.set(cell_count + 1, vertex_count * 2 - 1);
			}
			CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == expected);
			CHECK(mesh->is_poly_mesh_data_valid());
		}
		{
			Ref<ArrayPolyMeshND> mesh = base->duplicate();
			PackedInt32Array too_many;
			too_many.resize(cell_count + 1);
			too_many.fill(-1);
			mesh->set_poly_cell_boundary_pivot_overrides(too_many);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_poly_mesh_data_valid());
			ERR_PRINT_ON;
			mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ -1 });
			CHECK(mesh->is_poly_mesh_data_valid());
		}
		{
			Ref<ArrayPolyMeshND> mesh = base->duplicate();
			mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ 2, vertex_count - 1 });
			mesh->delete_poly_element(dimension - 1, 0);
			CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array({ vertex_count - 1 }));
			CHECK(mesh->is_poly_mesh_data_valid());
		}
		{
			Ref<ArrayPolyMeshND> mesh = base->duplicate();
			// Deleting an external pivot resets its override and shifts later pivot indices.
			mesh->append_vertex(VectorND::value_on_axis_with_dimension(7.0, 0, dimension));
			mesh->append_vertex(VectorND::value_on_axis_with_dimension(8.0, 0, dimension));
			mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ vertex_count, vertex_count + 1 });
			mesh->delete_poly_element(0, vertex_count);
			CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array({ -1, vertex_count }));
			CHECK(mesh->is_poly_mesh_data_valid());
			mesh->delete_poly_element(0, vertex_count);
			CHECK(mesh->get_poly_cell_boundary_pivot_overrides() == PackedInt32Array({ -1, -1 }));
			CHECK(mesh->is_poly_mesh_data_valid());
		}
	}
}

inline Ref<ArrayPolyMeshND> make_binding_test_mesh(const int p_dimension) {
	Ref<BoxPolyMeshND> box;
	box.instantiate();
	box->set_size(VectorND::fill(p_dimension, 2.0));
	Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
	TestMeshDataND::set_all_poly_normals(mesh, HashMap<Vector2i, Vector<Vector<VectorN>>>());
	TestMeshDataND::set_all_poly_texture_maps(mesh, HashMap<Vector2i, Vector<Vector<VectorM>>>());
	return mesh;
}

inline Vector<Vector<VectorN>> make_test_binding(const Ref<ArrayPolyMeshND> &p_mesh, const Vector2i p_key, const int p_value_dimension, const double p_offset = 100.0) {
	const Vector<PackedInt32Array> elements = p_mesh->get_all_poly_cell_poly_indices(p_key.x, p_key.y);
	Vector<Vector<VectorN>> binding;
	if (p_key.x == p_key.y) {
		Vector<VectorN> flat;
		for (int64_t i = 0; i < elements.size(); i++) {
			flat.append(VectorND::value_on_axis_with_dimension(p_offset + i, 0, p_value_dimension));
		}
		binding.append(flat);
	} else {
		for (int64_t i = 0; i < elements.size(); i++) {
			Vector<VectorN> data;
			for (const int32_t element : elements[i]) {
				data.append(VectorND::value_on_axis_with_dimension(p_offset + i * 1000 + element, 0, p_value_dimension));
			}
			binding.append(data);
		}
	}
	return binding;
}

TEST_CASE("[ArrayPolyMeshND] Dense binding shapes across dimensions") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		const Ref<ArrayPolyMeshND> base = make_binding_test_mesh(dimension);
		for (int geometry_dimension = 0; geometry_dimension <= dimension; geometry_dimension++) {
			for (int decomposition_dimension = 0; decomposition_dimension <= geometry_dimension; decomposition_dimension++) {
				const Vector2i key(geometry_dimension, decomposition_dimension);
				CAPTURE(key);
				for (int texture = 0; texture < 2; texture++) {
					CAPTURE(texture);
					Ref<ArrayPolyMeshND> mesh = base->duplicate();
					HashMap<Vector2i, Vector<Vector<VectorN>>> bindings;
					const Vector<Vector<VectorN>> full = make_test_binding(base, key, dimension - texture);
					auto set_binding = [&](const Vector<Vector<VectorN>> &p_binding) {
						bindings.insert(key, p_binding);
						if (texture) {
							TestMeshDataND::set_all_poly_texture_maps(mesh, bindings);
						} else {
							TestMeshDataND::set_all_poly_normals(mesh, bindings);
						}
					};
					set_binding(full);
					CHECK(mesh->is_poly_mesh_data_valid());
					set_binding(Vector<Vector<VectorN>>());
					CHECK(mesh->is_poly_mesh_data_valid());
					Vector<Vector<VectorN>> partial = full;
					const bool boundary_full_count = geometry_dimension == dimension - 1 && (decomposition_dimension == 0 || (!texture && decomposition_dimension == geometry_dimension));
					if (!boundary_full_count) {
						if (geometry_dimension == decomposition_dimension) {
							partial.ptrw()[0].resize(1);
						} else {
							partial.resize(1);
						}
						set_binding(partial);
						CHECK(mesh->is_poly_mesh_data_valid());
					}
					Vector<Vector<VectorN>> malformed = full;
					if (geometry_dimension == decomposition_dimension) {
						set_binding(Vector<Vector<VectorN>>{ Vector<VectorN>() });
						CHECK(mesh->is_poly_mesh_data_valid());
						malformed.append(Vector<VectorN>());
						set_binding(malformed);
						ERR_PRINT_OFF;
						CHECK_FALSE(mesh->is_poly_mesh_data_valid());
						ERR_PRINT_ON;
						malformed = full;
						malformed.ptrw()[0].append(VectorN());
					} else {
						partial = full;
						partial.set(0, Vector<VectorN>());
						set_binding(partial);
						CHECK(mesh->is_poly_mesh_data_valid());
						malformed.ptrw()[0].remove_at(0);
						set_binding(malformed);
						ERR_PRINT_OFF;
						CHECK_FALSE(mesh->is_poly_mesh_data_valid());
						ERR_PRINT_ON;
						malformed = full;
						malformed.append(Vector<VectorN>());
					}
					set_binding(malformed);
					ERR_PRINT_OFF;
					CHECK_FALSE(mesh->is_poly_mesh_data_valid());
					ERR_PRINT_ON;
				}
			}
		}
		for (const Vector2i key : { Vector2i(-1, 0), Vector2i(1, -1), Vector2i(1, 2), Vector2i(dimension + 1, dimension + 1) }) {
			Ref<ArrayPolyMeshND> mesh = base->duplicate();
			HashMap<Vector2i, Vector<Vector<VectorN>>> bindings;
			bindings.insert(key, key.x > dimension ? Vector<Vector<VectorN>>{ Vector<VectorN>{ VectorN() } } : Vector<Vector<VectorN>>());
			TestMeshDataND::set_all_poly_normals(mesh, bindings);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_poly_mesh_data_valid());
			ERR_PRINT_ON;
			TestMeshDataND::set_all_poly_normals(mesh, HashMap<Vector2i, Vector<Vector<VectorN>>>());
			TestMeshDataND::set_all_poly_texture_maps(mesh, bindings);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_poly_mesh_data_valid());
			ERR_PRINT_ON;
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Deletion maintains every dense binding") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		const Ref<ArrayPolyMeshND> base = make_binding_test_mesh(dimension);
		HashMap<Vector2i, Vector<Vector<VectorN>>> normals;
		HashMap<Vector2i, Vector<Vector<VectorM>>> texture_maps;
		for (int geometry_dimension = 0; geometry_dimension <= dimension; geometry_dimension++) {
			for (int decomposition_dimension = 0; decomposition_dimension <= geometry_dimension; decomposition_dimension++) {
				const Vector2i key(geometry_dimension, decomposition_dimension);
				normals.insert(key, make_test_binding(base, key, dimension));
				texture_maps.insert(key, make_test_binding(base, key, dimension - 1));
			}
		}
		for (int deleted_dimension = 0; deleted_dimension <= dimension; deleted_dimension++) {
			CAPTURE(deleted_dimension);
			Ref<ArrayPolyMeshND> mesh = base->duplicate();
			TestMeshDataND::set_all_poly_normals(mesh, normals);
			TestMeshDataND::set_all_poly_texture_maps(mesh, texture_maps);
			REQUIRE(mesh->is_poly_mesh_data_valid());
			mesh->delete_poly_element(deleted_dimension, 0);
			CHECK(mesh->is_poly_mesh_data_valid());
			const Vector2i flat_key(deleted_dimension, deleted_dimension);
			const auto remaining_normals = TestMeshDataND::get_all_poly_normals(mesh);
			const auto remaining_maps = TestMeshDataND::get_all_poly_texture_maps(mesh);
			CHECK(remaining_normals[flat_key][0].size() == normals[flat_key][0].size() - 1);
			CHECK(remaining_maps[flat_key][0].size() == texture_maps[flat_key][0].size() - 1);
			if (!remaining_normals[flat_key][0].is_empty()) {
				CHECK(remaining_normals[flat_key][0][0] == normals[flat_key][0][1]);
				CHECK(remaining_maps[flat_key][0][0] == texture_maps[flat_key][0][1]);
			}
			if (deleted_dimension > 0) {
				const Vector2i decomposed_key(deleted_dimension, 0);
				CHECK(remaining_normals[decomposed_key].size() == normals[decomposed_key].size() - 1);
				CHECK(remaining_maps[decomposed_key].size() == texture_maps[decomposed_key].size() - 1);
				if (!remaining_normals[decomposed_key].is_empty()) {
					CHECK(remaining_normals[decomposed_key][0] == normals[decomposed_key][1]);
					CHECK(remaining_maps[decomposed_key][0] == texture_maps[decomposed_key][1]);
				}
			}
			for (const auto &binding : remaining_normals) {
				const Vector<Vector<PackedInt32Array>> poly = mesh->get_poly_cell_indices();
				const int geometry_dimension = binding.key.x;
				int64_t count = 0;
				if (geometry_dimension == 0) {
					count = mesh->get_poly_cell_vertex_positions().size();
				} else if (geometry_dimension == 1) {
					count = mesh->get_edge_indices().size() / 2;
				} else if (geometry_dimension - 2 < poly.size()) {
					count = poly[geometry_dimension - 2].size();
				}
				CHECK((binding.key.x == binding.key.y ? binding.value[0].size() : binding.value.size()) == count);
				const auto texture_binding = remaining_maps[binding.key];
				CHECK((binding.key.x == binding.key.y ? texture_binding[0].size() : texture_binding.size()) == count);
			}
		}
		Ref<ArrayPolyMeshND> mesh = base->duplicate();
		const Vector2i key(1, 0);
		Vector<Vector<VectorN>> partial_normals = normals[key];
		Vector<Vector<VectorM>> partial_maps = texture_maps[key];
		partial_normals.resize(1);
		partial_maps.resize(1);
		normals.clear();
		texture_maps.clear();
		normals.insert(key, partial_normals);
		texture_maps.insert(key, partial_maps);
		TestMeshDataND::set_all_poly_normals(mesh, normals);
		TestMeshDataND::set_all_poly_texture_maps(mesh, texture_maps);
		mesh->delete_poly_element(1, mesh->get_edge_indices().size() / 2 - 1);
		CHECK(TestMeshDataND::get_all_poly_normals(mesh)[key] == partial_normals);
		CHECK(TestMeshDataND::get_all_poly_texture_maps(mesh)[key] == partial_maps);
		CHECK(mesh->is_poly_mesh_data_valid());
		mesh->delete_poly_element(1, 0);
		CHECK(TestMeshDataND::get_all_poly_normals(mesh)[key].is_empty());
		CHECK(TestMeshDataND::get_all_poly_texture_maps(mesh)[key].is_empty());
		CHECK(mesh->is_poly_mesh_data_valid());
	}
}

TEST_CASE("[ArrayPolyMeshND] Deduplication preserves missing and partial dense bindings") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension);
		mesh->calculate_boundary_normals();
		Vector<VectorN> boundary_normals = mesh->get_poly_cell_boundary_normals();
		for (int64_t i = 0; i < boundary_normals.size(); i++) {
			boundary_normals.set(i, VectorND::negate(boundary_normals[i]));
		}
		mesh->set_poly_cell_boundary_normals(boundary_normals);
		HashMap<Vector2i, Vector<Vector<VectorN>>> normals = TestMeshDataND::get_all_poly_normals(mesh);
		HashMap<Vector2i, Vector<Vector<VectorM>>> texture_maps;
		const Vector2i absent_key(dimension + 1, 0);
		const Vector2i absent_flat_key(dimension + 1, dimension + 1);
		normals.insert(absent_key, Vector<Vector<VectorN>>());
		texture_maps.insert(absent_flat_key, Vector<Vector<VectorM>>{ Vector<VectorM>() });
		for (int target_dimension = 1; target_dimension < dimension - 1; target_dimension++) {
			const Vector2i key(dimension - 1, target_dimension);
			Vector<Vector<VectorN>> binding = make_test_binding(mesh, key, dimension);
			Vector<Vector<VectorM>> texture_binding = make_test_binding(mesh, key, dimension - 1);
			binding.resize(2);
			texture_binding.resize(2);
			binding.set(0, Vector<VectorN>());
			texture_binding.set(0, Vector<VectorM>());
			normals.insert(key, binding);
			texture_maps.insert(key, texture_binding);
		}
		TestMeshDataND::set_all_poly_normals(mesh, normals);
		TestMeshDataND::set_all_poly_texture_maps(mesh, texture_maps);
		REQUIRE(mesh->is_poly_mesh_data_valid());
		mesh->deduplicate_all_elements();
		CHECK(mesh->is_poly_mesh_data_valid());
		const auto dedup_normals = TestMeshDataND::get_all_poly_normals(mesh);
		const auto dedup_maps = TestMeshDataND::get_all_poly_texture_maps(mesh);
		REQUIRE(dedup_normals.has(absent_key));
		CHECK(dedup_normals[absent_key].is_empty());
		REQUIRE(dedup_maps.has(absent_flat_key));
		CHECK(dedup_maps[absent_flat_key].size() == 1);
		CHECK(dedup_maps[absent_flat_key][0].is_empty());
		for (int target_dimension = 1; target_dimension < dimension - 1; target_dimension++) {
			const Vector2i key(dimension - 1, target_dimension);
			REQUIRE(dedup_normals[key].size() == 2);
			REQUIRE(dedup_maps[key].size() == 2);
			CHECK(dedup_normals[key][0].is_empty());
			CHECK(dedup_maps[key][0].is_empty());
			const auto elements = mesh->get_all_poly_cell_poly_indices(key.x, key.y);
			for (int64_t i = 0; i < elements[1].size(); i++) {
				CHECK(dedup_normals[key][1][i][0] == 1100.0 + elements[1][i]);
				CHECK(dedup_maps[key][1][i][0] == 1100.0 + elements[1][i]);
			}
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Deduplication remaps reversed edge vertex bindings") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension);
		const PackedInt32Array original_edges = mesh->get_edge_indices();
		PackedInt32Array edges = { original_edges[1], original_edges[0] };
		edges.append_array(original_edges);
		mesh->set_edge_vertex_indices(edges);
		Vector<Vector<PackedInt32Array>> poly = mesh->get_poly_cell_indices();
		for (int64_t face = 0; face < poly[0].size(); face++) {
			PackedInt32Array face_edges = poly[0][face];
			for (int64_t i = 0; i < face_edges.size(); i++) {
				face_edges.set(i, face_edges[i] + 1);
			}
			poly.ptrw()[0].set(face, face_edges);
		}
		mesh->set_poly_cell_indices(poly);
		HashMap<Vector2i, Vector<Vector<VectorN>>> normals;
		HashMap<Vector2i, Vector<Vector<VectorM>>> texture_maps;
		for (const Vector2i key : { Vector2i(1, 0), Vector2i(2, 0) }) {
			// Use only vertex IDs as tags; the duplicate edge's outer position will disappear.
			auto binding = make_test_binding(mesh, key, dimension);
			auto texture_binding = make_test_binding(mesh, key, dimension - 1);
			for (int64_t i = 0; i < binding.size(); i++) {
				for (int64_t j = 0; j < binding[i].size(); j++) {
					binding.ptrw()[i].ptrw()[j].ptrw()[0] -= i * 1000;
					texture_binding.ptrw()[i].ptrw()[j].ptrw()[0] -= i * 1000;
				}
			}
			normals.insert(key, binding);
			texture_maps.insert(key, texture_binding);
		}
		TestMeshDataND::set_all_poly_normals(mesh, normals);
		TestMeshDataND::set_all_poly_texture_maps(mesh, texture_maps);
		mesh->deduplicate_all_elements();
		CHECK(mesh->is_poly_mesh_data_valid());
		CHECK(mesh->get_edge_indices().size() == original_edges.size());
		for (const Vector2i key : { Vector2i(1, 0), Vector2i(2, 0) }) {
			const auto elements = mesh->get_all_poly_cell_poly_indices(key.x, key.y);
			const auto output_normals = TestMeshDataND::get_all_poly_normals(mesh)[key];
			const auto output_maps = TestMeshDataND::get_all_poly_texture_maps(mesh)[key];
			REQUIRE(output_normals.size() == elements.size());
			REQUIRE(output_maps.size() == elements.size());
			for (int64_t i = 0; i < elements.size(); i++) {
				REQUIRE(output_normals[i].size() == elements[i].size());
				REQUIRE(output_maps[i].size() == elements[i].size());
				for (int64_t j = 0; j < elements[i].size(); j++) {
					CHECK(output_normals[i][j][0] == 100.0 + elements[i][j]);
					CHECK(output_maps[i][j][0] == 100.0 + elements[i][j]);
				}
			}
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Coincident vertices shrink decomposed binding cardinality") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		const VectorN right = VectorND::value_on_axis_with_dimension(1.0, 0, dimension);
		const VectorN up = VectorND::value_on_axis_with_dimension(1.0, 1, dimension);
		mesh->set_poly_cell_vertex_positions(Vector<VectorN>{ VectorND::zero(dimension), right, up, up });
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 0 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ Vector<PackedInt32Array>{ PackedInt32Array{ 0, 1, 2, 3 } } });
		const Vector2i key(2, 0);
		HashMap<Vector2i, Vector<Vector<VectorN>>> normals;
		HashMap<Vector2i, Vector<Vector<VectorM>>> maps;
		normals.insert(key, make_test_binding(mesh, key, dimension));
		maps.insert(key, make_test_binding(mesh, key, dimension - 1));
		TestMeshDataND::set_all_poly_normals(mesh, normals);
		TestMeshDataND::set_all_poly_texture_maps(mesh, maps);
		REQUIRE(normals[key][0].size() == 4);
		mesh->deduplicate_all_elements();
		CHECK(mesh->is_poly_mesh_data_valid());
		const auto output_normals = TestMeshDataND::get_all_poly_normals(mesh)[key][0];
		const auto output_maps = TestMeshDataND::get_all_poly_texture_maps(mesh)[key][0];
		REQUIRE(output_normals.size() == 3);
		REQUIRE(output_maps.size() == 3);
		CHECK(output_normals[2][0] == 102.0);
		CHECK(output_maps[2][0] == 102.0);
	}
}

TEST_CASE("[ArrayPolyMeshND] Dense merge bindings preserve prefixes and geometry offsets") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		const Ref<ArrayPolyMeshND> base = make_binding_test_mesh(dimension);
		const Ref<TransformND> transform = TransformND::from_position_scale(VectorND::fill(dimension, 7.0), VectorND::fill(dimension, 2.0));
		for (int geometry_dimension = 0; geometry_dimension <= dimension; geometry_dimension++) {
			for (int decomposition_dimension = 0; decomposition_dimension <= geometry_dimension; decomposition_dimension++) {
				const Vector2i key(geometry_dimension, decomposition_dimension);
				CAPTURE(key);
				const int64_t element_count = base->get_all_poly_cell_poly_indices(key.x, key.y).size();
				const bool flat = key.x == key.y;
				const bool boundary_normals = key == Vector2i(dimension - 1, dimension - 1) || key == Vector2i(dimension - 1, 0);
				for (int states = 0; states < 9; states++) {
					CAPTURE(states);
					// Each input is absent, a partial prefix, or full. Boundary views require full counts.
					const int first_state = states % 3;
					const int other_state = states / 3;
					if (boundary_normals && (first_state == 1 || other_state == 1)) {
						continue;
					}
					const int presence = (first_state != 0 ? 1 : 0) | (other_state != 0 ? 2 : 0);
					Vector<Vector<VectorN>> first_normals = make_test_binding(base, key, dimension);
					Vector<Vector<VectorN>> other_normals = make_test_binding(base, key, dimension, 500.0);
					Vector<Vector<VectorM>> first_maps = make_test_binding(base, key, dimension - 1);
					Vector<Vector<VectorM>> other_maps = make_test_binding(base, key, dimension - 1, 500.0);
					if (first_state == 1) {
						if (flat) {
							first_normals.ptrw()[0].resize(1);
							first_maps.ptrw()[0].resize(1);
						} else {
							first_normals.resize(1);
							first_maps.resize(1);
						}
					}
					if (other_state == 1) {
						if (flat) {
							other_normals.ptrw()[0].resize(1);
							other_maps.ptrw()[0].resize(1);
						} else {
							other_normals.resize(1);
							other_maps.resize(1);
						}
					}
					Ref<ArrayPolyMeshND> mesh = base->duplicate();
					Ref<ArrayPolyMeshND> other = base->duplicate();
					HashMap<Vector2i, Vector<Vector<VectorN>>> normals;
					HashMap<Vector2i, Vector<Vector<VectorM>>> maps;
					if (presence & 1) {
						normals.insert(key, first_normals);
						maps.insert(key, first_maps);
						TestMeshDataND::set_all_poly_normals(mesh, normals);
						TestMeshDataND::set_all_poly_texture_maps(mesh, maps);
					}
					if (presence & 2) {
						normals.insert(key, other_normals);
						maps.insert(key, other_maps);
						TestMeshDataND::set_all_poly_normals(other, normals);
						TestMeshDataND::set_all_poly_texture_maps(other, maps);
					}
					const bool warns_missing_normals = !boundary_normals && presence != 0 && (presence != 3 || (element_count > 1 && (first_state == 1 || other_state == 1)));
					if (warns_missing_normals) {
						ERR_PRINT_OFF;
						mesh->merge_with(other, transform);
						ERR_PRINT_ON;
					} else {
						mesh->merge_with(other, transform);
					}
					CHECK(mesh->is_poly_mesh_data_valid());
					const auto merged_normals = TestMeshDataND::get_all_poly_normals(mesh);
					const auto merged_maps = TestMeshDataND::get_all_poly_texture_maps(mesh);
					if (presence == 0) {
						CHECK_FALSE(merged_normals.has(key));
						CHECK_FALSE(merged_maps.has(key));
						continue;
					}
					REQUIRE(merged_normals.has(key));
					REQUIRE(merged_maps.has(key));
					REQUIRE((flat ? merged_normals[key][0].size() : merged_normals[key].size()) == element_count * 2);
					REQUIRE((flat ? merged_maps[key][0].size() : merged_maps[key].size()) == element_count * 2);
					for (int half = 0; half < 2; half++) {
						const auto input_normals = half == 0 ? first_normals : other_normals;
						const auto input_maps = half == 0 ? first_maps : other_maps;
						const bool has_binding = (presence & (1 << half)) != 0;
						const int64_t input_count = has_binding ? (flat ? input_normals[0].size() : input_normals.size()) : 0;
						for (int64_t i = 0; i < element_count; i++) {
							const int64_t output_index = half * element_count + i;
							if (flat) {
								VectorN expected_normal;
								VectorM expected_map;
								if (i < input_count) {
									expected_normal = half == 0 ? input_normals[0][i] : transform->xform_basis(input_normals[0][i]);
									expected_map = input_maps[0][i];
								}
								if (i < input_count || !boundary_normals) {
									CHECK(merged_normals[key][0][output_index] == expected_normal);
								}
								CHECK(merged_maps[key][0][output_index] == expected_map);
							} else {
								Vector<VectorN> expected_normals;
								Vector<VectorM> expected_maps;
								if (i < input_count) {
									expected_normals = input_normals[i];
									expected_maps = input_maps[i];
									if (half == 1) {
										for (int64_t j = 0; j < expected_normals.size(); j++) {
											expected_normals.set(j, transform->xform_basis(expected_normals[j]));
										}
									}
								}
								if (i < input_count || !boundary_normals) {
									CHECK(merged_normals[key][output_index] == expected_normals);
								}
								CHECK(merged_maps[key][output_index] == expected_maps);
							}
						}
					}
					if (presence & 2) {
						CHECK(TestMeshDataND::get_all_poly_normals(other)[key] == other_normals);
						CHECK(TestMeshDataND::get_all_poly_texture_maps(other)[key] == other_maps);
					}
				}
			}
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Merge preserves empty binding representations") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		const Ref<ArrayPolyMeshND> base = make_binding_test_mesh(dimension);
		HashMap<Vector2i, Vector<Vector<VectorN>>> empty_bindings;
		empty_bindings.insert(Vector2i(0, 0), Vector<Vector<VectorN>>{ Vector<VectorN>() });
		empty_bindings.insert(Vector2i(1, 0), Vector<Vector<VectorN>>());
		empty_bindings.insert(Vector2i(2, 2), Vector<Vector<VectorN>>{ Vector<VectorN>() });
		empty_bindings.insert(Vector2i(dimension - 1, dimension - 1), Vector<Vector<VectorN>>{ Vector<VectorN>() });
		empty_bindings.insert(Vector2i(dimension - 1, 0), Vector<Vector<VectorN>>());
		empty_bindings.insert(Vector2i(dimension + 1, 0), Vector<Vector<VectorN>>());
		empty_bindings.insert(Vector2i(dimension + 1, dimension + 1), Vector<Vector<VectorN>>{ Vector<VectorN>() });
		Ref<ArrayPolyMeshND> with_empty = base->duplicate();
		TestMeshDataND::set_all_poly_normals(with_empty, empty_bindings);
		TestMeshDataND::set_all_poly_texture_maps(with_empty, empty_bindings);
		Ref<ArrayPolyMeshND> without = base->duplicate();
		without->merge_with(with_empty);
		CHECK(without->is_poly_mesh_data_valid());
		CHECK(TestMeshDataND::get_all_poly_normals(without).is_empty());
		CHECK(TestMeshDataND::get_all_poly_texture_maps(without).is_empty());
		with_empty->merge_with(base);
		CHECK(with_empty->is_poly_mesh_data_valid());
		const auto normals = TestMeshDataND::get_all_poly_normals(with_empty);
		const auto maps = TestMeshDataND::get_all_poly_texture_maps(with_empty);
		CHECK(normals.size() == empty_bindings.size());
		CHECK(maps.size() == empty_bindings.size());
		for (const auto &binding : empty_bindings) {
			CHECK(normals[binding.key] == binding.value);
			CHECK(maps[binding.key] == binding.value);
		}
		while (!with_empty->get_poly_cell_indices().is_empty()) {
			with_empty->delete_poly_element(2, 0);
		}
		with_empty->deduplicate_all_elements();
		CHECK(with_empty->is_poly_mesh_data_valid());
		CHECK(TestMeshDataND::get_all_poly_normals(with_empty).size() == empty_bindings.size());
		CHECK(TestMeshDataND::get_all_poly_texture_maps(with_empty).size() == empty_bindings.size());
		for (const auto &binding : empty_bindings) {
			CHECK(TestMeshDataND::get_all_poly_normals(with_empty)[binding.key] == binding.value);
			CHECK(TestMeshDataND::get_all_poly_texture_maps(with_empty)[binding.key] == binding.value);
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Self merge snapshots attributes and refreshes primed caches") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension);
		const Vector2i key(dimension - 1, 0);
		const Vector2i flat_key(0, 0);
		mesh->calculate_boundary_normals();
		auto normals = TestMeshDataND::get_all_poly_normals(mesh);
		HashMap<Vector2i, Vector<Vector<VectorM>>> texture_maps;
		for (const Vector2i binding_key : { key, flat_key }) {
			normals.insert(binding_key, make_test_binding(mesh, binding_key, dimension));
			texture_maps.insert(binding_key, make_test_binding(mesh, binding_key, dimension - 1));
		}
		TestMeshDataND::set_all_poly_normals(mesh, normals);
		TestMeshDataND::set_all_poly_texture_maps(mesh, texture_maps);
		const int64_t simplex_index_count = mesh->get_simplex_cell_vertex_indices().size();
		const auto simplex_positions = mesh->get_simplex_cell_positions();
		const auto simplex_normals = TestMeshDataND::get_simplex_normals(mesh);
		const auto simplex_maps = TestMeshDataND::get_simplex_texture_map(mesh);
		const int64_t vertex_count = mesh->get_poly_cell_vertex_positions().size();
		REQUIRE(simplex_index_count > 0);
		REQUIRE(simplex_positions.size() == simplex_index_count);
		REQUIRE(simplex_normals.size() == simplex_index_count);
		REQUIRE(simplex_maps.size() == simplex_index_count);
		const Ref<TransformND> transform = TransformND::from_position_scale(VectorND::fill(dimension, 10.0), VectorND::fill(dimension, 2.0));
		mesh->merge_with(mesh, transform);
		CHECK(mesh->is_poly_mesh_data_valid());
		CHECK(mesh->get_poly_cell_vertex_positions().size() == vertex_count * 2);
		CHECK(mesh->get_simplex_cell_vertex_indices().size() == simplex_index_count * 2);
		const auto merged_normals = TestMeshDataND::get_all_poly_normals(mesh);
		const auto merged_maps = TestMeshDataND::get_all_poly_texture_maps(mesh);
		for (const auto &binding : normals) {
			const auto merged = merged_normals[binding.key];
			if (binding.key.x == binding.key.y) {
				const int64_t count = binding.value[0].size();
				REQUIRE(merged[0].size() == count * 2);
				for (int64_t i = 0; i < count; i++) {
					CHECK(merged[0][i] == binding.value[0][i]);
					CHECK(merged[0][i + count] == transform->xform_basis(binding.value[0][i]));
				}
			} else {
				const int64_t count = binding.value.size();
				REQUIRE(merged.size() == count * 2);
				for (int64_t i = 0; i < count; i++) {
					CHECK(merged[i] == binding.value[i]);
					REQUIRE(merged[i + count].size() == binding.value[i].size());
					for (int64_t j = 0; j < binding.value[i].size(); j++) {
						CHECK(merged[i + count][j] == transform->xform_basis(binding.value[i][j]));
					}
				}
			}
		}
		for (const auto &binding : texture_maps) {
			const auto merged = merged_maps[binding.key];
			if (binding.key.x == binding.key.y) {
				Vector<VectorM> expected = binding.value[0];
				expected.append_array(binding.value[0]);
				CHECK(merged[0] == expected);
			} else {
				const int64_t count = binding.value.size();
				REQUIRE(merged.size() == count * 2);
				for (int64_t i = 0; i < count; i++) {
					CHECK(merged[i] == binding.value[i]);
					CHECK(merged[i + count] == binding.value[i]);
				}
			}
		}
		const auto merged_simplex_positions = mesh->get_simplex_cell_positions();
		const auto merged_simplex_normals = TestMeshDataND::get_simplex_normals(mesh);
		const auto merged_simplex_maps = TestMeshDataND::get_simplex_texture_map(mesh);
		REQUIRE(merged_simplex_positions.size() == simplex_index_count * 2);
		REQUIRE(merged_simplex_normals.size() == simplex_index_count * 2);
		REQUIRE(merged_simplex_maps.size() == simplex_index_count * 2);
		for (int64_t i = 0; i < simplex_index_count; i++) {
			CHECK(merged_simplex_positions[i] == simplex_positions[i]);
			CHECK(merged_simplex_positions[i + simplex_index_count] == transform->xform(simplex_positions[i]));
			CHECK(merged_simplex_normals[i] == simplex_normals[i]);
			CHECK(merged_simplex_normals[i + simplex_index_count] == transform->xform_basis(simplex_normals[i]));
			CHECK(merged_simplex_maps[i] == simplex_maps[i]);
			CHECK(merged_simplex_maps[i + simplex_index_count] == simplex_maps[i]);
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Compact boundary geometry survives deleting the dimension anchor") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		// Embed an (N-1)-cube as one boundary cell, with an unused full-dimensional anchor.
		Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension - 1);
		Vector<VectorN> positions = { VectorND::fill(dimension, 99.0) };
		for (const VectorN &box_position : mesh->get_poly_cell_vertex_positions()) {
			VectorN position = VectorND::add(box_position, VectorND::fill(dimension - 1, 1.0));
			while (!position.is_empty() && position[position.size() - 1] == 0.0) {
				position.resize(position.size() - 1);
			}
			positions.append(position);
		}
		PackedInt32Array edges = mesh->get_edge_indices();
		for (int64_t i = 0; i < edges.size(); i++) {
			edges.set(i, edges[i] + 1);
		}
		mesh->set_poly_cell_vertex_positions(positions);
		mesh->set_edge_vertex_indices(edges);
		REQUIRE(mesh->is_poly_mesh_data_valid());
		REQUIRE(positions[1].is_empty());
		Ref<ArrayPolyMeshND> expanded = mesh->duplicate();
		Vector<VectorN> expanded_positions;
		for (const VectorN &position : positions) {
			expanded_positions.append(VectorND::with_dimension(position, dimension));
		}
		expanded->set_poly_cell_vertex_positions(expanded_positions);
		mesh->calculate_boundary_normals();
		expanded->calculate_boundary_normals();
		const Vector<VectorN> boundary_normals = mesh->get_poly_cell_boundary_normals();
		REQUIRE(boundary_normals.size() == 1);
		REQUIRE(boundary_normals[0].size() == dimension);
		CHECK(VectorND::is_equal_approx(boundary_normals[0], expanded->get_poly_cell_boundary_normals()[0]));
		for (int axis = 0; axis < dimension; axis++) {
			CHECK(Math::abs(boundary_normals[0][axis]) == doctest::Approx(axis == dimension - 1 ? 1.0 : 0.0));
		}
		const Vector<VectorN> simplex_positions = mesh->get_simplex_cell_positions();
		const Vector<VectorN> expanded_simplex_positions = expanded->get_simplex_cell_positions();
		REQUIRE(simplex_positions.size() > 0);
		REQUIRE(simplex_positions.size() == expanded_simplex_positions.size());
		CHECK(mesh->get_simplex_cell_vertex_indices() == expanded->get_simplex_cell_vertex_indices());
		for (int64_t i = 0; i < simplex_positions.size(); i++) {
			CHECK(VectorND::is_equal_approx(simplex_positions[i], expanded_simplex_positions[i]));
		}
		const Ref<ArrayCellMeshND> cell_mesh = mesh->to_array_cell_mesh();
		CHECK(cell_mesh->is_mesh_data_valid());

		mesh->delete_poly_element(0, 0);
		CHECK(mesh->get_dimension() == dimension);
		CHECK(mesh->is_mesh_data_valid());
		const Vector<VectorN> remaining_positions = mesh->get_poly_cell_vertex_positions();
		REQUIRE(remaining_positions.size() == positions.size() - 1);
		CHECK(remaining_positions[0] == VectorND::zero(dimension));
		for (int64_t i = 1; i < remaining_positions.size(); i++) {
			CHECK(remaining_positions[i] == positions[i + 1]);
		}
		const Vector<VectorN> remaining_simplex_positions = mesh->get_simplex_cell_positions();
		REQUIRE(remaining_simplex_positions.size() == simplex_positions.size());
		for (int64_t i = 0; i < simplex_positions.size(); i++) {
			CHECK(VectorND::is_equal_approx(remaining_simplex_positions[i], simplex_positions[i]));
		}
		mesh->calculate_boundary_normals();
		CHECK(mesh->get_poly_cell_boundary_normals() == boundary_normals);

		Ref<ArrayPolyMeshND> point_mesh;
		point_mesh.instantiate();
		point_mesh->set_poly_cell_vertex_positions({ VectorND::zero(dimension), VectorN(), VectorN{ 2.0 } });
		point_mesh->delete_poly_element(0, 2);
		CHECK(point_mesh->get_dimension() == dimension);
		CHECK(point_mesh->get_poly_cell_vertex_positions()[1].is_empty());
		point_mesh->delete_poly_element(0, 0);
		CHECK(point_mesh->get_dimension() == dimension);
		CHECK(point_mesh->get_poly_cell_vertex_positions()[0] == VectorND::zero(dimension));
		point_mesh->delete_poly_element(0, 0);
		CHECK(point_mesh->get_dimension() == 0);
		CHECK(point_mesh->get_poly_cell_vertex_positions().is_empty());
		CHECK(point_mesh->is_mesh_data_valid());
	}
}

TEST_CASE("[ArrayPolyMeshND] All dense bindings accept compact values and reject excess components") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		for (const Vector2i key : { Vector2i(0, 0), Vector2i(1, 0), Vector2i(dimension - 1, 0), Vector2i(dimension - 1, dimension - 1) }) {
			Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension);
			Vector<Vector<VectorN>> normals = make_test_binding(mesh, key, dimension);
			Vector<Vector<VectorM>> maps = make_test_binding(mesh, key, dimension - 1);
			for (int64_t i = 0; i < normals.size(); i++) {
				Vector<VectorN> cell_normals = normals[i];
				Vector<VectorM> cell_maps = maps[i];
				for (int64_t j = 0; j < cell_normals.size(); j++) {
					cell_normals.set(j, j % 2 == 0 ? VectorN() : VectorN{ 2.0 });
					cell_maps.set(j, j % 2 == 0 ? VectorM() : VectorM{ 3.0 });
				}
				normals.set(i, cell_normals);
				maps.set(i, cell_maps);
			}
			HashMap<Vector2i, Vector<Vector<VectorN>>> all_normals;
			HashMap<Vector2i, Vector<Vector<VectorM>>> all_maps;
			all_normals.insert(key, normals);
			all_maps.insert(key, maps);
			TestMeshDataND::set_all_poly_normals(mesh, all_normals);
			TestMeshDataND::set_all_poly_texture_maps(mesh, all_maps);
			CHECK(mesh->is_mesh_data_valid());
			const Ref<ArrayCellMeshND> cell_mesh = mesh->to_array_cell_mesh();
			CHECK(cell_mesh->is_mesh_data_valid());
			CHECK(TestMeshDataND::get_all_poly_normals(mesh)[key] == normals);
			CHECK(TestMeshDataND::get_all_poly_texture_maps(mesh)[key] == maps);
			Vector<VectorN> invalid_normals = normals[0];
			invalid_normals.set(0, VectorND::zero(dimension + 1));
			all_normals[key].set(0, invalid_normals);
			TestMeshDataND::set_all_poly_normals(mesh, all_normals);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_poly_mesh_data_valid());
			ERR_PRINT_ON;
			all_normals[key] = normals;
			TestMeshDataND::set_all_poly_normals(mesh, all_normals);
			Vector<VectorM> invalid_maps = maps[0];
			invalid_maps.set(0, VectorND::zero(dimension));
			all_maps[key].set(0, invalid_maps);
			TestMeshDataND::set_all_poly_texture_maps(mesh, all_maps);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_poly_mesh_data_valid());
			ERR_PRINT_ON;
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Fitting existing compact texture coordinates includes omitted zero components") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		for (const int first_pattern : { 0, 2 }) {
			CAPTURE(dimension);
			CAPTURE(first_pattern);
			Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension);
			const Vector<VectorM> patterns = { VectorM(), VectorM{ 2.0 }, VectorM{ 2.0, 2.0 }, VectorM{ 0.0, 2.0 } };
			Vector<Vector<VectorM>> texture_map;
			for (const PackedInt32Array &cell_vertices : mesh->get_all_boundary_cell_vertex_indices(false)) {
				Vector<VectorM> cell_map;
				for (int64_t i = 0; i < cell_vertices.size(); i++) {
					cell_map.append(patterns[(i + first_pattern) % patterns.size()]);
				}
				if (first_pattern == 2) {
					cell_map.set(0, VectorND::with_dimension(cell_map[0], dimension - 1));
				}
				texture_map.append(cell_map);
			}
			TestMeshDataND::set_poly_texture_map(mesh, texture_map);
			REQUIRE(mesh->is_poly_mesh_data_valid());
			mesh->unwrap_texture_map(ArrayPolyMeshND::UNWRAP_MODE_EACH_CELL_FILLS, 1.0, false, true);
			CHECK(mesh->is_mesh_data_valid());
			const Vector<Vector<VectorM>> fitted = TestMeshDataND::get_poly_texture_map(mesh);
			REQUIRE(fitted.size() == texture_map.size());
			for (int64_t i = 0; i < fitted.size(); i++) {
				REQUIRE(fitted[i].size() == texture_map[i].size());
				for (int64_t j = 0; j < fitted[i].size(); j++) {
					REQUIRE(fitted[i][j].size() == dimension - 1);
					for (int axis = 0; axis < dimension - 1; axis++) {
						const double expected = 0.25 + VectorND::get_component(texture_map[i][j], axis) * 0.25;
						CHECK(fitted[i][j][axis] == doctest::Approx(expected));
					}
				}
			}
		}
	}
}
TEST_CASE("[ArrayPolyMeshND] Double sided attributes follow vertex identity and preserve missing data") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		// Missing keys, empty arrays, missing cells, full data, texture-only data, and normal-only data.
		for (int pattern = 0; pattern < 7; pattern++) {
			CAPTURE(pattern);
			Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension);
			TestMeshDataND::set_all_poly_normals(mesh, HashMap<Vector2i, Vector<Vector<VectorN>>>());
			TestMeshDataND::set_all_poly_texture_maps(mesh, HashMap<Vector2i, Vector<Vector<VectorM>>>());
			const Vector<PackedInt32Array> cell_vertices = mesh->get_all_boundary_cell_vertex_indices(false);
			Vector<Vector<VectorN>> normals;
			Vector<Vector<VectorM>> texture_map;
			for (int64_t cell = 0; cell < cell_vertices.size(); cell++) {
				Vector<VectorN> cell_normals;
				Vector<VectorM> cell_map;
				for (const int32_t vertex : cell_vertices[cell]) {
					cell_normals.append(vertex % 3 == 0 ? VectorN() : VectorN{ double(vertex + 1), double(cell + 2) });
					cell_map.append(vertex % 3 == 0 ? VectorM() : VectorM{ vertex + 0.25, cell + 0.5 });
				}
				if (pattern == 2 || (pattern == 3 && (cell == 0 || cell == cell_vertices.size() - 1))) {
					cell_normals.clear();
				}
				if (pattern == 2 || (pattern == 3 && cell == cell_vertices.size() / 2)) {
					cell_map.clear();
				}
				normals.append(cell_normals);
				texture_map.append(cell_map);
			}
			if (pattern == 1 || pattern == 5) {
				normals.clear();
			}
			if (pattern == 1 || pattern == 6) {
				texture_map.clear();
			}
			if (pattern == 1) {
				HashMap<Vector2i, Vector<PackedInt32Array>> empty_bindings;
				empty_bindings.insert(Vector2i(dimension - 1, 0), Vector<PackedInt32Array>());
				mesh->set_all_poly_cell_normal_indices(empty_bindings);
				mesh->set_all_poly_cell_texture_map_indices(empty_bindings);
			} else if (pattern != 0) {
				TestMeshDataND::set_poly_normals(mesh, normals);
				TestMeshDataND::set_poly_texture_map(mesh, texture_map);
			}
			// A short pivot array includes an explicit missing sentinel and implicit missing tail.
			mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array{ cell_vertices[0][0], -1, cell_vertices[2][0] });
			mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, false);
			REQUIRE(mesh->is_poly_mesh_data_valid());
			if (pattern == 3) {
				ERR_PRINT_OFF; // Validation samples the deliberately partial attributes.
			}
			const bool initially_valid = mesh->is_mesh_data_valid();
			if (pattern == 3) {
				ERR_PRINT_ON;
			}
			REQUIRE(initially_valid);
			for (int pass = 0; pass < 3; pass++) {
				CAPTURE(pass);
				const auto before_geometry = mesh->get_poly_cell_indices();
				const auto before_vertices = mesh->get_all_boundary_cell_vertex_indices(false);
				const auto before_normals = TestMeshDataND::get_poly_normals(mesh);
				const auto before_texture_map = TestMeshDataND::get_poly_texture_map(mesh);
				const auto before_boundary_normals = mesh->get_poly_cell_boundary_normals();
				const PackedInt32Array before_pivots = mesh->get_poly_cell_boundary_pivot_overrides();
				mesh->make_double_sided(pass != 2);
				if (pattern == 1) {
					const Vector2i key(dimension - 1, 0);
					CHECK(mesh->get_all_poly_cell_normal_indices().has(key));
					CHECK(mesh->get_all_poly_cell_texture_map_indices().has(key));
				}
				const auto after_geometry = mesh->get_poly_cell_indices();
				if (pattern == 3) {
					ERR_PRINT_OFF; // This getter validates and samples the deliberately partial attributes.
				}
				const auto after_vertices = mesh->get_all_boundary_cell_vertex_indices(false);
				if (pattern == 3) {
					ERR_PRINT_ON;
				}
				const auto after_normals = TestMeshDataND::get_poly_normals(mesh);
				const auto after_texture_map = TestMeshDataND::get_poly_texture_map(mesh);
				const auto after_boundary_normals = mesh->get_poly_cell_boundary_normals();
				const PackedInt32Array after_pivots = mesh->get_poly_cell_boundary_pivot_overrides();
				if (pass == 1) {
					CHECK(after_geometry == before_geometry);
					CHECK(after_normals == before_normals);
					CHECK(after_texture_map == before_texture_map);
					CHECK(after_boundary_normals == before_boundary_normals);
					CHECK(after_pivots == before_pivots);
					continue;
				}
				const int64_t original_count = before_vertices.size();
				REQUIRE(after_vertices.size() == original_count * 2);
				REQUIRE(after_boundary_normals.size() == original_count * 2);
				REQUIRE(after_normals.size() == before_normals.size() * 2);
				REQUIRE(after_texture_map.size() == before_texture_map.size() * 2);
				bool order_changed = false;
				for (int64_t cell = 0; cell < original_count; cell++) {
					const int64_t flipped = original_count + cell;
					CHECK(after_geometry[dimension - 3][cell] == before_geometry[dimension - 3][cell]);
					CHECK(after_vertices[cell] == before_vertices[cell]);
					CHECK(after_boundary_normals[cell] == before_boundary_normals[cell]);
					const auto expected_boundary_normal = VectorND::negate(before_boundary_normals[cell]);
					CHECK(VectorND::is_equal_exact(after_boundary_normals[flipped], expected_boundary_normal));
					const int32_t expected_pivot = cell < before_pivots.size() ? before_pivots[cell] : -1;
					const int32_t original_pivot = cell < after_pivots.size() ? after_pivots[cell] : -1;
					const int32_t flipped_pivot = flipped < after_pivots.size() ? after_pivots[flipped] : -1;
					CHECK(original_pivot == expected_pivot);
					CHECK(flipped_pivot == expected_pivot);
					order_changed |= before_vertices[cell] != after_vertices[flipped];
					if (!before_normals.is_empty()) {
						CHECK(after_normals[cell] == before_normals[cell]);
						REQUIRE(after_normals[flipped].size() == before_normals[cell].size());
					}
					if (!before_texture_map.is_empty()) {
						CHECK(after_texture_map[cell] == before_texture_map[cell]);
						REQUIRE(after_texture_map[flipped].size() == before_texture_map[cell].size());
					}
					for (int64_t slot = 0; slot < after_vertices[flipped].size(); slot++) {
						const int64_t source_slot = before_vertices[cell].find(after_vertices[flipped][slot]);
						REQUIRE(source_slot >= 0);
						if (!before_normals.is_empty() && !before_normals[cell].is_empty()) {
							const auto expected_normal = VectorND::negate(before_normals[cell][source_slot]);
							CHECK(VectorND::is_equal_exact(after_normals[flipped][slot], expected_normal));
						}
						if (!before_texture_map.is_empty() && !before_texture_map[cell].is_empty()) {
							CHECK(after_texture_map[flipped][slot] == before_texture_map[cell][source_slot]);
						}
					}
				}
				CHECK(order_changed);
				for (int64_t volume = 0; volume < before_geometry[dimension - 2].size(); volume++) {
					CHECK(after_geometry[dimension - 2][volume].size() == before_geometry[dimension - 2][volume].size() * 2);
					for (const int32_t cell : before_geometry[dimension - 2][volume]) {
						CHECK(after_geometry[dimension - 2][volume].has(cell + original_count));
					}
				}
				CHECK(mesh->is_poly_mesh_data_valid());
				const bool valid = mesh->is_mesh_data_valid();
				CHECK(valid);
			}
		}
	}
}

TEST_CASE("[ArrayPolyMeshND] Every indexed binding validates its value range") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		CAPTURE(dimension);
		for (int geometry_dimension = 0; geometry_dimension <= dimension; geometry_dimension++) {
			for (int decomposition_dimension = 0; decomposition_dimension <= geometry_dimension; decomposition_dimension++) {
				const Vector2i key(geometry_dimension, decomposition_dimension);
				CAPTURE(key);
				for (const bool texture : { false, true }) {
					CAPTURE(texture);
					for (const bool negative : { false, true }) {
						CAPTURE(negative);
						Ref<ArrayPolyMeshND> mesh = make_binding_test_mesh(dimension);
						TestMeshDataND::DenseBindings dense;
						dense.insert(key, make_test_binding(mesh, key, dimension - (texture ? 1 : 0)));
						TestMeshDataND::set_all_poly_bindings(mesh, dense, texture);
						REQUIRE(mesh->is_poly_mesh_data_valid());
						HashMap<Vector2i, Vector<PackedInt32Array>> bindings = texture ? mesh->get_all_poly_cell_texture_map_indices() : mesh->get_all_poly_cell_normal_indices();
						Vector<PackedInt32Array> binding = bindings[key];
						REQUIRE_FALSE(binding.is_empty());
						PackedInt32Array indices = binding[0];
						REQUIRE_FALSE(indices.is_empty());
						const int64_t value_count = texture ? mesh->get_poly_cell_texture_map_values().size() : mesh->get_poly_cell_normal_values().size();
						indices.set(0, negative ? -1 : value_count);
						binding.set(0, indices);
						bindings.insert(key, binding);
						if (texture) {
							mesh->set_all_poly_cell_texture_map_indices(bindings);
						} else {
							mesh->set_all_poly_cell_normal_indices(bindings);
						}
						ERR_PRINT_OFF;
						CHECK_FALSE(mesh->is_poly_mesh_data_valid());
						ERR_PRINT_ON;
					}
				}
			}
		}
	}
}
TEST_CASE("[ArrayPolyMeshND] Double sided empty boundary levels are unchanged") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		for (const bool idempotent : { false, true }) {
			Ref<ArrayPolyMeshND> mesh;
			mesh.instantiate();
			mesh->set_poly_cell_vertex_positions(Vector<VectorN>{ VectorND::zero(dimension) });
			Vector<Vector<PackedInt32Array>> poly;
			poly.resize(dimension - 2);
			mesh->set_poly_cell_indices(poly);
			REQUIRE(mesh->is_mesh_data_valid());
			mesh->make_double_sided(idempotent);
			CHECK(mesh->get_poly_cell_indices() == poly);
			CHECK(mesh->get_all_poly_cell_normal_indices().is_empty());
			CHECK(mesh->get_all_poly_cell_texture_map_indices().is_empty());
			CHECK(mesh->get_poly_cell_normal_values().is_empty());
			CHECK(mesh->get_poly_cell_texture_map_values().is_empty());
			CHECK(mesh->get_poly_cell_boundary_pivot_overrides().is_empty());
			CHECK(mesh->get_simplex_cell_vertex_indices().is_empty());
			CHECK(mesh->get_normal_values().is_empty());
			CHECK(mesh->get_texture_map_values().is_empty());
			CHECK(mesh->is_mesh_data_valid());
		}
	}
}

} // namespace TestArrayPolyMeshND
