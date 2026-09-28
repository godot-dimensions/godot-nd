#include "poly_mesh_builder_nd.h"

#include "../../../math/math_nd.h"
#include "../../../math/vector_nd.h"

Ref<ArrayPolyMeshND> PolyMeshBuilderND::convert_mesh_3d_to_nd_faces_only(const Ref<Mesh> &p_mesh_3d, const int p_which_surface, const bool p_deduplicate) {
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
		CRASH_COND(surface_arrays.size() < Mesh::ARRAY_MAX); // Mesh should always return surfaces arrays of length Mesh::ARRAY_MAX, even if some of them are empty.
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
	ret->set_poly_cell_vertex_positions(output_vertices);
	ret->set_poly_cell_indices(Vector<Vector<PackedInt32Array>>{ output_face_indices });
	if (!output_face_boundary_normals.is_empty()) {
		ret->set_poly_cell_boundary_normals(output_face_boundary_normals);
	}
	if (!output_face_vertex_normals.is_empty()) {
		// The output mesh is 3D, so its cell-to-vertex data binding key is (2, 0).
		ret->set_poly_cell_dense_normals(Vector2i(2, 0), output_face_vertex_normals);
	}
	if (!output_face_texture_maps.is_empty()) {
		ret->set_poly_cell_dense_texture_map(Vector2i(2, 0), output_face_texture_maps);
	}
	return ret;
}

int64_t PolyMeshBuilderND::_find_corner_value(const int32_t p_vertex, const PackedInt32Array &p_source_elements, const Vector<PackedInt32Array> &p_source_corners, const Vector<Vector<PackedFloat64Array>> &p_source_values, const CornerSampleMode p_mode, PackedFloat64Array &r_value) {
	int64_t hits = 0;
	PackedFloat64Array sum;
	for (const int32_t source : p_source_elements) {
		if (source < 0 || source >= p_source_corners.size() || source >= p_source_values.size()) {
			continue;
		}
		const int64_t corner = p_source_corners[source].find(p_vertex);
		if (corner == -1 || corner >= p_source_values[source].size()) {
			continue;
		}
		if (p_mode == CORNER_SAMPLE_FIRST_FOUND) {
			r_value = p_source_values[source][corner];
			return 1;
		}
		sum = VectorND::add(sum, p_source_values[source][corner]);
		hits++;
	}
	if (hits > 0) {
		r_value = VectorND::divide_scalar(sum, (double)hits);
	}
	return hits;
}

int64_t PolyMeshBuilderND::_sample_corner_values(const PackedInt32Array &p_new_corners, const PackedInt32Array &p_source_elements, const Vector<PackedInt32Array> &p_source_corners, const Vector<Vector<PackedFloat64Array>> &p_source_values, const CornerSampleMode p_mode, const PackedFloat64Array &p_fallback, Vector<PackedFloat64Array> &r_values, const bool p_normalize, const Vector<PackedInt32Array> *p_derived_vertex_sources, const int64_t p_first_derived_vertex) {
	r_values.resize(p_new_corners.size());
	int64_t sampled_count = 0;
	for (int64_t i = 0; i < p_new_corners.size(); i++) {
		const int32_t vertex = p_new_corners[i];
		PackedFloat64Array value = p_fallback;
		int64_t hits = 0;
		if (p_derived_vertex_sources != nullptr && vertex >= p_first_derived_vertex) {
			const int64_t derived_index = vertex - p_first_derived_vertex;
			PackedFloat64Array sum;
			if (derived_index < p_derived_vertex_sources->size()) {
				for (const int32_t source_vertex : (*p_derived_vertex_sources)[derived_index]) {
					PackedFloat64Array source_value;
					if (_find_corner_value(source_vertex, p_source_elements, p_source_corners, p_source_values, p_mode, source_value) > 0) {
						sum = VectorND::add(sum, source_value);
						hits++;
					}
				}
			}
			if (hits > 0) {
				value = VectorND::divide_scalar(sum, (double)hits);
			}
		} else {
			hits = _find_corner_value(vertex, p_source_elements, p_source_corners, p_source_values, p_mode, value);
		}
		if (hits > 0) {
			sampled_count++;
			if (p_normalize && VectorND::length_squared(value) > (double)CMP_EPSILON) {
				value = VectorND::normalized(value);
			}
		}
		r_values.set(i, value);
	}
	return sampled_count;
}

void PolyMeshBuilderND::_rematch_extruded_corner_values(Vector<Vector<PackedFloat64Array>> &r_values, const Vector<PackedInt32Array> &p_input_corners, const Vector<PackedInt32Array> &p_output_corners, const int32_t p_input_vertex_count) {
	const int64_t input_element_count = p_input_corners.size();
	for (int64_t copy = 0; copy < 2; copy++) {
		for (int64_t input_index = 0; input_index < input_element_count; input_index++) {
			const int64_t output_index = input_index + copy * input_element_count;
			if (output_index >= r_values.size() || output_index >= p_output_corners.size() || r_values[output_index].is_empty()) {
				continue; // Missing data stays missing.
			}
			const Vector<PackedFloat64Array> &old_values = r_values[output_index];
			PackedInt32Array old_keys = p_input_corners[input_index];
			PackedInt32Array old_positions;
			old_positions.resize(old_values.size());
			for (int32_t i = 0; i < old_keys.size(); i++) {
				old_keys.set(i, old_keys[i] + (int32_t)copy * p_input_vertex_count);
			}
			for (int32_t i = 0; i < old_positions.size(); i++) {
				old_positions.set(i, i);
			}
			// Match the corners by vertex identity, then read the values from the matched positions.
			const PackedInt32Array new_positions = MathND::remap_int32s_by_matching_keys(old_keys, p_output_corners[output_index], old_positions);
			CRASH_COND(new_positions.has(-1)); // Both lists hold the same vertices, since the extruded elements are copies.
			Vector<PackedFloat64Array> new_values;
			new_values.resize(new_positions.size());
			for (int64_t i = 0; i < new_positions.size(); i++) {
				new_values.set(i, old_values[new_positions[i]]);
			}
			r_values.set(output_index, new_values);
		}
	}
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
	ret->transform_mesh(TransformND::from_position(VectorND::negate(extrusion_vector)));
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
		const Vector<VectorN> &input_vertices = p_input_mesh->get_poly_cell_vertex_positions();
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
		// This function works with dense normal and texture map data, sampled from
		// the indexed data on read and converted back to indexed data when written.
		HashMap<Vector2i, Vector<Vector<VectorN>>> all_poly_cell_normals;
		const HashMap<Vector2i, Vector<PackedInt32Array>> all_poly_cell_normal_indices = ret->get_all_poly_cell_normal_indices();
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &normal_kv : all_poly_cell_normal_indices) {
			all_poly_cell_normals.insert(normal_kv.key, ret->get_poly_cell_dense_normals(normal_kv.key));
		}
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
					// Also flip the cell's orientation to match.
					PackedInt32Array boundary_cell = boundary_cells[extruded_cell_index];
					PolyMeshND::flip_poly_cell_orientation(boundary_cell, output_boundary_dim_index);
					boundary_cells.set(extruded_cell_index, boundary_cell);
				}
			}
			// Write the corrected boundary cell data.
			poly_cell_indices.set(output_boundary_dim_index, boundary_cells);
			ret->set_poly_cell_indices(poly_cell_indices);
			// The boundary normals themselves will be recalculated at the end of this function,
			// but write the result back anyway for internal consistency.
			all_poly_cell_normals.insert(output_per_cell_key, Vector<Vector<VectorN>>{ per_cell_normals });
			ret->set_poly_cell_dense_normals(output_per_cell_key, Vector<Vector<VectorN>>{ per_cell_normals });
		} else {
			// Otherwise, if there is no data to copy over, calculate new boundary normals for the extruded cells.
			// Ensure boundary cells are correctly oriented with outward facing normals. This only works when we
			// have volumetric cells to indicate which side of a boundary cell is the inside vs the outside,
			// which came from the input mesh's own boundary and volumetric cells.
			const int64_t output_volumetric_dim_index = int64_t(output_dimension) - 2;
			if (output_boundary_dim_index >= 0 && poly_cell_indices.size() > output_volumetric_dim_index) {
				const Vector<VectorN> &output_vertices = ret->get_poly_cell_vertex_positions();
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
							// This boundary cell is facing inward, so flip its orientation to make the normal face outward.
							PackedInt32Array boundary_cell = boundary_cells[boundary_cell_index];
							PolyMeshND::flip_poly_cell_orientation(boundary_cell, output_boundary_dim_index);
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
			// The values are in the input mesh's vertex order, which the extruded mesh may list differently.
			const Vector<PackedInt32Array> all_input_level_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 2, false);
			_rematch_extruded_corner_values(input_level_vert_normals, p_input_mesh->get_all_poly_cell_vertex_indices(input_dimension - 1, false), all_input_level_vert, (int32_t)p_input_mesh->get_poly_cell_vertex_positions().size());
			all_poly_cell_normals.insert(input_cell_to_vert_key, input_level_vert_normals);
			ret->set_poly_cell_dense_normals(input_cell_to_vert_key, input_level_vert_normals);
			// Now transfer the input cell vertex normals to the extruded cell vertex normals.
			const Vector<VectorN> &per_cell_normals = ret->get_poly_cell_boundary_normals();
			const Vector<PackedInt32Array> all_cell_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 1, false);
			Vector<Vector<VectorN>> cell_to_vert_normals = all_poly_cell_normals.has(output_cell_to_vert_key) ? all_poly_cell_normals[output_cell_to_vert_key] : Vector<Vector<VectorN>>();
			cell_to_vert_normals.resize(all_cell_vert.size());
			for (int input_cell_index = 0; input_cell_index < input_boundary_cell_count; input_cell_index++) {
				// The second copy being offset by the input cell count is guaranteed because we start with `merge_with`.
				const PackedInt32Array copy_cells = { (int32_t)input_cell_index, (int32_t)(input_cell_index + input_boundary_cell_count) };
				const int64_t cell_index = boundary_to_extruded_cell[input_cell_index];
				// A vertex in neither copy has no vertex normal data in this cell, so it uses the cell's boundary normal.
				Vector<VectorN> cell_vert_normals;
				_sample_corner_values(all_cell_vert[cell_index], copy_cells, all_input_level_vert, input_level_vert_normals, CORNER_SAMPLE_FIRST_FOUND, per_cell_normals[cell_index], cell_vert_normals);
				cell_to_vert_normals.set(cell_index, cell_vert_normals);
			}
			all_poly_cell_normals.insert(output_cell_to_vert_key, cell_to_vert_normals);
			ret->set_poly_cell_dense_normals(output_cell_to_vert_key, cell_to_vert_normals);
		}
		// Copy over the vertex texture maps from the original boundary cells, if that data is present.
		HashMap<Vector2i, Vector<Vector<VectorM>>> all_poly_cell_texture_maps;
		const HashMap<Vector2i, Vector<PackedInt32Array>> ret_all_poly_cell_texture_map_indices = ret->get_all_poly_cell_texture_map_indices();
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &tex_map_kv : ret_all_poly_cell_texture_map_indices) {
			all_poly_cell_texture_maps.insert(tex_map_kv.key, ret->get_poly_cell_dense_texture_map(tex_map_kv.key));
		}
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
			// The values are in the input mesh's vertex order, which the extruded mesh may list differently.
			const Vector<PackedInt32Array> all_input_level_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 2, false);
			_rematch_extruded_corner_values(input_level_texture_maps, p_input_mesh->get_all_poly_cell_vertex_indices(input_dimension - 1, false), all_input_level_vert, (int32_t)p_input_mesh->get_poly_cell_vertex_positions().size());
			all_poly_cell_texture_maps.insert(input_cell_to_vert_key, input_level_texture_maps);
			ret->set_poly_cell_dense_texture_map(input_cell_to_vert_key, input_level_texture_maps);
			// Now transfer the input cell vertex texture maps to the extruded cell vertex texture maps.
			const Vector<PackedInt32Array> all_cell_vert = ret->get_all_poly_cell_vertex_indices(output_dimension - 1, false);
			Vector<Vector<VectorM>> cell_to_vert_texture_maps = all_poly_cell_texture_maps.has(output_cell_to_vert_key) ? all_poly_cell_texture_maps[output_cell_to_vert_key] : Vector<Vector<VectorM>>();
			cell_to_vert_texture_maps.resize(all_cell_vert.size());
			for (int input_cell_index = 0; input_cell_index < input_boundary_cell_count; input_cell_index++) {
				// The second copy being offset by the input cell count is guaranteed because we start with `merge_with`.
				const PackedInt32Array copy_cells = { (int32_t)input_cell_index, (int32_t)(input_cell_index + input_boundary_cell_count) };
				const int64_t cell_index = boundary_to_extruded_cell[input_cell_index];
				// A vertex in neither copy has no vertex texture map data in this cell, so it uses a default value.
				Vector<VectorM> cell_vert_texture_maps;
				_sample_corner_values(all_cell_vert[cell_index], copy_cells, all_input_level_vert, input_level_texture_maps, CORNER_SAMPLE_FIRST_FOUND, VectorND::zero(output_texture_dimension), cell_vert_texture_maps);
				cell_to_vert_texture_maps.set(cell_index, cell_vert_texture_maps);
			}
			all_poly_cell_texture_maps.insert(output_cell_to_vert_key, cell_to_vert_texture_maps);
			ret->set_poly_cell_dense_texture_map(output_cell_to_vert_key, cell_to_vert_texture_maps);
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

