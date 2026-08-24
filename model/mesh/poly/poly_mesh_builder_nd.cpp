#include "poly_mesh_builder_nd.h"

#include "../../../math/vector_nd.h"

Ref<ArrayPolyMeshND> PolyMeshBuilderND::convert_mesh_3d_to_nd_faces_only(const Ref<ArrayMesh> &p_mesh_3d, const int p_which_surface, const bool p_deduplicate) {
	Ref<ArrayPolyMeshND> ret;
	ret.instantiate();
	ERR_FAIL_COND_V_MSG(p_mesh_3d.is_null(), ret, "Input mesh is null.");
	const int surface_count = p_mesh_3d->get_surface_count();
	ERR_FAIL_COND_V_MSG(surface_count == 0, ret, "Input mesh has no surfaces.");
	if (p_which_surface != -1) {
		ERR_FAIL_INDEX_V_MSG(p_which_surface, surface_count, ret, "Invalid surface index.");
	}
	int start_surface = p_which_surface == -1 ? 0 : p_which_surface;
	int end_surface = p_which_surface == -1 ? surface_count : p_which_surface + 1;
	// The output is a 3-dimensional poly mesh, so the faces are the boundary cells,
	// the face normals are the boundary normals, and the texture space is 2-dimensional.
	Vector<VectorN> output_vertices;
	Vector<VectorN> output_face_boundary_normals;
	Vector<Vector<VectorN>> output_face_vertex_normals;
	Vector<Vector<VectorM>> output_face_texture_maps;
	Vector<PackedInt32Array> output_face_indices;
	for (int surface_index = start_surface; surface_index < end_surface; surface_index++) {
		const Array surface_arrays = p_mesh_3d->surface_get_arrays(surface_index);
		CRASH_COND(surface_arrays.size() < Mesh::ARRAY_MAX); // ArrayMesh should always return surfaces arrays of length Mesh::ARRAY_MAX, even if some of them are empty.
		const PackedVector3Array surface_vertices = PackedVector3Array(surface_arrays[Mesh::ARRAY_VERTEX]);
		const PackedVector3Array surface_normals = PackedVector3Array(surface_arrays[Mesh::ARRAY_NORMAL]);
		const PackedVector2Array surface_uvs = PackedVector2Array(surface_arrays[Mesh::ARRAY_TEX_UV]);
		ERR_FAIL_COND_V_MSG(surface_normals.size() > 0 && surface_normals.size() != surface_vertices.size(), ret, "Surface normals array size does not match vertices array size.");
		ERR_FAIL_COND_V_MSG(surface_uvs.size() > 0 && surface_uvs.size() != surface_vertices.size(), ret, "Surface texture map UVs array size does not match vertices array size.");
		PackedInt32Array surface_indices = surface_arrays.size() > Mesh::ARRAY_INDEX ? PackedInt32Array(surface_arrays[Mesh::ARRAY_INDEX]) : PackedInt32Array();
		bool is_indexed = surface_indices.size() > 0;
		if (!is_indexed) {
			// Standardize everything to indexed format for easier processing.
			surface_indices.resize(surface_vertices.size());
			for (int32_t i = 0; i < (int32_t)surface_indices.size(); i++) {
				surface_indices.set(i, i);
			}
		}
		// Append vertices, deduplicating with existing vertices along the way.
		PackedInt32Array surface_verts_to_inserted;
		surface_verts_to_inserted.resize(surface_vertices.size());
		for (int64_t vertex_index = 0; vertex_index < surface_vertices.size(); vertex_index++) {
			const VectorN vert_nd = VectorND::from_3d(surface_vertices[vertex_index]);
			if (p_deduplicate) {
				const int existing_index = (int)output_vertices.find(vert_nd);
				if (existing_index != -1) {
					surface_verts_to_inserted.set(vertex_index, existing_index);
					continue;
				}
			}
			surface_verts_to_inserted.set(vertex_index, (int32_t)output_vertices.size());
			output_vertices.append(vert_nd);
		}
		// Read the triangle vertex indices from the index array.
		ERR_FAIL_COND_V_MSG(surface_indices.size() % 3 != 0, ret, "Indexed surface index count is not a multiple of 3, so it cannot be converted to faces.");
		for (int64_t vertex_index = 0; vertex_index < surface_indices.size(); vertex_index += 3) {
			const int32_t orig_v0 = surface_indices[vertex_index + 0];
			const int32_t orig_v1 = surface_indices[vertex_index + 2];
			const int32_t orig_v2 = surface_indices[vertex_index + 1];
			// Always deduplicate edges for indexed 3D mesh inputs.
			// Indexed 3D meshes have control over edge merging via the vertex indexing already.
			const bool deduplicate_edges = is_indexed || p_deduplicate;
			const int32_t e0 = (int32_t)ret->append_edge_indices(surface_verts_to_inserted[orig_v0], surface_verts_to_inserted[orig_v1], deduplicate_edges);
			const int32_t e1 = (int32_t)ret->append_edge_indices(surface_verts_to_inserted[orig_v1], surface_verts_to_inserted[orig_v2], deduplicate_edges);
			const int32_t e2 = (int32_t)ret->append_edge_indices(surface_verts_to_inserted[orig_v0], surface_verts_to_inserted[orig_v2], deduplicate_edges);
			output_face_indices.append(PackedInt32Array{ e0, e1, e2 });
			// Append face boundary normal for this face.
			const Vector3 face_a = surface_vertices[orig_v1] - surface_vertices[orig_v0];
			const Vector3 face_b = surface_vertices[orig_v2] - surface_vertices[orig_v0];
			const Vector3 face_boundary_normal = face_a.cross(face_b).normalized();
			output_face_boundary_normals.append(VectorND::from_3d(face_boundary_normal));
			// Append face vertex normals for this face if they exist.
			if (!surface_normals.is_empty()) {
				Vector<VectorN> face_vertex_normals = {
					VectorND::from_3d(surface_normals[orig_v0]),
					VectorND::from_3d(surface_normals[orig_v1]),
					VectorND::from_3d(surface_normals[orig_v2]),
				};
				output_face_vertex_normals.append(face_vertex_normals);
			}
			// Append face texture maps if they exist.
			if (!surface_uvs.is_empty()) {
				Vector<VectorM> face_texture_maps = {
					VectorND::from_2d(surface_uvs[orig_v0]),
					VectorND::from_2d(surface_uvs[orig_v1]),
					VectorND::from_2d(surface_uvs[orig_v2]),
				};
				output_face_texture_maps.append(face_texture_maps);
			}
		}
	}
	ret->set_poly_cell_vertices(output_vertices);
	ret->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ output_face_indices });
	if (!output_face_boundary_normals.is_empty()) {
		ret->set_poly_cell_boundary_normals(output_face_boundary_normals);
	}
	if (!output_face_vertex_normals.is_empty()) {
		ret->set_poly_cell_vertex_normals(output_face_vertex_normals);
	}
	if (!output_face_texture_maps.is_empty()) {
		ret->set_poly_cell_texture_map(output_face_texture_maps);
	}
	return ret;
}

