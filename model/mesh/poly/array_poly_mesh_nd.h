#pragma once

#include "../../../math/transform_nd.h"
#include "poly_mesh_nd.h"

#if GODOT_HAS_TYPED_DICTIONARY
#include "core/variant/typed_dictionary.h"
#endif

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
		UNWRAP_MODE_EACH_ISLAND_FILLS,
		UNWRAP_MODE_TILE_ISLANDS,
	};

private:
	// 0: Each 2D face made up of 1D edge indices.
	// 1: Each 3D cell made up of 2D face indices.
	// 2: Each 4D cell made up of 3D cell indices.
	// And so on. For an N-dimensional mesh, index N - 3 holds the (N-1)-dimensional
	// boundary/surface cells, and index N - 2 optionally holds the N-dimensional
	// volumetric cells for encoding hypervolumes.
	Vector<Vector<PackedInt32Array>> _poly_cell_indices;
	Vector<VectorN> _poly_cell_vertex_positions;
	// The key's X is the geometry dimension, Y is the decomposition dimension.
	// See G4MFMeshSurfaceBindingGeometry4D for more details.
	HashMap<Vector2i, Vector<Vector<VectorN>>> _all_poly_cell_normals;
	HashMap<Vector2i, Vector<Vector<VectorM>>> _all_poly_cell_texture_maps;
	PackedInt32Array _poly_cell_boundary_pivot_overrides;
	// Seams always refer to the (N-2)-dimensional borders between (N-1)-dimensional
	// boundary cells. For 3D meshes those are edges, for 4D meshes those are 2D faces, etc.
	HashSet<int32_t> _seam_indices;
	PackedInt32Array _edge_vertex_indices;

	Vector<PackedInt32Array> _get_member_to_cell_map() const;
	int64_t _get_boundary_member_count() const;
	PackedInt32Array _collect_cells_in_island_internal(const int64_t p_start_cell, const Vector<PackedInt32Array> &p_member_to_cell_map);
	bool _validate_data_binding_shape_internal(const Vector2i p_key, const Vector<Vector<VectorN>> &p_binding, const int p_value_dimension, const String &p_binding_name) const;
	void _delete_data_binding_element_internal(const int32_t p_dimension, const int32_t p_index);
	void _delete_edge_internal(const int32_t p_index);
	void _delete_vertex_internal(const int32_t p_index);
	void _delete_poly_cell_element_internal(const int32_t p_dimension, const int32_t p_index);
	bool _unwrap_texture_map_island_cell(const PackedInt32Array &p_cells_in_island, const int64_t p_current_cell_index_index, const Vector<PackedInt32Array> &p_cell_vert);
	void _unwrap_texture_map_island_internal(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing);
	void _fit_island_texture_map_into_box(const PackedInt32Array &p_cells_in_island, const VectorM &p_target_position, const VectorM &p_target_size, const bool p_proportional);
	static VectorMi _tiles_for_island_count(const int32_t p_island_count, const int64_t p_texture_dimension);
	static inline int32_t _ceil_div(int32_t p_a, int32_t p_b) {
		return (p_a + p_b - 1) / p_b;
	}

protected:
	static void _bind_methods();
	virtual bool _validate_poly_mesh_data_only() override;

public:
	// Append and delete functions.
	int64_t append_edge_points(const VectorN &p_point_a, const VectorN &p_point_b, const bool p_deduplicate = true);
	int64_t append_edge_indices(int32_t p_index_a, int32_t p_index_b, const bool p_deduplicate = true);
	int64_t append_poly_cell(const int32_t p_dimension, const PackedInt32Array &p_cell, const bool p_deduplicate = true);
	int32_t append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices = true);
	PackedInt32Array append_vertices(const TypedArray<VectorN> &p_vertices, const bool p_deduplicate_vertices = true);
	void delete_poly_element(const int32_t p_dimension, const int32_t p_index);

	// Normal calculation functions.
	void calculate_boundary_normals(const ComputeNormalsMode p_mode = COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, const bool p_keep_existing = false);
	void set_flat_shading_normals(const ComputeNormalsMode p_mode = COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, const bool p_recalculate_boundary_normals = true);
	void set_smooth_shading_normals(const ComputeNormalsMode p_mode = COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY, const bool p_recalculate_boundary_normals = true);
	void make_double_sided(const bool p_idempotent = true);
	PackedInt32Array make_single_cell_from_all_cells(const int32_t p_cell_dimension) const;

	// Texture map and seam functions.
	void calculate_seams(const double p_angle_threshold_radians = Math_TAU / 8.0, const bool p_discard_seams_within_islands = false);
	PackedInt32Array collect_cells_in_island(const int64_t p_start_cell);
	Vector<PackedInt32Array> collect_all_islands();
	TypedArray<PackedInt32Array> collect_all_islands_bind();
	void unwrap_texture_map_island(const PackedInt32Array &p_cells_in_island, const bool p_keep_existing = false);
	void unwrap_texture_map(const UnwrapTextureMapMode p_mode, const double p_padding = 0.0, const bool p_proportional = true, const bool p_keep_existing = false);
	void transform_texture_map(const Ref<TransformND> &p_texture_transform);

	// Misc functions.
	void deduplicate_all_elements();
	void transform_vertices(const Ref<TransformND> &p_transform);
	void merge_with(const Ref<PolyMeshND> &p_other, const Ref<TransformND> &p_transform = Ref<TransformND>());

	// Getters and setters.
	HashMap<Vector2i, Vector<Vector<VectorN>>> get_all_poly_cell_normals() const;
	void set_all_poly_cell_normals(const HashMap<Vector2i, Vector<Vector<VectorN>>> &p_all_poly_cell_normals);
	HashMap<Vector2i, Vector<Vector<VectorM>>> get_all_poly_cell_texture_maps() const;
	void set_all_poly_cell_texture_maps(const HashMap<Vector2i, Vector<Vector<VectorM>>> &p_all_poly_cell_texture_maps);

