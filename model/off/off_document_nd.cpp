#include "off_document_nd.h"

#include "../cell/cell_material_nd.h"
#include "../mesh_instance_nd.h"
#include "../wire/wire_material_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#endif

void OFFDocumentND::_count_unique_edges_from_faces() {
	_edge_count = 0;
	HashSet<Vector2i> unique_items;
	Vector<PackedInt32Array> face_cell_indices = _cell_face_indices[0];
	for (int face_number = 0; face_number < face_cell_indices.size(); face_number++) {
		PackedInt32Array face_vertex_indices = face_cell_indices[face_number];
		for (int face_index = 0; face_index < face_vertex_indices.size(); face_index++) {
			const int second_index = (face_index + 1) % face_vertex_indices.size();
			Vector2i edge_indices = Vector2i(face_vertex_indices[face_index], face_vertex_indices[second_index]);
			if (edge_indices.x > edge_indices.y) {
				SWAP(edge_indices.x, edge_indices.y);
			}
			if (unique_items.has(edge_indices)) {
				continue;
			}
			unique_items.insert(edge_indices);
			_edge_count++;
		}
	}
}

int OFFDocumentND::_find_or_insert_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int vertex_count = _vertices.size();
	if (p_deduplicate_vertices) {
		for (int vertex_number = 0; vertex_number < vertex_count; vertex_number++) {
			if (_vertices[vertex_number] == p_vertex) {
				return vertex_number;
			}
		}
	}
	_vertices.append(p_vertex);
	return vertex_count;
}

// OFF stores cells with indices to faces, but this provides indices of vertices.
Vector<Vector<PackedInt32Array>> OFFDocumentND::_calculate_cell_vertex_indices() {
	const int64_t cell_face_indices_size = _cell_face_indices.size();
	Vector<Vector<PackedInt32Array>> ret;
	if (cell_face_indices_size == 0) {
		return ret;
	}
	ret.resize(cell_face_indices_size);
	// Special case: OFF faces already store vertices.
	Vector<PackedInt32Array> prev_dimension_cell_vertex_indices = _cell_face_indices[0];
	ret.set(0, prev_dimension_cell_vertex_indices);
	// General cases: OFF cells store faces from the previous dimension.
	for (int64_t i = 1; i < cell_face_indices_size; i++) {
		const Vector<PackedInt32Array> cell_face_indices = _cell_face_indices[i];
		Vector<PackedInt32Array> dim_cell_vertex_indices;
		dim_cell_vertex_indices.resize(cell_face_indices.size());
		for (int64_t cell_number = 0; cell_number < cell_face_indices.size(); cell_number++) {
			PackedInt32Array face_indices = cell_face_indices[cell_number];
			const int64_t face_size = face_indices.size();
			PackedInt32Array this_cell_vertex_indices;
			for (int64_t face_number = 0; face_number < face_size; face_number++) {
				const int32_t face_index = face_indices[face_number];
				ERR_CONTINUE(face_index >= prev_dimension_cell_vertex_indices.size());
				const PackedInt32Array face_vertex_indices = prev_dimension_cell_vertex_indices[face_index];
				for (int64_t vertex_number = 0; vertex_number < face_vertex_indices.size(); vertex_number++) {
					const int32_t vertex_index = face_vertex_indices[vertex_number];
					if (!this_cell_vertex_indices.has(vertex_index)) {
						this_cell_vertex_indices.append(vertex_index);
					}
				}
			}
			dim_cell_vertex_indices.set(cell_number, this_cell_vertex_indices);
		}
		prev_dimension_cell_vertex_indices = dim_cell_vertex_indices;
		ret.set(i, dim_cell_vertex_indices);
	}
	return ret;
}

