#include "off_document_nd.h"

#include "../../math/vector_nd.h"
#include "../mesh/cell/cell_material_nd.h"
#include "../mesh/mesh_instance_nd.h"
#include "../mesh/poly/poly_material_nd.h"
#include "../mesh/wire/wire_material_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#elif GODOT_MODULE
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#endif

void OFFDocumentND::_count_unique_edges_from_faces() {
	_edge_count = 0;
	if (_cell_face_indices.size() == 0) {
		return;
	}
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

int64_t OFFDocumentND::_find_or_insert_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices) {
	const int64_t vertex_count = _vertex_positions.size();
	if (p_deduplicate_vertices) {
		for (int64_t vertex_number = 0; vertex_number < vertex_count; vertex_number++) {
			if (VectorND::is_equal_exact(_vertex_positions[vertex_number], p_vertex)) {
				return vertex_number;
			}
		}
	}
	_vertex_positions.append(p_vertex);
	return vertex_count;
}

PackedInt32Array OFFDocumentND::_insert_simplex_facets(const PackedInt32Array &p_simplex_vertex_indices, const bool p_deduplicate, Vector<SortedIndicesMap> &r_lookup_maps) {
	// The facets of a simplex are the simplexes made by omitting one vertex each. Swapping the first two
	// vertices of every other facet keeps the facets consistently oriented as a closed boundary, which
	// for tetrahedra produces the same triangle winding as the 4D module's OFF export.
	const int64_t vertex_count = p_simplex_vertex_indices.size();
	PackedInt32Array facet_indices;
	facet_indices.resize(vertex_count);
	for (int64_t omitted = 0; omitted < vertex_count; omitted++) {
		PackedInt32Array facet_vertex_indices = p_simplex_vertex_indices;
		facet_vertex_indices.remove_at(omitted);
		if (omitted % 2 == 0) {
			const int32_t swap_tmp = facet_vertex_indices[0];
			facet_vertex_indices.set(0, facet_vertex_indices[1]);
			facet_vertex_indices.set(1, swap_tmp);
		}
		facet_indices.set(omitted, _find_or_insert_simplex_cell(facet_vertex_indices, p_deduplicate, r_lookup_maps));
	}
	return facet_indices;
}

int32_t OFFDocumentND::_find_or_insert_simplex_cell(const PackedInt32Array &p_simplex_vertex_indices, const bool p_deduplicate, Vector<SortedIndicesMap> &r_lookup_maps) {
	// OFF stores 2D faces at index 0 as vertex indices, 3D cells at index 1 as face indices, and so on.
	const int64_t cell_dim_index = p_simplex_vertex_indices.size() - 3;
	ERR_FAIL_INDEX_V(cell_dim_index, _cell_face_indices.size(), -1);
	PackedInt32Array sorted_vertex_indices;
	if (p_deduplicate) {
		// A simplex is fully determined by its set of vertices, regardless of their order.
		sorted_vertex_indices = p_simplex_vertex_indices;
		sorted_vertex_indices.sort();
		const int32_t *existing_index = r_lookup_maps[cell_dim_index].getptr(sorted_vertex_indices);
		if (existing_index != nullptr) {
			return *existing_index;
		}
	}
	const PackedInt32Array members = cell_dim_index == 0 ? p_simplex_vertex_indices : _insert_simplex_facets(p_simplex_vertex_indices, p_deduplicate, r_lookup_maps);
	const int32_t new_index = (int32_t)_cell_face_indices[cell_dim_index].size();
	_cell_face_indices.write[cell_dim_index].append(members);
	if (p_deduplicate) {
		r_lookup_maps.write[cell_dim_index].insert(sorted_vertex_indices, new_index);
	}
	return new_index;
}