// Coplanar face merging. This intentionally only operates on 2D faces, such as the triangles of a 3D mesh that was
// triangulated when exported (for example to glTF), and does not generalize to merging cells of higher dimensions.

// The vertices of a face whose edges are stored in loop order: vertex i is the one shared by edges i and i + 1.
// Returns an empty array if consecutive edges do not share a vertex, or if a vertex repeats, so the face is not a
// single simple closed loop.
PackedInt32Array PolyMeshBuilderND::_get_loop_face_vertices(const PackedInt32Array &p_face_edges, const PackedInt32Array &p_edge_vertex_indices) {
	const int64_t edge_count = p_face_edges.size();
	PackedInt32Array vertices;
	if (edge_count < 3) {
		return vertices;
	}
	for (int64_t i = 0; i < edge_count; i++) {
		const int32_t a0 = p_edge_vertex_indices[p_face_edges[i] * 2];
		const int32_t a1 = p_edge_vertex_indices[p_face_edges[i] * 2 + 1];
		const int32_t b0 = p_edge_vertex_indices[p_face_edges[(i + 1) % edge_count] * 2];
		const int32_t b1 = p_edge_vertex_indices[p_face_edges[(i + 1) % edge_count] * 2 + 1];
		const int32_t shared_vertex = (a0 == b0 || a0 == b1) ? a0 : ((a1 == b0 || a1 == b1) ? a1 : -1);
		if (shared_vertex == -1 || vertices.has(shared_vertex)) {
			return PackedInt32Array();
		}
		vertices.append(shared_vertex);
	}
	return vertices;
}

// Rotates a loop-ordered face so that the edges it shares with another face come last, and returns the edges
// before them. Fails if the shared edges are not one contiguous run, if nothing is shared, or if everything is.
bool PolyMeshBuilderND::_rotate_out_shared_edges(const PackedInt32Array &p_face_edges, const PackedInt32Array &p_other_face_edges, PackedInt32Array &r_remainder) {
	const int64_t edge_count = p_face_edges.size();
	int64_t shared_count = 0;
	int64_t remainder_start = -1;
	for (int64_t i = 0; i < edge_count; i++) {
		if (p_other_face_edges.has(p_face_edges[i])) {
			shared_count++;
		} else if (p_other_face_edges.has(p_face_edges[(i + edge_count - 1) % edge_count])) {
			remainder_start = i; // An unshared edge right after a shared one starts the remainder.
		}
	}
	if (shared_count == 0 || shared_count == edge_count || remainder_start == -1) {
		return false;
	}
	r_remainder.clear();
	for (int64_t i = 0; i < edge_count - shared_count; i++) {
		const int32_t edge_index = p_face_edges[(remainder_start + i) % edge_count];
		if (p_other_face_edges.has(edge_index)) {
			return false; // A second shared run, so the faces touch along more than one run of edges.
		}
		r_remainder.append(edge_index);
	}
	return true;
}

// Finds an orthonormal basis of the 2D plane a face lies in, from its first vertex and the two directions that
// span it most robustly. Fails for a degenerate face whose vertices are all collinear within the tolerance.
bool PolyMeshBuilderND::_get_face_plane_basis(const PackedInt32Array &p_vertex_loop, const Vector<VectorN> &p_positions, const double p_sin_tolerance, VectorN &r_origin, VectorN &r_basis_u, VectorN &r_basis_v) {
	r_origin = p_positions[p_vertex_loop[0]];
	// The first basis direction goes to the farthest vertex, and the second is the largest rejection from it.
	double best_length_squared = 0.0;
	for (int64_t i = 1; i < p_vertex_loop.size(); i++) {
		const VectorN offset = VectorND::subtract(p_positions[p_vertex_loop[i]], r_origin);
		const double length_squared = VectorND::length_squared(offset);
		if (length_squared > best_length_squared) {
			best_length_squared = length_squared;
			r_basis_u = offset;
		}
	}
	if (best_length_squared == 0.0) {
		return false;
	}
	r_basis_u = VectorND::normalized(r_basis_u);
	double best_rejection_ratio = p_sin_tolerance;
	bool found_v = false;
	for (int64_t i = 1; i < p_vertex_loop.size(); i++) {
		const VectorN offset = VectorND::subtract(p_positions[p_vertex_loop[i]], r_origin);
		const VectorN rejection = VectorND::subtract(offset, VectorND::multiply_scalar(r_basis_u, VectorND::dot(r_basis_u, offset)));
		const double ratio = VectorND::length(rejection) / MAX(VectorND::length(offset), (double)CMP_EPSILON);
		if (ratio > best_rejection_ratio) {
			best_rejection_ratio = ratio;
			r_basis_v = rejection;
			found_v = true;
		}
	}
	if (!found_v) {
		return false;
	}
	r_basis_v = VectorND::normalized(r_basis_v);
	return true;
}

// Checks whether two loop-ordered faces sharing a run of edges lie in the same plane and merge into one convex
// face. If so, writes the merged face's edges, in loop order, without the shared edges.
bool PolyMeshBuilderND::_try_merge_coplanar_face_pair(const PackedInt32Array &p_face_a_edges, const PackedInt32Array &p_face_b_edges, const PackedInt32Array &p_edge_vertex_indices, const Vector<VectorN> &p_positions, const double p_sin_tolerance, PackedInt32Array &r_merged_edges) {
	// Coplanar means every vertex of B lies in A's plane, up to the angular tolerance relative to its distance.
	const PackedInt32Array loop_a = _get_loop_face_vertices(p_face_a_edges, p_edge_vertex_indices);
	const PackedInt32Array loop_b = _get_loop_face_vertices(p_face_b_edges, p_edge_vertex_indices);
	VectorN origin, basis_u, basis_v;
	if (loop_a.is_empty() || loop_b.is_empty() || !_get_face_plane_basis(loop_a, p_positions, p_sin_tolerance, origin, basis_u, basis_v)) {
		return false;
	}
	for (const int32_t vertex_index : loop_b) {
		const VectorN offset = VectorND::subtract(p_positions[vertex_index], origin);
		const VectorN in_plane = VectorND::add(VectorND::multiply_scalar(basis_u, VectorND::dot(basis_u, offset)), VectorND::multiply_scalar(basis_v, VectorND::dot(basis_v, offset)));
		if (VectorND::length(VectorND::subtract(offset, in_plane)) > p_sin_tolerance * VectorND::length(offset)) {
			return false;
		}
	}
	// Splice the two loops around their shared run: A's remaining edges followed by B's, which may need reversing
	// to continue where A's end. The result must itself be a single simple loop, which rules out pinched shapes.
	PackedInt32Array remainder_a, remainder_b;
	if (!_rotate_out_shared_edges(p_face_a_edges, p_face_b_edges, remainder_a) || !_rotate_out_shared_edges(p_face_b_edges, p_face_a_edges, remainder_b)) {
		return false;
	}
	PackedInt32Array merged_edges = remainder_a;
	merged_edges.append_array(remainder_b);
	PackedInt32Array merged_loop = _get_loop_face_vertices(merged_edges, p_edge_vertex_indices);
	if (merged_loop.is_empty()) {
		remainder_b.reverse();
		merged_edges = remainder_a;
		merged_edges.append_array(remainder_b);
		merged_loop = _get_loop_face_vertices(merged_edges, p_edge_vertex_indices);
		if (merged_loop.is_empty()) {
			return false;
		}
	}
	// The merged polygon must be convex, since faces are triangulated as fans from one of their vertices.
	// Consecutive edges must all turn the same way in the plane, allowing straight continuations.
	const int64_t loop_size = merged_loop.size();
	double turn_sign = 0.0;
	int64_t first_corner = -1;
	for (int64_t i = 0; i < loop_size; i++) {
		const VectorN previous = p_positions[merged_loop[(i + loop_size - 1) % loop_size]];
		const VectorN current = p_positions[merged_loop[i]];
		const VectorN next = p_positions[merged_loop[(i + 1) % loop_size]];
		const VectorN edge_1 = VectorND::subtract(current, previous);
		const VectorN edge_2 = VectorND::subtract(next, current);
		const double edge_1_u = VectorND::dot(basis_u, edge_1);
		const double edge_1_v = VectorND::dot(basis_v, edge_1);
		const double edge_2_u = VectorND::dot(basis_u, edge_2);
		const double edge_2_v = VectorND::dot(basis_v, edge_2);
		const double cross = edge_1_u * edge_2_v - edge_1_v * edge_2_u;
		const double edge_lengths = Math::sqrt((edge_1_u * edge_1_u + edge_1_v * edge_1_v) * (edge_2_u * edge_2_u + edge_2_v * edge_2_v));
		if (Math::abs(cross) <= p_sin_tolerance * edge_lengths) {
			continue; // Straight continuation.
		}
		if (turn_sign == 0.0) {
			turn_sign = SIGN(cross);
			first_corner = i;
		} else if (SIGN(cross) != turn_sign) {
			return false; // Concave corner.
		}
	}
	if (first_corner == -1) {
		return false; // Every vertex is a straight continuation, so the merged polygon is degenerate.
	}
	// Rotate the loop to start at the two edges meeting at a real corner, since the face's canonical span is
	// taken from its first two edges, and a straight continuation would make that span degenerate.
	r_merged_edges.clear();
	for (int64_t i = 0; i < loop_size; i++) {
		r_merged_edges.append(merged_edges[(first_corner + i) % loop_size]);
	}
	return true;
}

// Merging faces changes the edge and face lists, and therefore the sub-elements of everything above them. Per-vertex
// bindings, and bindings of higher cells to their cells, are keyed by elements the merge leaves in place.
bool PolyMeshBuilderND::_is_binding_affected_by_face_merge(const Vector2i &p_key) {
	return p_key.x == 1 || p_key.x == 2 || (p_key.x >= 3 && p_key.y <= 2);
}

