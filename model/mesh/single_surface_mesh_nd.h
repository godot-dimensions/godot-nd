#pragma once

#include "material_nd.h"
#include "mesh_nd.h"

class ArrayWireMeshND;
class WireMeshND;

class SingleSurfaceMeshND : public MeshND {
	GDCLASS(SingleSurfaceMeshND, MeshND);

	Ref<MaterialND> _material;

protected:
	static void _bind_methods();

public:
	bool has_edge_indices(int p_first, int p_second);

	Ref<ArrayWireMeshND> to_array_wire_mesh();
	virtual Ref<WireMeshND> to_wire_mesh();

	virtual Ref<RectND> get_rect_bounds() override;
	virtual int get_dimension() override;

	Ref<MaterialND> get_material() const;
	void set_material(const Ref<MaterialND> &p_material);
	virtual Ref<MaterialND> get_fallback_material();
	virtual void validate_material_for_mesh(const Ref<MaterialND> &p_material) override;

	virtual PackedInt32Array get_edge_indices();
	virtual Vector<VectorN> get_edge_positions();
	virtual Vector<VectorN> get_vertex_positions();
	virtual Vector<VectorN> get_normal_values();
	virtual Vector<VectorM> get_texture_map_values();
	TypedArray<VectorN> get_edge_positions_bind();
	TypedArray<VectorN> get_vertex_positions_bind();
	TypedArray<VectorN> get_normal_values_bind();
	TypedArray<VectorM> get_texture_map_values_bind();

	GDVIRTUAL0R(PackedInt32Array, _get_edge_indices);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_vertex_positions);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_normal_values);
	GDVIRTUAL0R(TypedArray<VectorM>, _get_texture_map_values);

	GDVIRTUAL0R(Ref<MaterialND>, _get_fallback_material);
};
