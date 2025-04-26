#pragma once

#include "../../godot_nd_defines.h"

#include "../cell/array_cell_mesh_nd.h"
#include "../wire/array_wire_mesh_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#elif GODOT_MODULE
#include "core/io/resource.h"
#include "scene/resources/mesh.h"
#endif

class OFFDocumentND : public Resource {
	GDCLASS(OFFDocumentND, Resource);

	Vector<PackedColorArray> _cell_colors;
	Vector<Vector<PackedInt32Array>> _cell_face_indices;
	Vector<VectorN> _vertices;
	int _dimension = 3;
	int _edge_count = 0;
	bool _has_any_cell_colors = false;

	void _count_unique_edges_from_faces();
	int _find_or_insert_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices = true);
	Vector<Vector<PackedInt32Array>> _calculate_cell_vertex_indices();
	Vector<Vector<PackedInt32Array>> _calculate_simplex_vertex_indices(const Vector<Vector<PackedInt32Array>> &p_cell_vertex_indices);

protected:
	static void _bind_methods();

public:
	Vector<PackedColorArray> get_cell_colors() const { return _cell_colors; }
	void set_cell_colors(const Vector<PackedColorArray> &p_cell_colors) { _cell_colors = p_cell_colors; }

	Vector<Vector<PackedInt32Array>> get_cell_face_indices() const { return _cell_face_indices; }
	void set_cell_face_indices(const Vector<Vector<PackedInt32Array>> &p_cell_face_indices) { _cell_face_indices = p_cell_face_indices; }

	int get_edge_count() const { return _edge_count; }
	void set_edge_count(const int p_edge_count) { _edge_count = p_edge_count; }

	Vector<VectorN> get_vertices() const { return _vertices; }
	void set_vertices(const Vector<VectorN> &p_vertices) { _vertices = p_vertices; }

	Ref<ArrayCellMeshND> generate_array_cell_mesh_nd();
	Ref<ArrayWireMeshND> generate_wire_mesh_nd(const bool p_deduplicate_edges = true);
	Node *generate_node(const bool p_deduplicate_edges = true);

	static Ref<OFFDocumentND> load_from_file(const String &p_path);
	void save_to_file(const String &p_path);
};