Ref<ArrayPolyMeshND> PolyMeshBuilderND::extrude_linear(const Ref<ArrayPolyMeshND> &p_input_mesh, const VectorN &p_extrusion_vector) {
	Ref<ArrayPolyMeshND> ret;
	ret.instantiate();
	ERR_FAIL_COND_V_MSG(p_input_mesh.is_null() || !p_input_mesh->is_mesh_data_valid(), ret, "Input mesh is not valid, so extrusion cannot be performed.");
	const int input_dimension = p_input_mesh->get_dimension();
	VectorN extrusion_vector = p_extrusion_vector;
	if (extrusion_vector.is_empty()) {
		// Default to extruding one unit along a new axis one dimension above the input mesh.
		extrusion_vector = VectorND::value_on_axis_with_dimension(1.0, input_dimension, input_dimension + 1);
	}
	const int output_dimension = MAX(input_dimension, (int)extrusion_vector.size());
	// The boundary cells of the output mesh are (output_dimension - 1)-dimensional cells at this index.
	const int64_t output_boundary_dim_index = int64_t(output_dimension) - 3;
	// When the extrusion adds a dimension, the input mesh's boundary cells are extruded
	// into the boundary cells of the output mesh, allowing normals to be carried over.
	const bool extrusion_adds_dimension = output_dimension == input_dimension + 1;
	// Extract and copy a bunch of data from the input mesh.
	// Start by copying the input mesh's data into the output mesh twice,
	// offset by the extrusion vector in both negative and positive directions.
	ret = p_input_mesh->duplicate();
	ret->transform_vertices(TransformND::from_position(VectorND::negate(extrusion_vector)));
	ret->merge_with(p_input_mesh, TransformND::from_position(extrusion_vector));
	Vector<Vector<PackedInt32Array>> poly_cell_indices = ret->get_poly_cell_indices();
	// The two copies aren't connected yet, so it's safe to blindly force their normals outward (if any).
	if (output_boundary_dim_index >= 0 && poly_cell_indices.size() > output_boundary_dim_index) {
		ret->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);
		poly_cell_indices = ret->get_poly_cell_indices();
	}
	// Mark all existing seam-level elements as seams since they will become sharp borders (usually 90 degrees).
	// Seams refer to the (output_dimension - 2)-dimensional borders between boundary cells,
	// which are the edges for 3D meshes, the 2D faces for 4D meshes, and so on.
	const int64_t seam_dim_index = int64_t(output_dimension) - 4;
	if (seam_dim_index >= -1) {
		int64_t seam_element_count = 0;
		if (seam_dim_index == -1) {
			seam_element_count = ret->get_edge_indices().size() / 2;
		} else if (seam_dim_index < poly_cell_indices.size()) {
			seam_element_count = poly_cell_indices[seam_dim_index].size();
		}
		HashSet<int32_t> seam_indices;
		for (int64_t seam_index = 0; seam_index < seam_element_count; seam_index++) {
			seam_indices.insert((int32_t)seam_index);
		}
		ret->set_seam_indices(seam_indices);
	}
	// Now connect everything. Work with the raw data to avoid costly safety checks
	// present inside of the ArrayPolyMeshND's functions. All of this should be valid
	// as long as the input mesh is valid, which is checked at the start of this function.
	// Form new edges between the vertices of the two copies of the input mesh.
	const PackedInt32Array &input_edge_indices = p_input_mesh->get_edge_indices();
	const int32_t input_edge_count = (int32_t)(input_edge_indices.size() / 2);
	PackedInt32Array vertex_to_extruded_edge;
	{
		const Vector<VectorN> &input_vertices = p_input_mesh->get_poly_cell_vertices();
		PackedInt32Array edge_indices = ret->get_edge_indices();
		int64_t edge_indices_iter = edge_indices.size();
		const int32_t input_vertex_count = (int32_t)input_vertices.size();
		vertex_to_extruded_edge.resize(input_vertex_count);
		edge_indices.resize(edge_indices.size() + input_vertex_count * 2);
		for (int input_vertex_index = 0; input_vertex_index < input_vertex_count; input_vertex_index++) {
			vertex_to_extruded_edge.set(input_vertex_index, edge_indices_iter / 2);
			edge_indices.set(edge_indices_iter, input_vertex_index);
			edge_indices.set(edge_indices_iter + 1, input_vertex_index + input_vertex_count);
			edge_indices_iter += 2;
		}
		ret->set_edge_vertex_indices(edge_indices);
	}
	// Form new faces between the edges of the two copies of the input mesh.
	PackedInt32Array edge_to_extruded_face;
	edge_to_extruded_face.resize(input_edge_count);
	const Vector<Vector<PackedInt32Array>> &input_poly_cell_indices = p_input_mesh->get_poly_cell_indices();
	if (input_poly_cell_indices.size() > 0) {
		Vector<PackedInt32Array> face_indices = poly_cell_indices[0];
		for (int32_t input_edge_index = 0; input_edge_index < input_edge_count; input_edge_index++) {
			const int32_t vertex_index_a = input_edge_indices[input_edge_index * 2];
			const int32_t vertex_index_b = input_edge_indices[input_edge_index * 2 + 1];
			// Create a directed loop of edges:
			const PackedInt32Array new_face = {
				input_edge_index, // First copy.
				vertex_to_extruded_edge[vertex_index_a], // Extruded from vertex A.
				input_edge_index + input_edge_count, // Second copy.
				vertex_to_extruded_edge[vertex_index_b], // Extruded from vertex B.
			};
			const int32_t new_face_index = face_indices.size();
			face_indices.append(new_face);
			edge_to_extruded_face.set(input_edge_index, new_face_index);
		}
		poly_cell_indices.set(0, face_indices);
		// 0: Edges to extruded faces. 1: Faces to extruded cells. 2: Cells to extruded volumes. And so on.
		Vector<PackedInt32Array> all_cell_to_extruded_cell;
		all_cell_to_extruded_cell.append(edge_to_extruded_face);
		// Form new cells between the faces of the two copies of the input mesh, and so on.
		for (int input_poly_index = 0; input_poly_index < input_poly_cell_indices.size(); input_poly_index++) {
			// This code operates between dimensions, so "prev" and "next" are adjacent dimensions.
			// Also, in terms of dimensional index, "prev" equals "input", and "next" equals "output".
			const Vector<PackedInt32Array> &input_cells = input_poly_cell_indices[input_poly_index];
			const PackedInt32Array &prev_dim_cell_to_extruded_cell = all_cell_to_extruded_cell[input_poly_index];
			const int32_t next_dim_poly_index = input_poly_index + 1;
			if (next_dim_poly_index >= poly_cell_indices.size()) {
				// This may only happen once per call to `extrude_linear`.
				poly_cell_indices.resize_initialized(next_dim_poly_index + 1);
			}
			Vector<PackedInt32Array> next_dim_cell_indices = poly_cell_indices[next_dim_poly_index];
			PackedInt32Array next_dim_cell_to_extruded_cell;
			for (int cell_index = 0; cell_index < input_cells.size(); cell_index++) {
				const PackedInt32Array cell_to_lower_dim_indices = input_cells[cell_index];
				PackedInt32Array new_cell = { cell_index }; // First copy.
				for (int elem_index = 0; elem_index < cell_to_lower_dim_indices.size(); elem_index++) {
					// Extruded from lower dimension element. For example, when extruding from faces
					// to cells, prev_dim_cell_to_extruded_cell holds the mapping from edges to extruded
					// faces, and cell_to_lower_dim_indices holds the edges that make up this face,
					// therefore this lets us get the faces created from extruding the edges of this face,
					// which are the sideways faces needed to make the new cell.
					new_cell.append(prev_dim_cell_to_extruded_cell[cell_to_lower_dim_indices[elem_index]]);
				}
				new_cell.append(cell_index + input_cells.size()); // Second copy.
				const int32_t new_cell_index = next_dim_cell_indices.size();
				next_dim_cell_indices.append(new_cell);
				next_dim_cell_to_extruded_cell.append(new_cell_index);
			}
			poly_cell_indices.set(next_dim_poly_index, next_dim_cell_indices);
			all_cell_to_extruded_cell.append(next_dim_cell_to_extruded_cell);
		}
		ret->set_poly_cell_indices(poly_cell_indices);
		// Figure out the boundary normals for the extruded cells. Start by calculating them as-is.
		// Then, depending on the input mesh's data, these will be rectified in some way.
		ret->set_poly_cell_boundary_normals(Vector<VectorN>());
		if (output_boundary_dim_index >= 0) {
			ret->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		}
		CRASH_COND(!ret->is_mesh_data_valid());
		// Data binding keys relative to the input and output dimensions. When the extrusion adds
		// a dimension, the input's boundary data becomes auxiliary data one level below the
		// output's boundary, like how face normals relate to a 4D mesh's cell normals.
		const Vector2i input_per_cell_key = Vector2i(input_dimension - 1, input_dimension - 1);
		const Vector2i input_cell_to_vert_key = Vector2i(input_dimension - 1, 0);
		const Vector2i output_per_cell_key = Vector2i(output_dimension - 1, output_dimension - 1);
		const Vector2i output_cell_to_vert_key = Vector2i(output_dimension - 1, 0);
		// The mapping from the input mesh's boundary cells to the cells extruded from them, which
		// are the boundary cells of the output mesh when the extrusion adds a dimension.
		const bool has_boundary_to_extruded_cell = extrusion_adds_dimension && output_boundary_dim_index >= 0 && output_boundary_dim_index < all_cell_to_extruded_cell.size();
		const PackedInt32Array boundary_to_extruded_cell = has_boundary_to_extruded_cell ? all_cell_to_extruded_cell[output_boundary_dim_index] : PackedInt32Array();
		// Copy over the normals from the original boundary cells, if that data is present.
		HashMap<Vector2i, Vector<Vector<VectorN>>> all_poly_cell_normals = ret->get_all_poly_cell_normals();
		if (has_boundary_to_extruded_cell && all_poly_cell_normals.has(input_per_cell_key) && all_poly_cell_normals[input_per_cell_key].size() == 1) {
			const Vector<VectorN> &input_boundary_normals = all_poly_cell_normals[input_per_cell_key][0];
			CRASH_COND(input_boundary_normals.size() < boundary_to_extruded_cell.size());
			Vector<PackedInt32Array> boundary_cells = poly_cell_indices[output_boundary_dim_index];
			Vector<VectorN> per_cell_normals = ret->get_poly_cell_boundary_normals();
			for (int32_t input_cell_index = 0; input_cell_index < boundary_to_extruded_cell.size(); input_cell_index++) {
				const VectorN input_normal = input_boundary_normals[input_cell_index];
				const int32_t extruded_cell_index = boundary_to_extruded_cell[input_cell_index];
				const VectorN cell_normal = per_cell_normals[extruded_cell_index];
				// The exact boundary cell normal needs to depend on the orientation of the cell,
				// but we can flip it the other way if it's backwards compared to the input's normal.
				if (VectorND::dot(input_normal, cell_normal) < 0.0) {
					per_cell_normals.set(extruded_cell_index, VectorND::negate(cell_normal));
					// Also swap the cell's first two elements to flip its orientation.
					PackedInt32Array boundary_cell = boundary_cells[extruded_cell_index];
					int32_t temp = boundary_cell[0];
					boundary_cell.set(0, boundary_cell[1]);
					boundary_cell.set(1, temp);
					boundary_cells.set(extruded_cell_index, boundary_cell);
				}
			}
			// Write the corrected boundary cell data.
			poly_cell_indices.set(output_boundary_dim_index, boundary_cells);
			ret->set_poly_cell_indices(poly_cell_indices);
			// The boundary normals themselves will be recalculated at the end of this function,
			// but write the result back anyway for internal consistency.
			all_poly_cell_normals.insert(output_per_cell_key, Vector<Vector<VectorN>>{ per_cell_normals });
			ret->set_all_poly_cell_normals(all_poly_cell_normals);
		} else {
			// Otherwise, if there is no data to copy over, calculate new boundary normals for the extruded cells.
			// Ensure boundary cells are correctly oriented with outward facing normals. This only works when we
			// have volumetric cells to indicate which side of a boundary cell is the inside vs the outside,
			// which came from the input mesh's own boundary and volumetric cells.
			const int64_t output_volumetric_dim_index = int64_t(output_dimension) - 2;
			if (output_boundary_dim_index >= 0 && poly_cell_indices.size() > output_volumetric_dim_index) {
				const Vector<VectorN> &output_vertices = ret->get_poly_cell_vertices();
				const Vector<PackedInt32Array> volume_vert = ret->get_all_poly_cell_vertex_indices(output_dimension, false);
				const Vector<PackedInt32Array> volume_cells = poly_cell_indices[output_volumetric_dim_index];
				const Vector<PackedInt32Array> boundary_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 1, false);
				const Vector<VectorN> boundary_normals = ret->get_poly_cell_boundary_normals();
				Vector<PackedInt32Array> boundary_cells = poly_cell_indices[output_boundary_dim_index];
				CRASH_COND(volume_vert.size() != volume_cells.size() || boundary_vert.size() != boundary_cells.size() || boundary_normals.size() != boundary_cells.size());
				// Compute the average position of each boundary cell.
				Vector<VectorN> boundary_average_pos;
				boundary_average_pos.resize(boundary_cells.size());
				for (int64_t boundary_cell_index = 0; boundary_cell_index < boundary_cells.size(); boundary_cell_index++) {
					const PackedInt32Array &boundary_cell_vert_indices = boundary_vert[boundary_cell_index];
					VectorN average_pos;
					for (int64_t i = 0; i < boundary_cell_vert_indices.size(); i++) {
						average_pos = VectorND::add(average_pos, output_vertices[boundary_cell_vert_indices[i]]);
					}
					average_pos = VectorND::divide_scalar(average_pos, boundary_cell_vert_indices.size());
					boundary_average_pos.set(boundary_cell_index, average_pos);
				}
				// Iterate over each volume.
				for (int64_t volume_index = 0; volume_index < volume_cells.size(); volume_index++) {
					// Compute the average position of this volume.
					const PackedInt32Array &volume_cell_vert_indices = volume_vert[volume_index];
					VectorN volume_average_pos;
					for (int64_t i = 0; i < volume_cell_vert_indices.size(); i++) {
						volume_average_pos = VectorND::add(volume_average_pos, output_vertices[volume_cell_vert_indices[i]]);
					}
					volume_average_pos = VectorND::divide_scalar(volume_average_pos, volume_cell_vert_indices.size());
					// Ensure each boundary cell of this volume is oriented with its normal facing outward from the volume.
					const PackedInt32Array &volume_cell_boundary_indices = volume_cells[volume_index];
					for (int64_t i = 0; i < volume_cell_boundary_indices.size(); i++) {
						const int64_t boundary_cell_index = volume_cell_boundary_indices[i];
						const VectorN out = VectorND::subtract(boundary_average_pos[boundary_cell_index], volume_average_pos);
						if (VectorND::dot(boundary_normals[boundary_cell_index], out) < 0.0) {
							// This boundary cell is facing inward, so swap the first two members to flip the normal to face outward.
							PackedInt32Array boundary_cell = boundary_cells[boundary_cell_index];
							int32_t temp = boundary_cell[0];
							boundary_cell.set(0, boundary_cell[1]);
							boundary_cell.set(1, temp);
							boundary_cells.set(boundary_cell_index, boundary_cell);
						}
					}
				}
				poly_cell_indices.set(output_boundary_dim_index, boundary_cells);
				ret->set_poly_cell_indices(poly_cell_indices);
			}
			// The new boundary cells created from the extrusion may have inconsistent normal directions.
			// Fix them, using the two copies of the input mesh's boundary cells as authoritative references.
			if (output_boundary_dim_index >= 0 && input_poly_cell_indices.size() > output_boundary_dim_index) {
				const int32_t input_boundary_cell_count = (int32_t)input_poly_cell_indices[output_boundary_dim_index].size();
				if (input_boundary_cell_count > 0) {
					PackedInt32Array authoritative_boundary_cells;
					authoritative_boundary_cells.resize(input_boundary_cell_count * 2);
					for (int32_t i = 0; i < input_boundary_cell_count * 2; i++) {
						authoritative_boundary_cells.set(i, i);
					}
					make_boundary_normals_topologically_consistent(ret, authoritative_boundary_cells);
				}
			}
		}
		// Copy over the vertex normals from the original boundary cells, if that data is present.
		// Unlike per-cell normals, there is no need to rectify or generate fallback data when missing.
		if (has_boundary_to_extruded_cell && all_poly_cell_normals.has(input_cell_to_vert_key)) {
			Vector<Vector<VectorN>> input_level_vert_normals = all_poly_cell_normals[input_cell_to_vert_key];
			// This data currently only contains one copy of the input cell vertex normals, copy it again.
			const int64_t input_boundary_cell_count = input_poly_cell_indices[output_boundary_dim_index - 1].size();
			input_level_vert_normals.resize(input_boundary_cell_count); // Just in case the original size was smaller due to missing data. Empty entries are fine.
			input_level_vert_normals.append_array(input_level_vert_normals); // New size will be 2x the input boundary cell count.
			all_poly_cell_normals.insert(input_cell_to_vert_key, input_level_vert_normals);
			// Now transfer the input cell vertex normals to the extruded cell vertex normals.
			const Vector<VectorN> &per_cell_normals = ret->get_poly_cell_boundary_normals();
			const Vector<PackedInt32Array> all_input_level_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 2, false);
			const Vector<PackedInt32Array> all_cell_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 1, false);
			Vector<Vector<VectorN>> cell_to_vert_normals = all_poly_cell_normals.has(output_cell_to_vert_key) ? all_poly_cell_normals[output_cell_to_vert_key] : Vector<Vector<VectorN>>();
			cell_to_vert_normals.resize(all_cell_vert.size());
			for (int input_cell_index = 0; input_cell_index < input_boundary_cell_count; input_cell_index++) {
				const Vector<VectorN> &input_cell_vert_normals = input_level_vert_normals[input_cell_index];
				const PackedInt32Array &first_copy_cell_vert = all_input_level_vert[input_cell_index];
				// The second copy being offset by the input cell count is guaranteed because we start with `merge_with`.
				const PackedInt32Array &second_copy_cell_vert = all_input_level_vert[input_cell_index + input_boundary_cell_count];
				const int64_t cell_index = boundary_to_extruded_cell[input_cell_index];
				const PackedInt32Array &this_cell_vert = all_cell_vert[cell_index];
				Vector<VectorN> cell_vert_normals;
				cell_vert_normals.resize(this_cell_vert.size());
				for (int64_t vert_in_cell = 0; vert_in_cell < this_cell_vert.size(); vert_in_cell++) {
					const int32_t vert_index = this_cell_vert[vert_in_cell];
					const int64_t vert_in_first_copy = first_copy_cell_vert.find(vert_index);
					const int64_t vert_in_second_copy = second_copy_cell_vert.find(vert_index);
					if (vert_in_first_copy != -1 && vert_in_first_copy < input_cell_vert_normals.size()) {
						cell_vert_normals.set(vert_in_cell, input_cell_vert_normals[vert_in_first_copy]);
					} else if (vert_in_second_copy != -1 && vert_in_second_copy < input_cell_vert_normals.size()) {
						cell_vert_normals.set(vert_in_cell, input_cell_vert_normals[vert_in_second_copy]);
					} else {
						// Neither copy has vertex normal data for this vertex in this cell, so just use the cell's boundary normal.
						cell_vert_normals.set(vert_in_cell, per_cell_normals[cell_index]);
					}
				}
				cell_to_vert_normals.set(cell_index, cell_vert_normals);
			}
			all_poly_cell_normals.insert(output_cell_to_vert_key, cell_to_vert_normals);
			ret->set_all_poly_cell_normals(all_poly_cell_normals);
		}
		// Copy over the vertex texture maps from the original boundary cells, if that data is present.
		HashMap<Vector2i, Vector<Vector<VectorM>>> all_poly_cell_texture_maps = ret->get_all_poly_cell_texture_maps();
		if (has_boundary_to_extruded_cell && all_poly_cell_texture_maps.has(input_cell_to_vert_key)) {
			Vector<Vector<VectorM>> input_level_texture_maps = all_poly_cell_texture_maps[input_cell_to_vert_key];
			// This data currently only contains one copy of the input cell vertex texture maps, copy it again.
			const int64_t input_boundary_cell_count = input_poly_cell_indices[output_boundary_dim_index - 1].size();
			const int64_t output_texture_dimension = output_dimension - 1;
			// Special case for texture maps: The second copy should be offset in the new texture axis direction.
			input_level_texture_maps.resize(input_boundary_cell_count * 2);
			for (int64_t input_cell_index = 0; input_cell_index < input_boundary_cell_count; input_cell_index++) {
				Vector<VectorM> cell_vert_texture_map = input_level_texture_maps[input_cell_index];
				for (int64_t vert_index = 0; vert_index < cell_vert_texture_map.size(); vert_index++) {
					VectorM tex_coord = VectorND::with_dimension(cell_vert_texture_map[vert_index], output_texture_dimension);
					// Adding 1.0 instead of setting to 1.0 handles the case of existing non-zero values in the input texture maps.
					tex_coord.set(output_texture_dimension - 1, tex_coord[output_texture_dimension - 1] + 1.0);
					cell_vert_texture_map.set(vert_index, tex_coord);
				}
				input_level_texture_maps.set(input_cell_index + input_boundary_cell_count, cell_vert_texture_map);
			}
			all_poly_cell_texture_maps.insert(input_cell_to_vert_key, input_level_texture_maps);
			// Now transfer the input cell vertex texture maps to the extruded cell vertex texture maps.
			const Vector<PackedInt32Array> all_input_level_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 2, false);
			const Vector<PackedInt32Array> all_cell_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 1, false);
			Vector<Vector<VectorM>> cell_to_vert_texture_maps = all_poly_cell_texture_maps.has(output_cell_to_vert_key) ? all_poly_cell_texture_maps[output_cell_to_vert_key] : Vector<Vector<VectorM>>();
			cell_to_vert_texture_maps.resize(all_cell_vert.size());
			for (int input_cell_index = 0; input_cell_index < input_boundary_cell_count; input_cell_index++) {
				const Vector<VectorM> &first_copy_cell_vert_texture_maps = input_level_texture_maps[input_cell_index];
				const PackedInt32Array &first_copy_cell_vert_ind = all_input_level_vert[input_cell_index];
				// The second copy being offset by the input cell count is guaranteed because we start with `merge_with`.
				const Vector<VectorM> &second_copy_cell_vert_texture_maps = input_level_texture_maps[input_cell_index + input_boundary_cell_count];
				const PackedInt32Array &second_copy_cell_vert_ind = all_input_level_vert[input_cell_index + input_boundary_cell_count];
				const int64_t cell_index = boundary_to_extruded_cell[input_cell_index];
				const PackedInt32Array &this_cell_vert = all_cell_vert[cell_index];
				Vector<VectorM> cell_vert_texture_maps;
				cell_vert_texture_maps.resize(this_cell_vert.size());
				for (int64_t vert_in_cell = 0; vert_in_cell < this_cell_vert.size(); vert_in_cell++) {
					const int32_t vert_index = this_cell_vert[vert_in_cell];
					const int64_t vert_in_first_copy = first_copy_cell_vert_ind.find(vert_index);
					const int64_t vert_in_second_copy = second_copy_cell_vert_ind.find(vert_index);
					if (vert_in_first_copy != -1 && vert_in_first_copy < first_copy_cell_vert_texture_maps.size()) {
						cell_vert_texture_maps.set(vert_in_cell, first_copy_cell_vert_texture_maps[vert_in_first_copy]);
					} else if (vert_in_second_copy != -1 && vert_in_second_copy < second_copy_cell_vert_texture_maps.size()) {
						cell_vert_texture_maps.set(vert_in_cell, second_copy_cell_vert_texture_maps[vert_in_second_copy]);
					} else {
						// Neither copy has vertex texture map data for this vertex in this cell, so just use a default value.
						cell_vert_texture_maps.set(vert_in_cell, VectorND::zero(output_texture_dimension));
					}
				}
				cell_to_vert_texture_maps.set(cell_index, cell_vert_texture_maps);
			}
			all_poly_cell_texture_maps.insert(output_cell_to_vert_key, cell_to_vert_texture_maps);
			ret->set_all_poly_cell_texture_maps(all_poly_cell_texture_maps);
		}
	}
	// Overwrite the cells and recalculate the normals again to ensure data consistency.
	if (output_boundary_dim_index >= 0 && ret->get_poly_cell_indices().size() > output_boundary_dim_index) {
		ret->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
	}
	CRASH_COND(!ret->is_mesh_data_valid());
	return ret;
}

