#pragma once

#include "../../../model/off/off_document_nd.h"

#include "tests/test_macros.h"

namespace TestOFFDocumentND {
// A 4D 5-cell (4-simplex): 5 vertices, 10 edges, 10 triangular faces, and 5 tetrahedral cells.
// Each face lists 3 vertex indices, and each cell lists the 4 face indices that bound it.
static const char *FIVE_CELL_OFF_TEXT =
		"4OFF\n"
		"5 10 10 5\n"
		"0 0 0 0\n"
		"1 0 0 0\n"
		"0 1 0 0\n"
		"0 0 1 0\n"
		"0 0 0 1\n"
		"3 0 1 2\n"
		"3 0 1 3\n"
		"3 0 1 4\n"
		"3 0 2 3\n"
		"3 0 2 4\n"
		"3 0 3 4\n"
		"3 1 2 3\n"
		"3 1 2 4\n"
		"3 1 3 4\n"
		"3 2 3 4\n"
		"4 0 1 3 6\n"
		"4 0 2 4 7\n"
		"4 1 2 5 8\n"
		"4 3 4 5 9\n"
		"4 6 7 8 9\n";

TEST_CASE("[OFFDocumentND] Export writes the face count and round-trips through import") {
	Ref<OFFDocumentND> imported = OFFDocumentND::import_load_from_byte_array(String(FIVE_CELL_OFF_TEXT).to_utf8_buffer());
	REQUIRE(imported.is_valid());
	REQUIRE(imported->get_vertex_positions().size() == 5);
	REQUIRE(imported->get_cell_face_indices().size() == 2);
	CHECK(imported->get_cell_face_indices()[0].size() == 10);
	CHECK(imported->get_cell_face_indices()[1].size() == 5);
	CHECK(imported->get_edge_count() == 10);

	const PackedByteArray exported_bytes = imported->export_save_to_byte_array();
	const String exported_text = String::utf8((const char *)exported_bytes.ptr(), exported_bytes.size());
	const PackedStringArray exported_lines = exported_text.split("\n", false);
	REQUIRE(exported_lines.size() > 2);
	CHECK(exported_lines[0] == "4OFF");
	CHECK_MESSAGE(exported_lines[1] == "5 10 10 5", "The size line must be vertex, face, edge, and then higher cell counts, not the number of cell dimension levels.");

	Ref<OFFDocumentND> reimported = OFFDocumentND::import_load_from_byte_array(exported_text.to_utf8_buffer());
	REQUIRE(reimported.is_valid());
	REQUIRE(reimported->get_vertex_positions().size() == 5);
	for (int i = 0; i < 5; i++) {
		CHECK(VectorND::is_equal_approx(reimported->get_vertex_positions()[i], imported->get_vertex_positions()[i]));
	}
	REQUIRE(reimported->get_cell_face_indices().size() == 2);
	CHECK(reimported->get_cell_face_indices()[0].size() == 10);
	CHECK(reimported->get_cell_face_indices()[1].size() == 5);
	CHECK(reimported->get_edge_count() == 10);
	for (int dim = 0; dim < 2; dim++) {
		for (int i = 0; i < reimported->get_cell_face_indices()[dim].size(); i++) {
			CHECK(reimported->get_cell_face_indices()[dim][i] == imported->get_cell_face_indices()[dim][i]);
		}
	}

	Ref<ArrayCellMeshND> cell_mesh = reimported->import_generate_array_cell_mesh_nd();
	REQUIRE(cell_mesh.is_valid());
	CHECK(cell_mesh->get_dimension() == 4);
	CHECK(cell_mesh->get_vertex_positions().size() == 5);
	// 5 tetrahedral cells, each a single 3-simplex with 4 vertex indices.
	CHECK(cell_mesh->get_simplex_cell_vertex_indices().size() == 20);
	CHECK(cell_mesh->get_simplex_cell_count() == 5);
}
} // namespace TestOFFDocumentND
