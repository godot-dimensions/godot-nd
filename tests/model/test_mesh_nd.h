#pragma once

#include "../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../model/mesh/wire/array_wire_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestMeshND {
TEST_CASE("[MeshND] Rect bounds include the local origin") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertices(Vector<VectorN>({ VectorN{ 1, 1, 1, 1 }, VectorN{ 3, 3, 3, 3 } }));
	// Even though every vertex is offset from the origin, the bounds must still include it.
	const Ref<RectND> bounds = mesh->get_rect_bounds();
	CHECK_MESSAGE(VectorND::is_equal_exact(bounds->get_position(), VectorN{ 0, 0, 0, 0 }), "MeshND get_rect_bounds should always include the mesh's local origin.");
	CHECK_MESSAGE(VectorND::is_equal_exact(bounds->get_end(), VectorN{ 3, 3, 3, 3 }), "MeshND get_rect_bounds should still reach every vertex.");
}

TEST_CASE("[ArrayWireMeshND] Bounds cache invalidation on merge") {
	Ref<ArrayWireMeshND> mesh1;
	mesh1.instantiate();
	mesh1->set_vertices(Vector<VectorN>({ VectorN{ -1, -1, -1, -1 }, VectorN{ 1, 1, 1, 1 } }));
	mesh1->append_edge_indices(0, 1);

	Ref<RectND> bounds1 = mesh1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds1->get_position(), VectorN{ -1, -1, -1, -1 }));
	CHECK(VectorND::is_equal_exact(bounds1->get_size(), VectorN{ 2, 2, 2, 2 }));

	// Create a second mesh with vertices outside the first mesh's bounds.
	Ref<ArrayWireMeshND> mesh2;
	mesh2.instantiate();
	mesh2->set_vertices(Vector<VectorN>({ VectorN{ 5, 5, 5, 5 }, VectorN{ 10, 10, 10, 10 } }));
	mesh2->append_edge_indices(0, 1);

	mesh1->merge_with(mesh2, TransformND::identity_transform(4));

	// Bounds should expand to include the merged vertices.
	Ref<RectND> bounds_after_merge = mesh1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_position(), VectorN{ -1, -1, -1, -1 }));
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_end(), VectorN{ 10, 10, 10, 10 }));
}

TEST_CASE("[ArrayCellMeshND] Bounds cache invalidation on merge") {
	Ref<ArrayCellMeshND> cell1;
	cell1.instantiate();
	cell1->set_vertices(Vector<VectorN>({ VectorN{ 0, 0, 0 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 0, 1 } }));
	cell1->set_simplex_cell_indices(PackedInt32Array({ 0, 1, 2, 3 }));

	Ref<RectND> bounds1 = cell1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds1->get_position(), VectorN{ 0, 0, 0 }));
	CHECK(VectorND::is_equal_exact(bounds1->get_end(), VectorN{ 1, 1, 1 }));

	// Create a second cell mesh with vertices outside the first mesh's bounds.
	Ref<ArrayCellMeshND> cell2;
	cell2.instantiate();
	cell2->set_vertices(Vector<VectorN>({ VectorN{ 5, 5, 5 }, VectorN{ 6, 5, 5 }, VectorN{ 5, 6, 5 }, VectorN{ 5, 5, 6 } }));
	cell2->set_simplex_cell_indices(PackedInt32Array({ 0, 1, 2, 3 }));

	cell1->merge_with(cell2, TransformND::identity_transform(3));

	// Bounds should expand to include the merged vertices.
	Ref<RectND> bounds_after_merge = cell1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_position(), VectorN{ 0, 0, 0 }));
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_end(), VectorN{ 6, 6, 6 }));
}

TEST_CASE("[MeshND] Cross-section mesh is lazily created and cached") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_name("TestMesh");
	mesh->set_vertices(Vector<VectorN>({ VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 } }));

	const Ref<ArrayMesh> cross_section = mesh->get_cross_section_mesh();
	CHECK_MESSAGE(cross_section.is_valid(), "MeshND get_cross_section_mesh should lazily instantiate the cross-section mesh.");
	CHECK_MESSAGE(cross_section->get_name() == "TestMesh Cross-Section Mesh", "MeshND get_cross_section_mesh should name the mesh after the resource it came from.");

	// The same instance must be handed out on subsequent calls, both while clean and after being marked dirty.
	CHECK_MESSAGE(mesh->get_cross_section_mesh() == cross_section, "MeshND get_cross_section_mesh should return the same instance while the cache is clean.");
	mesh->set_vertices(Vector<VectorN>({ VectorN{ 2, 2, 2, 2 }, VectorN{ 3, 3, 3, 3 } }));
	CHECK_MESSAGE(mesh->get_cross_section_mesh() == cross_section, "MeshND get_cross_section_mesh should reuse the same instance after being marked dirty.");
}

TEST_CASE("[MeshND] Bounds cache persists across multiple accesses") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertices(Vector<VectorN>({ VectorN{ -2, -2, -2, -2 }, VectorN{ 3, 3, 3, 3 } }));

	// Access bounds multiple times - should use the cached value.
	Ref<RectND> bounds1 = mesh->get_rect_bounds();
	Ref<RectND> bounds2 = mesh->get_rect_bounds();
	Ref<RectND> bounds3 = mesh->get_rect_bounds();

	CHECK(bounds1 == bounds2);
	CHECK(bounds2 == bounds3);
	CHECK(VectorND::is_equal_exact(bounds1->get_position(), VectorN{ -2, -2, -2, -2 }));
	CHECK(VectorND::is_equal_exact(bounds1->get_end(), VectorN{ 3, 3, 3, 3 }));

	// Changing the vertices must invalidate the cache and produce a new Ref.
	mesh->set_vertices(Vector<VectorN>({ VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 } }));
	Ref<RectND> bounds_after_change = mesh->get_rect_bounds();
	CHECK(bounds_after_change != bounds1);
	CHECK(VectorND::is_equal_exact(bounds_after_change->get_end(), VectorN{ 1, 1, 1, 1 }));
}
} // namespace TestMeshND
