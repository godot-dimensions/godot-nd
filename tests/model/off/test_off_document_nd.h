#pragma once

#include "../../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../../model/mesh/cell/cell_material_nd.h"
#include "../../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../../model/mesh/poly/poly_material_nd.h"
#include "../../../model/off/off_document_nd.h"

#include "tests/test_macros.h"

namespace TestOFFDocumentND {
static String export_off_to_string(const Ref<OFFDocumentND> &p_off_document) {
	const PackedByteArray exported_bytes = p_off_document->export_save_to_byte_array();
	return String::utf8((const char *)exported_bytes.ptr(), exported_bytes.size());
}

// Builds an N-dimensional cell mesh with two (N-1)-simplex cells that share a facet: the first
// cell uses vertices 0 to N-1, and the second cell uses vertices 1 to N. The vertices are the
// origin followed by the unit vectors of every axis, so all N+1 vertices are distinct.
static Ref<ArrayCellMeshND> make_two_simplex_cell_mesh(const int p_dimension) {
	Ref<ArrayCellMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertex_positions;
	vertex_positions.append(VectorND::zero(p_dimension));
	for (int axis = 0; axis < p_dimension; axis++) {
		vertex_positions.append(VectorND::value_on_axis_with_dimension(1.0, axis, p_dimension));
	}
	mesh->set_vertex_positions(vertex_positions);
	PackedInt32Array cell_vertex_indices;
	for (int index = 0; index < p_dimension; index++) {
		cell_vertex_indices.append(index);
	}
	for (int index = 1; index <= p_dimension; index++) {
		cell_vertex_indices.append(index);
	}
	mesh->set_simplex_cell_vertex_indices(cell_vertex_indices);
	return mesh;
}
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

