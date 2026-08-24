#pragma once

#include "../../math/math_nd.h"
#include "../../math/transform_nd.h"
#include "../../math/vector_nd.h"
#include "../../model/mesh/poly/array_poly_mesh_nd.h"
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
		CHECK(mesh->get_poly_cell_vertices().size() == 2);
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
		CHECK(mesh->get_poly_cell_vertices().size() == 3);
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
		const Vector<Vector<VectorN>> vertex_normals = mesh->get_poly_cell_vertex_normals();
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
		const Vector<VectorN> vertices = mesh->get_poly_cell_vertices();
		const Vector<PackedInt32Array> cell_vertex_indices = mesh->get_all_boundary_cell_vertex_indices(false);
		const Vector<Vector<VectorN>> vertex_normals = mesh->get_poly_cell_vertex_normals();
		REQUIRE(vertex_normals.size() == 8);
		for (int64_t cell_index = 0; cell_index < 8; cell_index++) {
			REQUIRE(vertex_normals[cell_index].size() == cell_vertex_indices[cell_index].size());
			for (int64_t vertex_in_cell = 0; vertex_in_cell < cell_vertex_indices[cell_index].size(); vertex_in_cell++) {
				// Each box vertex is used by cells whose normals are the signed axes matching
				// the vertex's coordinate signs, so the average is the corner diagonal.
				const VectorN expected = VectorND::normalized(vertices[cell_vertex_indices[cell_index][vertex_in_cell]]);
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
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_texture_map();
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
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_texture_map();
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
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_texture_map();
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
		mesh->set_poly_cell_texture_map(Vector<Vector<VectorM>>());
		mesh->unwrap_texture_map_island(PackedInt32Array{ 0 });
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_texture_map();
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
		const Vector<Vector<VectorM>> original = mesh->get_poly_cell_texture_map();
		mesh->transform_texture_map(TransformND::from_position(VectorN{ 10.0, 20.0, 30.0 }));
		const Vector<Vector<VectorM>> transformed = mesh->get_poly_cell_texture_map();
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
		const Vector<VectorN> vertices = mesh->get_poly_cell_vertices();
		REQUIRE(vertices.size() == 4);
		CHECK(VectorND::is_equal_approx(vertices[0], VectorN{ 1.0, 2.0, 3.0, 4.0 }));
		CHECK(VectorND::is_equal_approx(vertices[1], VectorN{ 2.0, 2.0, 3.0, 4.0 }));
		CHECK(mesh->is_poly_mesh_data_valid());
	}
}

TEST_CASE("[ArrayPolyMeshND] Merge meshes") {
	SUBCASE("Merging two tetrahedra with an offset adjusts all indices") {
		Ref<ArrayPolyMeshND> mesh = TestPolyMeshND::make_tetrahedron_cell_mesh();
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		mesh->merge_with(other, TransformND::from_position(VectorN{ 10.0, 0.0, 0.0, 0.0 }));
		const Vector<VectorN> vertices = mesh->get_poly_cell_vertices();
		REQUIRE(vertices.size() == 8);
		CHECK(VectorND::is_equal_approx(vertices[4], VectorN{ 10.0, 0.0, 0.0, 0.0 }));
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
		CHECK(mesh->get_poly_cell_vertices().size() == 7);
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
	CHECK(mesh->get_poly_cell_vertices().size() == 5);
	CHECK(mesh->get_edge_indices().size() == 14);
	CHECK(mesh->get_poly_cell_indices()[0].size() == 5);
	mesh->deduplicate_all_elements();
	CHECK(mesh->get_poly_cell_vertices().size() == 4);
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
		HashMap<Vector2i, Vector<Vector<VectorN>>> all_normals = mesh->get_all_poly_cell_normals();
		REQUIRE(all_normals.has(Vector2i(3, 3)));
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		other->set_all_poly_cell_normals(all_normals);
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
		mesh->set_poly_cell_texture_map(texture_map);
		const TypedDictionary<Vector2i, Array> normals_dict = mesh->get_all_poly_cell_normals_bind();
		const TypedDictionary<Vector2i, Array> texture_maps_dict = mesh->get_all_poly_cell_texture_maps_bind();
		CHECK_MESSAGE(normals_dict.has(Vector2i(3, 3)), "The boundary normals of a 4D mesh use the geometry dimension 3 key.");
		CHECK_MESSAGE(texture_maps_dict.has(Vector2i(3, 0)), "The texture map of a 4D mesh uses the cell-to-vertex decomposition key.");
		Ref<ArrayPolyMeshND> other = TestPolyMeshND::make_tetrahedron_cell_mesh();
		other->set_all_poly_cell_normals_bind(normals_dict);
		other->set_all_poly_cell_texture_maps_bind(texture_maps_dict);
		const Vector<VectorN> round_trip_normals = other->get_poly_cell_boundary_normals();
		REQUIRE(round_trip_normals.size() == 1);
		CHECK(VectorND::is_equal_approx(round_trip_normals[0], mesh->get_poly_cell_boundary_normals()[0]));
		const Vector<Vector<VectorM>> round_trip_texture_map = other->get_poly_cell_texture_map();
		REQUIRE(round_trip_texture_map.size() == 1);
		REQUIRE(round_trip_texture_map[0].size() == 4);
		CHECK(VectorND::is_equal_approx(round_trip_texture_map[0][1], VectorM{ 1.0, 0.0, 0.0 }));
	}
#endif // GODOT_HAS_TYPED_DICTIONARY

	SUBCASE("Geometry round trip") {
		Ref<ArrayPolyMeshND> copy;
		copy.instantiate();
		copy->set_poly_cell_vertices(mesh->get_poly_cell_vertices());
		copy->set_edge_vertex_indices(mesh->get_edge_indices());
		copy->set_poly_cell_indices(mesh->get_poly_cell_indices());
		CHECK((copy->get_edge_indices() == mesh->get_edge_indices()));
		CHECK(copy->get_poly_cell_indices().size() == 2);
		CHECK(copy->is_poly_mesh_data_valid());
	}
}
} // namespace TestArrayPolyMeshND