// Rebuilds one binding of value indices after faces were merged. Each new element takes the values of its
// sub-elements from the old elements behind it (`p_element_sources`, or each element itself when empty), matched
// by sub-element identity after the old sub-element indices are translated through `p_sub_element_old_to_new`
// (empty when that dimension did not change). A new element whose sub-elements cannot all be matched, such as
// one whose sources had no data, gets an empty entry, meaning missing data. Flat bindings take the value of each
// new element's first source.
Vector<PackedInt32Array> PolyMeshBuilderND::_remap_binding_after_face_merge(const Vector2i &p_key, const Vector<PackedInt32Array> &p_old_binding, const Vector<PackedInt32Array> &p_element_sources, const Vector<PackedInt32Array> &p_old_sub_elements, const Vector<PackedInt32Array> &p_new_sub_elements, const PackedInt32Array &p_sub_element_old_to_new) {
	Vector<PackedInt32Array> ret;
	if (p_key.x == p_key.y) {
		ERR_FAIL_COND_V(p_old_binding.size() != 1 || p_element_sources.is_empty(), ret);
		const PackedInt32Array &old_flat = p_old_binding[0];
		PackedInt32Array new_flat;
		for (int64_t i = 0; i < p_element_sources.size(); i++) {
			const int32_t source = p_element_sources[i][0];
			if (source >= old_flat.size()) {
				break; // Flat bindings may be shorter than the element count, so the new one stops where the data does.
			}
			new_flat.append(old_flat[source]);
		}
		ret.append(new_flat);
		return ret;
	}
	const int64_t new_count = p_new_sub_elements.size();
	ERR_FAIL_COND_V(!p_element_sources.is_empty() && p_element_sources.size() != new_count, ret);
	ret.resize(new_count);
	for (int64_t i = 0; i < new_count; i++) {
		// Concatenate the sources' sub-elements and values into two parallel arrays, so one lookup covers them all.
		PackedInt32Array old_keys;
		PackedInt32Array old_values;
		const PackedInt32Array sources = p_element_sources.is_empty() ? PackedInt32Array{ (int32_t)i } : p_element_sources[i];
		for (const int32_t source : sources) {
			if (source >= p_old_binding.size() || source >= p_old_sub_elements.size() || p_old_binding[source].size() != p_old_sub_elements[source].size()) {
				continue; // This source has no data, or malformed data.
			}
			old_keys.append_array(p_sub_element_old_to_new.is_empty() ? p_old_sub_elements[source] : MathND::remap_int32_array(p_old_sub_elements[source], p_sub_element_old_to_new));
			old_values.append_array(p_old_binding[source]);
		}
		const PackedInt32Array new_values = MathND::remap_int32s_by_matching_keys(old_keys, p_new_sub_elements[i], old_values);
		if (!new_values.has(-1)) {
			ret.set(i, new_values);
		}
	}
	return ret;
}

int64_t PolyMeshBuilderND::merge_coplanar_faces(const Ref<ArrayPolyMeshND> &p_mesh_nd, const double p_angle_tolerance_radians) {
	ERR_FAIL_COND_V_MSG(p_mesh_nd.is_null() || !p_mesh_nd->is_mesh_data_valid(), 0, "PolyMeshBuilderND: Cannot merge the coplanar faces of an invalid mesh.");
	Vector<Vector<PackedInt32Array>> poly_cell_indices = p_mesh_nd->get_poly_cell_indices();
	if (poly_cell_indices.is_empty()) {
		return 0;
	}
	const int dimension = p_mesh_nd->get_dimension();
	// For 3D meshes, the faces are the boundary cells themselves, so their orientation, boundary normals, pivot
	// overrides, and seams (which are edges) follow the merge. For higher dimensions, the faces are members of
	// other cells, like in 4D, and only the boundary cells above them need their orientation restored.
	const int64_t boundary_dim_index = int64_t(dimension) - 3;
	const bool faces_are_boundary_cells = boundary_dim_index == 0;
	const bool has_boundary_cells = boundary_dim_index >= 0 && poly_cell_indices.size() > boundary_dim_index;
	const PackedInt32Array edge_vertex_indices = p_mesh_nd->get_edge_indices();
	const int64_t edge_count = edge_vertex_indices.size() / 2;
	// Positions may be stored with fewer components than the mesh dimension, so expand them for the plane math.
	Vector<VectorN> positions = p_mesh_nd->get_poly_cell_vertex_positions();
	for (int64_t i = 0; i < positions.size(); i++) {
		positions.set(i, VectorND::with_dimension(positions[i], dimension));
	}
	const double sin_tolerance = Math::sin(p_angle_tolerance_radians);
	Vector<PackedInt32Array> faces = poly_cell_indices[0];
	const int64_t old_face_count = faces.size();
	const bool has_cells = poly_cell_indices.size() > 1;
	// Mesh validation only warns about faces whose edges are not stored in loop order, but merging relies on it.
	for (int64_t face_index = 0; face_index < old_face_count; face_index++) {
		ERR_FAIL_COND_V_MSG(_get_loop_face_vertices(faces[face_index], edge_vertex_indices).is_empty(), 0, "PolyMeshBuilderND: Cannot merge coplanar faces because the edges of face " + itos(face_index) + " are not stored as a single closed loop.");
	}
	// Snapshot everything that the merge invalidates: the bindings as value indices, along with the sub-elements
	// each entry is listed by, the seams, the pivot overrides, and the boundary normals, which restore the boundary
	// cells' orientation at the end.
	Vector<VectorN> old_boundary_normals;
	if (has_boundary_cells) {
		if (p_mesh_nd->get_poly_cell_boundary_normals().size() != poly_cell_indices[boundary_dim_index].size()) {
			p_mesh_nd->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		}
		old_boundary_normals = p_mesh_nd->get_poly_cell_boundary_normals();
	}
	const PackedInt32Array old_pivot_overrides = p_mesh_nd->get_poly_cell_boundary_pivot_overrides();
	const HashMap<Vector2i, Vector<PackedInt32Array>> old_normal_indices = p_mesh_nd->get_all_poly_cell_normal_indices();
	const HashMap<Vector2i, Vector<PackedInt32Array>> old_texture_map_indices = p_mesh_nd->get_all_poly_cell_texture_map_indices();
	HashMap<Vector2i, Vector<PackedInt32Array>> old_sub_elements;
	for (const HashMap<Vector2i, Vector<PackedInt32Array>> *old_bindings : { &old_normal_indices, &old_texture_map_indices }) {
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : *old_bindings) {
			if (_is_binding_affected_by_face_merge(kv.key) && kv.key.y < kv.key.x && !old_sub_elements.has(kv.key)) {
				old_sub_elements.insert(kv.key, p_mesh_nd->get_all_poly_cell_poly_indices(kv.key.x, kv.key.y));
			}
		}
	}
	const HashSet<int32_t> old_seams = p_mesh_nd->get_seam_indices();
	// Merge greedily, one adjacent coplanar pair at a time, until no pair is left. Each surviving face remembers
	// which original faces it absorbed, for resampling the bindings at the end.
	Vector<PackedInt32Array> face_sources;
	face_sources.resize(old_face_count);
	Vector<bool> face_alive;
	face_alive.resize(old_face_count);
	for (int64_t face_index = 0; face_index < old_face_count; face_index++) {
		face_sources.set(face_index, PackedInt32Array{ (int32_t)face_index });
		face_alive.write[face_index] = true;
	}
	// The faces using each edge, kept up to date as faces merge, so that a merged face can merge again in the same
	// pass. The passes repeat until one of them merges nothing.
	Vector<PackedInt32Array> edge_faces;
	edge_faces.resize(edge_count);
	for (int64_t face_index = 0; face_index < old_face_count; face_index++) {
		for (const int32_t edge_index : faces[face_index]) {
			edge_faces.write[edge_index].append((int32_t)face_index);
		}
	}
	int64_t merge_count = 0;
	bool merged_any = true;
	while (merged_any) {
		merged_any = false;
		for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
			if (edge_faces[edge_index].size() != 2) {
				continue; // Only an edge shared by exactly two faces can join them.
			}
			const int32_t face_a = edge_faces[edge_index][0];
			const int32_t face_b = edge_faces[edge_index][1];
			if (faces_are_boundary_cells) {
				// For 3D meshes, a seam edge borders a texture island, so faces are not merged across it. Faces that
				// face opposite ways cannot share one boundary normal, so they are not merged either.
				if (old_seams.has((int32_t)edge_index)) {
					continue;
				}
				if (VectorND::dot(old_boundary_normals[face_sources[face_a][0]], old_boundary_normals[face_sources[face_b][0]]) <= 0.0) {
					continue;
				}
			}
			PackedInt32Array merged_edges;
			if (!_try_merge_coplanar_face_pair(faces[face_a], faces[face_b], edge_vertex_indices, positions, sin_tolerance, merged_edges)) {
				continue;
			}
			// B's edges now belong to A, except the shared ones, which are interior to the merged face and unused.
			for (const int32_t b_edge_index : faces[face_b]) {
				PackedInt32Array &b_edge_faces = edge_faces.write[b_edge_index];
				if (merged_edges.has(b_edge_index)) {
					b_edge_faces.set(b_edge_faces.find(face_b), face_a);
				} else {
					b_edge_faces.clear();
				}
			}
			faces.set(face_a, merged_edges);
			faces.set(face_b, PackedInt32Array());
			face_alive.write[face_b] = false;
			face_sources.write[face_a].append_array(face_sources[face_b]);
			merge_count++;
			merged_any = true;
		}
	}
	if (merge_count == 0) {
		return 0;
	}
	// Compact the surviving faces and map every original face to its new index.
	PackedInt32Array old_to_new_face;
	old_to_new_face.resize(old_face_count);
	Vector<PackedInt32Array> new_faces;
	Vector<PackedInt32Array> new_face_sources;
	for (int64_t face_index = 0; face_index < old_face_count; face_index++) {
		if (!face_alive[face_index]) {
			continue;
		}
		const int32_t new_index = (int32_t)new_faces.size();
		new_faces.append(faces[face_index]);
		new_face_sources.append(face_sources[face_index]);
		for (const int32_t source : face_sources[face_index]) {
			old_to_new_face.set(source, new_index);
		}
	}
	// Compact the edges, dropping those that were interior to merged faces, and point the faces at the new indices.
	PackedInt32Array old_to_new_edge;
	old_to_new_edge.resize(edge_count);
	old_to_new_edge.fill(-1);
	PackedInt32Array new_edge_vertex_indices;
	Vector<PackedInt32Array> edge_sources;
	for (int64_t edge_index = 0; edge_index < edge_count; edge_index++) {
		if (edge_faces[edge_index].is_empty()) {
			continue; // No face uses this edge anymore.
		}
		old_to_new_edge.set(edge_index, (int32_t)edge_sources.size());
		edge_sources.append(PackedInt32Array{ (int32_t)edge_index });
		new_edge_vertex_indices.append(edge_vertex_indices[edge_index * 2]);
		new_edge_vertex_indices.append(edge_vertex_indices[edge_index * 2 + 1]);
	}
	MathND::remap_int32_arrays(new_faces, old_to_new_edge, false);
	poly_cell_indices.set(0, new_faces);
	// Cells now reference the merged faces, once each, and still need their first two faces to share an edge.
	if (has_cells) {
		Vector<PackedInt32Array> cells = poly_cell_indices[1];
		MathND::remap_int32_arrays(cells, old_to_new_face, true);
		for (int64_t cell_index = 0; cell_index < cells.size(); cell_index++) {
			if (cells[cell_index].size() >= 2) {
				MathND::ensure_first_two_indices_share_common_int32(cells.write[cell_index], new_faces);
			}
		}
		poly_cell_indices.set(1, cells);
	}
	// Every new element lists the old elements it came from: a merged face has several, edges have one, and cells
	// above the faces are left empty, meaning each is its own source. Sub-element indices are translated per
	// dimension, where vertices and cells keep their indices.
	Vector<Vector<PackedInt32Array>> element_sources_by_dimension;
	element_sources_by_dimension.resize(dimension + 1);
	element_sources_by_dimension.set(1, edge_sources);
	element_sources_by_dimension.set(2, new_face_sources);
	Vector<PackedInt32Array> sub_element_old_to_new_by_dimension;
	sub_element_old_to_new_by_dimension.resize(dimension + 1);
	sub_element_old_to_new_by_dimension.set(1, old_to_new_edge);
	sub_element_old_to_new_by_dimension.set(2, old_to_new_face);
	// Remove every binding that the new topology invalidates before changing it, so the mesh stays valid in between.
	const HashMap<Vector2i, Vector<PackedInt32Array>> *old_bindings_by_type[2] = { &old_normal_indices, &old_texture_map_indices };
	HashMap<Vector2i, Vector<PackedInt32Array>> new_normal_indices;
	HashMap<Vector2i, Vector<PackedInt32Array>> new_texture_map_indices;
	HashMap<Vector2i, Vector<PackedInt32Array>> *new_bindings_by_type[2] = { &new_normal_indices, &new_texture_map_indices };
	for (int which_binding = 0; which_binding < 2; which_binding++) {
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : *old_bindings_by_type[which_binding]) {
			if (!_is_binding_affected_by_face_merge(kv.key)) {
				new_bindings_by_type[which_binding]->insert(kv.key, kv.value);
			}
		}
	}
	p_mesh_nd->set_all_poly_cell_normal_indices(new_normal_indices);
	p_mesh_nd->set_all_poly_cell_texture_map_indices(new_texture_map_indices);
	if (faces_are_boundary_cells) {
		p_mesh_nd->set_poly_cell_boundary_pivot_overrides(PackedInt32Array());
	}
	p_mesh_nd->set_edge_vertex_indices(new_edge_vertex_indices);
	p_mesh_nd->set_poly_cell_indices(poly_cell_indices);
	// For 3D meshes, each merged face keeps the pivot override of its first source, if that vertex is still in it.
	if (faces_are_boundary_cells && !old_pivot_overrides.is_empty()) {
		PackedInt32Array new_pivot_overrides;
		new_pivot_overrides.resize(new_faces.size());
		for (int64_t face_index = 0; face_index < new_faces.size(); face_index++) {
			const int32_t source = new_face_sources[face_index][0];
			const int32_t pivot = source < old_pivot_overrides.size() ? old_pivot_overrides[source] : -1;
			const bool pivot_in_face = pivot >= 0 && _get_loop_face_vertices(new_faces[face_index], new_edge_vertex_indices).has(pivot);
			new_pivot_overrides.set(face_index, pivot_in_face ? pivot : -1);
		}
		p_mesh_nd->set_poly_cell_boundary_pivot_overrides(new_pivot_overrides);
	}
	ERR_FAIL_COND_V_MSG(!p_mesh_nd->is_mesh_data_valid(), merge_count, "PolyMeshBuilderND: Merging coplanar faces left the mesh invalid.");
	// Merging or reordering may have flipped the orientation of the boundary cells. Restore it before reading the new
	// sub-element orders, so that the bindings are rebuilt against the final topology. For 3D meshes, each merged face
	// takes the boundary normal of its first source, which all of its sources share.
	if (has_boundary_cells) {
		Vector<VectorN> desired_boundary_normals;
		if (faces_are_boundary_cells) {
			desired_boundary_normals.resize(new_faces.size());
			for (int64_t face_index = 0; face_index < new_faces.size(); face_index++) {
				desired_boundary_normals.set(face_index, old_boundary_normals[new_face_sources[face_index][0]]);
			}
		} else if (old_boundary_normals.size() == poly_cell_indices[boundary_dim_index].size()) {
			desired_boundary_normals = old_boundary_normals;
		}
		if (!desired_boundary_normals.is_empty()) {
			p_mesh_nd->orient_cells_to_boundary_normals(desired_boundary_normals);
		}
	}
	// Rebuild the affected bindings on the merged topology, on top of whatever the mesh holds now.
	new_normal_indices = p_mesh_nd->get_all_poly_cell_normal_indices();
	new_texture_map_indices = p_mesh_nd->get_all_poly_cell_texture_map_indices();
	for (int which_binding = 0; which_binding < 2; which_binding++) {
		for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : *old_bindings_by_type[which_binding]) {
			if (!_is_binding_affected_by_face_merge(kv.key)) {
				continue;
			}
			ERR_CONTINUE_MSG(kv.key.x > dimension || kv.key.y > dimension, "PolyMeshBuilderND: Invalid data binding key " + String(Variant(kv.key)) + ". Discarding it.");
			const Vector<PackedInt32Array> new_sub_elements = kv.key.y < kv.key.x ? p_mesh_nd->get_all_poly_cell_poly_indices(kv.key.x, kv.key.y) : Vector<PackedInt32Array>();
			const Vector<PackedInt32Array> *old_subs = old_sub_elements.getptr(kv.key);
			new_bindings_by_type[which_binding]->insert(kv.key, _remap_binding_after_face_merge(kv.key, kv.value, element_sources_by_dimension[kv.key.x], old_subs != nullptr ? *old_subs : Vector<PackedInt32Array>(), new_sub_elements, sub_element_old_to_new_by_dimension[kv.key.y]));
		}
	}
	p_mesh_nd->set_all_poly_cell_normal_indices(new_normal_indices);
	p_mesh_nd->set_all_poly_cell_texture_map_indices(new_texture_map_indices);
	// Seams are the members of the boundary cells: edges for 3D meshes, and faces for 4D meshes. Seam edges are never
	// interior to a merged face, since faces are not merged across them. Higher dimensional seams are unaffected.
	if (faces_are_boundary_cells) {
		p_mesh_nd->set_seam_indices(MathND::remap_int32_set(old_seams, old_to_new_edge));
	} else if (boundary_dim_index == 1) {
		p_mesh_nd->set_seam_indices(MathND::remap_int32_set(old_seams, old_to_new_face));
	}
	ERR_FAIL_COND_V_MSG(!p_mesh_nd->is_mesh_data_valid(), merge_count, "PolyMeshBuilderND: Merging coplanar faces left the mesh's data bindings invalid.");
	return merge_count;
}

