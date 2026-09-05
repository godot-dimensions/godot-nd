#pragma once

#include "../../../../model/mesh/wire/box_wire_mesh_nd.h"
#include "../../../../model/mesh/wire/wire_mesh_builder_nd.h"

#include "tests/test_macros.h"

namespace TestWireMeshBuilderND {
TEST_CASE("[WireMeshBuilderND] Extrude linear") {
	SUBCASE("Extruding a box gives a box one dimension higher") {
		for (int dimension = 2; dimension <= 4; dimension++) {
			Ref<BoxWireMeshND> box;
			box.instantiate();
			box->set_size(VectorND::fill(dimension, 1.0));
			Ref<ArrayWireMeshND> array_box;
			array_box.instantiate();
			array_box->set_vertex_positions(box->get_vertex_positions());
			array_box->set_edge_indices(box->get_edge_indices());
			const VectorN extrusion_vector = VectorND::value_on_axis_with_dimension(1.0, dimension, dimension + 1);
			Ref<ArrayWireMeshND> extruded = WireMeshBuilderND::extrude_linear(array_box, extrusion_vector);
			REQUIRE(extruded.is_valid());
			const Vector<VectorN> vertex_positions = extruded->get_vertex_positions();
			CHECK_MESSAGE(vertex_positions.size() == (int64_t(2) << dimension), "The extruded box must have two copies of the input vertices.");
			CHECK_MESSAGE(extruded->is_mesh_data_valid(), "The extruded box must have valid wire mesh data.");
			for (const VectorN &vertex : vertex_positions) {
				CHECK_MESSAGE(vertex.size() == dimension + 1, "The extruded box's vertices must have one more dimension than the input.");
			}
		}
	}
	SUBCASE("Extruding a 2D square with an explicit vector gives a 3D cube") {
		Ref<ArrayWireMeshND> square;
		square.instantiate();
		square->append_edge_points(VectorN{ -0.5, -0.5 }, VectorN{ 0.5, -0.5 });
		square->append_edge_points(VectorN{ 0.5, -0.5 }, VectorN{ 0.5, 0.5 });
		square->append_edge_points(VectorN{ 0.5, 0.5 }, VectorN{ -0.5, 0.5 });
		square->append_edge_points(VectorN{ -0.5, 0.5 }, VectorN{ -0.5, -0.5 });
		Ref<ArrayWireMeshND> extruded = WireMeshBuilderND::extrude_linear(square, VectorN{ 0.0, 0.0, 0.5 });
		REQUIRE(extruded.is_valid());
		const Vector<VectorN> vertex_positions = extruded->get_vertex_positions();
		REQUIRE(vertex_positions.size() == 8);
		CHECK(VectorND::is_equal_approx(vertex_positions[0], VectorN{ -0.5, -0.5, -0.5 }));
		CHECK(VectorND::is_equal_approx(vertex_positions[4], VectorN{ -0.5, -0.5, 0.5 }));
		const PackedInt32Array edge_indices = extruded->get_edge_indices();
		CHECK_MESSAGE(edge_indices.size() == (4 + 4 + 4) * 2, "The extruded cube must have 4 edges per copy of the square plus 4 connecting edges.");
		CHECK(extruded->is_mesh_data_valid());
	}
	SUBCASE("Extruding an invalid mesh returns an empty mesh") {
		Ref<ArrayWireMeshND> invalid;
		invalid.instantiate();
		invalid->set_edge_indices(PackedInt32Array{ 0, 1 }); // References vertices that do not exist.
		ERR_PRINT_OFF; // The invalid mesh prints errors, which are expected here.
		Ref<ArrayWireMeshND> extruded = WireMeshBuilderND::extrude_linear(invalid);
		ERR_PRINT_ON;
		REQUIRE(extruded.is_valid());
		CHECK(extruded->get_vertex_positions().is_empty());
	}
}
} // namespace TestWireMeshBuilderND