Vector<Vector<PackedInt32Array>> OFFDocumentND::_calculate_simplex_vertex_indices(const Vector<Vector<PackedInt32Array>> &p_cell_vertex_indices) {
	const int64_t cell_face_indices_size = _cell_face_indices.size();
	CRASH_COND(p_cell_vertex_indices.size() != cell_face_indices_size);
	Vector<Vector<PackedInt32Array>> ret;
	if (cell_face_indices_size == 0) {
		return ret;
	}
	ret.resize(cell_face_indices_size);
	// Special case: OFF faces store vertices, so we need to compose them into edges (1D simplexes) for triangles.
	Vector<PackedInt32Array> prev_dimension_simplex_vertex_indices;
	{
		Vector<PackedInt32Array> prev_dimension_cell_vertex_indices = p_cell_vertex_indices[0];
		for (int64_t i = 0; i < prev_dimension_cell_vertex_indices.size(); i++) {
			const PackedInt32Array polygon_vertex_indices = prev_dimension_cell_vertex_indices[i];
			const int64_t triangle_count = polygon_vertex_indices.size() - 2;
			ERR_CONTINUE(triangle_count < 1);
			PackedInt32Array triangle_vertex_indices;
			triangle_vertex_indices.resize(3 * triangle_count);
			// This logic assumes that the face is a flat convex 2D polygon with vertices in a consistent winding order.
			// We break it into triangles by connecting the first vertex to every pair of adjacent vertices.
			const int32_t pivot_vertex_index = polygon_vertex_indices[0];
			int64_t simplex_iter = 0;
			for (int64_t triangle_index = 0; triangle_index < triangle_count; triangle_index++) {
				triangle_vertex_indices.set(simplex_iter++, pivot_vertex_index);
				triangle_vertex_indices.set(simplex_iter++, polygon_vertex_indices[triangle_index + 1]);
				triangle_vertex_indices.set(simplex_iter++, polygon_vertex_indices[triangle_index + 2]);
			}
			prev_dimension_simplex_vertex_indices.append(triangle_vertex_indices);
		}
	}
	ret.set(0, prev_dimension_simplex_vertex_indices);
	// General cases: OFF cells store faces from the previous dimension (which we've calculated simplexes for).
	// This code generates more advanced simplexes for each successive dimension.
	for (int64_t i = 1; i < cell_face_indices_size; i++) {
		const Vector<PackedInt32Array> dim_cell_face_indices = _cell_face_indices[i];
		const Vector<PackedInt32Array> dim_cell_vertex_indices = p_cell_vertex_indices[i];
		CRASH_COND(dim_cell_face_indices.size() != dim_cell_vertex_indices.size());
		const int64_t prev_dimension_verts_per_simplex = i + 2;
		const int64_t current_dimension_verts_per_simplex = i + 3;
		Vector<PackedInt32Array> dim_simplex_vertex_indices;
		dim_simplex_vertex_indices.resize(dim_cell_face_indices.size());
		int32_t prev_cell_pivot_vertex = -1;
		for (int cell_number = 0; cell_number < dim_cell_face_indices.size(); cell_number++) {
			const PackedInt32Array face_indices = dim_cell_face_indices[cell_number];
			const PackedInt32Array vertex_indices = dim_cell_vertex_indices[cell_number];
			ERR_FAIL_COND_V(vertex_indices.size() < 2, ret);
			PackedInt32Array simplex_indices;
			int64_t simplex_iter = 0;
			const int64_t face_size = face_indices.size();
			// This logic assumes that the face is a "flat" convex polytope where all vertices are "visible" to the chosen pivot vertex.
			const int32_t pivot_vertex_index = (vertex_indices[0] == prev_cell_pivot_vertex) ? vertex_indices[1] : vertex_indices[0];
			for (int64_t face_number = 0; face_number < face_size; face_number++) {
				PackedInt32Array face_vertex_indices = prev_dimension_simplex_vertex_indices[face_indices[face_number]];
				if (face_vertex_indices.has(pivot_vertex_index)) {
					// Skip any faces connected to the pivot vertex.
					// For example, if making tetrahedra out of a box, we only want the 3 opposite faces to connect 6 tetrahedra.
					continue;
				}
				int64_t face_vert_iter = 0;
				const int64_t face_simplex_count = face_vertex_indices.size() / prev_dimension_verts_per_simplex;
				simplex_indices.resize(simplex_iter + current_dimension_verts_per_simplex * face_simplex_count);
				for (int64_t new_simplex_number = 0; new_simplex_number < face_simplex_count; new_simplex_number++) {
					simplex_indices.set(simplex_iter++, pivot_vertex_index);
					for (int64_t face_simplex_number = 0; face_simplex_number < prev_dimension_verts_per_simplex; face_simplex_number++) {
						simplex_indices.set(simplex_iter++, face_vertex_indices[face_vert_iter++]);
					}
				}
			}
			dim_simplex_vertex_indices.set(cell_number, simplex_indices);
			prev_cell_pivot_vertex = pivot_vertex_index;
		}
		ret.set(i, dim_simplex_vertex_indices);
		prev_dimension_simplex_vertex_indices = dim_simplex_vertex_indices;
	}
	return ret;
}

