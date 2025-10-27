#pragma once

#include "../../../math/transform_nd.h"
#include "poly_mesh_nd.h"

class ArrayPolyMeshND : public PolyMeshND {
	GDCLASS(ArrayPolyMeshND, PolyMeshND);

public:
	enum ComputeNormalsMode {
		COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY,
		COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION,
		COMPUTE_NORMALS_MODE_FORCE_OUTWARD_OVERRIDE_CELL_ORIENTATION,
	};

	enum UnwrapTextureMapMode {
		UNWRAP_MODE_AUTOMATIC,
		UNWRAP_MODE_EACH_CELL_FILLS,
		UNWRAP_MODE_TILE_CELLS,
		UNWRAP_MODE_ISLANDS_FROM_SEAMS,
	};

private:
	// 0: Each 2D face is made up of 1D edge indices.
	// 1: Each 3D cell is made up of 2D face indices.
	// 2: Each 4D cell is made up of 3D cell indices.
	// And so on for higher dimensions.
	Vector<Vector<PackedInt32Array>> _poly_cell_indices;
	Vector<VectorN> _poly_cell_vertices;
	// Normals and UVW map always refer to boundary/surface cells (the N-1 dimensional cells).
	Vector<VectorN> _poly_cell_boundary_normals;
	Vector<Vector<VectorN>> _poly_cell_vertex_normals;
	Vector<Vector<VectorM>> _poly_cell_texture_map;
	// Seams always refer to borders between N-1 dimensional cells, so N-2 dimensional elements.
	HashSet<int32_t> _seam_indices;
	PackedInt32Array _edge_vertex_indices;

	static int64_t _append_edge_indices_internal(int32_t p_index_a, int32_t p_index_b, const bool p_deduplicate, PackedInt32Array &r_edge_indices);
	static int64_t _append_face_internal(const PackedInt32Array &p_face, Vector<PackedInt32Array> &r_all_face_edge_indices);
	static VectorN _compute_cell_normal(const PackedInt32Array &p_cell_first_face, const PackedInt32Array &p_cell_second_face, const PackedInt32Array &p_edge_vertex_indices, const Vector<VectorN> &p_vertices);
	static Vector<PackedInt32Array> _compose_triangles_into_faces(const Vector<VectorN> &p_vertices, const Vector<PackedInt32Array> &p_triangle_vertex_indices, PackedInt32Array &r_edge_indices);
	static PackedInt32Array _save_triangle_vertex_indices_as_faces_and_cell(const Vector<PackedInt32Array> &p_last_triangle_vertex_indices, const VectorN &p_last_simplex_normal, const Vector<VectorN> &p_vertices, Vector<PackedInt32Array> &r_all_face_edge_indices, PackedInt32Array &r_edge_vertex_indices);
	PackedInt32Array _get_cell_4_vertices_starting_from_face(const int64_t p_cell, const int64_t p_start_face) const;
	Vector<VectorN> _get_cell_world_span_seed(const int64_t p_which_cell, int32_t &p_pivot) const;
	void _transform_cell_to_texture_space(const TransformND &p_world_to_texcoord, const Vector<PackedInt32Array> &p_cell_vert, const int64_t p_cell_index, const int32_t p_pivot);
	Vector<PackedInt32Array> _get_face_to_cell_map() const;
	PackedInt32Array _collect_cells_in_island_internal(const int64_t p_start_cell, const Vector<PackedInt32Array> &p_face_to_cell_map);
	bool _unwrap_texture_map_island_cell(const PackedInt32Array &p_cells_in_island, const int64_t p_current_cell_index_index, const Vector<PackedInt32Array> &p_cell_vert);
	void _unwrap_texture_map_island_internal(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing);
	void _fit_island_texture_map_into_aabb(const PackedInt32Array &p_cells_in_island, const AABB &p_target_aabb);
	static VectorMi _tiles_for_island_count(const int32_t p_island_count, const int p_dimension);
	static inline int32_t _ceil_div(int32_t p_a, int32_t p_b) {
		return (p_a + p_b - 1) / p_b;
	}

protected:
	static void _bind_methods();

public:
	int64_t append_edge_points(const VectorN &p_point_a, const VectorN &p_point_b, const bool p_deduplicate = true);
	int64_t append_edge_indices(int32_t p_index_a, int32_t p_index_b, const bool p_deduplicate = true);
	int append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices = true);
	PackedInt32Array append_vertices(const Vector<VectorN> &p_vertices, const bool p_deduplicate_vertices = true);

	void calculate_boundary_normals(const ComputeNormalsMode p_mode, const bool p_keep_existing = false);
	void set_flat_shading_normals(const ComputeNormalsMode p_mode, const bool p_recalculate_boundary_normals = true);
	void calculate_seam_faces(const double p_angle_threshold_radians = Math_TAU / 8.0, const bool p_discard_seams_within_islands = false);
	PackedInt32Array collect_cells_in_island(const int64_t p_start_cell);
	Vector<PackedInt32Array> collect_all_islands();
	void unwrap_texture_map_island(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing = false);
	void unwrap_texture_map(const UnwrapTextureMapMode p_mode, const double p_padding = 0.0, const bool p_keep_existing = false);
	void transform_texture_map(const Ref<TransformND> &p_texture_transform);

	void merge_with(const Ref<PolyMeshND> &p_other, const Ref<TransformND> &p_transform);
	static Ref<ArrayPolyMeshND> reconstruct_from_cell_mesh(const Ref<CellMeshND> &p_cell_mesh);

	virtual PackedInt32Array get_edge_indices() override;
	void set_edge_vertex_indices(const PackedInt32Array &p_edge_indices);

	virtual Vector<Vector<PackedInt32Array>> get_poly_cell_indices() override;
	void set_poly_cell_indices(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices);
	void set_poly_cell_indices_bind(const TypedArray<Array> &p_poly_cell_indices);

	virtual Vector<VectorN> get_poly_cell_boundary_normals() override;
	void set_poly_cell_boundary_normals(const Vector<VectorN> &p_poly_cell_boundary_normals);

	virtual Vector<Vector<VectorN>> get_poly_cell_vertex_normals() override;
	void set_poly_cell_vertex_normals(const Vector<Vector<VectorN>> &p_poly_cell_vertex_normals);
	void set_poly_cell_vertex_normals_bind(const TypedArray<Vector<VectorN>> &p_poly_cell_vertex_normals);

	virtual Vector<Vector<VectorM>> get_poly_cell_texture_map() override;
	void set_poly_cell_texture_map(const Vector<Vector<VectorM>> &p_poly_cell_texture_map);
	void set_poly_cell_texture_map_bind(const TypedArray<Vector<VectorM>> &p_poly_cell_texture_map);

	virtual HashSet<int32_t> get_seam_indices() const override { return _seam_indices; }
	PackedInt32Array get_seam_indices_bind() const;
	void set_seam_indices(const HashSet<int32_t> &p_seam_indices);
	void set_seam_indices_bind(const PackedInt32Array &p_seam_indices);

	virtual Vector<VectorN> get_poly_cell_vertices() override;
	void set_poly_cell_vertices(const Vector<VectorN> &p_vertices);
};

VARIANT_ENUM_CAST(ArrayPolyMeshND::ComputeNormalsMode);
VARIANT_ENUM_CAST(ArrayPolyMeshND::UnwrapTextureMapMode);
