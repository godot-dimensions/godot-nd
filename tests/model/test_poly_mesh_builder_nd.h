#pragma once

#include "../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../model/mesh/poly/poly_mesh_builder_nd.h"

#include "tests/test_macros.h"

namespace TestPolyMeshBuilderND {
// Makes a 3D ArrayMesh of a unit square facing +Z, made of two triangles, with normals and UVs.
inline Ref<ArrayMesh> make_quad_array_mesh_3d() {
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	PackedVector3Array vertices = { Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(1, 1, 0), Vector3(0, 1, 0) };
	PackedVector3Array normals = { Vector3(0, 0, 1), Vector3(0, 0, 1), Vector3(0, 0, 1), Vector3(0, 0, 1) };
	PackedVector2Array uvs = { Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1) };
	// Godot 3D uses clockwise winding order for front faces.
	PackedInt32Array indices = { 0, 2, 1, 0, 3, 2 };
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_INDEX] = indices;
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	return mesh;
}

TEST_CASE("[SceneTree][PolyMeshBuilderND] Convert 3D mesh to ND faces only") {
	Ref<ArrayPolyMeshND> converted = PolyMeshBuilderND::convert_mesh_3d_to_nd_faces_only(make_quad_array_mesh_3d());
	REQUIRE(converted.is_valid());
	CHECK_MESSAGE(converted->get_dimension() == 3, "The converted mesh must be 3-dimensional.");
	CHECK_MESSAGE(converted->is_poly_mesh_data_valid(), "The converted mesh must have valid poly mesh data.");
	CHECK_MESSAGE(converted->get_poly_cell_vertices().size() == 4, "The vertices of the two triangles must be deduplicated.");
	CHECK_MESSAGE(converted->get_edge_indices().size() == 5 * 2, "The quad must have 4 outer edges and 1 shared diagonal edge.");
	const Vector<Vector<PackedInt32Array>> poly_cell_indices = converted->get_poly_cell_indices();
	REQUIRE(poly_cell_indices.size() == 1);
	CHECK_MESSAGE(poly_cell_indices[0].size() == 2, "The quad must convert into 2 triangular faces.");
	// For a 3-dimensional mesh, the faces are the boundary cells, so the face normals are boundary normals.
	const VectorN pos_z = VectorN{ 0.0, 0.0, 1.0 };
	const Vector<VectorN> boundary_normals = converted->get_poly_cell_boundary_normals();
	REQUIRE(boundary_normals.size() == 2);
	CHECK_MESSAGE(VectorND::is_equal_approx(boundary_normals[0], pos_z), "The +Z facing quad must convert with +Z boundary normals.");
	CHECK_MESSAGE(VectorND::is_equal_approx(boundary_normals[1], pos_z), "The +Z facing quad must convert with +Z boundary normals.");
	// The face orientations must reproduce the stored normals, so the data is internally consistent.
	Ref<ArrayPolyMeshND> recalculated = converted->duplicate();
	recalculated->set_poly_cell_boundary_normals(Vector<VectorN>());
	recalculated->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
	const Vector<VectorN> orientation_normals = recalculated->get_poly_cell_boundary_normals();
	REQUIRE(orientation_normals.size() == 2);
	CHECK_MESSAGE(VectorND::is_equal_approx(orientation_normals[0], pos_z), "The face orientations must reproduce the stored normals.");
	CHECK_MESSAGE(VectorND::is_equal_approx(orientation_normals[1], pos_z), "The face orientations must reproduce the stored normals.");
	// The vertex normals and texture map must carry through from the 3D mesh data.
	const Vector<Vector<VectorN>> vertex_normals = converted->get_poly_cell_vertex_normals();
	REQUIRE(vertex_normals.size() == 2);
	for (int64_t face_index = 0; face_index < vertex_normals.size(); face_index++) {
		REQUIRE(vertex_normals[face_index].size() == 3);
		for (int64_t vert_index = 0; vert_index < 3; vert_index++) {
			// ArrayMesh stores normals with octahedral compression, so allow a loose tolerance.
			CHECK_MESSAGE(VectorND::distance_to(vertex_normals[face_index][vert_index], pos_z) < 0.0001, "The vertex normals must carry through from the 3D mesh data.");
		}
	}
	const Vector<Vector<VectorM>> texture_map = converted->get_poly_cell_texture_map();
	REQUIRE(texture_map.size() == 2);
	for (int64_t face_index = 0; face_index < texture_map.size(); face_index++) {
		REQUIRE(texture_map[face_index].size() == 3);
		for (int64_t vert_index = 0; vert_index < 3; vert_index++) {
			const VectorM texcoord = texture_map[face_index][vert_index];
			REQUIRE_MESSAGE(texcoord.size() == 2, "The texture map of a 3-dimensional mesh must have 2-dimensional coordinates.");
			// The quad's UVs match the XY coordinates of its vertices.
			const int32_t vertex_index = converted->get_all_face_vertex_indices()[face_index][vert_index];
			const VectorN vertex = converted->get_poly_cell_vertices()[vertex_index];
			CHECK_MESSAGE(texcoord[0] == doctest::Approx(vertex[0]), "The texture map coordinates must carry through from the 3D mesh UVs.");
			CHECK_MESSAGE(texcoord[1] == doctest::Approx(vertex[1]), "The texture map coordinates must carry through from the 3D mesh UVs.");
		}
	}
}

