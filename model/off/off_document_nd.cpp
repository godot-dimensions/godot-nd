#include "off_document_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
//#include <godot_cpp/classes/mesh_instance3d.hpp>
//#include <godot_cpp/classes/standard_material3d.hpp>
//#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#elif GODOT_MODULE
//#include "scene/3d/mesh_instance_3d.h"
//#include "scene/resources/surface_tool.h"
#endif

#include "../mesh_instance_nd.h"
#include "../wire/wire_material_nd.h"

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

	ClassDB::bind_method(D_METHOD("generate_wire_mesh_nd", "deduplicate_edges"), &OFFDocumentND::generate_wire_mesh_nd, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("generate_node", "deduplicate_edges"), &OFFDocumentND::generate_node, DEFVAL(true));

	ClassDB::bind_static_method("OFFDocumentND", D_METHOD("load_from_file", "path"), &OFFDocumentND::load_from_file);
	ClassDB::bind_method(D_METHOD("save_to_file", "path"), &OFFDocumentND::save_to_file);
}