int64_t PolyMeshBuilderND::delete_interior(const Ref<ArrayPolyMeshND> &p_mesh_nd) {
	ERR_FAIL_COND_V_MSG(p_mesh_nd.is_null() || !p_mesh_nd->is_mesh_data_valid(), 0, "PolyMeshBuilderND: Cannot delete the volumes of an invalid mesh.");
	Vector<Vector<PackedInt32Array>> poly_cell_indices = p_mesh_nd->get_poly_cell_indices();
	// The volumes are the N-dimensional cells of an N-dimensional mesh, and their members are the boundary cells.
	// If the mesh has no volumes, there is nothing to delete. No boundary cells could be interior without volumes.
	const int dimension = p_mesh_nd->get_dimension();
	const int64_t volume_dim_index = int64_t(dimension) - 2;
	if (volume_dim_index < 0 || poly_cell_indices.size() <= volume_dim_index) {
		return 0;
	}
	// Gather the interior boundary cells first. A boundary cell shared by two volumes lies between them, inside the
	// solid, so it is not part of its boundary. The list is in descending order, so that deleting them leaves the
	// lower indices valid.
	PackedInt32Array interior_cells;
	{
		const int64_t boundary_cell_count = volume_dim_index == 0 ? p_mesh_nd->get_edge_indices().size() / 2 : poly_cell_indices[volume_dim_index - 1].size();
		PackedInt32Array cell_volume_counts;
		cell_volume_counts.resize(boundary_cell_count);
		cell_volume_counts.fill(0);
		for (const PackedInt32Array &volume : poly_cell_indices[volume_dim_index]) {
			for (const int32_t cell_index : volume) {
				cell_volume_counts.set(cell_index, cell_volume_counts[cell_index] + 1);
			}
		}
		for (int64_t cell_index = cell_volume_counts.size() - 1; cell_index >= 0; cell_index--) {
			if (cell_volume_counts[cell_index] > 1) {
				interior_cells.append((int32_t)cell_index);
			}
		}
	}
	// Drop the volumes and the bindings that refer to them before deleting any cells, so that those deletions have
	// no volumes to cascade into and adjust.
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : p_mesh_nd->get_all_poly_cell_normal_indices()) {
		if (kv.key.x >= dimension) {
			p_mesh_nd->set_poly_cell_dense_normals(kv.key, Vector<Vector<VectorN>>());
		}
	}
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &kv : p_mesh_nd->get_all_poly_cell_texture_map_indices()) {
		if (kv.key.x >= dimension) {
			p_mesh_nd->set_poly_cell_dense_texture_map(kv.key, Vector<Vector<VectorM>>());
		}
	}
	poly_cell_indices.resize(volume_dim_index);
	p_mesh_nd->set_poly_cell_indices(poly_cell_indices);
	for (const int32_t cell_index : interior_cells) {
		p_mesh_nd->delete_poly_element(dimension - 1, cell_index);
	}
	return interior_cells.size();
}

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
	const Vector<VectorN> &poly_vertices = p_mesh_nd->get_poly_cell_vertex_positions();
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
					// This adjacent boundary cell is facing the wrong way, so flip its orientation.
					PackedInt32Array adjacent_boundary_cell = boundary_cells[adjacent_cell_index];
					PolyMeshND::flip_poly_cell_orientation(adjacent_boundary_cell, boundary_dim_index);
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

// Subdivision helper functions. See `subdivide_elements` below for an overview.

int32_t PolyMeshBuilderND::_subdivide_append_vertex(SubdivisionContext &r_ctx, const VectorN &p_position, const PackedInt32Array &p_source_vertices) {
	const int32_t index = (int32_t)r_ctx.new_vertices.size();
	r_ctx.new_vertices.append(p_position);
	r_ctx.new_vertex_sources.append(p_source_vertices);
	return index;
}

int32_t PolyMeshBuilderND::_subdivide_get_or_create_edge(SubdivisionContext &r_ctx, const int32_t p_vertex_a, const int32_t p_vertex_b, const int32_t p_parent) {
	const int64_t key = (int64_t(MIN(p_vertex_a, p_vertex_b)) << 32) | int64_t(MAX(p_vertex_a, p_vertex_b));
	const int32_t *existing = r_ctx.new_edge_map.getptr(key);
	if (existing != nullptr) {
		return *existing;
	}
	const int32_t index = (int32_t)(r_ctx.new_edges.size() / 2);
	r_ctx.new_edges.append(MIN(p_vertex_a, p_vertex_b));
	r_ctx.new_edges.append(MAX(p_vertex_a, p_vertex_b));
	r_ctx.new_edge_parents.append(p_parent);
	r_ctx.new_edge_map[key] = index;
	return index;
}

int32_t PolyMeshBuilderND::_subdivide_append_cell(SubdivisionContext &r_ctx, const int64_t p_level, const PackedInt32Array &p_members, const int32_t p_parent) {
	PackedInt32Array members = p_members;
	_subdivide_repair_first_two(r_ctx, p_level, members);
	const int32_t index = (int32_t)r_ctx.new_levels[p_level].size();
	r_ctx.new_levels.write[p_level].append(members);
	r_ctx.new_level_parents.write[p_level].append(p_parent);
	return index;
}

int32_t PolyMeshBuilderND::_subdivide_get_edge_piece_at(const SubdivisionContext &p_ctx, const int32_t p_old_edge, const int32_t p_old_vertex) {
	const PackedInt32Array &pieces = p_ctx.edge_pieces[p_old_edge];
	// The first piece contains the old edge's first vertex, the second piece contains the second.
	return p_ctx.old_edges[p_old_edge * 2] == p_old_vertex ? pieces[0] : pieces[1];
}

bool PolyMeshBuilderND::_subdivide_old_element_has_vertex(const SubdivisionContext &p_ctx, const int64_t p_element_dim, const int32_t p_element_index, const int32_t p_vertex) {
	if (p_element_dim == 1) {
		return p_ctx.old_edges[p_element_index * 2] == p_vertex || p_ctx.old_edges[p_element_index * 2 + 1] == p_vertex;
	}
	return p_ctx.old_level_vertices[p_element_dim - 2][p_element_index].has(p_vertex);
}

bool PolyMeshBuilderND::_subdivide_old_element_contains(const SubdivisionContext &p_ctx, const int64_t p_outer_dim, const int32_t p_outer_index, const int64_t p_inner_dim, const int32_t p_inner_index) {
	if (p_inner_dim == 1) {
		return _subdivide_old_element_has_vertex(p_ctx, p_outer_dim, p_outer_index, p_ctx.old_edges[p_inner_index * 2]) &&
				_subdivide_old_element_has_vertex(p_ctx, p_outer_dim, p_outer_index, p_ctx.old_edges[p_inner_index * 2 + 1]);
	}
	const PackedInt32Array &inner_vertices = p_ctx.old_level_vertices[p_inner_dim - 2][p_inner_index];
	for (const int32_t vertex : inner_vertices) {
		if (!_subdivide_old_element_has_vertex(p_ctx, p_outer_dim, p_outer_index, vertex)) {
			return false;
		}
	}
	return true;
}

