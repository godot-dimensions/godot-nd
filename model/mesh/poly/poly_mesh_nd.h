#pragma once

#include "../cell/cell_material_nd.h"
#include "../cell/cell_mesh_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#elif GODOT_MODULE
class ArrayMesh;
#endif

class ArrayPolyMeshND;

class PolyMeshND : public CellMeshND {
	GDCLASS(PolyMeshND, CellMeshND);

	PackedInt32Array _simplex_cell_vertex_indices_cache;
	PackedInt32Array _simplex_cell_source_poly_cells;
	PackedInt32Array _simplex_cell_normal_indices_cache;
	PackedInt32Array _simplex_cell_texture_map_indices_cache;
	Vector<VectorN> _simplex_cell_vertex_positions_cache; // Superset of the polytope cell vertices.
	Vector<VectorN> _simplex_cell_boundary_normals_cache;
	Vector<VectorN> _simplex_cell_normal_values_cache; // Superset of the polytope cell normals.
	Vector<VectorM> _simplex_cell_texture_map_values_cache; // Superset of the polytope cell texture maps.
	bool _is_poly_mesh_data_valid = false;

	static VectorM _average_vector_m(const Vector<VectorM> &p_vector_m_array);
	static inline bool _do_edges_have_common_vertex(const int32_t p_edge1_a, const int32_t p_edge1_b, const int32_t p_edge2_a, const int32_t p_edge2_b) {
		return (p_edge1_a == p_edge2_a) || (p_edge1_a == p_edge2_b) || (p_edge1_b == p_edge2_a) || (p_edge1_b == p_edge2_b);
	}

	static int32_t _get_lowest_vertex_of_cell_excluding(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_cell_dim_index, const int64_t p_which_cell, const HashSet<int32_t> &p_excluded_vertices);
	static PackedInt32Array _get_canonical_span_vertex_index_sequence(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_indices_dim_index, const int64_t p_which_cell);
	static PackedInt32Array _get_cell_face_4_vertex_index_sequence(const PackedInt32Array &p_all_edge_indices, const PackedInt32Array &p_face1_edge_indices, const PackedInt32Array &p_face2_edge_indices);
	static PackedInt32Array _get_face_edge_3_vertex_index_sequence(const int32_t p_edge1_a, const int32_t p_edge1_b, const int32_t p_edge2_a, const int32_t p_edge2_b);
	static PackedInt32Array _get_edges_of_poly_cell(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const int64_t p_cell_dim_index, const int64_t p_which_cell);
	static PackedInt32Array _get_vertex_indices_of_face(const PackedInt32Array &p_all_edge_indices, const PackedInt32Array &p_face_edge_indices);
	static bool _does_pivot_conflict_with_descendants(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const Vector<Vector<PackedInt32Array>> &p_level_cell_vertices, const Vector<PackedInt32Array> &p_level_pivots, const int64_t p_cell_dim_index, const int64_t p_which_cell, const int32_t p_pivot_vertex);
	static void _impose_pivot_on_descendants(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const Vector<Vector<PackedInt32Array>> &p_level_cell_vertices, Vector<PackedInt32Array> &r_level_pivots, const int64_t p_cell_dim_index, const int64_t p_which_cell, const int32_t p_pivot_vertex);
	void _decompose_boundary_cells_into_simplexes();
	bool _infer_vertex_texcoord_from_cell_pivot_override(const PackedInt32Array &p_source_cell_vertices, const Vector<VectorM> &p_source_cell_texture_map, const int32_t p_target_vertex, VectorM &r_texcoord);

protected:
	// A decent margin under MeshND::MAX_VERTICES to avoid overflows.
	static constexpr int64_t MAX_POLY_VERTICES = MeshND::MAX_VERTICES * 3 / 4;

	static void _bind_methods();
	virtual bool validate_mesh_data() override;
	virtual bool _validate_poly_mesh_data_only();

	// Protected helper functions used by both PolyMeshND and ArrayPolyMeshND, and the shape generators.
	static PackedInt32Array _get_vertex_indices_of_poly_cell(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_cell_dim_index, const int64_t p_which_cell, const bool p_start_with_canonical_span);
	// The boundary cells of an N-dimensional mesh are (N-1)-dimensional, at poly cell dim index N - 3.
	int64_t _get_boundary_poly_dim_index() { return int64_t(get_dimension()) - 3; }
	// Data binding keys: X is the geometry dimension, Y is the decomposition dimension.
	// Normals and texture maps usually refer to the (N-1)-dimensional boundary cells.
	Vector2i _get_per_cell_key() {
		const int dim = get_dimension();
		return Vector2i(dim - 1, dim - 1);
	}
	Vector2i _get_cell_to_vert_key() {
		const int dim = get_dimension();
		return Vector2i(dim - 1, 0);
	}
	Vector<PackedInt32Array> _get_vertex_indices_of_boundary_cells(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const int64_t p_boundary_dim_index, const bool p_start_with_canonical_span);
	Vector<VectorN> _compute_boundary_normals_based_on_cell_orientation(const Vector<PackedInt32Array> &p_boundary_cell_vertex_indices, const bool p_keep_existing);
	// Solves for the coefficients that express the target vector as a linear combination of the
	// given linearly independent span vectors, ignoring any component outside of the span.
	static bool _solve_coordinates_in_span(const Vector<VectorN> &p_span_vectors, const VectorN &p_target, VectorN &r_coordinates);
	// Picks a linearly independent spanning subset of the given points relative to the first, filling
	// the indices of the picked points. Returns the amount of independent directions found.
	static int64_t _pick_spanning_vertices(const Vector<VectorN> &p_all_vertices, const PackedInt32Array &p_candidate_vertex_indices, const int64_t p_max_directions, PackedInt32Array &r_picked_vertex_indices);
	// Swaps the first two members of any boundary cell whose orientation-derived normal disagrees with the given normal.
	static void _orient_cells_to_match_normals(Vector<Vector<PackedInt32Array>> &r_poly_cell_indices, const PackedInt32Array &p_all_edge_indices, const Vector<VectorN> &p_vertices, const Vector<VectorN> &p_target_normals, const int64_t p_cell_dim_index);

public:
	// Flips the orientation of a poly cell's member list. Faces are flipped by reversing the
	// whole edge loop to keep it in a connected loop order, while higher-dimensional cells
	// are flipped by swapping the first two members, since the rest of their order is free.
	static void flip_poly_cell_orientation(PackedInt32Array &r_cell_members, const int64_t p_cell_dim_index);