#if GODOT_HAS_TYPED_DICTIONARY
	using PolyDataDictionary = TypedDictionary<Vector2i, Array>;
#else
	// Godot 4.3 and earlier do not have TypedDictionary, so use a plain Dictionary.
	// The dictionaries must still be bound so they are kept by duplication and serialization.
	using PolyDataDictionary = Dictionary;
#endif // GODOT_HAS_TYPED_DICTIONARY
	PolyDataDictionary get_all_poly_cell_normals_bind() const;
	void set_all_poly_cell_normals_bind(const PolyDataDictionary &p_all_poly_cell_normals);
	PolyDataDictionary get_all_poly_cell_texture_maps_bind() const;
	void set_all_poly_cell_texture_maps_bind(const PolyDataDictionary &p_all_poly_cell_texture_maps);

	virtual PackedInt32Array get_edge_indices() override { return _edge_vertex_indices; }
	void set_edge_vertex_indices(const PackedInt32Array &p_edge_indices);

	virtual Vector<Vector<PackedInt32Array>> get_poly_cell_indices() override { return _poly_cell_indices; }
	void set_poly_cell_indices(const Vector<Vector<PackedInt32Array>> &p_poly_cell_indices);
	void set_poly_cell_indices_bind(const TypedArray<Array> &p_poly_cell_indices);

	virtual Vector<VectorN> get_poly_cell_boundary_normals() override;
	void set_poly_cell_boundary_normals(const Vector<VectorN> &p_poly_cell_boundary_normals);
	void set_poly_cell_boundary_normals_bind(const TypedArray<VectorN> &p_poly_cell_boundary_normals);

	virtual PackedInt32Array get_poly_cell_boundary_pivot_overrides() override;
	void set_poly_cell_boundary_pivot_overrides(const PackedInt32Array &p_poly_cell_boundary_pivot_overrides);

	virtual Vector<Vector<VectorN>> get_poly_cell_vertex_normals() override;
	void set_poly_cell_vertex_normals(const Vector<Vector<VectorN>> &p_poly_cell_vertex_normals);
	void set_poly_cell_vertex_normals_bind(const TypedArray<Array> &p_poly_cell_vertex_normals);

	virtual Vector<Vector<VectorM>> get_poly_cell_texture_map() override;
	void set_poly_cell_texture_map(const Vector<Vector<VectorM>> &p_poly_cell_texture_map);
	void set_poly_cell_texture_map_bind(const TypedArray<Array> &p_poly_cell_texture_map);

	virtual HashSet<int32_t> get_seam_indices() const override { return HashSet<int32_t>(_seam_indices); }
	PackedInt32Array get_seam_indices_bind() const;
	void set_seam_indices(const HashSet<int32_t> &p_seam_indices);
	void set_seam_indices_bind(const PackedInt32Array &p_seam_indices);

	virtual Vector<VectorN> get_poly_cell_vertex_positions() override;
	void set_poly_cell_vertex_positions(const Vector<VectorN> &p_vertex_positions);
	void set_poly_cell_vertex_positions_bind(const TypedArray<VectorN> &p_vertex_positions);
};

VARIANT_ENUM_CAST(ArrayPolyMeshND::ComputeNormalsMode);
VARIANT_ENUM_CAST(ArrayPolyMeshND::UnwrapTextureMapMode);
