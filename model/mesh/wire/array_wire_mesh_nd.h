#pragma once

#include "../../../math/transform_nd.h"
#include "wire_mesh_nd.h"

class ArrayWireMeshND : public WireMeshND {
	GDCLASS(ArrayWireMeshND, WireMeshND);

	PackedInt32Array _edge_vertex_indices;
	Vector<VectorN> _vertex_positions;

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override;

public:
	void append_edge_points(const VectorN &p_point_a, const VectorN &p_point_b, const bool p_deduplicate_vertices = true);
	void append_edge_indices(int p_index_a, int p_index_b);
	int append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices = true);
	PackedInt32Array append_vertices(const Vector<VectorN> &p_vertices, const bool p_deduplicate_vertices = true);
	PackedInt32Array append_vertices_bind(const TypedArray<VectorN> &p_vertices, const bool p_deduplicate_vertices = true);

	void deduplicate_all_elements();
	void transform_vertices(const Ref<TransformND> &p_transform);
	void merge_with(const Ref<ArrayWireMeshND> &p_array_wire_mesh_nd, const Ref<TransformND> &p_transform);

	virtual PackedInt32Array get_edge_indices() override;
	void set_edge_indices(const PackedInt32Array &p_edge_indices);

	virtual Vector<VectorN> get_vertex_positions() override;
	void set_vertex_positions(const Vector<VectorN> &p_vertex_positions);
	void set_vertex_positions_bind(const TypedArray<VectorN> &p_vertex_positions);
	void set_dimension(int p_dimension);
};
