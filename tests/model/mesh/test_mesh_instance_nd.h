#pragma once

#include "../../../model/mesh/mesh_instance_nd.h"
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

TEST_CASE("[MeshInstanceND] Bounds with no mesh set") {
	MeshInstanceND mesh_instance;
	const Ref<TransformND> to_target = TransformND::from_position(VectorN{ 5, 5, 5, 5 });
	const Ref<RectND> bounds = mesh_instance.get_rect_bounds(to_target);
	CHECK(VectorND::is_equal_exact(bounds->get_position(), VectorN{ 5, 5, 5, 5 }));
	CHECK(!bounds->has_any_size());
}
} // namespace TestMeshInstanceND
