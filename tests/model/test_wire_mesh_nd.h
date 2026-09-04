#pragma once

#include "../../math/vector_nd.h"
#include "../../model/mesh/wire/array_wire_mesh_nd.h"
#include "../../model/mesh/wire/box_wire_mesh_nd.h"
#include "../../model/mesh/wire/orthoplex_wire_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestWireMeshND {
TEST_CASE("[ArrayWireMeshND] Append Edge Points") {
	Ref<ArrayWireMeshND> array_wire_mesh;
	array_wire_mesh.instantiate();
	array_wire_mesh->append_edge_points(VectorN{ 0, 0, 0 }, VectorN{ 1, 0, 0 });
	array_wire_mesh->append_edge_points(VectorN{ 0, 0, 0 }, VectorN{ 0, 1, 0 });
	array_wire_mesh->append_edge_points(VectorN{ 0, 0, 0 }, VectorN{ 0, 0, 1 });
	{
		const Vector<VectorN> vertex_positions = array_wire_mesh->get_vertex_positions();
		const PackedInt32Array edge_indices = array_wire_mesh->get_edge_indices();
		CHECK(vertex_positions.size() == 4);
		CHECK(edge_indices.size() == 6);
		const PackedInt32Array correct_edge_indices = { 0, 1, 0, 2, 0, 3 };
		CHECK(edge_indices == correct_edge_indices);
		const Vector<VectorN> correct_edge_positions = { VectorN{ 0, 0, 0 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 0, 0 }, VectorN{ 0, 0, 1 } };
		CHECK(VectorND::is_equal_exact_array(array_wire_mesh->get_edge_positions(), correct_edge_positions));
	}
	const bool deduplicate = false;
	array_wire_mesh->append_edge_points(VectorN{ 0, 0, 0 }, VectorN{ 1, 1, 1 }, deduplicate);
	{
		const Vector<VectorN> vertex_positions = array_wire_mesh->get_vertex_positions();
		const PackedInt32Array edge_indices = array_wire_mesh->get_edge_indices();
		CHECK(vertex_positions.size() == 6);
		CHECK(edge_indices.size() == 8);
		const PackedInt32Array correct_edge_indices = { 0, 1, 0, 2, 0, 3, 4, 5 };
		CHECK(edge_indices == correct_edge_indices);
		const Vector<VectorN> correct_edge_positions = { VectorN{ 0, 0, 0 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 0, 0 }, VectorN{ 0, 0, 1 }, VectorN{ 0, 0, 0 }, VectorN{ 1, 1, 1 } };
		CHECK(VectorND::is_equal_exact_array(array_wire_mesh->get_edge_positions(), correct_edge_positions));
	}
}

TEST_CASE("[BoxWireMeshND] Edges and Vertices") {
	Ref<BoxWireMeshND> box_wire_mesh;
	box_wire_mesh.instantiate();
	box_wire_mesh->set_size(VectorN{ 1, 2, 3 });
	const Vector<VectorN> vertex_positions = box_wire_mesh->get_vertex_positions();
	const PackedInt32Array edge_indices = box_wire_mesh->get_edge_indices();
	CHECK(vertex_positions.size() == 8);
	CHECK(edge_indices.size() == 24);
	CHECK(VectorND::is_equal_exact(box_wire_mesh->get_half_extents(), VectorN{ 0.5, 1, 1.5 }));
	CHECK(VectorND::is_equal_exact(box_wire_mesh->get_size(), VectorN{ 1, 2, 3 }));
	const PackedInt32Array correct_edge_indices = { 0, 1, 0, 2, 0, 4, 1, 3, 1, 5, 2, 3, 2, 6, 3, 7, 4, 5, 4, 6, 5, 7, 6, 7 };
	CHECK(edge_indices == correct_edge_indices);
}

