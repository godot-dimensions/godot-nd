#pragma once

#include "../../math/transform_nd.h"
#include "wire_mesh_nd.h"

class ArrayWireMeshND : public WireMeshND {
	GDCLASS(ArrayWireMeshND, WireMeshND);

	PackedInt32Array _edge_indices;
	Vector<VectorN> _vertices;

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override;

public:
	void append_edge_points(const VectorN &p_point_a, const VectorN &p_point_b, const bool p_deduplicate_vertices = true);
	void append_edge_indices(int p_index_a, int p_index_b);
	int append_vertex(const VectorN &p_vertex, const bool p_deduplicate_vertices = true);
	PackedInt32Array append_vertices(const Vector<VectorN> &p_vertices, const bool p_deduplicate_vertices = true);
	PackedInt32Array append_vertices_bind(const TypedArray<VectorN> &p_vertices, const bool p_deduplicate_vertices = true);

	void merge_with(const Ref<ArrayWireMeshND> &p_array_wire_mesh_nd, const Ref<TransformND> &p_transform);

	virtual PackedInt32Array get_edge_indices() override;
	void set_edge_indices(const PackedInt32Array &p_edge_indices);

	virtual Vector<VectorN> get_vertices() override;
	void set_vertices(const Vector<VectorN> &p_vertices);
	void set_vertices_bind(const TypedArray<VectorN> &p_vertices);
};