void OFFDocumentND::_export_convert_cell_colors_nd(const Ref<CellMeshND> &p_mesh) {
	// Every cell dimension needs a color array, even if empty, so that they line up with the cell face indices.
	_cell_colors.clear();
	_cell_colors.resize(_cell_face_indices.size());
	_has_any_cell_colors = false;
	const int64_t last_dim_index = _cell_face_indices.size() - 1;
	if (last_dim_index < 0) {
		return;
	}
	const Ref<MaterialND> material = p_mesh->get_material();
	if (material.is_null() || !(material->get_albedo_source_flags() & MaterialND::COLOR_SOURCE_FLAG_PER_CELL)) {
		return;
	}
	PackedColorArray boundary_cell_colors;
	const Ref<PolyMeshND> poly_mesh = p_mesh;
	if (poly_mesh.is_valid()) {
		const Ref<PolyMaterialND> poly_material = material;
		if (poly_material.is_valid() && !poly_material->get_poly_albedo_color_array().is_empty()) {
			// Poly materials store one color per poly boundary cell, which is exactly what OFF needs.
			boundary_cell_colors = poly_material->get_poly_albedo_color_array();
		} else if (last_dim_index == int64_t(p_mesh->get_dimension()) - 3) {
			// Otherwise the colors are per simplex cell, so map each simplex back to the boundary poly cell it was decomposed from.
			const PackedColorArray simplex_colors = material->get_albedo_color_array();
			const int64_t simplex_count = poly_mesh->get_simplex_cell_count();
			const int64_t boundary_cell_count = _cell_face_indices[last_dim_index].size();
			boundary_cell_colors.resize(boundary_cell_count);
			boundary_cell_colors.fill(Color(1.0f, 1.0f, 1.0f));
			// Iterate backwards so that the first simplex of each poly cell provides the color.
			for (int64_t simplex_index = MIN(simplex_count, simplex_colors.size()) - 1; simplex_index >= 0; simplex_index--) {
				const int32_t source_cell = poly_mesh->get_source_poly_cell_for_simplex_cell(simplex_index);
				if (source_cell >= 0 && source_cell < boundary_cell_count) {
					boundary_cell_colors.set(source_cell, simplex_colors[simplex_index]);
				}
			}
		}
	} else {
		// Simplex cells are exported one-to-one in order, so the colors line up as-is.
		boundary_cell_colors = material->get_albedo_color_array();
	}
	_cell_colors.set(last_dim_index, boundary_cell_colors);
	_has_any_cell_colors = !boundary_cell_colors.is_empty();
}