TEST_CASE("[SceneTree][PolyMeshBuilderND] Extrude linear") {
	SUBCASE("Extruding a converted flat quad adds a dimension and carries data over") {
		Ref<ArrayPolyMeshND> converted = PolyMeshBuilderND::convert_mesh_3d_to_nd_faces_only(make_quad_array_mesh_3d());
		// The default extrusion vector is one unit along a new axis one dimension above the input.
		Ref<ArrayPolyMeshND> extruded = PolyMeshBuilderND::extrude_linear(converted);
		REQUIRE(extruded.is_valid());
		CHECK_MESSAGE(extruded->get_dimension() == 4, "Extruding a 3-dimensional mesh by default must give a 4-dimensional mesh.");
		CHECK_MESSAGE(extruded->is_poly_mesh_data_valid(), "The extruded mesh must have valid poly mesh data.");
		CHECK_MESSAGE(extruded->get_poly_cell_vertices().size() == 8, "The extruded quad must have two copies of the input vertices.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = extruded->get_poly_cell_indices();
		REQUIRE(poly_cell_indices.size() == 2);
		CHECK_MESSAGE(poly_cell_indices[0].size() == 2 * 2 + 5, "The extruded quad must have two copies of the faces plus one face per input edge.");
		CHECK_MESSAGE(poly_cell_indices[1].size() == 2, "The extruded quad must have one 3D cell per input face.");
		// The input face normals must carry over to the extruded boundary cells.
		const VectorN pos_z_4d = VectorN{ 0.0, 0.0, 1.0, 0.0 };
		const Vector<VectorN> boundary_normals = extruded->get_poly_cell_boundary_normals();
		REQUIRE(boundary_normals.size() == 2);
		CHECK_MESSAGE(VectorND::is_equal_approx(boundary_normals[0], pos_z_4d), "The input face normals must carry over to the extruded boundary cells.");
		CHECK_MESSAGE(VectorND::is_equal_approx(boundary_normals[1], pos_z_4d), "The input face normals must carry over to the extruded boundary cells.");
		// All faces present at the time of merging must be marked as seams.
		CHECK_MESSAGE(extruded->get_seam_indices().size() == 4, "Both copies of the input faces must be marked as seams.");
		// The vertex normals and texture maps must be transferred to the extruded cells.
		const HashMap<Vector2i, Vector<Vector<VectorN>>> all_normals = extruded->get_all_poly_cell_normals();
		CHECK_MESSAGE(all_normals.has(Vector2i(3, 0)), "The extruded cells must have vertex normals transferred from the input faces.");
		const HashMap<Vector2i, Vector<Vector<VectorM>>> all_texture_maps = extruded->get_all_poly_cell_texture_maps();
		REQUIRE_MESSAGE(all_texture_maps.has(Vector2i(3, 0)), "The extruded cells must have texture maps transferred from the input faces.");
		REQUIRE_MESSAGE(all_texture_maps.has(Vector2i(2, 0)), "The face texture maps must be doubled to cover both copies of the input faces.");
		const Vector<Vector<VectorM>> face_texture_maps = all_texture_maps[Vector2i(2, 0)];
		REQUIRE(face_texture_maps.size() == 4);
		for (int64_t vert_index = 0; vert_index < face_texture_maps[2].size(); vert_index++) {
			REQUIRE(face_texture_maps[2][vert_index].size() == 3);
			CHECK_MESSAGE(face_texture_maps[2][vert_index][2] == doctest::Approx(1.0), "The second copy of the face texture maps must be offset by one unit in the new texture axis.");
		}
	}
	SUBCASE("Extruding a solid box gives a box one dimension higher") {
		for (int dimension = 3; dimension <= 4; dimension++) {
			Ref<BoxPolyMeshND> box;
			box.instantiate();
			box->set_size(VectorND::fill(dimension, 1.0));
			Ref<ArrayPolyMeshND> extruded = PolyMeshBuilderND::extrude_linear(box->to_array_poly_mesh());
			REQUIRE(extruded.is_valid());
			CHECK_MESSAGE(extruded->get_dimension() == dimension + 1, "Extruding a box must give a mesh one dimension higher.");
			CHECK_MESSAGE(extruded->is_poly_mesh_data_valid(), "The extruded box must have valid poly mesh data.");
			CHECK_MESSAGE(extruded->get_poly_cell_vertices().size() == (int64_t(2) << dimension), "The extruded box must have two copies of the input vertices.");
			const Vector<Vector<PackedInt32Array>> poly_cell_indices = extruded->get_poly_cell_indices();
			REQUIRE(poly_cell_indices.size() == dimension);
			CHECK_MESSAGE(poly_cell_indices[dimension - 2].size() == 2 * (dimension + 1), "The extruded box must have the boundary cell count of a box one dimension higher.");
			CHECK_MESSAGE(poly_cell_indices[dimension - 1].size() == 1, "The extruded box must have a single volumetric cell.");
			CHECK_MESSAGE(poly_cell_indices[dimension - 1][0].size() == 2 * (dimension + 1), "The volumetric cell of the extruded box must contain all boundary cells.");
			// Every boundary normal must be an outward-facing unit vector along an axis.
			const Vector<VectorN> boundary_normals = extruded->get_poly_cell_boundary_normals();
			const Vector<PackedInt32Array> boundary_vert = extruded->get_all_poly_cell_vertex_indices(dimension, false);
			const Vector<VectorN> vertices = extruded->get_poly_cell_vertices();
			REQUIRE(boundary_normals.size() == 2 * (dimension + 1));
			REQUIRE(boundary_vert.size() == boundary_normals.size());
			for (int64_t cell_index = 0; cell_index < boundary_normals.size(); cell_index++) {
				const VectorN normal = boundary_normals[cell_index];
				CHECK_MESSAGE(VectorND::length(normal) == doctest::Approx(1.0), "The extruded box's boundary normals must be unit length.");
				VectorN center;
				for (int64_t i = 0; i < boundary_vert[cell_index].size(); i++) {
					center = VectorND::add(center, vertices[boundary_vert[cell_index][i]]);
				}
				center = VectorND::divide_scalar(center, boundary_vert[cell_index].size());
				CHECK_MESSAGE(VectorND::dot(normal, center) > 0.0, "The extruded box's boundary normals must point outward.");
			}
		}
	}
	SUBCASE("Extruding a 2D square with an explicit vector gives a 3D cube") {
		// A 2-dimensional square with 2D vertices, 4 edges, and 1 face, without any normals.
		Ref<ArrayPolyMeshND> square;
		square.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ -0.5, -0.5 },
			VectorN{ 0.5, -0.5 },
			VectorN{ -0.5, 0.5 },
			VectorN{ 0.5, 0.5 },
		};
		square->set_poly_cell_vertices(vertices);
		square->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 0, 2, 1, 3, 2, 3 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 2, 3, 1 });
		square->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
		Ref<ArrayPolyMeshND> extruded = PolyMeshBuilderND::extrude_linear(square, VectorN{ 0.0, 0.0, 0.5 });
		REQUIRE(extruded.is_valid());
		CHECK_MESSAGE(extruded->get_dimension() == 3, "Extruding a 2-dimensional square along Z must give a 3-dimensional cube.");
		CHECK_MESSAGE(extruded->is_poly_mesh_data_valid(), "The extruded cube must have valid poly mesh data.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = extruded->get_poly_cell_indices();
		REQUIRE(poly_cell_indices.size() == 2);
		CHECK_MESSAGE(poly_cell_indices[0].size() == 6, "The extruded cube must have 6 faces.");
		CHECK_MESSAGE(poly_cell_indices[1].size() == 1, "The extruded cube must have a single volumetric cell.");
		// Even without input normals, the volumetric cell allows orienting all normals outward.
		const Vector<VectorN> boundary_normals = extruded->get_poly_cell_boundary_normals();
		const Vector<PackedInt32Array> boundary_vert = extruded->get_all_poly_cell_vertex_indices(2, false);
		const Vector<VectorN> out_vertices = extruded->get_poly_cell_vertices();
		REQUIRE(boundary_normals.size() == 6);
		for (int64_t cell_index = 0; cell_index < boundary_normals.size(); cell_index++) {
			VectorN center;
			for (int64_t i = 0; i < boundary_vert[cell_index].size(); i++) {
				center = VectorND::add(center, out_vertices[boundary_vert[cell_index][i]]);
			}
			center = VectorND::divide_scalar(center, boundary_vert[cell_index].size());
			CHECK_MESSAGE(VectorND::dot(boundary_normals[cell_index], center) > 0.0, "The extruded cube's boundary normals must point outward.");
		}
		// All edges present at the time of merging must be marked as seams for the 3D output.
		CHECK_MESSAGE(extruded->get_seam_indices().size() == 8, "Both copies of the input edges must be marked as seams.");
	}
}