	bool is_poly_mesh_data_valid();
	void reset_poly_mesh_data_validation();

	Vector<PackedInt32Array> get_all_face_vertex_indices();
	TypedArray<PackedInt32Array> get_all_face_vertex_indices_bind();
	Vector<PackedInt32Array> get_all_boundary_cell_vertex_indices(const bool p_start_with_canonical_span);
	TypedArray<PackedInt32Array> get_all_boundary_cell_vertex_indices_bind(const bool p_start_with_canonical_span);
	Vector<PackedInt32Array> get_all_poly_cell_vertex_indices(const int p_cell_dimension, const bool p_start_with_canonical_span);
	TypedArray<PackedInt32Array> get_all_poly_cell_vertex_indices_bind(const int p_cell_dimension, const bool p_start_with_canonical_span);
	Vector<PackedInt32Array> get_all_poly_cell_poly_indices(const int p_cell_dimension, const int p_decomposition_dimension);
	TypedArray<PackedInt32Array> get_all_poly_cell_poly_indices_bind(const int p_cell_dimension, const int p_decomposition_dimension);
	void poly_mesh_clear_cache(const bool p_normals_only = false);
	Ref<ArrayPolyMeshND> to_array_poly_mesh();

	int32_t get_source_poly_cell_for_simplex_cell(const int32_t p_simplex_cell_index) const;

	// Reads the poly cell vertices instead of MeshND's simplex vertices, because reading the
	// simplex vertices would trigger the simplex decomposition, which needs the dimension itself.
	virtual int get_dimension() override;

	virtual Vector<Vector<PackedInt32Array>> get_poly_cell_indices();
	virtual Vector<VectorN> get_poly_cell_vertex_positions();
	virtual Vector<VectorN> get_poly_cell_normal_values();
	virtual Vector<VectorM> get_poly_cell_texture_map_values();
	virtual Vector<VectorN> get_poly_cell_boundary_normals();
	virtual PackedInt32Array get_poly_cell_boundary_pivot_overrides();
	virtual Vector<PackedInt32Array> get_poly_cell_normal_indices();
	virtual Vector<PackedInt32Array> get_poly_cell_texture_map_indices();
	virtual HashSet<int32_t> get_seam_indices() const { return HashSet<int32_t>(); }

	TypedArray<Array> get_poly_cell_indices_bind();
	TypedArray<VectorN> get_poly_cell_vertex_positions_bind();
	TypedArray<VectorN> get_poly_cell_normal_values_bind();
	TypedArray<VectorM> get_poly_cell_texture_map_values_bind();
	TypedArray<VectorN> get_poly_cell_boundary_normals_bind();
	TypedArray<PackedInt32Array> get_poly_cell_normal_indices_bind();
	TypedArray<PackedInt32Array> get_poly_cell_texture_map_indices_bind();

	virtual PackedInt32Array get_simplex_cell_vertex_indices() override;
	virtual PackedInt32Array get_simplex_cell_normal_indices() override;
	virtual PackedInt32Array get_simplex_cell_texture_map_indices() override;
	virtual Vector<VectorN> get_simplex_cell_boundary_normals() override;
	virtual Vector<VectorN> get_vertex_positions() override;
	virtual Vector<VectorN> get_normal_values() override;
	virtual Vector<VectorM> get_texture_map_values() override;

	GDVIRTUAL0R(TypedArray<Array>, _get_poly_cell_indices);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_poly_cell_vertex_positions);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_poly_cell_normal_values);
	GDVIRTUAL0R(TypedArray<VectorM>, _get_poly_cell_texture_map_values);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_poly_cell_boundary_normals);
	GDVIRTUAL0R(PackedInt32Array, _get_poly_cell_boundary_pivot_overrides);
	GDVIRTUAL0R(TypedArray<PackedInt32Array>, _get_poly_cell_normal_indices);
	GDVIRTUAL0R(TypedArray<PackedInt32Array>, _get_poly_cell_texture_map_indices);
};
