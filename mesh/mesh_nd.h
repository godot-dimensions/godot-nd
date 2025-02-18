#pragma once

#include "material_nd.h"

#if GDEXTENSION
#include <godot_cpp/variant/typed_array.hpp>
#elif GODOT_MODULE
#include "core/variant/typed_array.h"
#endif

class ArrayWireMeshND;
class WireMeshND;

class MeshND : public Resource {
	GDCLASS(MeshND, Resource);

	Ref<MaterialND> _material;
	bool _is_mesh_data_valid = false;

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data();

public:
	static PackedInt32Array deduplicate_edge_indices(const PackedInt32Array &p_items);
	bool has_edge_indices(int p_first, int p_second);

	bool is_mesh_data_valid();
	void reset_mesh_data_validation();

	Ref<ArrayWireMeshND> to_array_wire_mesh();
	virtual Ref<WireMeshND> to_wire_mesh();

	Ref<MaterialND> get_material() const;
	void set_material(const Ref<MaterialND> &p_material);

	virtual PackedInt32Array get_edge_indices();
	virtual Vector<VectorN> get_edge_positions();
	TypedArray<VectorN> get_edge_positions_bind();
	virtual Vector<VectorN> get_vertices();
	TypedArray<VectorN> get_vertices_bind();

	GDVIRTUAL0R(PackedInt32Array, _get_edge_indices);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_vertices);
	GDVIRTUAL0R(bool, _validate_mesh_data);
};