VectorN PolyMeshBuilderND::_subdivide_old_element_center(const SubdivisionContext &p_ctx, const int64_t p_element_dim, const int32_t p_element_index, PackedInt32Array *r_source_vertices) {
	PackedInt32Array source_vertices;
	if (p_element_dim == 1) {
		source_vertices.append(p_ctx.old_edges[p_element_index * 2]);
		source_vertices.append(p_ctx.old_edges[p_element_index * 2 + 1]);
	} else {
		source_vertices = p_ctx.old_level_vertices[p_element_dim - 2][p_element_index];
	}
	VectorN center;
	for (const int32_t vertex : source_vertices) {
		center = VectorND::add(center, p_ctx.old_vertices[vertex]);
	}
	center = VectorND::divide_scalar(center, source_vertices.size());
	if (r_source_vertices != nullptr) {
		*r_source_vertices = source_vertices;
	}
	return center;
}

int32_t PolyMeshBuilderND::_subdivide_classify(SubdivisionContext &r_ctx, const int64_t p_level, const int32_t p_index) {
	const int32_t memo = r_ctx.classification[p_level][p_index];
	if (memo != SUBDIV_CLASS_UNKNOWN) {
		return memo;
	}
	int32_t result = SUBDIV_CLASS_OTHER;
	const int64_t element_dim = p_level + 2;
	const int64_t member_count = r_ctx.old_levels[p_level][p_index].size();
	const int64_t vertex_count = r_ctx.old_level_vertices[p_level][p_index].size();
	if (p_level == 0) {
		// A triangle is a 2D simplex, and a quadrilateral is treated as a 2D box.
		if (member_count == 3 && vertex_count == 3) {
			result = SUBDIV_CLASS_SIMPLEX;
		} else if (member_count == 4 && vertex_count == 4) {
			result = SUBDIV_CLASS_BOX;
		}
	} else {
		const PackedInt32Array &members = r_ctx.old_levels[p_level][p_index];
		if (vertex_count == element_dim + 1 && member_count == element_dim + 1) {
			result = SUBDIV_CLASS_SIMPLEX;
			for (const int32_t member : members) {
				if (_subdivide_classify(r_ctx, p_level - 1, member) != SUBDIV_CLASS_SIMPLEX) {
					result = SUBDIV_CLASS_OTHER;
					break;
				}
			}
		} else if (vertex_count == (int64_t(1) << element_dim) && member_count == 2 * element_dim) {
			result = SUBDIV_CLASS_BOX;
			for (const int32_t member : members) {
				if (_subdivide_classify(r_ctx, p_level - 1, member) != SUBDIV_CLASS_BOX) {
					result = SUBDIV_CLASS_OTHER;
					break;
				}
			}
		} else if (vertex_count == 2 * element_dim && member_count == (int64_t(1) << element_dim)) {
			result = SUBDIV_CLASS_ORTHOPLEX;
			for (const int32_t member : members) {
				if (_subdivide_classify(r_ctx, p_level - 1, member) != SUBDIV_CLASS_SIMPLEX) {
					result = SUBDIV_CLASS_OTHER;
					break;
				}
			}
		}
	}
	r_ctx.classification.write[p_level].set(p_index, result);
	return result;
}

bool PolyMeshBuilderND::_subdivide_new_elements_touch(const SubdivisionContext &p_ctx, const int64_t p_level, const int32_t p_a, const int32_t p_b) {
	// Two new elements touch when they share a member (or a vertex, in the case of edges).
	if (p_level < 0) {
		const int32_t a1 = p_ctx.new_edges[p_a * 2];
		const int32_t a2 = p_ctx.new_edges[p_a * 2 + 1];
		const int32_t b1 = p_ctx.new_edges[p_b * 2];
		const int32_t b2 = p_ctx.new_edges[p_b * 2 + 1];
		return a1 == b1 || a1 == b2 || a2 == b1 || a2 == b2;
	}
	const PackedInt32Array &members_a = p_ctx.new_levels[p_level][p_a];
	const PackedInt32Array &members_b = p_ctx.new_levels[p_level][p_b];
	for (const int32_t member : members_a) {
		if (members_b.has(member)) {
			return true;
		}
	}
	return false;
}

void PolyMeshBuilderND::_subdivide_repair_first_two(SubdivisionContext &r_ctx, const int64_t p_level, PackedInt32Array &r_members) {
	if (r_members.size() < 2) {
		return;
	}
	const int64_t member_level = p_level - 1;
	const PackedInt32Array &parents = member_level < 0 ? r_ctx.new_edge_parents : r_ctx.new_level_parents[member_level];
	// The first two members of a cell must share a common element to encode the orientation.
	// They should also come from different parents, because pieces of the same subdivided
	// element are coplanar, which would make the orientation degenerate.
	if (_subdivide_new_elements_touch(r_ctx, member_level, r_members[0], r_members[1]) && parents[r_members[0]] != parents[r_members[1]]) {
		return;
	}
	// Pass 1 requires distinct parents, pass 2 falls back to any touching pair.
	for (int pass = 0; pass < 2; pass++) {
		for (int64_t i = 0; i < r_members.size(); i++) {
			for (int64_t j = i + 1; j < r_members.size(); j++) {
				if (pass == 0 && parents[r_members[i]] == parents[r_members[j]]) {
					continue;
				}
				if (!_subdivide_new_elements_touch(r_ctx, member_level, r_members[i], r_members[j])) {
					continue;
				}
				PackedInt32Array reordered;
				reordered.append(r_members[i]);
				reordered.append(r_members[j]);
				for (int64_t rest = 0; rest < r_members.size(); rest++) {
					if (rest != i && rest != j) {
						reordered.append(r_members[rest]);
					}
				}
				r_members = reordered;
				return;
			}
		}
	}
}

void PolyMeshBuilderND::_subdivide_order_face_loop(const SubdivisionContext &p_ctx, PackedInt32Array &r_members) {
	// Reorders the given new edges into a connected loop order, as faces should have.
	const int64_t member_count = r_members.size();
	if (member_count < 3) {
		return;
	}
	PackedInt32Array ordered = { r_members[0] };
	int32_t current_vertex = p_ctx.new_edges[r_members[0] * 2 + 1];
	while (ordered.size() < member_count) {
		bool found = false;
		for (const int32_t member : r_members) {
			if (ordered.has(member)) {
				continue;
			}
			const int32_t vertex_a = p_ctx.new_edges[member * 2];
			const int32_t vertex_b = p_ctx.new_edges[member * 2 + 1];
			if (vertex_a == current_vertex) {
				current_vertex = vertex_b;
			} else if (vertex_b == current_vertex) {
				current_vertex = vertex_a;
			} else {
				continue;
			}
			ordered.append(member);
			found = true;
			break;
		}
		if (!found) {
			return; // The edges do not form a closed loop, keep the original order.
		}
	}
	r_members = ordered;
}

int32_t PolyMeshBuilderND::_subdivide_cone(SubdivisionContext &r_ctx, SubdivisionRefined &r_refined, const int64_t p_element_level, const int32_t p_element_index) {
	// Cones the given new element to the refined cell's center vertex, giving an element one dimension higher.
	const int64_t memo_key = ((p_element_level + 2) << 32) | int64_t(p_element_index);
	const int32_t *existing = r_refined.cone_by_element.getptr(memo_key);
	if (existing != nullptr) {
		return *existing;
	}
	int32_t cone_index;
	if (p_element_level < 0) {
		// Coning an edge gives a triangle face.
		const int32_t vertex_a = r_ctx.new_edges[p_element_index * 2];
		const int32_t vertex_b = r_ctx.new_edges[p_element_index * 2 + 1];
		PackedInt32Array members = {
			p_element_index,
			_subdivide_get_or_create_edge(r_ctx, vertex_a, r_refined.center_vertex, r_ctx.internal_parent_counter--),
			_subdivide_get_or_create_edge(r_ctx, vertex_b, r_refined.center_vertex, r_ctx.internal_parent_counter--),
		};
		cone_index = _subdivide_append_cell(r_ctx, 0, members, r_ctx.internal_parent_counter--);
	} else {
		PackedInt32Array members = { p_element_index };
		const PackedInt32Array element_members = r_ctx.new_levels[p_element_level][p_element_index];
		for (const int32_t element_member : element_members) {
			members.append(_subdivide_cone(r_ctx, r_refined, p_element_level - 1, element_member));
		}
		cone_index = _subdivide_append_cell(r_ctx, p_element_level + 1, members, r_ctx.internal_parent_counter--);
	}
	r_refined.cone_by_element[memo_key] = cone_index;
	return cone_index;
}

void PolyMeshBuilderND::_subdivide_collect_closure(const SubdivisionContext &p_ctx, const int64_t p_level, const int32_t p_index, Vector<PackedInt32Array> &r_closure_by_dim) {
	// Collects the old element indices in the cell's closure, indexed by geometric dimension.
	const int64_t element_dim = p_level + 2;
	r_closure_by_dim.clear();
	r_closure_by_dim.resize(element_dim);
	Vector<HashSet<int32_t>> seen;
	seen.resize(element_dim);
	PackedInt32Array frontier = p_ctx.old_levels[p_level][p_index];
	for (int64_t dim = element_dim - 1; dim >= 1; dim--) {
		PackedInt32Array next_frontier;
		for (const int32_t element : frontier) {
			if (seen[dim].has(element)) {
				continue;
			}
			seen.write[dim].insert(element);
			r_closure_by_dim.write[dim].append(element);
			if (dim >= 2) {
				next_frontier.append_array(p_ctx.old_levels[dim - 2][element]);
			}
		}
		frontier = next_frontier;
	}
}

int32_t PolyMeshBuilderND::_subdivide_internal_element(SubdivisionContext &r_ctx, const int64_t p_level, const int32_t p_cell_index, const Vector<PackedInt32Array> &p_closure_by_dim, const int64_t p_sub_dim, const int32_t p_sub_index) {
	// For box-style refinements, creates the internal element of the cell "across" the given
	// sub-element. For example, a subdivided cube has an internal wall quad across each of its
	// edges, and an internal edge from each face's center to the cube's center.
	SubdivisionRefined *refined = r_ctx.refined_levels.write[p_level].getptr(p_cell_index);
	CRASH_COND(refined == nullptr);
	const int64_t memo_key = (p_sub_dim << 32) | int64_t(p_sub_index);
	{
		const int32_t *existing = refined->internal_by_subelement.getptr(memo_key);
		if (existing != nullptr) {
			return *existing;
		}
	}
	const int64_t cell_dim = p_level + 2;
	const int64_t internal_dim = cell_dim - p_sub_dim;
	int32_t internal_index;
	if (internal_dim == 1) {
		// The internal element is an edge from the sub-element's center to the cell's center.
		const int32_t sub_center = p_sub_dim == 1 ? r_ctx.edge_mid_vertex[p_sub_index] : r_ctx.refined_levels[p_sub_dim - 2][p_sub_index].center_vertex;
		internal_index = _subdivide_get_or_create_edge(r_ctx, sub_center, refined->center_vertex, r_ctx.internal_parent_counter--);
	} else {
		PackedInt32Array members;
		// Members on the cell's boundary: the internal elements of each member across the sub-element.
		const PackedInt32Array cell_members = r_ctx.old_levels[p_level][p_cell_index];
		for (const int32_t member : cell_members) {
			if (!_subdivide_old_element_contains(r_ctx, cell_dim - 1, member, p_sub_dim, p_sub_index)) {
				continue;
			}
			if (cell_dim - 1 == 2) {
				// The member is a face, whose internal spoke edges were stored during face refinement.
				const int32_t *spoke = r_ctx.refined_levels[0][member].internal_by_subelement.getptr(memo_key);
				CRASH_COND(spoke == nullptr);
				members.append(*spoke);
			} else {
				Vector<PackedInt32Array> member_closure;
				_subdivide_collect_closure(r_ctx, p_level - 1, member, member_closure);
				members.append(_subdivide_internal_element(r_ctx, p_level - 1, member, member_closure, p_sub_dim, p_sub_index));
			}
		}
		// Members inside the cell: the internal elements across every element one dimension
		// above the sub-element that contains it.
		for (const int32_t above : p_closure_by_dim[p_sub_dim + 1]) {
			if (_subdivide_old_element_contains(r_ctx, p_sub_dim + 1, above, p_sub_dim, p_sub_index)) {
				members.append(_subdivide_internal_element(r_ctx, p_level, p_cell_index, p_closure_by_dim, p_sub_dim + 1, above));
			}
		}
		if (internal_dim == 2) {
			// Internal wall faces gather their edges out of order, so restore the loop order.
			_subdivide_order_face_loop(r_ctx, members);
		}
		internal_index = _subdivide_append_cell(r_ctx, internal_dim - 2, members, r_ctx.internal_parent_counter--);
		refined = r_ctx.refined_levels.write[p_level].getptr(p_cell_index);
	}
	refined->internal_by_subelement[memo_key] = internal_index;
	return internal_index;
}

