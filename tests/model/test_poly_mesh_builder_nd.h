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

// Sums the areas of the triangulated simplexes of a 3-dimensional mesh, optionally
// filtered to the simplexes that came from one source face.
inline double sum_simplex_areas(const Ref<ArrayPolyMeshND> &p_mesh, const int32_t p_source_face = -1) {
	const PackedInt32Array simplex_indices = p_mesh->get_simplex_cell_indices();
	const Vector<VectorN> vertices = p_mesh->get_vertices();
	double total = 0.0;
	for (int64_t simplex_start = 0; simplex_start < simplex_indices.size(); simplex_start += 3) {
		if (p_source_face != -1 && p_mesh->get_source_poly_cell_for_simplex_cell((int32_t)(simplex_start / 3)) != p_source_face) {
			continue;
		}
		Vector<VectorN> edges = {
			VectorND::subtract(vertices[simplex_indices[simplex_start + 1]], vertices[simplex_indices[simplex_start]]),
			VectorND::subtract(vertices[simplex_indices[simplex_start + 2]], vertices[simplex_indices[simplex_start]]),
		};
		total += VectorND::length(VectorND::perpendicular(edges)) / 2.0;
	}
	return total;
}

// Makes a flat 3D mesh of a single pentagon face with no three collinear vertices, with area 10.
inline Ref<ArrayPolyMeshND> make_pentagon_face_mesh() {
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertices = {
		VectorN{ 0.0, 0.0, 0.0 },
		VectorN{ 2.0, 0.0, 0.0 },
		VectorN{ 3.0, 2.0, 0.0 },
		VectorN{ 1.0, 4.0, 0.0 },
		VectorN{ -1.0, 2.0, 0.0 },
	};
	mesh->set_poly_cell_vertices(vertices);
	mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 4, 0, 4 });
	Vector<PackedInt32Array> faces;
	faces.append(PackedInt32Array{ 0, 1, 2, 3, 4 });
	mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
	return mesh;
}

// Makes a solid 3D mesh of a single tetrahedron cell, with faces and a volumetric cell.
inline Ref<ArrayPolyMeshND> make_solid_tetrahedron_mesh() {
	Ref<ArrayPolyMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertices = {
		VectorN{ 0.0, 0.0, 0.0 },
		VectorN{ 1.0, 0.0, 0.0 },
		VectorN{ 0.0, 1.0, 0.0 },
		VectorN{ 0.0, 0.0, 1.0 },
	};
	mesh->set_poly_cell_vertices(vertices);
	mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 0, 2, 0, 3, 1, 2, 1, 3, 2, 3 });
	Vector<PackedInt32Array> faces;
	faces.append(PackedInt32Array{ 0, 3, 1 }); // Vertices 0, 1, 2.
	faces.append(PackedInt32Array{ 0, 4, 2 }); // Vertices 0, 1, 3.
	faces.append(PackedInt32Array{ 1, 5, 2 }); // Vertices 0, 2, 3.
	faces.append(PackedInt32Array{ 3, 5, 4 }); // Vertices 1, 2, 3.
	Vector<PackedInt32Array> volumes;
	volumes.append(PackedInt32Array{ 0, 1, 2, 3 });
	mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces, volumes });
	mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);
	return mesh;
}

