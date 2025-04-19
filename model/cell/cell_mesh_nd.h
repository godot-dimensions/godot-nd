#pragma once

#include "../mesh_nd.h"

class ArrayCellMeshND;

class CellMeshND : public MeshND {
	GDCLASS(CellMeshND, MeshND);

	Vector<VectorN> _cell_positions_cache;

	static int64_t _binomial_coefficient(const int64_t n, const int64_t k);
	static void _generate_combinations_recursive(const PackedInt32Array &p_items, const int64_t p_count, const int64_t p_choose, const int64_t p_start, const int64_t p_depth, int &r_result_index, PackedInt32Array &r_current, Vector<PackedInt32Array> &r_result);
	static Vector<PackedInt32Array> _generate_combinations(const PackedInt32Array &p_items, int64_t p_choose);
	static Vector<PackedInt32Array> _determine_opposing_faces(const Vector<VectorN> &p_vertices, const PackedInt32Array &p_cell_indices_without_pivot, const int p_dimension, const int p_pivot_index, const Vector<VectorN> &p_cell_normals, Vector<VectorN> &r_out_normals);

protected:
	static void _bind_methods();
	PackedInt32Array _edge_indices_cache;
	Vector<VectorN> _edge_positions_cache;
	int _dimension = 0;

public:
	int get_dimension() const { return _dimension; }
	void set_dimension(const int p_dimension) { _dimension = p_dimension; }

	void cell_mesh_clear_cache();
	virtual void validate_material_for_mesh(const Ref<MaterialND> &p_material) override;
	Ref<ArrayCellMeshND> to_array_cell_mesh();

	virtual int get_cell_count();
	virtual int get_indices_per_cell() const;
	virtual PackedInt32Array get_cell_indices();
	virtual Vector<VectorN> get_cell_positions();
	virtual Vector<VectorN> get_cell_normals();

	static Vector<PackedInt32Array> decompose_polytope_cell_into_simplexes(const Vector<VectorN> &p_vertices, const PackedInt32Array &p_cell_indices, const int p_dimension, const int p_last_pivot, const Vector<VectorN> &p_cell_normals);

	static PackedInt32Array calculate_edge_indices_from_cell_indices(const PackedInt32Array &p_cell_indices, const int p_dimension, const bool p_deduplicate = true);
	virtual PackedInt32Array get_edge_indices() override;
	virtual Vector<VectorN> get_edge_positions() override;

	GDVIRTUAL0R(PackedInt32Array, _get_cell_indices);
};