Ref<OFFDocumentND> OFFDocumentND::export_convert_mesh_nd(const Ref<CellMeshND> &p_mesh, const bool p_deduplicate_faces) {
	Ref<OFFDocumentND> off_document;
	ERR_FAIL_COND_V(p_mesh.is_null(), off_document);
	const int dimension = p_mesh->get_dimension();
	ERR_FAIL_COND_V_MSG(dimension < 3, off_document, "OFFDocumentND: Only meshes with 3 or more dimensions can be converted to OFF, because OFF needs 2D faces made of vertices.");
	off_document.instantiate();
	off_document->_dimension = dimension;
	// OFF stores 2D faces at index 0, 3D cells at index 1, and so on, up to the (N-1)-dimensional boundary cells of an N-dimensional mesh.
	const int64_t boundary_dim_index = dimension - 3;
	const Ref<PolyMeshND> poly_mesh = p_mesh;
	if (poly_mesh.is_valid()) {
		// PolyMeshND uses hierarchical geometry like OFF, but each face references edges instead of vertices.
		const Vector<Vector<PackedInt32Array>> poly_cell_indices = poly_mesh->get_poly_cell_indices();
		ERR_FAIL_COND_V_MSG(poly_cell_indices.is_empty(), off_document, "OFFDocumentND: Cannot convert a poly mesh without any faces to OFF.");
		off_document->_vertex_positions = poly_mesh->get_poly_cell_vertex_positions();
		// Skip any N-dimensional hypervolume cells, OFF only stores the boundary cells and below.
		const int64_t cell_dim_count = MIN(poly_cell_indices.size(), boundary_dim_index + 1);
		off_document->_cell_face_indices.resize(cell_dim_count);
		off_document->_cell_face_indices.set(0, poly_mesh->get_all_face_vertex_indices());
		for (int64_t i = 1; i < cell_dim_count; i++) {
			off_document->_cell_face_indices.set(i, poly_cell_indices[i]);
		}
	} else {
		// CellMeshND references its simplex cells by vertex indices, but OFF files reference cells by their
		// facets one dimension down, so build the whole hierarchy of faces, cells, etc. from each simplex.
		off_document->_vertex_positions = p_mesh->get_vertex_positions();
		const PackedInt32Array simplex_vertex_indices = p_mesh->get_simplex_cell_vertex_indices();
		const int64_t indices_per_simplex = p_mesh->get_indices_per_simplex_cell();
		ERR_FAIL_COND_V_MSG(indices_per_simplex != dimension, off_document, "OFFDocumentND: Cannot convert a mesh whose simplex cells are not (N-1)-simplexes with N vertex indices to OFF.");
		ERR_FAIL_COND_V_MSG(simplex_vertex_indices.size() % indices_per_simplex != 0, off_document, "OFFDocumentND: Cannot convert a mesh whose simplex cell vertex indices are not a multiple of the dimension to OFF.");
		off_document->_cell_face_indices.resize(boundary_dim_index + 1);
		Vector<SortedIndicesMap> lookup_maps;
		lookup_maps.resize(boundary_dim_index + 1);
		for (int64_t i = 0; i < simplex_vertex_indices.size(); i += indices_per_simplex) {
			const PackedInt32Array cell_vertex_indices = simplex_vertex_indices.slice(i, i + indices_per_simplex);
			// The boundary simplex cells themselves are never deduplicated, so that per-cell colors line up with the mesh.
			const PackedInt32Array members = boundary_dim_index == 0 ? cell_vertex_indices : off_document->_insert_simplex_facets(cell_vertex_indices, p_deduplicate_faces, lookup_maps);
			off_document->_cell_face_indices.write[boundary_dim_index].append(members);
		}
	}
	off_document->_export_convert_cell_colors_nd(p_mesh);
	off_document->_count_unique_edges_from_faces();
	return off_document;
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

Ref<ArrayCellMeshND> OFFDocumentND::import_generate_array_cell_mesh_nd() {
	Ref<ArrayCellMeshND> cell_mesh;
	cell_mesh.instantiate();
	cell_mesh->set_dimension(_dimension);
	cell_mesh->set_vertex_positions(_vertex_positions);
	ERR_FAIL_COND_V_MSG(_cell_face_indices.is_empty(), cell_mesh, "OFFDocumentND: This OFF document does not contain any cells, so it cannot be converted to a cell mesh. Perhaps this is a vertex-only OFF file, or a 0D or 1D OFF file?");
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
	cell_mesh->set_simplex_cell_vertex_indices(packed_cell_indices);
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

Ref<ArrayWireMeshND> OFFDocumentND::import_generate_wire_mesh_nd(const bool p_deduplicate_edges) {
	Ref<ArrayWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_vertex_positions(_vertex_positions);
	if (_cell_face_indices.is_empty()) {
		if (_vertex_positions.size() == 2) {
			// Special case: OFF file with just two vertices and one implicit edge (such as a 1D OFF file).
			wire_mesh->append_edge_indices(0, 1);
			return wire_mesh;
		}
		if (_dimension < 2) {
			return wire_mesh;
		}
		ERR_FAIL_V_MSG(wire_mesh, "OFFDocumentND: Cannot generate wire mesh from OFF document with no cell face indices.");
	}
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

Node *OFFDocumentND::import_generate_node(const bool p_deduplicate_edges) {
	MeshInstanceND *mesh_instance_nd = memnew(MeshInstanceND);
	mesh_instance_nd->set_dimension(_dimension);
	Ref<ArrayWireMeshND> wire_mesh = import_generate_wire_mesh_nd(p_deduplicate_edges);
	mesh_instance_nd->set_mesh(wire_mesh);
	return mesh_instance_nd;
}

enum class OFFDocumentNDReadState {
	READ_SIZE,
	READ_VERTICES,
	READ_CELLS,
};

Ref<OFFDocumentND> OFFDocumentND::import_load_from_byte_array(const PackedByteArray &p_data) {
	ERR_FAIL_COND_V_MSG(p_data.is_empty(), Ref<OFFDocumentND>(), "OFF import: Error: Given byte array is empty.");
#if GDEXTENSION
	const String as_string = p_data.get_string_from_utf8();
#elif GODOT_MODULE
	String as_string;
	if (p_data.size() > 0) {
		const uint8_t *r = p_data.ptr();
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR < 5
		as_string.parse_utf8((const char *)r, p_data.size(), true);
#else
		as_string = String::utf8((const char *)r, p_data.size());
#endif
	}
#endif
	return OFFDocumentND::_import_load_from_raw_text(as_string, "(in-memory data)");
}

Ref<OFFDocumentND> OFFDocumentND::import_load_from_file(const String &p_path) {
#if GDEXTENSION
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(file.is_null(), Ref<OFFDocumentND>(), "OFF import: Error: Could not open file " + p_path + ".");
#elif GODOT_MODULE
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
	ERR_FAIL_COND_V_MSG(err != OK, Ref<OFFDocumentND>(), "OFF import: Error: Could not open file " + p_path + ".");
#endif
	const String file_text = file->get_as_text();
	return _import_load_from_raw_text(file_text, p_path);
}

Ref<OFFDocumentND> OFFDocumentND::_import_load_from_raw_text(const String &p_raw_text, const String &p_path) {
	Ref<OFFDocumentND> off_document;
	off_document.instantiate();
	OFFDocumentNDReadState read_state = OFFDocumentNDReadState::READ_SIZE;
	Vector<PackedInt32Array> dim_cell_face_indices;
	PackedColorArray dim_cell_colors;
	// Index 0 is 2D cells (triangles), index 1 is 3D cells (tetrahedra), etc.
	PackedInt32Array cell_counts;
	int current_cell_dimension = 0;
	int current_cell_index = 0;
	int current_vertex_index = 0;
	int vertex_count = 0;
	int min_items_per_line = 3;
	bool can_warn = true;
	if (p_raw_text.contains("\r")) {
		WARN_PRINT("OFF import: Warning: OFF file " + p_path + " contains carriage return characters (\\r). Remove them to silence this warning.");
		can_warn = false;
	}
	const PackedStringArray lines = p_raw_text.split("\n", false);
	for (const String &line : lines) {
		if (line.is_empty() || line.begins_with("#")) {
			continue;
		}
		if (line.contains("OFF")) {
			// "OFF" by itself is 3D OFF.
			if (line == "OFF" || !is_digit(line[0])) {
				off_document->_dimension = 3;
			} else {
				const int declared_dimension = line.to_int();
				off_document->_dimension = declared_dimension;
				if (declared_dimension < 3) {
					min_items_per_line = declared_dimension;
				}
			}
			continue;
		}
		PackedStringArray items = line.split(" ", false);
		const int item_count = items.size();
		if (item_count < min_items_per_line) {
			if (can_warn) {
				can_warn = false;
				WARN_PRINT("Warning: OFF file " + p_path + " contains invalid line: '" + line + "'. Skipping this line and attempting to read the rest of the file anyway.");
			}
			continue;
		}
		switch (read_state) {
			case OFFDocumentNDReadState::READ_SIZE: {
				vertex_count = items[0].to_int();
				off_document->_vertex_positions.resize(vertex_count);
				if (item_count == 2) {
					// Special case: 2D OFF file with only vertices and components.
					cell_counts.resize(1);
					cell_counts.set(0, items[1].to_int());
					off_document->_cell_face_indices.resize(1);
					off_document->_cell_colors.resize(1);
				} else if (item_count > 2) {
					// OFF stores sizes in the order 0D, 2D, 1D, 3D, 4D, ... :(
					off_document->_edge_count = items[2].to_int();
					// Copy 2D to slot index 2 to change the order to 0D, 2D, 2D, 3D, 4D, ... :)
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
				off_document->_vertex_positions.set(current_vertex_index, vertex);
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

String OFFDocumentND::_vector_n_to_off_nd(const VectorN &p_vector) {
	ERR_FAIL_COND_V(p_vector.size() == 0, String());
	String ret = String::num(p_vector[0]);
	if (p_vector[0] == (real_t)(int64_t)p_vector[0]) {
		ret += String(".0");
	}
	for (int i = 1; i < p_vector.size(); i++) {
		ret += " " + String::num(p_vector[i]);
		if (p_vector[i] == (real_t)(int64_t)p_vector[i]) {
			ret += String(".0");
		}
	}
	return ret;
}

String OFFDocumentND::_color_to_off_string_nd(const Color &p_color) {
	return " " + String::num_int64(p_color.r * 255.0f) + " " + String::num_int64(p_color.g * 255.0f) + " " + String::num_int64(p_color.b * 255.0f);
}

String OFFDocumentND::_cell_to_off_string_nd(const PackedInt32Array &p_face) {
	String ret = String::num_int64(p_face.size());
	for (int i = 0; i < p_face.size(); i++) {
		ret += String(" ") + String::num_int64(p_face[i]);
	}
	return ret;
}

String OFFDocumentND::_cell_dimension_index_to_off_comment(const int p_dimension) {
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

PackedByteArray OFFDocumentND::export_save_to_byte_array() {
	const String file_contents = _export_save_to_string();
	return file_contents.to_utf8_buffer();
}

void OFFDocumentND::export_save_to_file(const String &p_path) {
	const String base_dir = ProjectSettings::get_singleton()->globalize_path(p_path.get_base_dir());
	if (!base_dir.is_empty()) {
		Error dir_err = DirAccess::make_dir_recursive_absolute(base_dir);
		ERR_FAIL_COND_MSG(dir_err != OK, "Error: Failed to create base directory for export file: " + base_dir);
	}
#if GDEXTENSION
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(file.is_null(), "Error: Could not open file " + p_path + " for writing.");
#elif GODOT_MODULE
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_MSG(err != OK, "Error: Could not open file " + p_path + " for writing.");
#endif
	const String file_contents = _export_save_to_string();
	file->store_string(file_contents);
	file->close();
}

String OFFDocumentND::_export_save_to_string() {
	if (_edge_count == 0) {
		_count_unique_edges_from_faces();
	}
	PackedStringArray lines;
	if (_dimension == 3) {
		lines.append("OFF");
	} else {
		lines.append(String::num_int64(_dimension) + "OFF");
	}
	String size_line = String::num_int64(_vertex_positions.size());
	if (_cell_face_indices.size() > 0) {
		size_line += " " + String::num_int64(_cell_face_indices[0].size()) + " " + String::num_int64(_edge_count);
		for (int i = 1; i < _cell_face_indices.size(); i++) {
			Vector<PackedInt32Array> dim_cell_face_indices = _cell_face_indices[i];
			size_line += " " + String::num_int64(dim_cell_face_indices.size());
		}
	}
	lines.append(size_line);
	lines.append("\n# Vertices");
	for (int i = 0; i < _vertex_positions.size(); i++) {
		lines.append(_vector_n_to_off_nd(_vertex_positions[i]));
	}
	for (int dim = 0; dim < _cell_face_indices.size(); dim++) {
		Vector<PackedInt32Array> dim_cell_face_indices = _cell_face_indices[dim];
		ERR_FAIL_COND_V(dim_cell_face_indices.size() == 0, String());
		PackedColorArray dim_cell_colors = _cell_colors[dim];
		lines.append(_cell_dimension_index_to_off_comment(dim));
		for (int i = 0; i < dim_cell_face_indices.size(); i++) {
			String cell_str = _cell_to_off_string_nd(dim_cell_face_indices[i]);
			if (i < dim_cell_colors.size() && !dim_cell_colors[i].is_equal_approx(Color(1.0f, 1.0f, 1.0f))) {
				cell_str += _color_to_off_string_nd(dim_cell_colors[i]);
			}
			lines.append(cell_str);
		}
	}
	return String("\n").join(lines) + String("\n");
}

void OFFDocumentND::set_dimension(const int p_dimension) {
	ERR_FAIL_COND(p_dimension < 0);
	_dimension = p_dimension;
	// Resize all vectors to match the new dimension (using with_dimension to ensure correct initialization).
	for (int64_t i = 0; i < _vertex_positions.size(); i++) {
		_vertex_positions.set(i, VectorND::with_dimension(_vertex_positions[i], p_dimension));
	}
}

void OFFDocumentND::_bind_methods() {
	ClassDB::bind_static_method("OFFDocumentND", D_METHOD("export_convert_mesh_nd", "mesh", "deduplicate_faces"), &OFFDocumentND::export_convert_mesh_nd, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("export_save_to_byte_array"), &OFFDocumentND::export_save_to_byte_array);
	ClassDB::bind_method(D_METHOD("export_save_to_file", "path"), &OFFDocumentND::export_save_to_file);

	ClassDB::bind_static_method("OFFDocumentND", D_METHOD("import_load_from_byte_array", "data"), &OFFDocumentND::import_load_from_byte_array);
	ClassDB::bind_static_method("OFFDocumentND", D_METHOD("import_load_from_file", "path"), &OFFDocumentND::import_load_from_file);
	ClassDB::bind_method(D_METHOD("import_generate_array_cell_mesh_nd"), &OFFDocumentND::import_generate_array_cell_mesh_nd);
	ClassDB::bind_method(D_METHOD("import_generate_wire_mesh_nd", "deduplicate_edges"), &OFFDocumentND::import_generate_wire_mesh_nd, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("import_generate_node", "deduplicate_edges"), &OFFDocumentND::import_generate_node, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("get_dimension"), &OFFDocumentND::get_dimension);
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &OFFDocumentND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension"), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_edge_count"), &OFFDocumentND::get_edge_count);
	ClassDB::bind_method(D_METHOD("set_edge_count", "edge_count"), &OFFDocumentND::set_edge_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "edge_count"), "set_edge_count", "get_edge_count");
}