TEST_CASE("[PolyMeshBuilderND] Subdivide box boundary cells") {
	for (int dimension = 3; dimension <= 4; dimension++) {
		Ref<BoxPolyMeshND> box;
		box.instantiate();
		box->set_size(VectorND::fill(dimension, 2.0));
		Ref<ArrayPolyMeshND> mesh = box->to_array_poly_mesh();
		const Vector<VectorN> old_normals = mesh->get_poly_cell_boundary_normals();
		const int64_t old_boundary_count = 2 * dimension;
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, dimension - 1, PackedInt32Array());
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The subdivided box must have valid poly mesh data.");
		const int64_t pieces_per_cell = int64_t(1) << (dimension - 1);
		CHECK_MESSAGE(new_pieces.size() == old_boundary_count * pieces_per_cell, "Each boundary cell of the box must subdivide into 2^(N-1) sub-boxes.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		CHECK_MESSAGE(poly_cell_indices[dimension - 3].size() == old_boundary_count * pieces_per_cell, "The boundary level must contain exactly the sub-boxes.");
		CHECK_MESSAGE(poly_cell_indices[dimension - 2].size() == 1, "The volumetric cell must be conformed, not subdivided.");
		CHECK_MESSAGE(poly_cell_indices[dimension - 2][0].size() == old_boundary_count * pieces_per_cell, "The conformed volumetric cell must reference all sub-boxes.");
		// Subdividing the surface adds edge midpoints and element centers, but no interior vertices.
		int64_t expected_vertices = int64_t(1) << dimension; // Original vertices.
		expected_vertices += dimension * (int64_t(1) << (dimension - 1)); // Edge midpoints.
		if (dimension == 3) {
			expected_vertices += 6; // Face centers.
		} else {
			expected_vertices += 24 + 8; // Face centers and boundary cell centers.
		}
		CHECK_MESSAGE(mesh->get_poly_cell_vertices().size() == expected_vertices, "The subdivided box must have the expected number of vertices.");
		// Each piece must inherit its parent's boundary normal, and the cell orientations must match.
		const Vector<VectorN> new_normals = mesh->get_poly_cell_boundary_normals();
		REQUIRE(new_normals.size() == new_pieces.size());
		for (int64_t parent = 0; parent < old_boundary_count; parent++) {
			for (int64_t piece_num = 0; piece_num < pieces_per_cell; piece_num++) {
				const int32_t piece = new_pieces[parent * pieces_per_cell + piece_num];
				CHECK_MESSAGE(VectorND::is_equal_approx(new_normals[piece], old_normals[parent]), "Each sub-box must inherit its parent's boundary normal.");
			}
		}
		Ref<ArrayPolyMeshND> recalculated = mesh->duplicate();
		recalculated->set_poly_cell_boundary_normals(Vector<VectorN>());
		recalculated->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> oriented_normals = recalculated->get_poly_cell_boundary_normals();
		for (int64_t i = 0; i < new_normals.size(); i++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(oriented_normals[i], new_normals[i]), "The cell orientations must reproduce the inherited normals.");
		}
		// The geometry is unchanged, so the signed distance must be unchanged.
		CHECK_MESSAGE(mesh->get_signed_distance_to_mesh(VectorND::value_on_axis_with_dimension(2.0, 0, dimension), nullptr, nullptr) == doctest::Approx(1.0), "The subdivided box must have the same signed distances as before.");
		CHECK_MESSAGE(mesh->get_signed_distance_to_mesh(VectorND::zero(dimension), nullptr, nullptr) == doctest::Approx(-1.0), "The subdivided box must have the same signed distances as before.");
		// Every face must have its edges in a connected loop order, including internal walls.
		const PackedInt32Array all_edges = mesh->get_edge_indices();
		for (const PackedInt32Array &face : poly_cell_indices[0]) {
			for (int64_t i = 0; i < face.size(); i++) {
				const int32_t edge_a = face[i];
				const int32_t edge_b = face[(i + 1) % face.size()];
				const bool connected = all_edges[edge_a * 2] == all_edges[edge_b * 2] || all_edges[edge_a * 2] == all_edges[edge_b * 2 + 1] || all_edges[edge_a * 2 + 1] == all_edges[edge_b * 2] || all_edges[edge_a * 2 + 1] == all_edges[edge_b * 2 + 1];
				CHECK_MESSAGE(connected, "Every face of the subdivided box must have its edges in a connected loop order.");
			}
		}
		// The texture map must carry through with interpolated values for the new vertices.
		const Vector<Vector<VectorM>> texture_map = mesh->get_poly_cell_texture_map();
		REQUIRE(texture_map.size() == new_pieces.size());
		const Vector<PackedInt32Array> cell_vertex_indices = mesh->get_all_boundary_cell_vertex_indices(false);
		for (int64_t cell_index = 0; cell_index < texture_map.size(); cell_index++) {
			CHECK_MESSAGE(texture_map[cell_index].size() == cell_vertex_indices[cell_index].size(), "Each sub-box must have interpolated texture map coordinates for all of its vertices.");
		}
	}
}