PackedInt32Array PolyMeshBuilderND::_subdivide_face_vertex_walk(const SubdivisionContext &p_ctx, const int32_t p_face_index) {
	// Walks the face's edge loop and returns its vertices in boundary order, starting with
	// the face's first two edges so that the walk is consistent with the face's orientation.
	const PackedInt32Array &face_edges = p_ctx.old_levels[0][p_face_index];
	const int32_t edge0_a = p_ctx.old_edges[face_edges[0] * 2];
	const int32_t edge0_b = p_ctx.old_edges[face_edges[0] * 2 + 1];
	const int32_t edge1_a = p_ctx.old_edges[face_edges[1] * 2];
	const int32_t edge1_b = p_ctx.old_edges[face_edges[1] * 2 + 1];
	const int32_t shared = (edge0_a == edge1_a || edge0_a == edge1_b) ? edge0_a : edge0_b;
	PackedInt32Array walk = {
		edge0_a == shared ? edge0_b : edge0_a,
		shared,
		edge1_a == shared ? edge1_b : edge1_a,
	};
	while (walk.size() < face_edges.size()) {
		const int32_t current = walk[walk.size() - 1];
		const int32_t previous = walk[walk.size() - 2];
		bool found = false;
		for (const int32_t edge : face_edges) {
			const int32_t vertex_a = p_ctx.old_edges[edge * 2];
			const int32_t vertex_b = p_ctx.old_edges[edge * 2 + 1];
			const int32_t other = vertex_a == current ? vertex_b : (vertex_b == current ? vertex_a : -1);
			if (other == -1 || other == previous || walk.has(other)) {
				continue;
			}
			walk.append(other);
			found = true;
			break;
		}
		if (!found) {
			break; // Malformed face, return what we have.
		}
	}
	return walk;
}

void PolyMeshBuilderND::_subdivide_refine_face(SubdivisionContext &r_ctx, const int32_t p_face_index) {
	r_ctx.refined_levels.write[0].insert(p_face_index, SubdivisionRefined());
	SubdivisionRefined *refined = r_ctx.refined_levels.write[0].getptr(p_face_index);
	const PackedInt32Array vertex_sequence = _subdivide_face_vertex_walk(r_ctx, p_face_index);
	const int64_t n = vertex_sequence.size();
	// Map each vertex pair to the old edge connecting them.
	HashMap<int64_t, int32_t> pair_to_edge;
	for (const int32_t edge : r_ctx.old_levels[0][p_face_index]) {
		const int32_t vertex_a = r_ctx.old_edges[edge * 2];
		const int32_t vertex_b = r_ctx.old_edges[edge * 2 + 1];
		pair_to_edge[(int64_t(MIN(vertex_a, vertex_b)) << 32) | int64_t(MAX(vertex_a, vertex_b))] = edge;
	}
	PackedInt32Array boundary_edges; // The old edge for each vertex to the next vertex, in walk order.
	for (int64_t j = 0; j < n; j++) {
		const int32_t vertex_a = vertex_sequence[j];
		const int32_t vertex_b = vertex_sequence[(j + 1) % n];
		const int32_t *edge = pair_to_edge.getptr((int64_t(MIN(vertex_a, vertex_b)) << 32) | int64_t(MAX(vertex_a, vertex_b)));
		CRASH_COND(edge == nullptr);
		boundary_edges.append(*edge);
	}
	if (n == 3) {
		// A triangle subdivides into 3 corner triangles and 1 central medial triangle.
		for (int64_t j = 0; j < n; j++) {
			const int32_t vertex = vertex_sequence[j];
			const int32_t next_edge = boundary_edges[j];
			const int32_t prev_edge = boundary_edges[(j + n - 1) % n];
			const int32_t cut_edge = _subdivide_get_or_create_edge(r_ctx, r_ctx.edge_mid_vertex[next_edge], r_ctx.edge_mid_vertex[prev_edge], r_ctx.internal_parent_counter--);
			refined->cut_piece_by_vertex[vertex] = cut_edge;
			PackedInt32Array corner_members = {
				_subdivide_get_edge_piece_at(r_ctx, next_edge, vertex),
				cut_edge,
				_subdivide_get_edge_piece_at(r_ctx, prev_edge, vertex),
			};
			const int32_t corner = _subdivide_append_cell(r_ctx, 0, corner_members, p_face_index);
			refined->corner_piece_by_vertex[vertex] = corner;
			refined->all_pieces.append(corner);
		}
		PackedInt32Array central_members;
		for (int64_t j = 0; j < n; j++) {
			central_members.append(refined->cut_piece_by_vertex[vertex_sequence[j]]);
		}
		const int32_t central = _subdivide_append_cell(r_ctx, 0, central_members, p_face_index);
		refined->central_pieces.append(central);
		refined->all_pieces.append(central);
	} else {
		// An n-gon subdivides into n corner quadrilaterals around a center vertex.
		PackedInt32Array center_sources;
		const VectorN center_position = _subdivide_old_element_center(r_ctx, 2, p_face_index, &center_sources);
		refined->center_vertex = _subdivide_append_vertex(r_ctx, center_position, center_sources);
		for (int64_t j = 0; j < n; j++) {
			const int32_t edge = boundary_edges[j];
			const int32_t spoke = _subdivide_get_or_create_edge(r_ctx, r_ctx.edge_mid_vertex[edge], refined->center_vertex, r_ctx.internal_parent_counter--);
			refined->internal_by_subelement[(int64_t(1) << 32) | int64_t(edge)] = spoke;
		}
		for (int64_t j = 0; j < n; j++) {
			const int32_t vertex = vertex_sequence[j];
			const int32_t next_edge = boundary_edges[j];
			const int32_t prev_edge = boundary_edges[(j + n - 1) % n];
			PackedInt32Array corner_members = {
				_subdivide_get_edge_piece_at(r_ctx, next_edge, vertex),
				refined->internal_by_subelement[(int64_t(1) << 32) | int64_t(next_edge)],
				refined->internal_by_subelement[(int64_t(1) << 32) | int64_t(prev_edge)],
				_subdivide_get_edge_piece_at(r_ctx, prev_edge, vertex),
			};
			const int32_t corner = _subdivide_append_cell(r_ctx, 0, corner_members, p_face_index);
			refined->corner_piece_by_vertex[vertex] = corner;
			refined->all_pieces.append(corner);
		}
	}
}

void PolyMeshBuilderND::_subdivide_refine_cell(SubdivisionContext &r_ctx, const int64_t p_level, const int32_t p_index) {
	r_ctx.refined_levels.write[p_level].insert(p_index, SubdivisionRefined());
	SubdivisionRefined *refined = r_ctx.refined_levels.write[p_level].getptr(p_index);
	const PackedInt32Array members = r_ctx.old_levels[p_level][p_index];
	const PackedInt32Array vertices = r_ctx.old_level_vertices[p_level][p_index];
	const int32_t classification = _subdivide_classify(r_ctx, p_level, p_index);
	if (classification == SUBDIV_CLASS_SIMPLEX) {
		// A simplex subdivides into corner simplexes and a central rectified simplex.
		// Create the cut cells across each vertex, made of the members' cut pieces.
		for (const int32_t vertex : vertices) {
			PackedInt32Array cut_members;
			for (const int32_t member : members) {
				if (_subdivide_old_element_has_vertex(r_ctx, p_level + 1, member, vertex)) {
					cut_members.append(r_ctx.refined_levels[p_level - 1][member].cut_piece_by_vertex[vertex]);
				}
			}
			const int32_t cut = _subdivide_append_cell(r_ctx, p_level - 1, cut_members, r_ctx.internal_parent_counter--);
			refined->cut_piece_by_vertex[vertex] = cut;
		}
		for (const int32_t vertex : vertices) {
			PackedInt32Array corner_members;
			for (const int32_t member : members) {
				if (_subdivide_old_element_has_vertex(r_ctx, p_level + 1, member, vertex)) {
					corner_members.append(r_ctx.refined_levels[p_level - 1][member].corner_piece_by_vertex[vertex]);
				}
			}
			corner_members.append(refined->cut_piece_by_vertex[vertex]);
			const int32_t corner = _subdivide_append_cell(r_ctx, p_level, corner_members, p_index);
			refined->corner_piece_by_vertex[vertex] = corner;
			refined->all_pieces.append(corner);
		}
		PackedInt32Array central_members;
		for (const int32_t member : members) {
			central_members.append_array(r_ctx.refined_levels[p_level - 1][member].central_pieces);
		}
		for (const int32_t vertex : vertices) {
			central_members.append(refined->cut_piece_by_vertex[vertex]);
		}
		const int32_t central = _subdivide_append_cell(r_ctx, p_level, central_members, p_index);
		refined->central_pieces.append(central);
		refined->all_pieces.append(central);
	} else if (classification == SUBDIV_CLASS_BOX) {
		// A box subdivides into one sub-box per vertex, separated by internal walls.
		PackedInt32Array center_sources;
		const VectorN center_position = _subdivide_old_element_center(r_ctx, p_level + 2, p_index, &center_sources);
		refined->center_vertex = _subdivide_append_vertex(r_ctx, center_position, center_sources);
		Vector<PackedInt32Array> closure_by_dim;
		_subdivide_collect_closure(r_ctx, p_level, p_index, closure_by_dim);
		for (const int32_t vertex : vertices) {
			PackedInt32Array corner_members;
			for (const int32_t member : members) {
				if (_subdivide_old_element_has_vertex(r_ctx, p_level + 1, member, vertex)) {
					corner_members.append(r_ctx.refined_levels[p_level - 1][member].corner_piece_by_vertex[vertex]);
				}
			}
			for (const int32_t edge : closure_by_dim[1]) {
				if (_subdivide_old_element_has_vertex(r_ctx, 1, edge, vertex)) {
					corner_members.append(_subdivide_internal_element(r_ctx, p_level, p_index, closure_by_dim, 1, edge));
				}
			}
			// Re-fetch the pointer since the internal element helper may mutate the map's fields.
			refined = r_ctx.refined_levels.write[p_level].getptr(p_index);
			const int32_t corner = _subdivide_append_cell(r_ctx, p_level, corner_members, p_index);
			refined->corner_piece_by_vertex[vertex] = corner;
			refined->all_pieces.append(corner);
		}
	} else if (classification == SUBDIV_CLASS_ORTHOPLEX) {
		// An orthoplex subdivides into one half-size orthoplex per vertex, with a simplex
		// cone toward the center filling the hole left behind at each member.
		PackedInt32Array center_sources;
		const VectorN center_position = _subdivide_old_element_center(r_ctx, p_level + 2, p_index, &center_sources);
		refined->center_vertex = _subdivide_append_vertex(r_ctx, center_position, center_sources);
		for (const int32_t vertex : vertices) {
			PackedInt32Array corner_members;
			for (const int32_t member : members) {
				if (_subdivide_old_element_has_vertex(r_ctx, p_level + 1, member, vertex)) {
					corner_members.append(r_ctx.refined_levels[p_level - 1][member].corner_piece_by_vertex[vertex]);
				}
			}
			for (const int32_t member : members) {
				if (_subdivide_old_element_has_vertex(r_ctx, p_level + 1, member, vertex)) {
					const int32_t cut_piece = r_ctx.refined_levels[p_level - 1][member].cut_piece_by_vertex[vertex];
					corner_members.append(_subdivide_cone(r_ctx, *refined, p_level - 2, cut_piece));
				}
			}
			const int32_t corner = _subdivide_append_cell(r_ctx, p_level, corner_members, p_index);
			refined->corner_piece_by_vertex[vertex] = corner;
			refined->all_pieces.append(corner);
		}
		for (const int32_t member : members) {
			const int32_t central_piece = r_ctx.refined_levels[p_level - 1][member].central_pieces[0];
			const int32_t hole = _subdivide_cone(r_ctx, *refined, p_level - 1, central_piece);
			r_ctx.new_level_parents.write[p_level].set(hole, p_index);
			refined->central_pieces.append(hole);
			refined->all_pieces.append(hole);
		}
	} else {
		// Fallback for any other cell shape: cone every piece of the refined boundary to the centroid.
		PackedInt32Array center_sources;
		const VectorN center_position = _subdivide_old_element_center(r_ctx, p_level + 2, p_index, &center_sources);
		refined->center_vertex = _subdivide_append_vertex(r_ctx, center_position, center_sources);
		for (const int32_t member : members) {
			const PackedInt32Array member_pieces = r_ctx.refined_levels[p_level - 1][member].all_pieces;
			for (const int32_t piece : member_pieces) {
				const int32_t cone = _subdivide_cone(r_ctx, *refined, p_level - 1, piece);
				r_ctx.new_level_parents.write[p_level].set(cone, p_index);
				refined->all_pieces.append(cone);
			}
		}
	}
}