	const String exported_text = export_off_to_string(imported);
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

TEST_CASE("[OFFDocumentND] Export converts simplex cell meshes into the OFF hierarchy") {
	// 3D: The triangles are the OFF faces, so nothing needs to be built below them.
	{
		Ref<ArrayCellMeshND> mesh = make_two_simplex_cell_mesh(3);
		Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(mesh);
		REQUIRE(off_document.is_valid());
		CHECK(off_document->get_dimension() == 3);
		REQUIRE(off_document->get_cell_face_indices().size() == 1);
		CHECK(off_document->get_cell_face_indices()[0] == Vector<PackedInt32Array>{ PackedInt32Array{ 0, 1, 2 }, PackedInt32Array{ 1, 2, 3 } });
		const PackedStringArray lines = export_off_to_string(off_document).split("\n", false);
		REQUIRE(lines.size() > 2);
		CHECK(lines[0] == "OFF");
		CHECK_MESSAGE(lines[1] == "4 2 5", "Two triangles sharing an edge have 4 vertices, 2 faces, and 5 edges.");
	}
	// 4D: Two tetrahedra sharing a triangle. Each has 4 triangles, one of which is shared.
	{
		Ref<ArrayCellMeshND> mesh = make_two_simplex_cell_mesh(4);
		Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(mesh);
		REQUIRE(off_document.is_valid());
		CHECK(off_document->get_dimension() == 4);
		REQUIRE(off_document->get_cell_face_indices().size() == 2);
		CHECK(off_document->get_cell_face_indices()[0].size() == 7);
		CHECK(off_document->get_cell_face_indices()[1].size() == 2);
		// The first tetrahedron's faces must have the same winding as the 4D module's OFF export.
		const Vector<PackedInt32Array> faces = off_document->get_cell_face_indices()[0];
		CHECK(faces[0] == PackedInt32Array{ 2, 1, 3 });
		CHECK(faces[1] == PackedInt32Array{ 0, 2, 3 });
		CHECK(faces[2] == PackedInt32Array{ 1, 0, 3 });
		CHECK(faces[3] == PackedInt32Array{ 0, 1, 2 });
		CHECK(off_document->get_cell_face_indices()[1][0] == PackedInt32Array{ 0, 1, 2, 3 });
		CHECK_MESSAGE(off_document->get_cell_face_indices()[1][1].has(0), "The second tetrahedron must reuse the shared face instead of duplicating it.");
		const PackedStringArray lines = export_off_to_string(off_document).split("\n", false);
		REQUIRE(lines.size() > 2);
		CHECK(lines[0] == "4OFF");
		CHECK_MESSAGE(lines[1] == "5 7 9 2", "Two tetrahedra sharing a triangle have 5 vertices, 7 faces, 9 edges, and 2 cells.");
		// Without deduplication, every tetrahedron gets its own 4 faces.
		Ref<OFFDocumentND> duplicated = OFFDocumentND::export_convert_mesh_nd(mesh, false);
		REQUIRE(duplicated.is_valid());
		CHECK(duplicated->get_cell_face_indices()[0].size() == 8);
		CHECK(duplicated->get_cell_face_indices()[1].size() == 2);
		CHECK(duplicated->get_edge_count() == 9);
	}
	// 5D: Two 4-simplexes sharing a tetrahedron. Each has 5 tetrahedra and 10 triangles, sharing 1 and 4.
	{
		Ref<ArrayCellMeshND> mesh = make_two_simplex_cell_mesh(5);
		Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(mesh);
		REQUIRE(off_document.is_valid());
		CHECK(off_document->get_dimension() == 5);
		REQUIRE(off_document->get_cell_face_indices().size() == 3);
		CHECK(off_document->get_cell_face_indices()[0].size() == 16);
		CHECK(off_document->get_cell_face_indices()[1].size() == 9);
		CHECK(off_document->get_cell_face_indices()[2].size() == 2);
		const PackedStringArray lines = export_off_to_string(off_document).split("\n", false);
		REQUIRE(lines.size() > 2);
		CHECK(lines[0] == "5OFF");
		CHECK_MESSAGE(lines[1] == "6 16 14 9 2", "Two 4-simplexes sharing a tetrahedron have 6 vertices, 16 faces, 14 edges, 9 cells, and 2 tera.");
	}
}

TEST_CASE("[OFFDocumentND] Exported simplex cell meshes round-trip with per-cell colors") {
	for (int dimension = 3; dimension <= 5; dimension++) {
		Ref<ArrayCellMeshND> mesh = make_two_simplex_cell_mesh(dimension);
		Ref<CellMaterialND> material;
		material.instantiate();
		material->set_albedo_source_flags(MaterialND::COLOR_SOURCE_FLAG_PER_CELL);
		material->set_albedo_color_array(PackedColorArray{ Color(1.0f, 0.0f, 0.0f), Color(0.0f, 0.0f, 1.0f) });
		mesh->set_material(material);
		Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(mesh);
		REQUIRE(off_document.is_valid());
		REQUIRE(off_document->get_cell_colors().size() == off_document->get_cell_face_indices().size());
		CHECK(off_document->get_cell_colors()[off_document->get_cell_colors().size() - 1].size() == 2);

		Ref<OFFDocumentND> reimported = OFFDocumentND::import_load_from_byte_array(off_document->export_save_to_byte_array());
		REQUIRE(reimported.is_valid());
		CHECK(reimported->get_dimension() == dimension);
		CHECK(reimported->get_cell_face_indices().size() == off_document->get_cell_face_indices().size());
		for (int64_t dim = 0; dim < reimported->get_cell_face_indices().size(); dim++) {
			CHECK(reimported->get_cell_face_indices()[dim] == off_document->get_cell_face_indices()[dim]);
		}
		Ref<ArrayCellMeshND> cell_mesh = reimported->import_generate_array_cell_mesh_nd();
		REQUIRE(cell_mesh.is_valid());
		CHECK(cell_mesh->get_dimension() == dimension);
		CHECK(cell_mesh->get_vertex_positions().size() == dimension + 1);
		CHECK_MESSAGE(cell_mesh->get_simplex_cell_count() == 2, "Each exported simplex cell must import as exactly one simplex cell.");
		Ref<MaterialND> reimported_material = cell_mesh->get_material();
		REQUIRE(reimported_material.is_valid());
		CHECK((reimported_material->get_albedo_source_flags() & MaterialND::COLOR_SOURCE_FLAG_PER_CELL) != 0);
		REQUIRE(reimported_material->get_albedo_color_array().size() == 2);
		CHECK(reimported_material->get_albedo_color_array()[0].is_equal_approx(Color(1.0f, 0.0f, 0.0f)));
		CHECK(reimported_material->get_albedo_color_array()[1].is_equal_approx(Color(0.0f, 0.0f, 1.0f)));
	}
}

TEST_CASE("[OFFDocumentND] Export copies the poly mesh hierarchy without the hypervolume") {
	// A 3D cube: 8 vertices, 6 faces, 12 edges, and no cells since the faces are the boundary.
	{
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(3, 1.0));
		Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(box);
		REQUIRE(off_document.is_valid());
		CHECK(off_document->get_dimension() == 3);
		REQUIRE(off_document->get_cell_face_indices().size() == 1);
		CHECK(off_document->get_cell_face_indices()[0].size() == 6);
		const PackedStringArray lines = export_off_to_string(off_document).split("\n", false);
		REQUIRE(lines.size() > 2);
		CHECK(lines[0] == "OFF");
		CHECK(lines[1] == "8 6 12");
	}
	// A 4D tesseract: 16 vertices, 24 faces, 32 edges, 8 cells, and its own volume which OFF does not store.
	{
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(4, 1.0));
		REQUIRE(box->get_poly_cell_indices().size() == 3);
		Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(box);
		REQUIRE(off_document.is_valid());
		CHECK(off_document->get_dimension() == 4);
		REQUIRE(off_document->get_cell_face_indices().size() == 2);
		CHECK(off_document->get_cell_face_indices()[0] == box->get_all_face_vertex_indices());
		CHECK(off_document->get_cell_face_indices()[1] == box->get_poly_cell_indices()[1]);
		const PackedStringArray lines = export_off_to_string(off_document).split("\n", false);
		REQUIRE(lines.size() > 2);
		CHECK(lines[0] == "4OFF");
		CHECK(lines[1] == "16 24 32 8");
		Ref<OFFDocumentND> reimported = OFFDocumentND::import_load_from_byte_array(off_document->export_save_to_byte_array());
		REQUIRE(reimported.is_valid());
		Ref<ArrayCellMeshND> cell_mesh = reimported->import_generate_array_cell_mesh_nd();
		REQUIRE(cell_mesh.is_valid());
		CHECK(cell_mesh->get_dimension() == 4);
		CHECK(cell_mesh->get_vertex_positions().size() == 16);
		CHECK(cell_mesh->get_simplex_cell_count() > 0);
	}
	// A 5D penteract: 32 vertices, 80 faces, 80 edges, 40 cells, 10 tera.
	{
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(5, 1.0));
		Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(box);
		REQUIRE(off_document.is_valid());
		REQUIRE(off_document->get_cell_face_indices().size() == 3);
		const PackedStringArray lines = export_off_to_string(off_document).split("\n", false);
		REQUIRE(lines.size() > 2);
		CHECK(lines[0] == "5OFF");
		CHECK(lines[1] == "32 80 80 40 10");
	}
}