TEST_CASE("[PolyMeshBuilderND] Subdivide simplex and orthoplex volumes") {
	SUBCASE("A tetrahedron subdivides into 4 corner tetrahedra and a central octahedron") {
		Ref<ArrayPolyMeshND> mesh = make_solid_tetrahedron_mesh();
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, 3, PackedInt32Array());
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The subdivided tetrahedron must have valid poly mesh data.");
		CHECK_MESSAGE(new_pieces.size() == 5, "A tetrahedron must subdivide into 4 corner tetrahedra and 1 central octahedron.");
		CHECK_MESSAGE(mesh->get_poly_cell_vertices().size() == 10, "The subdivided tetrahedron must only add the 6 edge midpoints, with no center vertices.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		REQUIRE(poly_cell_indices.size() == 2);
		CHECK_MESSAGE(poly_cell_indices[0].size() == 4 * 4 + 4, "The subdivided tetrahedron must have 16 boundary face pieces and 4 interior cut faces.");
		CHECK_MESSAGE(poly_cell_indices[1].size() == 5, "The subdivided tetrahedron must have 5 volumetric cells.");
		int64_t tetrahedron_count = 0;
		int64_t octahedron_count = 0;
		for (const int32_t piece : new_pieces) {
			const int64_t member_count = poly_cell_indices[1][piece].size();
			if (member_count == 4) {
				tetrahedron_count++;
			} else if (member_count == 8) {
				octahedron_count++;
			}
		}
		CHECK_MESSAGE(tetrahedron_count == 4, "The subdivided tetrahedron must have 4 corner tetrahedra.");
		CHECK_MESSAGE(octahedron_count == 1, "The subdivided tetrahedron must have 1 central octahedron.");
		// The 4 interior cut faces are each shared by two volumetric cells, so they are excluded from rendering.
		CHECK_MESSAGE(mesh->get_simplex_cell_indices().size() == 16 * 3, "Only the 16 boundary face pieces must decompose into simplexes.");
		// The geometry is unchanged, so the signed distance must be unchanged.
		mesh->populate_inverse_metric_cache();
		CHECK_MESSAGE(Math::abs(mesh->get_signed_distance_to_mesh(VectorN{ -1.0, 0.0, 0.0 }, nullptr, nullptr)) == doctest::Approx(1.0), "The subdivided tetrahedron must have the same signed distances as before.");
	}
	SUBCASE("Subdividing twice cycles between tetrahedra and octahedra") {
		Ref<ArrayPolyMeshND> mesh = make_solid_tetrahedron_mesh();
		PolyMeshBuilderND::subdivide_elements(mesh, 3, PackedInt32Array());
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, 3, PackedInt32Array());
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The twice-subdivided tetrahedron must have valid poly mesh data.");
		// 4 tetrahedra each give 4 + 1 pieces, and the octahedron gives 6 corner octahedra + 8 hole tetrahedra.
		CHECK_MESSAGE(new_pieces.size() == 4 * 5 + 6 + 8, "The second subdivision must produce 34 pieces from the tet-oct rules.");
	}
	SUBCASE("An octahedron subdivides into 6 corner octahedra and 8 hole tetrahedra") {
		Ref<OrthoplexPolyMeshND> orthoplex;
		orthoplex.instantiate();
		orthoplex->set_size(VectorND::fill(3, 2.0));
		Ref<ArrayPolyMeshND> mesh = orthoplex->to_array_poly_mesh();
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, 3, PackedInt32Array());
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The subdivided octahedron must have valid poly mesh data.");
		CHECK_MESSAGE(new_pieces.size() == 6 + 8, "An octahedron must subdivide into 6 corner octahedra and 8 hole tetrahedra.");
		CHECK_MESSAGE(mesh->get_poly_cell_vertices().size() == 6 + 12 + 1, "The subdivided octahedron must add the 12 edge midpoints and 1 center vertex.");
		// The geometry is unchanged, so the signed distance must be unchanged.
		mesh->populate_inverse_metric_cache();
		CHECK_MESSAGE(mesh->get_signed_distance_to_mesh(VectorN{ 2.0, 0.0, 0.0 }, nullptr, nullptr) == doctest::Approx(1.0), "The subdivided octahedron must have the same signed distances as before.");
	}
	SUBCASE("A pentachoron subdivides into 5 corner pentachora and a central rectified pentachoron") {
		// A solid 4D simplex: 5 vertices, 10 edges, 10 triangles, 5 tetrahedra, 1 volumetric cell.
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0, 0.0, 0.0 },
			VectorN{ 1.0, 0.0, 0.0, 0.0 },
			VectorN{ 0.0, 1.0, 0.0, 0.0 },
			VectorN{ 0.0, 0.0, 1.0, 0.0 },
			VectorN{ 0.0, 0.0, 0.0, 1.0 },
		};
		mesh->set_poly_cell_vertices(vertices);
		PackedInt32Array edge_indices;
		HashMap<int32_t, int32_t> edge_map;
		for (int32_t a = 0; a < 5; a++) {
			for (int32_t b = a + 1; b < 5; b++) {
				edge_map[a * 8 + b] = (int32_t)(edge_indices.size() / 2);
				edge_indices.append(a);
				edge_indices.append(b);
			}
		}
		mesh->set_edge_vertex_indices(edge_indices);
		Vector<PackedInt32Array> faces;
		HashMap<int32_t, int32_t> face_map;
		for (int32_t a = 0; a < 5; a++) {
			for (int32_t b = a + 1; b < 5; b++) {
				for (int32_t c = b + 1; c < 5; c++) {
					face_map[(a * 8 + b) * 8 + c] = (int32_t)faces.size();
					faces.append(PackedInt32Array{ edge_map[a * 8 + b], edge_map[b * 8 + c], edge_map[a * 8 + c] });
				}
			}
		}
		Vector<PackedInt32Array> cells;
		HashMap<int32_t, int32_t> cell_map;
		for (int32_t a = 0; a < 5; a++) {
			for (int32_t b = a + 1; b < 5; b++) {
				for (int32_t c = b + 1; c < 5; c++) {
					for (int32_t d = c + 1; d < 5; d++) {
						cell_map[((a * 8 + b) * 8 + c) * 8 + d] = (int32_t)cells.size();
						cells.append(PackedInt32Array{
								face_map[(a * 8 + b) * 8 + c],
								face_map[(a * 8 + b) * 8 + d],
								face_map[(a * 8 + c) * 8 + d],
								face_map[(b * 8 + c) * 8 + d] });
					}
				}
			}
		}
		Vector<PackedInt32Array> volumes;
		volumes.append(PackedInt32Array{ 0, 1, 2, 3, 4 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces, cells, volumes });
		mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);
		REQUIRE(mesh->is_poly_mesh_data_valid());
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, 4, PackedInt32Array());
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The subdivided pentachoron must have valid poly mesh data.");
		CHECK_MESSAGE(new_pieces.size() == 6, "A pentachoron must subdivide into 5 corner pentachora and 1 central rectified pentachoron.");
		CHECK_MESSAGE(mesh->get_poly_cell_vertices().size() == 5 + 10, "The subdivided pentachoron must only add the 10 edge midpoints, with no center vertices.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		REQUIRE(poly_cell_indices.size() == 3);
		CHECK_MESSAGE(poly_cell_indices[1].size() == 5 * 5 + 5, "The subdivided pentachoron must have 25 boundary cell pieces and 5 interior cut cells.");
		CHECK_MESSAGE(poly_cell_indices[2].size() == 6, "The subdivided pentachoron must have 6 volumetric cells.");
		int64_t corner_count = 0;
		int64_t rectified_count = 0;
		for (const int32_t piece : new_pieces) {
			const int64_t member_count = poly_cell_indices[2][piece].size();
			if (member_count == 5) {
				corner_count++;
			} else if (member_count == 10) {
				rectified_count++;
			}
		}
		CHECK_MESSAGE(corner_count == 5, "The subdivided pentachoron must have 5 corner pentachora with 5 members each.");
		CHECK_MESSAGE(rectified_count == 1, "The central rectified pentachoron must have 10 members: 5 octahedra and 5 tetrahedra.");
	}
}

