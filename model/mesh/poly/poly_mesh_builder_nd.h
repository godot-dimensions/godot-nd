#pragma once

#include "array_poly_mesh_nd.h"

// Static helper class for ND poly mesh building functions.
class PolyMeshBuilderND : public Object {
	GDCLASS(PolyMeshBuilderND, Object);

	// These helper types and functions are for `subdivide_elements`.
	enum SubdivisionCellClass {
		SUBDIV_CLASS_UNKNOWN = 0,
		SUBDIV_CLASS_SIMPLEX,
		SUBDIV_CLASS_BOX,
		SUBDIV_CLASS_ORTHOPLEX,
		SUBDIV_CLASS_OTHER,
	};
	// The refinement of one subdivided element, with lookup tables for the parent rules.
	struct SubdivisionRefined {
		PackedInt32Array all_pieces;
		PackedInt32Array central_pieces;
		HashMap<int32_t, int32_t> corner_piece_by_vertex;
		HashMap<int32_t, int32_t> cut_piece_by_vertex;
		// Internal elements of box-style refinements, keyed by ((sub-dim << 32) | old sub-element index).
		HashMap<int64_t, int32_t> internal_by_subelement;
		// Cone elements toward the center vertex, keyed by (((new element level + 2) << 32) | new element index).
		HashMap<int64_t, int32_t> cone_by_element;
		int32_t center_vertex = -1;
	};
	struct SubdivisionContext {
		int64_t dimension = 0;
		// Old mesh data.
		Vector<VectorN> old_vertices;
		PackedInt32Array old_edges;
		Vector<Vector<PackedInt32Array>> old_levels;
		Vector<Vector<PackedInt32Array>> old_level_vertices;
		// Marked elements, which get fully subdivided.
		HashSet<int32_t> marked_edges;
		Vector<HashSet<int32_t>> marked_levels;
		// New mesh data being built.
		Vector<VectorN> new_vertices;
		Vector<PackedInt32Array> new_vertex_sources;
		PackedInt32Array new_edges;
		PackedInt32Array new_edge_parents;
		HashMap<int64_t, int32_t> new_edge_map;
		Vector<Vector<PackedInt32Array>> new_levels;
		Vector<PackedInt32Array> new_level_parents;
		// Remaps from old element indices to new element indices, -1 for subdivided elements.
		PackedInt32Array edge_remap;
		PackedInt32Array edge_mid_vertex;
		Vector<PackedInt32Array> edge_pieces;
		Vector<PackedInt32Array> level_remap;
		Vector<HashMap<int32_t, SubdivisionRefined>> refined_levels;
		Vector<PackedInt32Array> classification;
		// Unique negative parent ids for new elements internal to a subdivided element.
		int32_t internal_parent_counter = -2;
	};
	static int32_t _subdivide_append_vertex(SubdivisionContext &r_ctx, const VectorN &p_position, const PackedInt32Array &p_source_vertices);
	static int32_t _subdivide_get_or_create_edge(SubdivisionContext &r_ctx, const int32_t p_vertex_a, const int32_t p_vertex_b, const int32_t p_parent);
	static int32_t _subdivide_append_cell(SubdivisionContext &r_ctx, const int64_t p_level, const PackedInt32Array &p_members, const int32_t p_parent);
	static int32_t _subdivide_get_edge_piece_at(const SubdivisionContext &p_ctx, const int32_t p_old_edge, const int32_t p_old_vertex);
	static bool _subdivide_old_element_has_vertex(const SubdivisionContext &p_ctx, const int64_t p_element_dim, const int32_t p_element_index, const int32_t p_vertex);
	static bool _subdivide_old_element_contains(const SubdivisionContext &p_ctx, const int64_t p_outer_dim, const int32_t p_outer_index, const int64_t p_inner_dim, const int32_t p_inner_index);
	static VectorN _subdivide_old_element_center(const SubdivisionContext &p_ctx, const int64_t p_element_dim, const int32_t p_element_index, PackedInt32Array *r_source_vertices);
	static int32_t _subdivide_classify(SubdivisionContext &r_ctx, const int64_t p_level, const int32_t p_index);
	static bool _subdivide_new_elements_touch(const SubdivisionContext &p_ctx, const int64_t p_level, const int32_t p_a, const int32_t p_b);
	static void _subdivide_repair_first_two(SubdivisionContext &r_ctx, const int64_t p_level, PackedInt32Array &r_members);
	static void _subdivide_order_face_loop(const SubdivisionContext &p_ctx, PackedInt32Array &r_members);
	static int32_t _subdivide_cone(SubdivisionContext &r_ctx, SubdivisionRefined &r_refined, const int64_t p_element_level, const int32_t p_element_index);
	static void _subdivide_collect_closure(const SubdivisionContext &p_ctx, const int64_t p_level, const int32_t p_index, Vector<PackedInt32Array> &r_closure_by_dim);
	static int32_t _subdivide_internal_element(SubdivisionContext &r_ctx, const int64_t p_level, const int32_t p_cell_index, const Vector<PackedInt32Array> &p_closure_by_dim, const int64_t p_sub_dim, const int32_t p_sub_index);
	static PackedInt32Array _subdivide_face_vertex_walk(const SubdivisionContext &p_ctx, const int32_t p_face_index);
	static void _subdivide_refine_face(SubdivisionContext &r_ctx, const int32_t p_face_index);
	static void _subdivide_refine_cell(SubdivisionContext &r_ctx, const int64_t p_level, const int32_t p_index);
	static PackedInt32Array _subdivide_conform_face(SubdivisionContext &r_ctx, const int32_t p_face_index);

protected:
	static PolyMeshBuilderND *singleton;
	static void _bind_methods();

public:
	// These functions create new meshes from the given data.
	static Ref<ArrayPolyMeshND> convert_mesh_3d_to_nd_faces_only(const Ref<ArrayMesh> &p_mesh_3d, const int p_which_surface = -1, const bool p_deduplicate = true);
	static Ref<ArrayPolyMeshND> extrude_linear(const Ref<ArrayPolyMeshND> &p_input_mesh, const VectorN &p_extrusion_vector = VectorN());

	// In-place adjustments to the given mesh.
	static void make_boundary_normals_topologically_consistent(const Ref<ArrayPolyMeshND> &p_mesh_nd, const PackedInt32Array &p_authoritative);
	static PackedInt32Array subdivide_elements(const Ref<ArrayPolyMeshND> &p_input_mesh, const int p_dimension, const PackedInt32Array &p_elements = PackedInt32Array());

	static PolyMeshBuilderND *get_singleton() { return singleton; }
	PolyMeshBuilderND() { singleton = this; }
	~PolyMeshBuilderND() { singleton = nullptr; }
};