TEST_CASE("[OFFDocumentND] Export uses per-poly-cell colors from poly materials") {
	Ref<BoxPolyMeshND> box;
	box.instantiate();
	box->set_size(VectorND::fill(4, 1.0));
	PackedColorArray poly_colors;
	for (int i = 0; i < 8; i++) {
		poly_colors.append(Color(i / 8.0f, 0.0f, 1.0f - i / 8.0f));
	}
	Ref<PolyMaterialND> material;
	material.instantiate();
	material->set_albedo_source_flags(MaterialND::COLOR_SOURCE_FLAG_PER_CELL);
	material->set_poly_albedo_color_array(poly_colors);
	box->set_material(material);
	Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(box);
	REQUIRE(off_document.is_valid());
	REQUIRE(off_document->get_cell_colors().size() == 2);
	CHECK(off_document->get_cell_colors()[0].is_empty());
	CHECK(off_document->get_cell_colors()[1] == poly_colors);
	const PackedStringArray lines = export_off_to_string(off_document).split("\n", false);
	// The last 8 lines are the cells, each with 6 face indices followed by an RGB color, except the white one.
	REQUIRE(lines.size() > 8);
	const PackedStringArray last_cell_items = lines[lines.size() - 1].split(" ", false);
	CHECK_MESSAGE(last_cell_items.size() == 1 + 6 + 3, "A colored cell line is the face count, the face indices, and 3 color channels.");
	CHECK(last_cell_items[0] == "6");
}

TEST_CASE("[OFFDocumentND] Export rejects meshes below 3 dimensions") {
	Ref<ArrayCellMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertex_positions(Vector<VectorN>{ VectorN{ 0.0, 0.0 }, VectorN{ 1.0, 0.0 }, VectorN{ 0.0, 1.0 } });
	mesh->set_simplex_cell_vertex_indices(PackedInt32Array{ 0, 1, 1, 2 });
	ERR_PRINT_OFF;
	Ref<OFFDocumentND> off_document = OFFDocumentND::export_convert_mesh_nd(mesh);
	ERR_PRINT_ON;
	CHECK(off_document.is_null());
}
} // namespace TestOFFDocumentND
