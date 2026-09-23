#pragma once

#include "../../godot_nd_defines.h"
#include "../mesh/cell/array_cell_mesh_nd.h"
#include "../mesh/poly/array_poly_mesh_nd.h"
#include "../mesh/wire/array_wire_mesh_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#elif GODOT_MODULE
#include "core/io/resource.h"
#include "core/templates/hash_map.h"
#include "scene/resources/mesh.h"
#endif

class OFFDocumentND : public Resource {
	GDCLASS(OFFDocumentND, Resource);

	Vector<PackedColorArray> _cell_colors;
	Vector<Vector<PackedInt32Array>> _cell_face_indices;
	Vector<VectorN> _vertex_positions;
	int _dimension = 3;
	int _edge_count = 0;
	bool _has_any_cell_colors = false;

	static String _vector_n_to_off_nd(const VectorN &p_vertex);
	static String _color_to_off_string_nd(const Color &p_color);
	static String _cell_to_off_string_nd(const PackedInt32Array &p_face);
	static String _cell_dimension_index_to_off_comment(const int p_dimension);

	// Hashes a sorted array of vertex indices, used to deduplicate simplex sub-cells during export.
	struct SortedIndicesHasher {
		static uint32_t hash(const PackedInt32Array &p_sorted_indices) {
			uint32_t h = hash_murmur3_one_32(uint32_t(p_sorted_indices.size()));
			for (int64_t i = 0; i < p_sorted_indices.size(); i++) {
				h = hash_murmur3_one_32(uint32_t(p_sorted_indices[i]), h);
			}
			return hash_fmix32(h);
		}
	};
	using SortedIndicesMap = HashMap<PackedInt32Array, int32_t, SortedIndicesHasher>;

	void _count_unique_edges_from_faces();
	int64_t _find_or_insert_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices = true);
	// Export helpers for simplex cell meshes, which need the OFF hierarchy of faces, cells, etc. built from each simplex's vertex indices.
	PackedInt32Array _insert_simplex_facets(const PackedInt32Array &p_simplex_vertex_indices, const bool p_deduplicate, Vector<SortedIndicesMap> &r_lookup_maps);
	int32_t _find_or_insert_simplex_cell(const PackedInt32Array &p_simplex_vertex_indices, const bool p_deduplicate, Vector<SortedIndicesMap> &r_lookup_maps);
	void _export_convert_cell_colors_nd(const Ref<CellMeshND> &p_mesh);
	void _export_orient_boundary_cells_nd(const Vector<VectorN> &p_desired_normals);
	Vector<Vector<PackedInt32Array>> _calculate_cell_vertex_indices();
	Vector<Vector<PackedInt32Array>> _calculate_simplex_vertex_indices(const Vector<Vector<PackedInt32Array>> &p_cell_vertex_indices);

	String _export_save_to_string();
	static Ref<OFFDocumentND> _import_read_from_raw_text(const String &p_raw_text, const String &p_path);

protected:
	static void _bind_methods();

public:
	static Ref<OFFDocumentND> export_convert_mesh_nd(const Ref<CellMeshND> &p_mesh, const bool p_deduplicate_faces = true);
	PackedByteArray export_save_to_byte_array();
	void export_save_to_file(const String &p_path);

	static Ref<OFFDocumentND> import_read_from_byte_array(const PackedByteArray &p_data);
	static Ref<OFFDocumentND> import_read_from_file(const String &p_path);
	Ref<ArrayCellMeshND> import_generate_array_cell_mesh_nd();
	Ref<ArrayPolyMeshND> import_generate_array_poly_mesh_nd();
	Ref<ArrayWireMeshND> import_generate_wire_mesh_nd(const bool p_deduplicate_edges = true);
	Node *import_generate_node(const bool p_deduplicate_edges = true);

	Vector<PackedColorArray> get_cell_colors() const { return _cell_colors; }
	void set_cell_colors(const Vector<PackedColorArray> &p_cell_colors) { _cell_colors = p_cell_colors; }

	Vector<Vector<PackedInt32Array>> get_cell_face_indices() const { return _cell_face_indices; }
	void set_cell_face_indices(const Vector<Vector<PackedInt32Array>> &p_cell_face_indices) { _cell_face_indices = p_cell_face_indices; }

	int get_dimension() const { return _dimension; }
	void set_dimension(const int p_dimension);

	int get_edge_count() const { return _edge_count; }
	void set_edge_count(const int p_edge_count) { _edge_count = p_edge_count; }

	Vector<VectorN> get_vertex_positions() const { return _vertex_positions; }
	void set_vertex_positions(const Vector<VectorN> &p_vertex_positions) { _vertex_positions = p_vertex_positions; }
};
