#pragma once

#include "../../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../../model/mesh/cell/cell_material_nd.h"
#include "../../../model/mesh/mesh_instance_nd.h"
#include "../../../model/mesh/multi_surface_mesh_nd.h"
#include "../../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../../model/mesh/wire/array_wire_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestMeshInstanceND {
TEST_CASE("[MeshInstanceND] Bounds follow mesh data and target transform") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ -1, -1, -1, -1 }, VectorN{ 1, 1, 1, 1 } }));

	MeshInstanceND mesh_instance;
	mesh_instance.set_mesh(mesh);
	const Ref<TransformND> identity = TransformND::identity_transform(4);
	Ref<RectND> bounds = mesh_instance.get_rect_bounds(identity);
	CHECK(VectorND::is_equal_exact(bounds->get_position(), VectorN{ -1, -1, -1, -1 }));
	CHECK(VectorND::is_equal_exact(bounds->get_size(), VectorN{ 2, 2, 2, 2 }));

	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ -2, -2, -2, -2 }, VectorN{ 3, 3, 3, 3 } }));
	bounds = mesh_instance.get_rect_bounds(identity);
	CHECK(VectorND::is_equal_exact(bounds->get_position(), VectorN{ -2, -2, -2, -2 }));
	CHECK(VectorND::is_equal_exact(bounds->get_size(), VectorN{ 5, 5, 5, 5 }));

	const Ref<TransformND> to_target = TransformND::from_position(VectorN{ 10, 20, 30, 40 });
	bounds = mesh_instance.get_rect_bounds(to_target);
	CHECK(VectorND::is_equal_exact(bounds->get_position(), VectorN{ 8, 18, 28, 38 }));
	CHECK(VectorND::is_equal_exact(bounds->get_size(), VectorN{ 5, 5, 5, 5 }));
}

TEST_CASE("[MeshInstanceND] Material overrides apply per surface") {
	Ref<BoxPolyMeshND> box;
	box.instantiate();
	box->set_size(VectorND::fill(4, 1.0));
	Ref<CellMaterialND> red;
	red.instantiate();
	Ref<ArrayCellMeshND> surface_a = box->to_array_cell_mesh();
	surface_a->set_material(red);
	Ref<ArrayCellMeshND> surface_b = box->to_array_cell_mesh();
	Ref<MultiSurfaceMeshND> mesh;
	mesh.instantiate();
	mesh->set_surface_meshes({ surface_a, surface_b });
	MeshInstanceND mesh_instance;
	mesh_instance.set_mesh(mesh);
	// Without overrides, each surface uses its own material, or its fallback material if it has none.
	CHECK(mesh_instance.get_active_material(0) == red);
	CHECK(mesh_instance.get_active_material(1) == surface_b->get_fallback_material());
	CHECK(surface_b->get_fallback_material().is_valid());
	CHECK(mesh_instance.get_active_material(2).is_null());
	// A single override applies to every surface.
	Ref<CellMaterialND> blue;
	blue.instantiate();
	mesh_instance.set_material_override(blue);
	CHECK(mesh_instance.get_material_overrides().size() == 1);
	CHECK(mesh_instance.get_active_material(0) == blue);
	CHECK(mesh_instance.get_active_material(1) == blue);
	// Per-surface overrides, where a null entry falls through to the surface's own material.
	Ref<CellMaterialND> green;
	green.instantiate();
	mesh_instance.set_material_overrides({ Ref<MaterialND>(), green });
	CHECK(mesh_instance.get_material_override().is_null());
	CHECK(mesh_instance.get_active_material(0) == red);
	CHECK(mesh_instance.get_active_material(1) == green);
	// Clearing the single override clears them all.
	mesh_instance.set_material_override(Ref<MaterialND>());
	CHECK(mesh_instance.get_material_overrides().is_empty());
	CHECK(mesh_instance.get_active_material(1) == surface_b->get_fallback_material());
}

TEST_CASE("[MeshInstanceND] Bounds with no mesh set") {
	MeshInstanceND mesh_instance;
	const Ref<TransformND> to_target = TransformND::from_position(VectorN{ 5, 5, 5, 5 });
	const Ref<RectND> bounds = mesh_instance.get_rect_bounds(to_target);
	CHECK(VectorND::is_equal_exact(bounds->get_position(), VectorN{ 5, 5, 5, 5 }));
	CHECK(!bounds->has_any_size());
}
} // namespace TestMeshInstanceND