// In-place adjustments to the given mesh.

void PolyMeshBuilderND::make_boundary_normals_topologically_consistent(const Ref<ArrayPolyMeshND> &p_mesh_nd, const PackedInt32Array &p_authoritative) {
	// TODO: This function relies on averages and pivot overrides, which breaks in non-convex edge cases.
	// Properly solving this in ND is non-trivial, this can be improved in the future if needed.
	ERR_FAIL_COND_MSG(p_mesh_nd.is_null(), "Mesh is null.");
	const int dimension = p_mesh_nd->get_dimension();
	// The boundary cells are made of (dimension - 2)-dimensional members,
	// which are the edges for 3D meshes, the 2D faces for 4D meshes, and so on.
	const int64_t boundary_dim_index = int64_t(dimension) - 3;
	ERR_FAIL_COND_MSG(boundary_dim_index < 0, "Mesh must be at least 3-dimensional in order to have boundary normals.");
	Vector<Vector<PackedInt32Array>> poly_cell_indices = p_mesh_nd->get_poly_cell_indices();
	ERR_FAIL_COND_MSG(poly_cell_indices.size() <= boundary_dim_index, "Mesh must have boundary cells in order to have boundary normals.");
	// Clear out and calculate new normals. The correct ones will either be these, or the negatives of these.
	const Vector<VectorN> original_boundary_normals = p_mesh_nd->get_poly_cell_boundary_normals();
	p_mesh_nd->set_poly_cell_boundary_normals(Vector<VectorN>());
	p_mesh_nd->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
	CRASH_COND(!p_mesh_nd->is_mesh_data_valid());
	Vector<VectorN> boundary_normals = p_mesh_nd->get_poly_cell_boundary_normals();
	for (int64_t auth_index = 0; auth_index < p_authoritative.size(); auth_index++) {
		const int64_t boundary_cell_index = p_authoritative[auth_index];
		ERR_FAIL_INDEX_MSG(boundary_cell_index, boundary_normals.size(), "Authoritative boundary cell index is out of range.");
		ERR_FAIL_INDEX_MSG(boundary_cell_index, original_boundary_normals.size(), "Authoritative boundary cell index is out of range of the mesh's existing boundary normals.");
		boundary_normals.set(boundary_cell_index, original_boundary_normals[boundary_cell_index]);
	}
	// Build a map of members to the cells that reference them, which will be used to traverse adjacent boundary cells.
	Vector<PackedInt32Array> boundary_cells = poly_cell_indices[boundary_dim_index];
	const int64_t member_count = boundary_dim_index == 0 ? p_mesh_nd->get_edge_indices().size() / 2 : poly_cell_indices[boundary_dim_index - 1].size();
	Vector<PackedInt32Array> members_to_cells;
	members_to_cells.resize(member_count);
	for (int64_t cell_index = 0; cell_index < boundary_cells.size(); cell_index++) {
		const PackedInt32Array cell_member_indices = boundary_cells[cell_index];
		for (int64_t member_num = 0; member_num < cell_member_indices.size(); member_num++) {
			const int64_t member_index = cell_member_indices[member_num];
			PackedInt32Array cells_for_member = members_to_cells[member_index];
			cells_for_member.append(cell_index);
			members_to_cells.set(member_index, cells_for_member);
		}
	}
	// Compute the average position of each boundary cell.
	const Vector<VectorN> &poly_vertices = p_mesh_nd->get_poly_cell_vertices();
	const Vector<PackedInt32Array> boundary_vert = p_mesh_nd->get_all_poly_cell_vertex_indices(dimension - 1, false);
	const PackedInt32Array &boundary_pivot_overrides = p_mesh_nd->get_poly_cell_boundary_pivot_overrides();
	CRASH_COND(boundary_vert.size() != boundary_cells.size());
	Vector<VectorN> boundary_pivot_pos;
	boundary_pivot_pos.resize(boundary_cells.size());
	for (int64_t boundary_cell_index = 0; boundary_cell_index < boundary_cells.size(); boundary_cell_index++) {
		const PackedInt32Array &boundary_cell_vert_indices = boundary_vert[boundary_cell_index];
		VectorN average_pos;
		for (int64_t i = 0; i < boundary_cell_vert_indices.size(); i++) {
			average_pos = VectorND::add(average_pos, poly_vertices[boundary_cell_vert_indices[i]]);
		}
		average_pos = VectorND::divide_scalar(average_pos, boundary_cell_vert_indices.size());
		// If the pivot is overridden, use it, but also still take the average and use it for a slight offset.
		// This ensures that pivot overrides on the sides will still give something inside the cell.
		if (boundary_pivot_overrides.size() > boundary_cell_index && boundary_pivot_overrides[boundary_cell_index] != -1) {
			constexpr double ONE_PLUS_EPSILON = 1.0 + CMP_EPSILON;
			const int32_t pivot_vert_index = boundary_pivot_overrides[boundary_cell_index];
			average_pos = VectorND::divide_scalar(VectorND::add(VectorND::multiply_scalar(average_pos, ONE_PLUS_EPSILON - 1.0), poly_vertices[pivot_vert_index]), ONE_PLUS_EPSILON);
		}
		boundary_pivot_pos.set(boundary_cell_index, average_pos);
	}
	// Visit each boundary cell, check its neighbors, and flip normals as needed to ensure they are all consistent.
	PackedInt32Array settled = p_authoritative;
	PackedInt32Array to_visit_its_neighbors = p_authoritative;
	while (!to_visit_its_neighbors.is_empty()) {
		const int64_t this_cell_index = to_visit_its_neighbors[to_visit_its_neighbors.size() - 1];
		to_visit_its_neighbors.resize(to_visit_its_neighbors.size() - 1);
		const PackedInt32Array cell_member_indices = boundary_cells[this_cell_index];
		const VectorN this_cell_pivot = boundary_pivot_pos[this_cell_index];
		const VectorN this_cell_normal = boundary_normals[this_cell_index];
		for (int64_t member_num = 0; member_num < cell_member_indices.size(); member_num++) {
			const int64_t member_index = cell_member_indices[member_num];
			const PackedInt32Array adjacent_cells = members_to_cells[member_index];
			for (int64_t adjacent_cell_num = 0; adjacent_cell_num < adjacent_cells.size(); adjacent_cell_num++) {
				const int64_t adjacent_cell_index = adjacent_cells[adjacent_cell_num];
				if (adjacent_cell_index == this_cell_index) {
					continue;
				}
				if (settled.find(adjacent_cell_index) != -1) {
					continue;
				}
				to_visit_its_neighbors.append(adjacent_cell_index);
				settled.append(adjacent_cell_index);
				const VectorN adjacent_boundary_cell_average = boundary_pivot_pos[adjacent_cell_index];
				const VectorN adjacent_boundary_cell_normal = boundary_normals[adjacent_cell_index];
				const double source_to_adjacent = VectorND::dot(this_cell_normal, VectorND::subtract(adjacent_boundary_cell_average, this_cell_pivot));
				const double adjacent_to_source = VectorND::dot(adjacent_boundary_cell_normal, VectorND::subtract(this_cell_pivot, adjacent_boundary_cell_average));
				bool should_flip_adjacent = false;
				if (Math::is_zero_approx(source_to_adjacent) || Math::is_zero_approx(adjacent_to_source)) {
					// Coplanar boundary cells should just be oriented the same.
					if (VectorND::dot(this_cell_normal, adjacent_boundary_cell_normal) < 0.0) {
						should_flip_adjacent = true;
					}
				} else if (SIGN(source_to_adjacent) != SIGN(adjacent_to_source)) {
					should_flip_adjacent = true;
				}
				if (should_flip_adjacent) {
					// This adjacent boundary cell is facing the wrong way, so swap the first two members to flip.
					PackedInt32Array adjacent_boundary_cell = boundary_cells[adjacent_cell_index];
					int32_t temp = adjacent_boundary_cell[0];
					adjacent_boundary_cell.set(0, adjacent_boundary_cell[1]);
					adjacent_boundary_cell.set(1, temp);
					boundary_cells.set(adjacent_cell_index, adjacent_boundary_cell);
					boundary_normals.set(adjacent_cell_index, VectorND::negate(adjacent_boundary_cell_normal));
				}
			}
		}
	}
	poly_cell_indices.set(boundary_dim_index, boundary_cells);
	p_mesh_nd->set_poly_cell_indices(poly_cell_indices);
	p_mesh_nd->set_poly_cell_boundary_normals(boundary_normals);
}

PolyMeshBuilderND *PolyMeshBuilderND::singleton = nullptr;

void PolyMeshBuilderND::_bind_methods() {
	// These functions create new meshes from the given data.
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("convert_mesh_3d_to_nd_faces_only", "mesh_3d", "which_surface", "deduplicate"), &PolyMeshBuilderND::convert_mesh_3d_to_nd_faces_only, DEFVAL(-1), DEFVAL(true));
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("extrude_linear", "input_mesh", "extrusion_vector"), &PolyMeshBuilderND::extrude_linear, DEFVAL(VectorN()));
	// In-place adjustments to the given mesh.
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("make_boundary_normals_topologically_consistent", "mesh_nd", "authoritative_boundary_cells"), &PolyMeshBuilderND::make_boundary_normals_topologically_consistent);
}