TEST_CASE("[ArrayWireMeshND] Transform vertices") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->append_edge_points(VectorN{ 0.0, 0.0, 0.0 }, VectorN{ 1.0, 0.0, 0.0 });
	mesh->transform_vertices(TransformND::from_position(VectorN{ 1.0, 2.0, 3.0 }));
	const Vector<VectorN> vertex_positions = mesh->get_vertex_positions();
	REQUIRE(vertex_positions.size() == 2);
	CHECK(VectorND::is_equal_approx(vertex_positions[0], VectorN{ 1.0, 2.0, 3.0 }));
	CHECK(VectorND::is_equal_approx(vertex_positions[1], VectorN{ 2.0, 2.0, 3.0 }));
	CHECK(mesh->is_mesh_data_valid());
}

TEST_CASE("[ArrayWireMeshND] Merge meshes") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->append_edge_points(VectorN{ 0.0, 0.0, 0.0 }, VectorN{ 1.0, 0.0, 0.0 });
	Ref<ArrayWireMeshND> other;
	other.instantiate();
	other->append_edge_points(VectorN{ 0.0, 0.0, 0.0 }, VectorN{ 1.0, 0.0, 0.0 });
	mesh->merge_with(other, TransformND::from_position(VectorN{ 10.0, 0.0, 0.0 }));
	const Vector<VectorN> vertex_positions = mesh->get_vertex_positions();
	REQUIRE(vertex_positions.size() == 4);
	CHECK(VectorND::is_equal_approx(vertex_positions[2], VectorN{ 10.0, 0.0, 0.0 }));
	CHECK(VectorND::is_equal_approx(vertex_positions[3], VectorN{ 11.0, 0.0, 0.0 }));
	const PackedInt32Array edge_indices = mesh->get_edge_indices();
	const PackedInt32Array correct_edge_indices = { 0, 1, 2, 3 };
	CHECK(edge_indices == correct_edge_indices);
	CHECK(mesh->is_mesh_data_valid());
}

TEST_CASE("[ArrayWireMeshND] Deduplicate all elements") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->append_edge_points(VectorN{ 0.0, 0.0, 0.0 }, VectorN{ 1.0, 0.0, 0.0 });
	mesh->append_edge_points(VectorN{ 0.0, 0.0, 0.0 }, VectorN{ 0.0, 1.0, 0.0 });
	mesh->append_vertex(VectorN{ 0.0, 0.0, 0.0 }, false); // Duplicate of vertex 0.
	mesh->append_edge_indices(1, 3); // Becomes a duplicate of edge 0 (0, 1) after vertex dedup.
	CHECK(mesh->get_vertex_positions().size() == 4);
	CHECK(mesh->get_edge_indices().size() == 6);
	mesh->deduplicate_all_elements();
	CHECK(mesh->get_vertex_positions().size() == 3);
	const PackedInt32Array edge_indices = mesh->get_edge_indices();
	const PackedInt32Array correct_edge_indices = { 0, 1, 0, 2 };
	CHECK(edge_indices == correct_edge_indices);
	CHECK(mesh->is_mesh_data_valid());
}

TEST_CASE("[OrthoplexWireMeshND] Edges and Vertices") {
	Ref<OrthoplexWireMeshND> orthoplex_wire_mesh;
	orthoplex_wire_mesh.instantiate();
	orthoplex_wire_mesh->set_size(VectorN{ 1, 2, 3 });
	const Vector<VectorN> vertex_positions = orthoplex_wire_mesh->get_vertex_positions();
	const PackedInt32Array edge_indices = orthoplex_wire_mesh->get_edge_indices();
	CHECK(vertex_positions.size() == 6);
	CHECK(edge_indices.size() == 24);
	CHECK(VectorND::is_equal_exact(orthoplex_wire_mesh->get_half_extents(), VectorN{ 0.5, 1, 1.5 }));
	CHECK(VectorND::is_equal_exact(orthoplex_wire_mesh->get_size(), VectorN{ 1, 2, 3 }));
	const PackedInt32Array correct_edge_indices = { 0, 2, 0, 3, 1, 2, 1, 3, 0, 4, 0, 5, 1, 4, 1, 5, 2, 4, 2, 5, 3, 4, 3, 5 };
	CHECK(edge_indices == correct_edge_indices);
}
} // namespace TestWireMeshND
