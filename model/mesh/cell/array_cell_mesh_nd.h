#pragma once

#include "../../../math/transform_nd.h"
#include "cell_mesh_nd.h"

class ArrayCellMeshND : public CellMeshND {
	GDCLASS(ArrayCellMeshND, CellMeshND);

	PackedInt32Array _simplex_cell_indices;
	Vector<VectorN> _simplex_cell_boundary_normals;
	Vector<VectorN> _simplex_cell_vertex_normals;
	Vector<VectorN> _vertices;

	void _clear_cache();

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override;

public:
	int append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices = true);
	PackedInt32Array append_vertices(const Vector<VectorN> &p_vertices, const bool p_deduplicate_vertices = true);

	void merge_with(const Ref<ArrayCellMeshND> &p_other, const Ref<TransformND> &p_transform);

	virtual PackedInt32Array get_simplex_cell_indices() override;
	void set_simplex_cell_indices(const PackedInt32Array &p_simplex_cell_indices);

	virtual Vector<VectorN> get_simplex_cell_boundary_normals() override;
	void set_cell_boundary_normals(const Vector<VectorN> &p_simplex_cell_boundary_normals);
	void set_simplex_cell_boundary_normals_bind(const TypedArray<VectorN> &p_simplex_cell_boundary_normals);

	virtual Vector<VectorN> get_simplex_cell_vertex_normals() override;
	void set_simplex_cell_vertex_normals(const Vector<VectorN> &p_simplex_cell_vertex_normals);
	void set_simplex_cell_vertex_normals_bind(const TypedArray<VectorN> &p_simplex_cell_vertex_normals);

	virtual Vector<VectorN> get_vertices() override;
	void set_vertices(const Vector<VectorN> &p_vertices);
	void set_vertices_bind(const TypedArray<VectorN> &p_vertices);
	void set_dimension(int p_dimension);
};
