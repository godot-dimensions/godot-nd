#pragma once

#include "array_poly_mesh_nd.h"

// Static helper class for ND poly mesh building functions.
class PolyMeshBuilderND : public Object {
	GDCLASS(PolyMeshBuilderND, Object);

	// These helpers carry dense data bindings (normals and texture maps) across topology changes. A dense
	// element-to-vertex binding stores one array per element, with one value per corner of the element, in the
	// order that `PolyMeshND::get_all_poly_cell_vertex_indices(dim, false)` lists the element's vertices.
	// Normals and texture maps are both stored as `PackedFloat64Array` values, so one set of helpers serves both.
	enum CornerSampleMode {
		CORNER_SAMPLE_FIRST_FOUND, // Take the value from the first source element that has the vertex.
		CORNER_SAMPLE_AVERAGE, // Take the mean of the values from every source element that has the vertex.
	};
	// Looks up one vertex in the corner lists of the given source elements and reads its bound value.
	// Sources that are out of range, lack the vertex, or have fewer values than corners are skipped.
	// Returns the number of sources that had the vertex, and leaves `r_value` untouched when that is zero.
	static int64_t _find_corner_value(const int32_t p_vertex, const PackedInt32Array &p_source_elements, const Vector<PackedInt32Array> &p_source_corners, const Vector<Vector<PackedFloat64Array>> &p_source_values, const CornerSampleMode p_mode, PackedFloat64Array &r_value);
	// Samples a value for every corner of a new element from the corners of the given source elements.
	// Corners whose vertex is found in no source get `p_fallback`. With `p_normalize`, every sampled value is
	// normalized, which averaged normals need. When `p_derived_vertex_sources` is given, a corner vertex at or
	// above `p_first_derived_vertex` is a new vertex derived from the listed old vertices (such as an edge
	// midpoint), and gets the mean of whatever those old vertices sample to.
	// Returns the number of corners that received a sampled value rather than the fallback.
	static int64_t _sample_corner_values(const PackedInt32Array &p_new_corners, const PackedInt32Array &p_source_elements, const Vector<PackedInt32Array> &p_source_corners, const Vector<Vector<PackedFloat64Array>> &p_source_values, const CornerSampleMode p_mode, const PackedFloat64Array &p_fallback, Vector<PackedFloat64Array> &r_values, const bool p_normalize = false, const Vector<PackedInt32Array> *p_derived_vertex_sources = nullptr, const int64_t p_first_derived_vertex = INT64_MAX);

	// Rebinds dense corner values of the input mesh's boundary cells, copied twice into an extruded mesh, from the
	// input mesh's vertex order of those elements to the extruded mesh's vertex order. When an extrusion adds a
	// dimension, those elements are no longer boundary cells, so their vertices may be listed in a different order.
	// The first copy uses the input's vertex indices, and the second copy is offset by `p_input_vertex_count`.
	static void _rematch_extruded_corner_values(Vector<Vector<PackedFloat64Array>> &r_values, const Vector<PackedInt32Array> &p_input_corners, const Vector<PackedInt32Array> &p_output_corners, const int32_t p_input_vertex_count);

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
	// These are for `make_coplanar`.
	static PackedInt32Array _gather_element_vertices(const Vector<Vector<PackedInt32Array>> &p_levels, const PackedInt32Array &p_edge_vertex_indices, const int p_dimension, const int32_t p_index);
	static double _grow_flat_basis(const Vector<VectorN> &p_positions, const PackedInt32Array &p_vertices, const VectorN &p_origin, const int p_flat_dimension, Vector<VectorN> &r_basis);
	static double _flatness_deviation(const Vector<VectorN> &p_positions, const PackedInt32Array &p_vertices, const int p_flat_dimension);
	static bool _order_edges_into_loop(const PackedInt32Array &p_edges, const PackedInt32Array &p_edge_vertex_indices, PackedInt32Array &r_loop);
	static int64_t _make_element_coplanar(const Ref<ArrayPolyMeshND> &p_mesh_nd, const int p_dimension, const int32_t p_index, const double p_sin_tolerance, PackedInt32Array &r_pieces);
	// Coplanar face merging helpers. These intentionally only operate on 2D faces, such as the triangles of a 3D mesh.
	static PackedInt32Array _get_loop_face_vertices(const PackedInt32Array &p_face_edges, const PackedInt32Array &p_edge_vertex_indices);
	static bool _rotate_out_shared_edges(const PackedInt32Array &p_face_edges, const PackedInt32Array &p_other_face_edges, PackedInt32Array &r_remainder);
	static bool _get_face_plane_basis(const PackedInt32Array &p_vertex_loop, const Vector<VectorN> &p_positions, const double p_sin_tolerance, VectorN &r_origin, VectorN &r_basis_u, VectorN &r_basis_v);
	static bool _try_merge_coplanar_face_pair(const PackedInt32Array &p_face_a_edges, const PackedInt32Array &p_face_b_edges, const PackedInt32Array &p_edge_vertex_indices, const Vector<VectorN> &p_positions, const double p_sin_tolerance, PackedInt32Array &r_merged_edges);
	static bool _is_binding_affected_by_face_merge(const Vector2i &p_key);
	static Vector<PackedInt32Array> _remap_binding_after_face_merge(const Vector2i &p_key, const Vector<PackedInt32Array> &p_old_binding, const Vector<PackedInt32Array> &p_element_sources, const Vector<PackedInt32Array> &p_old_sub_elements, const Vector<PackedInt32Array> &p_new_sub_elements, const PackedInt32Array &p_sub_element_old_to_new);

protected:
	static PolyMeshBuilderND *singleton;
	static void _bind_methods();

public:
	// These functions create new meshes from the given data.
	static Ref<ArrayPolyMeshND> convert_mesh_3d_to_nd_faces_only(const Ref<Mesh> &p_mesh_3d, const int p_which_surface = -1, const bool p_deduplicate = true);
	static Ref<ArrayPolyMeshND> extrude_linear(const Ref<ArrayPolyMeshND> &p_input_mesh, const VectorN &p_extrusion_vector = VectorN());

	// In-place adjustments to the given mesh.
	static int64_t delete_interior(const Ref<ArrayPolyMeshND> &p_mesh_nd);
	static void make_boundary_normals_topologically_consistent(const Ref<ArrayPolyMeshND> &p_mesh_nd, const PackedInt32Array &p_authoritative);
	static int64_t make_coplanar(const Ref<ArrayPolyMeshND> &p_mesh_nd, const double p_angle_tolerance_radians = 0.001);
	static int64_t merge_coplanar_faces(const Ref<ArrayPolyMeshND> &p_mesh_nd, const double p_angle_tolerance_radians = 0.001);
	static PackedInt32Array subdivide_elements(const Ref<ArrayPolyMeshND> &p_input_mesh, const int p_dimension, const PackedInt32Array &p_elements = PackedInt32Array());

	static PolyMeshBuilderND *get_singleton() { return singleton; }
	PolyMeshBuilderND() { singleton = this; }
	~PolyMeshBuilderND() { singleton = nullptr; }
};