TEST_CASE("[PolyMeshBuilderND] Make boundary normals topologically consistent") {
	for (int dimension = 3; dimension <= 4; dimension++) {
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(dimension, 1.0));
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		const Vector<VectorN> expected_normals = mesh->get_poly_cell_boundary_normals();
		// Flip the orientation of every boundary cell except the first one.
		Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		Vector<PackedInt32Array> boundary_cells = poly_cell_indices[dimension - 3];
		for (int64_t cell_index = 1; cell_index < boundary_cells.size(); cell_index++) {
			PackedInt32Array boundary_cell = boundary_cells[cell_index];
			const int32_t temp = boundary_cell[0];
			boundary_cell.set(0, boundary_cell[1]);
			boundary_cell.set(1, temp);
			boundary_cells.set(cell_index, boundary_cell);
		}
		poly_cell_indices.set(dimension - 3, boundary_cells);
		mesh->set_poly_cell_indices(poly_cell_indices);
		mesh->set_poly_cell_boundary_normals(expected_normals);
		// The first boundary cell is authoritative, so the traversal must flip all the others back.
		PolyMeshBuilderND::make_boundary_normals_topologically_consistent(mesh, PackedInt32Array{ 0 });
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The mesh must have valid poly mesh data after making normals consistent.");
		const Vector<VectorN> result_normals = mesh->get_poly_cell_boundary_normals();
		REQUIRE(result_normals.size() == expected_normals.size());
		for (int64_t cell_index = 0; cell_index < result_normals.size(); cell_index++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(result_normals[cell_index], expected_normals[cell_index]), "All boundary normals must be restored to point outward.");
		}
	}
}
} // namespace TestPolyMeshBuilderND