Ref<ArrayCellMeshND> OFFDocumentND::generate_array_cell_mesh_nd() {
	Ref<ArrayCellMeshND> cell_mesh;
	cell_mesh.instantiate();
	cell_mesh->set_dimension(_dimension);
	cell_mesh->set_vertices(_vertices);
	const Vector<Vector<PackedInt32Array>> cell_vertex_indices = _calculate_cell_vertex_indices();
	const Vector<Vector<PackedInt32Array>> simplex_vertex_indices = _calculate_simplex_vertex_indices(cell_vertex_indices);
	const int64_t simplex_vertex_indices_size = simplex_vertex_indices.size();
	CRASH_COND(simplex_vertex_indices_size != _cell_colors.size());
	ERR_FAIL_COND_V(simplex_vertex_indices_size == 0, cell_mesh);
	// After those calculations, we can discard most of the data.
	// We just need the last array of the simplex vertex indices.
	const Vector<PackedInt32Array> last_simplex_vertex_indices = simplex_vertex_indices[simplex_vertex_indices_size - 1];
	const int64_t last_vertices_per_simplex = simplex_vertex_indices_size + 2;
	const PackedColorArray last_cell_colors = _cell_colors[simplex_vertex_indices_size - 1];
	int64_t cell_colors_iter = 0;
	PackedColorArray packed_cell_colors;
	PackedInt32Array packed_cell_indices;
	for (int i = 0; i < last_simplex_vertex_indices.size(); i++) {
		PackedInt32Array simplex_vertex_indices_for_cell = last_simplex_vertex_indices[i];
		packed_cell_indices.append_array(simplex_vertex_indices_for_cell);
		if (_has_any_cell_colors) {
			const int64_t simplexes_in_face = simplex_vertex_indices_for_cell.size() / last_vertices_per_simplex;
			packed_cell_colors.resize(cell_colors_iter + simplexes_in_face);
			for (int simplex_number = 0; simplex_number < simplexes_in_face; simplex_number++) {
				packed_cell_colors.set(cell_colors_iter++, last_cell_colors[i]);
			}
		}
	}
	cell_mesh->set_cell_indices(packed_cell_indices);
	if (_has_any_cell_colors) {
		Ref<CellMaterialND> cell_material;
		cell_material.instantiate();
		cell_material->set_albedo_source_flags(MaterialND::COLOR_SOURCE_FLAG_PER_CELL);
		cell_material->set_albedo_color_array(packed_cell_colors);
		cell_mesh->set_material(cell_material);
	}
	ERR_FAIL_COND_V_MSG(!cell_mesh->is_mesh_data_valid(), cell_mesh, "OFFDocumentND: Failed to import OFF as cell mesh, mesh data is not valid.");
	return cell_mesh;
}

Ref<ArrayWireMeshND> OFFDocumentND::generate_wire_mesh_nd(const bool p_deduplicate_edges) {
	Ref<ArrayWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_vertices(_vertices);
	Ref<WireMaterialND> wire_material;
	if (_has_any_cell_colors) {
		wire_material.instantiate();
		wire_material->set_albedo_source(WireMaterialND::WIRE_COLOR_SOURCE_PER_EDGE_ONLY);
		wire_mesh->set_material(wire_material);
	}
	Vector<PackedInt32Array> face_cell_indices = _cell_face_indices[0];
	PackedColorArray face_colors = _cell_colors[0];
	for (int face_number = 0; face_number < face_cell_indices.size(); face_number++) {
		PackedInt32Array face_indices = face_cell_indices[face_number];
		const int face_size = face_indices.size();
		for (int face_index = 0; face_index < face_size; face_index++) {
			const int second_index = (face_index + 1) % face_size;
			if (p_deduplicate_edges) {
				if (wire_mesh->has_edge_indices(face_indices[face_index], face_indices[second_index])) {
					continue;
				}
			}
			wire_mesh->append_edge_indices(face_indices[face_index], face_indices[second_index]);
			if (_has_any_cell_colors) {
				wire_material->append_albedo_color(face_colors[face_number]);
			}
		}
	}
	return wire_mesh;
}

Node *OFFDocumentND::generate_node(const bool p_deduplicate_edges) {
	MeshInstanceND *mesh_instance_nd = memnew(MeshInstanceND);
	mesh_instance_nd->set_dimension(_dimension);
	Ref<ArrayWireMeshND> wire_mesh = generate_wire_mesh_nd(p_deduplicate_edges);
	mesh_instance_nd->set_mesh(wire_mesh);
	return mesh_instance_nd;
}

enum class OFFDocumentNDReadState {
	READ_SIZE,
	READ_VERTICES,
	READ_CELLS,
};