TEST_CASE("[PolyMeshBuilderND] Subdivide with partial selection and conformance") {
	SUBCASE("Subdividing one of two cubes conforms the other") {
		// Two unit cubes side by side sharing one square face, with two volumetric cells.
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0, 0.0 },
			VectorN{ 0.0, 1.0, 0.0 },
			VectorN{ 0.0, 0.0, 1.0 },
			VectorN{ 0.0, 1.0, 1.0 },
			VectorN{ 1.0, 0.0, 0.0 },
			VectorN{ 1.0, 1.0, 0.0 },
			VectorN{ 1.0, 0.0, 1.0 },
			VectorN{ 1.0, 1.0, 1.0 },
			VectorN{ 2.0, 0.0, 0.0 },
			VectorN{ 2.0, 1.0, 0.0 },
			VectorN{ 2.0, 0.0, 1.0 },
			VectorN{ 2.0, 1.0, 1.0 },
		};
		mesh->set_poly_cell_vertices(vertices);
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 3, 2, 3, 0, 2, 4, 5, 5, 7, 6, 7, 4, 6, 8, 9, 9, 11, 10, 11, 8, 10, 0, 4, 1, 5, 2, 6, 3, 7, 4, 8, 5, 9, 6, 10, 7, 11 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3 });
		faces.append(PackedInt32Array{ 4, 5, 6, 7 }); // The shared middle square at X = 1.
		faces.append(PackedInt32Array{ 8, 9, 10, 11 });
		faces.append(PackedInt32Array{ 12, 7, 14, 3 });
		faces.append(PackedInt32Array{ 13, 5, 15, 1 });
		faces.append(PackedInt32Array{ 12, 4, 13, 0 });
		faces.append(PackedInt32Array{ 14, 6, 15, 2 });
		faces.append(PackedInt32Array{ 16, 11, 18, 7 });
		faces.append(PackedInt32Array{ 17, 9, 19, 5 });
		faces.append(PackedInt32Array{ 16, 8, 17, 4 });
		faces.append(PackedInt32Array{ 18, 10, 19, 6 });
		Vector<PackedInt32Array> volumes;
		volumes.append(PackedInt32Array{ 0, 3, 1, 4, 5, 6 });
		volumes.append(PackedInt32Array{ 2, 7, 1, 8, 9, 10 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces, volumes });
		REQUIRE(mesh->is_mesh_data_valid());
		// Subdivide only the first cube. The second cube must be conformed to the pieces.
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, 3, PackedInt32Array{ 0 });
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The partially subdivided mesh must have valid poly mesh data.");
		CHECK_MESSAGE(new_pieces.size() == 8, "The subdivided cube must become 8 sub-cubes.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		CHECK_MESSAGE(poly_cell_indices[1].size() == 9, "The mesh must have the 8 sub-cubes plus the conformed second cube.");
		CHECK_MESSAGE(poly_cell_indices[1][poly_cell_indices[1].size() - 1].size() == 5 + 4, "The conformed cube must reference its 5 surviving faces plus the 4 pieces of the shared face.");
		// Faces: 5 subdivided faces of cube A (20), 4 pieces of the shared face, 12 internal
		// walls of cube A, 4 conformed side faces of cube B, and 1 untouched far face.
		CHECK_MESSAGE(poly_cell_indices[0].size() == 20 + 4 + 12 + 4 + 1, "The face count must match the subdivision and conformance.");
		CHECK_MESSAGE(mesh->get_poly_cell_vertices().size() == 12 + 12 + 6 + 1, "Subdividing one cube must add its edge midpoints, face centers, and center.");
		// The geometry is unchanged, so the signed distance must be unchanged.
		mesh->populate_inverse_metric_cache();
		CHECK_MESSAGE(Math::abs(mesh->get_signed_distance_to_mesh(VectorN{ 3.0, 0.5, 0.5 }, nullptr, nullptr)) == doctest::Approx(1.0), "The partially subdivided mesh must have the same signed distances as before.");
		CHECK_MESSAGE(Math::abs(mesh->get_signed_distance_to_mesh(VectorN{ -1.0, 0.5, 0.5 }, nullptr, nullptr)) == doctest::Approx(1.0), "The partially subdivided mesh must have the same signed distances as before.");
	}
	SUBCASE("Subdividing a face preserves normals and interpolates attributes") {
		// A single flat quad face with a stored +Z normal, vertex normals, and a texture map.
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0, 0.0 },
			VectorN{ 2.0, 0.0, 0.0 },
			VectorN{ 2.0, 2.0, 0.0 },
			VectorN{ 0.0, 2.0, 0.0 },
		};
		mesh->set_poly_cell_vertices(vertices);
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 0, 3 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
		const VectorN pos_z = VectorN{ 0.0, 0.0, 1.0 };
		mesh->set_poly_cell_boundary_normals(Vector<VectorN>{ pos_z });
		// The texture map and vertex normals are ordered to match the cell's vertex sequence.
		const PackedInt32Array cell_vertices = mesh->get_all_boundary_cell_vertex_indices(false)[0];
		Vector<VectorN> cell_vertex_normals;
		Vector<VectorM> cell_texture_map;
		for (int64_t i = 0; i < cell_vertices.size(); i++) {
			cell_vertex_normals.append(pos_z);
			const VectorN vertex = vertices[cell_vertices[i]];
			cell_texture_map.append(VectorM{ vertex[0] / 2.0, vertex[1] / 2.0 });
		}
		mesh->set_poly_cell_vertex_normals(Vector<Vector<VectorN>>{ cell_vertex_normals });
		mesh->set_poly_cell_texture_map(Vector<Vector<VectorM>>{ cell_texture_map });
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, 2, PackedInt32Array());
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The subdivided quad must have valid poly mesh data.");
		CHECK_MESSAGE(new_pieces.size() == 4, "A quad must subdivide into 4 sub-quads.");
		CHECK_MESSAGE(mesh->get_poly_cell_vertices().size() == 4 + 4 + 1, "The subdivided quad must add 4 edge midpoints and 1 center vertex.");
		// Every piece must keep the +Z normal, both stored and orientation-derived.
		const Vector<VectorN> new_normals = mesh->get_poly_cell_boundary_normals();
		REQUIRE(new_normals.size() == 4);
		Ref<ArrayPolyMeshND> recalculated = mesh->duplicate();
		recalculated->set_poly_cell_boundary_normals(Vector<VectorN>());
		recalculated->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> oriented_normals = recalculated->get_poly_cell_boundary_normals();
		for (int64_t i = 0; i < 4; i++) {
			CHECK_MESSAGE(VectorND::is_equal_approx(new_normals[i], pos_z), "Each piece must inherit the parent's +Z normal.");
			CHECK_MESSAGE(VectorND::is_equal_approx(oriented_normals[i], pos_z), "Each piece's orientation must reproduce the parent's +Z normal.");
		}
		// The texture map was set to half the vertex position, and interpolation is linear,
		// so this relationship must also hold at the new midpoint and center vertices.
		const Vector<VectorN> new_vertices = mesh->get_poly_cell_vertices();
		const Vector<PackedInt32Array> new_cell_vertices = mesh->get_all_boundary_cell_vertex_indices(false);
		const Vector<Vector<VectorM>> new_texture_map = mesh->get_poly_cell_texture_map();
		const Vector<Vector<VectorN>> new_vertex_normals = mesh->get_poly_cell_vertex_normals();
		REQUIRE(new_texture_map.size() == 4);
		REQUIRE(new_vertex_normals.size() == 4);
		for (int64_t cell_index = 0; cell_index < 4; cell_index++) {
			REQUIRE(new_texture_map[cell_index].size() == new_cell_vertices[cell_index].size());
			REQUIRE(new_vertex_normals[cell_index].size() == new_cell_vertices[cell_index].size());
			for (int64_t vert_num = 0; vert_num < new_cell_vertices[cell_index].size(); vert_num++) {
				const VectorN vertex = new_vertices[new_cell_vertices[cell_index][vert_num]];
				const VectorM texcoord = new_texture_map[cell_index][vert_num];
				REQUIRE(texcoord.size() == 2);
				CHECK_MESSAGE(texcoord[0] == doctest::Approx(vertex[0] / 2.0), "The interpolated texture map must remain linear in the vertex positions.");
				CHECK_MESSAGE(texcoord[1] == doctest::Approx(vertex[1] / 2.0), "The interpolated texture map must remain linear in the vertex positions.");
				CHECK_MESSAGE(VectorND::is_equal_approx(new_vertex_normals[cell_index][vert_num], pos_z), "The interpolated vertex normals must remain +Z.");
			}
		}
	}
	SUBCASE("Subdividing one edge conforms the face that uses it") {
		Ref<ArrayPolyMeshND> mesh;
		mesh.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0, 0.0 },
			VectorN{ 2.0, 0.0, 0.0 },
			VectorN{ 2.0, 2.0, 0.0 },
			VectorN{ 0.0, 2.0, 0.0 },
		};
		mesh->set_poly_cell_vertices(vertices);
		mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 0, 3 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3 });
		mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
		const PackedInt32Array new_pieces = PolyMeshBuilderND::subdivide_elements(mesh, 1, PackedInt32Array{ 0 });
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The mesh must be valid after subdividing one edge.");
		CHECK_MESSAGE(new_pieces.size() == 2, "The subdivided edge must become 2 pieces.");
		CHECK_MESSAGE(mesh->get_poly_cell_vertices().size() == 5, "Subdividing one edge must add exactly one midpoint vertex.");
		CHECK_MESSAGE(mesh->get_edge_indices().size() == 5 * 2, "The mesh must have 3 surviving edges plus 2 pieces.");
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = mesh->get_poly_cell_indices();
		CHECK_MESSAGE(poly_cell_indices[0][0].size() == 5, "The face must be conformed into a 5-edge loop.");
	}
}