PackedInt32Array PolyMeshBuilderND::_subdivide_conform_face(SubdivisionContext &r_ctx, const int32_t p_face_index) {
	// Rebuilds an unsubdivided face's edge list in walk order, replacing subdivided edges
	// with their pieces. Faces with 5+ edges require walk order for correct triangulation.
	const PackedInt32Array vertex_sequence = _subdivide_face_vertex_walk(r_ctx, p_face_index);
	const int64_t n = vertex_sequence.size();
	HashMap<int64_t, int32_t> pair_to_edge;
	for (const int32_t edge : r_ctx.old_levels[0][p_face_index]) {
		const int32_t vertex_a = r_ctx.old_edges[edge * 2];
		const int32_t vertex_b = r_ctx.old_edges[edge * 2 + 1];
		pair_to_edge[(int64_t(MIN(vertex_a, vertex_b)) << 32) | int64_t(MAX(vertex_a, vertex_b))] = edge;
	}
	PackedInt32Array new_members;
	for (int64_t j = 0; j < n; j++) {
		const int32_t vertex_a = vertex_sequence[j];
		const int32_t vertex_b = vertex_sequence[(j + 1) % n];
		const int32_t *edge = pair_to_edge.getptr((int64_t(MIN(vertex_a, vertex_b)) << 32) | int64_t(MAX(vertex_a, vertex_b)));
		CRASH_COND(edge == nullptr);
		if (r_ctx.edge_remap[*edge] >= 0) {
			new_members.append(r_ctx.edge_remap[*edge]);
		} else {
			new_members.append(_subdivide_get_edge_piece_at(r_ctx, *edge, vertex_a));
			new_members.append(_subdivide_get_edge_piece_at(r_ctx, *edge, vertex_b));
		}
	}
	// Rotate the walk so the first two edges are not collinear pieces of the same old edge,
	// which would make the face's orientation degenerate.
	const int64_t member_count = new_members.size();
	for (int64_t rotation = 0; rotation < member_count; rotation++) {
		const int32_t edge_a = new_members[rotation];
		const int32_t edge_b = new_members[(rotation + 1) % member_count];
		const VectorN dir_a = VectorND::direction_to(r_ctx.new_vertices[r_ctx.new_edges[edge_a * 2]], r_ctx.new_vertices[r_ctx.new_edges[edge_a * 2 + 1]]);
		const VectorN dir_b = VectorND::direction_to(r_ctx.new_vertices[r_ctx.new_edges[edge_b * 2]], r_ctx.new_vertices[r_ctx.new_edges[edge_b * 2 + 1]]);
		if (Math::abs(VectorND::dot(dir_a, dir_b)) > 1.0 - (double)CMP_EPSILON) {
			continue; // Collinear, try the next rotation.
		}
		if (rotation == 0) {
			return new_members;
		}
		PackedInt32Array rotated;
		for (int64_t k = 0; k < member_count; k++) {
			rotated.append(new_members[(rotation + k) % member_count]);
		}
		return rotated;
	}
	return new_members; // Fully degenerate face, leave as is.
}