Ref<OFFDocumentND> OFFDocumentND::load_from_file(const String &p_path) {
	Ref<OFFDocumentND> off_document;
#if GDEXTENSION
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(file.is_null(), off_document, "Error: Could not open file " + p_path + ".");
#elif GODOT_MODULE
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
	ERR_FAIL_COND_V_MSG(err != OK, off_document, "Error: Could not open file " + p_path + ".");
#endif
	off_document.instantiate();
	bool can_warn = true;
	OFFDocumentNDReadState read_state = OFFDocumentNDReadState::READ_SIZE;
	Vector<PackedInt32Array> dim_cell_face_indices;
	PackedColorArray dim_cell_colors;
	// Index 0 is 2D cells (triangles), index 1 is 3D cells (tetrahedra), etc.
	PackedInt32Array cell_counts;
	int current_cell_dimension = 0;
	int current_cell_index = 0;
	int current_vertex_index = 0;
	int vertex_count = 0;
	while (!file->eof_reached()) {
		const String line = file->get_line();
		if (line.is_empty() || line.begins_with("#")) {
			continue;
		}
		if (line.contains("OFF")) {
			if (line == "OFF") {
				off_document->_dimension = 3;
			} else {
				const int dimension = line.to_int();
				if (dimension > 0) {
					off_document->_dimension = dimension;
				} else {
					return off_document;
				}
			}
			continue;
		}
		PackedStringArray items = line.split(" ", false);
		const int item_count = items.size();
		if (item_count < 3) {
			if (can_warn) {
				can_warn = false;
				WARN_PRINT("Warning: OFF file " + p_path + " contains invalid line: '" + line + "'. Skipping this line and attempting to read the rest of the file anyway.");
			}
			continue;
		}
		switch (read_state) {
			case OFFDocumentNDReadState::READ_SIZE: {
				vertex_count = items[0].to_int();
				off_document->_vertices.resize(vertex_count);
				// OFF stores sizes in the order 0D, 2D, 1D, 3D, 4D, ... :(
				if (item_count > 2) {
					off_document->_edge_count = items[2].to_int();
					// Change the order to 0D, 2D, 2D, 3D, 4D, ... :)
					items.set(2, items[1]);
					const int cell_counts_count = item_count - 2;
					cell_counts.resize(cell_counts_count);
					off_document->_cell_face_indices.resize(cell_counts_count);
					off_document->_cell_colors.resize(cell_counts_count);
					for (int i = 0; i < cell_counts_count; i++) {
						cell_counts.set(i, items[i + 2].to_int());
					}
				}
				read_state = OFFDocumentNDReadState::READ_VERTICES;
			} break;
			case OFFDocumentNDReadState::READ_VERTICES: {
				VectorN vertex;
				vertex.resize(item_count);
				for (int i = 0; i < item_count; i++) {
					vertex.set(i, items[i].to_float());
				}
				off_document->_vertices.set(current_vertex_index, vertex);
				current_vertex_index++;
				if (current_vertex_index >= vertex_count) {
					read_state = OFFDocumentNDReadState::READ_CELLS;
				}
			} break;
			case OFFDocumentNDReadState::READ_CELLS: {
				if (dim_cell_face_indices.is_empty()) {
					dim_cell_face_indices.resize(cell_counts[current_cell_dimension]);
					dim_cell_colors.resize(cell_counts[current_cell_dimension]);
				}
				const int this_cell_count = items[0].to_int();
				PackedInt32Array this_cell_indices;
				if (item_count > this_cell_count) {
					for (int i = 1; i <= this_cell_count; i++) {
						this_cell_indices.append(items[i].to_int());
					}
				}
				dim_cell_face_indices.set(current_cell_index, this_cell_indices);
				if (item_count > this_cell_count + 3) {
					// The last 3 numbers are the cell color on the range 0-255.
					dim_cell_colors.set(current_cell_index, Color(items[this_cell_count + 1].to_int() / 255.0f, items[this_cell_count + 2].to_int() / 255.0f, items[this_cell_count + 3].to_int() / 255.0f));
					off_document->_has_any_cell_colors = true;
				} else {
					dim_cell_colors.set(current_cell_index, Color(1.0f, 1.0f, 1.0f));
				}
				current_cell_index++;
				if (current_cell_index >= cell_counts[current_cell_dimension]) {
					current_cell_index = 0;
					off_document->_cell_face_indices.set(current_cell_dimension, dim_cell_face_indices);
					off_document->_cell_colors.set(current_cell_dimension, dim_cell_colors);
					dim_cell_face_indices = Vector<PackedInt32Array>();
					dim_cell_colors = PackedColorArray();
					current_cell_dimension++;
					if (current_cell_dimension >= cell_counts.size()) {
						return off_document;
					}
				}
			} break;
		}
	}
	return off_document;
}