TEST_CASE("[PolyMeshBuilderND] Flipping faces of 3D meshes preserves their triangulation") {
	SUBCASE("Topological consistency flips pentagon faces without breaking triangulation") {
		// Two pentagons sharing edge AB, folded like a tent. The second pentagon is a linear
		// image of the first, so its area is the first's area times the stretch factor.
		const double second_area_factor = Math::sqrt(1.25);
		for (int flip_second = 0; flip_second < 2; flip_second++) {
			Ref<ArrayPolyMeshND> mesh;
			mesh.instantiate();
			Vector<VectorN> vertices = {
				VectorN{ 0.0, 0.0, 0.0 },
				VectorN{ 2.0, 0.0, 0.0 },
				VectorN{ 3.0, 2.0, 0.0 },
				VectorN{ 1.0, 4.0, 0.0 },
				VectorN{ -1.0, 2.0, 0.0 },
				VectorN{ 3.0, -2.0, 1.0 },
				VectorN{ 1.0, -4.0, 2.0 },
				VectorN{ -1.0, -2.0, 1.0 },
			};
			mesh->set_poly_cell_vertices(vertices);
			mesh->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 4, 0, 4, 1, 5, 5, 6, 6, 7, 0, 7 });
			Vector<PackedInt32Array> faces;
			faces.append(PackedInt32Array{ 0, 1, 2, 3, 4 });
			if (flip_second == 0) {
				faces.append(PackedInt32Array{ 0, 5, 6, 7, 8 });
			} else {
				faces.append(PackedInt32Array{ 8, 7, 6, 5, 0 });
			}
			mesh->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
			const VectorN pos_z = VectorN{ 0.0, 0.0, 1.0 };
			Vector<VectorN> stored_normals = { pos_z, VectorN() };
			mesh->set_poly_cell_boundary_normals(stored_normals);
			REQUIRE(mesh->is_mesh_data_valid());
			// One of the two orientations of the second pentagon disagrees with the first,
			// so this call must flip it, and the flip must not break its triangulation.
			PolyMeshBuilderND::make_boundary_normals_topologically_consistent(mesh, PackedInt32Array{ 0 });
			CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "The mesh must be valid after making normals consistent.");
			CHECK_MESSAGE(sum_simplex_areas(mesh, 0) == doctest::Approx(10.0), "The first pentagon's triangulated area must match its polygon area.");
			CHECK_MESSAGE(sum_simplex_areas(mesh, 1) == doctest::Approx(10.0 * second_area_factor), "The second pentagon's triangulated area must match its polygon area, even if it was flipped.");
		}
	}
	SUBCASE("Extruding a pentagon gives a prism with the correct surface area") {
		// A 2-dimensional pentagon with 2D vertices, extruded along Z into a pentagonal prism.
		// The extrusion's normal calculations flip pentagon cap faces to point outward.
		Ref<ArrayPolyMeshND> pentagon;
		pentagon.instantiate();
		Vector<VectorN> vertices = {
			VectorN{ 0.0, 0.0 },
			VectorN{ 2.0, 0.0 },
			VectorN{ 3.0, 2.0 },
			VectorN{ 1.0, 4.0 },
			VectorN{ -1.0, 2.0 },
		};
		pentagon->set_poly_cell_vertices(vertices);
		pentagon->set_edge_vertex_indices(PackedInt32Array{ 0, 1, 1, 2, 2, 3, 3, 4, 0, 4 });
		Vector<PackedInt32Array> faces;
		faces.append(PackedInt32Array{ 0, 1, 2, 3, 4 });
		pentagon->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ faces });
		Ref<ArrayPolyMeshND> prism = PolyMeshBuilderND::extrude_linear(pentagon, VectorN{ 0.0, 0.0, 0.5 });
		CHECK_MESSAGE(prism->is_poly_mesh_data_valid(), "The extruded pentagonal prism must have valid poly mesh data.");
		double perimeter = 0.0;
		for (int64_t i = 0; i < 5; i++) {
			perimeter += VectorND::distance_to(vertices[i], vertices[(i + 1) % 5]);
		}
		CHECK_MESSAGE(sum_simplex_areas(prism) == doctest::Approx(2.0 * 10.0 + perimeter), "The prism's triangulated surface area must be two caps plus the sides.");
		// The normals must point outward, so an interior point must have a negative signed distance.
		prism->populate_inverse_metric_cache();
		CHECK_MESSAGE(prism->get_signed_distance_to_mesh(VectorN{ 1.0, 1.5, 0.0 }, nullptr, nullptr) == doctest::Approx(-0.5), "An interior point of the prism must be half a unit inside the caps.");
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
			PolyMeshND::flip_poly_cell_orientation(boundary_cell, dimension - 3);
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