PackedInt32Array PolyMeshBuilderND::subdivide_elements(const Ref<ArrayPolyMeshND> &p_input_mesh, const int p_dimension, const PackedInt32Array &p_elements) {
	// Subdivides the selected elements of the given dimension, in place, using the midpoint
	// subdivision family: edges split at their midpoints, triangles become 4 triangles, other
	// polygons become quads around a center vertex, simplexes become corner simplexes plus a
	// rectified central cell, boxes become 2^N sub-boxes, orthoplexes become corner orthoplexes
	// plus simplex cones, and any other cell is coned from its centroid over its refined boundary.
	// Elements below the selected ones are fully subdivided too (downward closure), while
	// unselected elements above them are conformed by referencing the pieces of their subdivided
	// members, which keeps everything crack-free, even under deformation.
	PackedInt32Array ret;
	ERR_FAIL_COND_V_MSG(p_input_mesh.is_null() || !p_input_mesh->is_mesh_data_valid(), ret, "Input mesh is not valid, so subdivision cannot be performed.");
	const int64_t dimension = p_input_mesh->get_dimension();
	ERR_FAIL_COND_V_MSG(p_dimension < 1 || p_dimension > dimension, ret, "Cannot subdivide elements of dimension " + itos(p_dimension) + " in a mesh of dimension " + itos(dimension) + ".");
	SubdivisionContext ctx;
	ctx.dimension = dimension;
	ctx.old_vertices = p_input_mesh->get_poly_cell_vertex_positions();
	ctx.old_edges = p_input_mesh->get_edge_indices();
	ctx.old_levels = p_input_mesh->get_poly_cell_indices();
	const int64_t level_count = ctx.old_levels.size();
	const int64_t selection_level = int64_t(p_dimension) - 2; // -1 means edges.
	ERR_FAIL_COND_V_MSG(selection_level >= level_count, ret, "The mesh has no elements of dimension " + itos(p_dimension) + " to subdivide.");
	const int64_t old_edge_count = ctx.old_edges.size() / 2;
	const int64_t selection_element_count = selection_level < 0 ? old_edge_count : ctx.old_levels[selection_level].size();
	// Gather and validate the selection. An empty selection means all elements of that dimension.
	PackedInt32Array selection = p_elements;
	if (selection.is_empty()) {
		selection.resize(selection_element_count);
		for (int64_t i = 0; i < selection_element_count; i++) {
			selection.set(i, (int32_t)i);
		}
	} else {
		for (int64_t i = 0; i < selection.size(); i++) {
			ERR_FAIL_INDEX_V_MSG(selection[i], selection_element_count, ret, "Element index " + itos(selection[i]) + " is out of range for dimension " + itos(p_dimension) + ".");
		}
	}
	if (selection.is_empty()) {
		return ret; // Nothing to subdivide.
	}
	// Cache the vertex indices of every element at every level of the old mesh.
	ctx.old_level_vertices.resize(level_count);
	for (int64_t level = 0; level < level_count; level++) {
		ctx.old_level_vertices.set(level, p_input_mesh->get_all_poly_cell_vertex_indices(level + 2, false));
	}
	// Mark the selection, then mark everything below it for full subdivision (downward closure).
	ctx.marked_levels.resize(level_count);
	if (selection_level < 0) {
		for (const int32_t element : selection) {
			ctx.marked_edges.insert(element);
		}
	} else {
		for (const int32_t element : selection) {
			ctx.marked_levels.write[selection_level].insert(element);
		}
		for (int64_t level = selection_level; level >= 0; level--) {
			for (const int32_t marked : ctx.marked_levels[level]) {
				for (const int32_t member : ctx.old_levels[level][marked]) {
					if (level == 0) {
						ctx.marked_edges.insert(member);
					} else {
						ctx.marked_levels.write[level - 1].insert(member);
					}
				}
			}
		}
	}
	// Capture the old boundary data so it can be restored and inherited after subdivision.
	const int64_t boundary_level = dimension - 3;
	const bool has_boundary_level = boundary_level >= 0 && boundary_level < level_count;
	Vector<VectorN> old_boundary_normals;
	bool had_stored_normals = false;
	if (has_boundary_level) {
		old_boundary_normals = p_input_mesh->get_poly_cell_boundary_normals();
		if (old_boundary_normals.size() == ctx.old_levels[boundary_level].size()) {
			had_stored_normals = true;
		} else {
			// The mesh has no stored boundary normals, but the cell orientations still encode
			// implicit normals, which must be preserved. Compute them on a throwaway copy.
			Ref<ArrayPolyMeshND> scratch = p_input_mesh->duplicate();
			scratch->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
			old_boundary_normals = scratch->get_poly_cell_boundary_normals();
		}
	}
	// Subdivision works with dense normal and texture map data, sampled from
	// the indexed data at the start and converted back to indexed at the end.
	HashMap<Vector2i, Vector<Vector<VectorN>>> old_all_normals;
	const HashMap<Vector2i, Vector<PackedInt32Array>> input_all_poly_cell_normal_indices = p_input_mesh->get_all_poly_cell_normal_indices();
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &old_normal_kv : input_all_poly_cell_normal_indices) {
		old_all_normals.insert(old_normal_kv.key, p_input_mesh->get_poly_cell_dense_normals(old_normal_kv.key));
	}
	HashMap<Vector2i, Vector<Vector<VectorM>>> old_all_texture_maps;
	const HashMap<Vector2i, Vector<PackedInt32Array>> input_all_poly_cell_texture_map_indices = p_input_mesh->get_all_poly_cell_texture_map_indices();
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &old_tex_map_kv : input_all_poly_cell_texture_map_indices) {
		old_all_texture_maps.insert(old_tex_map_kv.key, p_input_mesh->get_poly_cell_dense_texture_map(old_tex_map_kv.key));
	}
	const HashSet<int32_t> old_seams = p_input_mesh->get_seam_indices();
	const PackedInt32Array old_pivot_overrides = p_input_mesh->get_poly_cell_boundary_pivot_overrides();
	// Build the new mesh data, starting with the vertices and edges.
	ctx.new_vertices = ctx.old_vertices.duplicate();
	ctx.new_levels.resize(level_count);
	ctx.new_level_parents.resize(level_count);
	ctx.level_remap.resize(level_count);
	ctx.refined_levels.resize(level_count);
	ctx.classification.resize(level_count);
	for (int64_t level = 0; level < level_count; level++) {
		PackedInt32Array classification_init;
		classification_init.resize(ctx.old_levels[level].size());
		classification_init.fill(SUBDIV_CLASS_UNKNOWN);
		ctx.classification.set(level, classification_init);
	}
	ctx.edge_remap.resize(old_edge_count);
	ctx.edge_mid_vertex.resize(old_edge_count);
	ctx.edge_mid_vertex.fill(-1);
	ctx.edge_pieces.resize(old_edge_count);
	for (int64_t edge = 0; edge < old_edge_count; edge++) {
		const int32_t vertex_a = ctx.old_edges[edge * 2];
		const int32_t vertex_b = ctx.old_edges[edge * 2 + 1];
		if (ctx.marked_edges.has((int32_t)edge)) {
			const VectorN midpoint = VectorND::multiply_scalar(VectorND::add(ctx.old_vertices[vertex_a], ctx.old_vertices[vertex_b]), 0.5);
			const int32_t mid_vertex = _subdivide_append_vertex(ctx, midpoint, PackedInt32Array{ vertex_a, vertex_b });
			ctx.edge_mid_vertex.set(edge, mid_vertex);
			PackedInt32Array pieces = {
				_subdivide_get_or_create_edge(ctx, vertex_a, mid_vertex, (int32_t)edge),
				_subdivide_get_or_create_edge(ctx, mid_vertex, vertex_b, (int32_t)edge),
			};
			ctx.edge_pieces.set(edge, pieces);
			ctx.edge_remap.set(edge, -1);
		} else {
			ctx.edge_remap.set(edge, _subdivide_get_or_create_edge(ctx, vertex_a, vertex_b, (int32_t)edge));
		}
	}
	// Process each level from the bottom up, so that refinements can use their members' refinements.
	for (int64_t level = 0; level < level_count; level++) {
		const int64_t cell_count = ctx.old_levels[level].size();
		PackedInt32Array remap;
		remap.resize(cell_count);
		ctx.level_remap.set(level, remap);
		for (int64_t i = 0; i < cell_count; i++) {
			if (ctx.marked_levels[level].has((int32_t)i)) {
				ctx.level_remap.write[level].set(i, -1);
				if (level == 0) {
					_subdivide_refine_face(ctx, (int32_t)i);
				} else {
					_subdivide_refine_cell(ctx, level, (int32_t)i);
				}
				continue;
			}
			const PackedInt32Array &members = ctx.old_levels[level][i];
			bool any_member_subdivided = false;
			for (const int32_t member : members) {
				if (level == 0 ? ctx.marked_edges.has(member) : ctx.marked_levels[level - 1].has(member)) {
					any_member_subdivided = true;
					break;
				}
			}
			PackedInt32Array new_members;
			if (level == 0) {
				if (any_member_subdivided) {
					new_members = _subdivide_conform_face(ctx, (int32_t)i);
				} else {
					for (const int32_t member : members) {
						new_members.append(ctx.edge_remap[member]);
					}
				}
			} else {
				for (const int32_t member : members) {
					if (ctx.marked_levels[level - 1].has(member)) {
						new_members.append_array(ctx.refined_levels[level - 1][member].all_pieces);
					} else {
						new_members.append(ctx.level_remap[level - 1][member]);
					}
				}
			}
			ctx.level_remap.write[level].set(i, _subdivide_append_cell(ctx, level, new_members, (int32_t)i));
		}
	}
	// Write the new data into the mesh.
	p_input_mesh->set_poly_cell_vertex_positions(ctx.new_vertices);
	p_input_mesh->set_edge_vertex_indices(ctx.new_edges);
	p_input_mesh->set_poly_cell_indices(ctx.new_levels);
	p_input_mesh->set_all_poly_cell_normal_indices(HashMap<Vector2i, Vector<PackedInt32Array>>());
	p_input_mesh->set_all_poly_cell_texture_map_indices(HashMap<Vector2i, Vector<PackedInt32Array>>());
	p_input_mesh->set_poly_cell_normal_values(Vector<VectorN>());
	p_input_mesh->set_poly_cell_texture_map_values(Vector<VectorM>());
	// Remap the pivot overrides. Pieces of subdivided cells lose their parent's override.
	const int64_t new_boundary_count = has_boundary_level ? ctx.new_levels[boundary_level].size() : 0;
	if (has_boundary_level && !old_pivot_overrides.is_empty()) {
		PackedInt32Array new_pivot_overrides;
		new_pivot_overrides.resize(new_boundary_count);
		new_pivot_overrides.fill(-1);
		for (int64_t i = 0; i < old_pivot_overrides.size() && i < ctx.old_levels[boundary_level].size(); i++) {
			const int32_t new_index = ctx.level_remap[boundary_level][i];
			if (old_pivot_overrides[i] != -1 && new_index >= 0) {
				new_pivot_overrides.set(new_index, old_pivot_overrides[i]);
			}
		}
		p_input_mesh->set_poly_cell_boundary_pivot_overrides(new_pivot_overrides);
	} else {
		p_input_mesh->set_poly_cell_boundary_pivot_overrides(PackedInt32Array());
	}
	// Remap the seams, which refer to the (N-2)-dimensional borders between boundary cells.
	if (!old_seams.is_empty()) {
		HashSet<int32_t> new_seams;
		const int64_t seam_level = dimension - 4; // -1 means the flat edge array.
		for (const int32_t seam : old_seams) {
			if (seam_level < 0) {
				if (seam < 0 || seam >= old_edge_count) {
					continue;
				}
				if (ctx.edge_remap[seam] >= 0) {
					new_seams.insert(ctx.edge_remap[seam]);
				} else {
					new_seams.insert(ctx.edge_pieces[seam][0]);
					new_seams.insert(ctx.edge_pieces[seam][1]);
				}
			} else if (seam_level < level_count) {
				if (seam < 0 || seam >= ctx.old_levels[seam_level].size()) {
					continue;
				}
				if (ctx.level_remap[seam_level][seam] >= 0) {
					new_seams.insert(ctx.level_remap[seam_level][seam]);
				} else {
					for (const int32_t piece : ctx.refined_levels[seam_level][seam].all_pieces) {
						new_seams.insert(piece);
					}
				}
			}
		}
		p_input_mesh->set_seam_indices(new_seams);
	} else {
		p_input_mesh->set_seam_indices(HashSet<int32_t>());
	}
	// Orient every boundary cell so its orientation-derived normal matches its pre-subdivision
	// normal, inherited from its parent for new pieces. New interior cells have no desired normal.
	if (has_boundary_level) {
		Vector<VectorN> desired_normals;
		desired_normals.resize(new_boundary_count);
		for (int64_t i = 0; i < new_boundary_count; i++) {
			const int32_t parent = ctx.new_level_parents[boundary_level][i];
			if (parent >= 0 && parent < old_boundary_normals.size()) {
				desired_normals.set(i, old_boundary_normals[parent]);
			}
		}
		p_input_mesh->set_poly_cell_boundary_normals(Vector<VectorN>());
		p_input_mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY);
		const Vector<VectorN> oriented_normals = p_input_mesh->get_poly_cell_boundary_normals();
		Vector<PackedInt32Array> boundary_cells = ctx.new_levels[boundary_level];
		bool any_flipped = false;
		for (int64_t i = 0; i < new_boundary_count && i < oriented_normals.size(); i++) {
			const VectorN &desired = desired_normals[i];
			if (VectorND::is_zero_approx(desired) || VectorND::dot(oriented_normals[i], desired) >= 0.0) {
				continue;
			}
			PackedInt32Array boundary_cell = boundary_cells[i];
			PolyMeshND::flip_poly_cell_orientation(boundary_cell, boundary_level);
			boundary_cells.set(i, boundary_cell);
			any_flipped = true;
		}
		if (any_flipped) {
			ctx.new_levels.set(boundary_level, boundary_cells);
			p_input_mesh->set_poly_cell_indices(ctx.new_levels);
		}
		if (had_stored_normals) {
			p_input_mesh->set_poly_cell_boundary_normals(desired_normals);
		} else {
			p_input_mesh->set_poly_cell_boundary_normals(Vector<VectorN>());
		}
	}
	// Regenerate the boundary cell vertex normals and texture maps by interpolation.
	// New vertices average the values of their source vertices within each parent cell.
	if (has_boundary_level) {
		const Vector2i cell_to_vert_key = Vector2i((int32_t)dimension - 1, 0);
		const int64_t old_vertex_count = ctx.old_vertices.size();
		const Vector<Vector<VectorN>> *old_vertex_normals = old_all_normals.getptr(cell_to_vert_key);
		const Vector<Vector<VectorM>> *old_texture_maps = old_all_texture_maps.getptr(cell_to_vert_key);
		if (old_vertex_normals != nullptr || old_texture_maps != nullptr) {
			const Vector<PackedInt32Array> new_cell_vertex_indices = p_input_mesh->get_all_poly_cell_vertex_indices((int32_t)dimension - 1, false);
			Vector<Vector<VectorN>> new_vertex_normals;
			Vector<Vector<VectorM>> new_texture_maps;
			new_vertex_normals.resize(new_boundary_count);
			new_texture_maps.resize(new_boundary_count);
			for (int64_t i = 0; i < new_boundary_count; i++) {
				const int32_t parent = ctx.new_level_parents[boundary_level][i];
				if (parent < 0) {
					continue; // Interior cells have no vertex normals or texture maps.
				}
				const PackedInt32Array &parent_vertices = ctx.old_level_vertices[boundary_level][parent];
				const PackedInt32Array &new_cell_vertices = new_cell_vertex_indices[i];
				const PackedInt32Array parent_source = { parent };
				// Interpolate the vertex normals within the parent cell. New vertices average their source vertices.
				if (old_vertex_normals != nullptr && parent < old_vertex_normals->size() && (*old_vertex_normals)[parent].size() == parent_vertices.size()) {
					Vector<VectorN> cell_values;
					_sample_corner_values(new_cell_vertices, parent_source, ctx.old_level_vertices[boundary_level], *old_vertex_normals, CORNER_SAMPLE_AVERAGE, VectorN(), cell_values, true, &ctx.new_vertex_sources, old_vertex_count);
					new_vertex_normals.set(i, cell_values);
				}
				// Interpolate the texture map within the parent cell.
				if (old_texture_maps != nullptr && parent < old_texture_maps->size() && (*old_texture_maps)[parent].size() == parent_vertices.size()) {
					Vector<VectorM> cell_values;
					_sample_corner_values(new_cell_vertices, parent_source, ctx.old_level_vertices[boundary_level], *old_texture_maps, CORNER_SAMPLE_AVERAGE, VectorM(), cell_values, false, &ctx.new_vertex_sources, old_vertex_count);
					new_texture_maps.set(i, cell_values);
				}
			}
			if (old_vertex_normals != nullptr) {
				p_input_mesh->set_poly_cell_dense_normals(cell_to_vert_key, new_vertex_normals);
			}
			if (old_texture_maps != nullptr) {
				p_input_mesh->set_poly_cell_dense_texture_map(cell_to_vert_key, new_texture_maps);
			}
		}
	}
	// Warn about any other data bindings, whose element indices are stale after subdivision.
	for (const KeyValue<Vector2i, Vector<Vector<VectorN>>> &kv : old_all_normals) {
		if (kv.key != Vector2i((int32_t)dimension - 1, (int32_t)dimension - 1) && kv.key != Vector2i((int32_t)dimension - 1, 0)) {
			WARN_PRINT("PolyMeshBuilderND: Discarding normals data for binding key " + String(Variant(kv.key)) + " during subdivision.");
		}
	}
	for (const KeyValue<Vector2i, Vector<Vector<VectorM>>> &kv : old_all_texture_maps) {
		if (kv.key != Vector2i((int32_t)dimension - 1, 0)) {
			WARN_PRINT("PolyMeshBuilderND: Discarding texture map data for binding key " + String(Variant(kv.key)) + " during subdivision.");
		}
	}
	CRASH_COND(!p_input_mesh->is_mesh_data_valid());
	// Return the indices of the new elements created from the selected ones, so that callers
	// can keep track of the inputs. For example, in a Blender-like app, the user might select
	// a set of faces, subdivide them, and update the selection to the new subdivided faces.
	for (const int32_t element : selection) {
		if (selection_level < 0) {
			ret.append_array(ctx.edge_pieces[element]);
		} else {
			ret.append_array(ctx.refined_levels[selection_level][element].all_pieces);
		}
	}
	return ret;
}

PolyMeshBuilderND *PolyMeshBuilderND::singleton = nullptr;

void PolyMeshBuilderND::_bind_methods() {
	// These functions create new meshes from the given data.
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("convert_mesh_3d_to_nd_faces_only", "mesh_3d", "which_surface", "deduplicate"), &PolyMeshBuilderND::convert_mesh_3d_to_nd_faces_only, DEFVAL(-1), DEFVAL(true));
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("extrude_linear", "input_mesh", "extrusion_vector"), &PolyMeshBuilderND::extrude_linear, DEFVAL(VectorN()));
	// In-place adjustments to the given mesh.
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("delete_interior", "mesh_nd"), &PolyMeshBuilderND::delete_interior);
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("make_boundary_normals_topologically_consistent", "mesh_nd", "authoritative_boundary_cells"), &PolyMeshBuilderND::make_boundary_normals_topologically_consistent);
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("merge_coplanar_faces", "mesh_nd", "angle_tolerance_radians"), &PolyMeshBuilderND::merge_coplanar_faces, DEFVAL(0.001));
	ClassDB::bind_static_method("PolyMeshBuilderND", D_METHOD("subdivide_elements", "input_mesh", "dimension", "elements"), &PolyMeshBuilderND::subdivide_elements, DEFVAL(PackedInt32Array()));
}