String _vectorn_to_off_nd(const VectorN &p_vertex) {
	ERR_FAIL_COND_V(p_vertex.size() == 0, String());
	String ret = String::num(p_vertex[0]);
	if (p_vertex[0] == (real_t)(int64_t)p_vertex[0]) {
		ret += String(".0");
	}
	for (int i = 1; i < p_vertex.size(); i++) {
		ret += " " + String::num(p_vertex[i]);
		if (p_vertex[i] == (real_t)(int64_t)p_vertex[i]) {
			ret += String(".0");
		}
	}
	return ret;
}

String _color_to_off_string_nd(const Color &p_color) {
	return " " + String::num_int64(p_color.r * 255.0f) + " " + String::num_int64(p_color.g * 255.0f) + " " + String::num_int64(p_color.b * 255.0f);
}

String _cell_to_off_string_nd(const PackedInt32Array &p_face) {
	String ret = String::num(p_face.size());
	for (int i = 0; i < p_face.size(); i++) {
		ret += String(" ") + String::num_int64(p_face[i]);
	}
	return ret;
}

String _cell_dimension_index_to_off_comment(const int p_dimension) {
	// Skip 0D vertices and 1D edges, the former is handled separately and the latter is not stored in the OFF file.
	switch (p_dimension) {
		case 0:
			return "\n# Faces";
		case 1:
			return "\n# Cells";
		case 2:
			return "\n# Tera";
		case 3:
			return "\n# Peta";
		case 4:
			return "\n# Exa";
		case 5:
			return "\n# Zetta";
		case 6:
			return "\n# Yotta";
		case 7:
			return "\n# Ronna";
		case 8:
			return "\n# Quetta";
		default:
			return "\n# " + String::num_int64(p_dimension + 2) + "D-cell";
	}
}

void OFFDocumentND::save_to_file(const String &p_path) {
	if (_edge_count == 0) {
		_count_unique_edges_from_faces();
	}
#if GDEXTENSION
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(file.is_null(), "Error: Could not open file " + p_path + " for writing.");
#elif GODOT_MODULE
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_MSG(err != OK, "Error: Could not open file " + p_path + " for writing.");
#endif
	if (_dimension == 3) {
		file->store_line("OFF");
	} else {
		file->store_line(String::num_int64(_dimension) + "OFF");
	}
	String size_line = String::num_int64(_vertices.size());
	if (_cell_face_indices.size() > 0) {
		size_line += " " + String::num_int64(_cell_face_indices.size()) + " " + String::num_int64(_edge_count);
		for (int i = 1; i < _cell_face_indices.size(); i++) {
			Vector<PackedInt32Array> dim_cell_face_indices = _cell_face_indices[i];
			size_line += " " + String::num_int64(dim_cell_face_indices.size());
		}
	}
	file->store_line(size_line);
	file->store_line("\n# Vertices");
	for (int i = 0; i < _vertices.size(); i++) {
		file->store_line(_vectorn_to_off_nd(_vertices[i]));
	}
	for (int dim = 0; dim < _cell_face_indices.size(); dim++) {
		Vector<PackedInt32Array> dim_cell_face_indices = _cell_face_indices[dim];
		ERR_FAIL_COND(dim_cell_face_indices.size() == 0);
		PackedColorArray dim_cell_colors = _cell_colors[dim];
		file->store_line(_cell_dimension_index_to_off_comment(dim));
		for (int i = 0; i < dim_cell_face_indices.size(); i++) {
			String cell_str = _cell_to_off_string_nd(dim_cell_face_indices[i]);
			if (i < dim_cell_colors.size() && !dim_cell_colors[i].is_equal_approx(Color(1.0f, 1.0f, 1.0f))) {
				cell_str += _color_to_off_string_nd(dim_cell_colors[i]);
			}
			file->store_line(cell_str);
		}
	}
}

void OFFDocumentND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_edge_count"), &OFFDocumentND::get_edge_count);
	ClassDB::bind_method(D_METHOD("set_edge_count", "edge_count"), &OFFDocumentND::set_edge_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "edge_count"), "set_edge_count", "get_edge_count");

	ClassDB::bind_method(D_METHOD("generate_array_cell_mesh_nd"), &OFFDocumentND::generate_array_cell_mesh_nd);
	ClassDB::bind_method(D_METHOD("generate_wire_mesh_nd", "deduplicate_edges"), &OFFDocumentND::generate_wire_mesh_nd, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("generate_node", "deduplicate_edges"), &OFFDocumentND::generate_node, DEFVAL(true));

	ClassDB::bind_static_method("OFFDocumentND", D_METHOD("load_from_file", "path"), &OFFDocumentND::load_from_file);
	ClassDB::bind_method(D_METHOD("save_to_file", "path"), &OFFDocumentND::save_to_file);
}
