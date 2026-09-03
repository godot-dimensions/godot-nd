#pragma once

#include "../../math/rect_nd.h"
#include "material_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#elif GODOT_MODULE
#include "core/variant/typed_array.h"
#include "scene/resources/mesh.h"
#endif

class ArrayWireMeshND;
class WireMeshND;

class MeshND : public Resource {
	GDCLASS(MeshND, Resource);

	Ref<RectND> _rect_bounds;
	Ref<MaterialND> _material;
	bool _is_mesh_data_valid = false;
	bool _is_proxy_mesh_3d_dirty = true;
	bool _is_rect_bounds_dirty = true;

protected:
	// Slightly under the 32-bit integer limit to avoid overflows.
	static constexpr int64_t MAX_VERTICES = 2147483640;
	Ref<ArrayMesh> _proxy_mesh_3d;

	static void _bind_methods();
	virtual bool validate_mesh_data();
	// Call when the mesh is modified to indicate that
	// the 3D proxy mesh used for rendering needs to be updated.
	void mark_proxy_mesh_3d_dirty() { _is_proxy_mesh_3d_dirty = true; }
	void mark_mesh_bounds_and_proxy_mesh_3d_dirty() {
		_is_proxy_mesh_3d_dirty = true;
		_is_rect_bounds_dirty = true;
	}
	// Called when the proxy mesh is requested and the proxy mesh has been marked dirty.
	// Update the mesh referenced by _proxy_mesh_3d to match the current state of the mesh.
	virtual void update_proxy_mesh_3d();

public:
	static PackedInt32Array deduplicate_edge_indices(const PackedInt32Array &p_items);
	bool has_edge_indices(int p_first, int p_second);

	bool is_mesh_data_valid();
	void reset_mesh_data_validation();
	virtual void validate_material_for_mesh(const Ref<MaterialND> &p_material);

	Ref<ArrayWireMeshND> to_array_wire_mesh();
	virtual Ref<WireMeshND> to_wire_mesh();

	Ref<RectND> get_rect_bounds();
	// Returns the proxy 3D mesh associated with this ND mesh.
	Ref<ArrayMesh> get_proxy_mesh_3d();

	Ref<MaterialND> get_material() const;
	void set_material(const Ref<MaterialND> &p_material);

	virtual PackedInt32Array get_edge_indices();
	virtual Vector<VectorN> get_edge_positions();
	virtual Vector<VectorN> get_vertex_positions();
	virtual Vector<VectorN> get_normal_values();
	virtual Vector<VectorM> get_texture_map_values();
	TypedArray<VectorN> get_edge_positions_bind();
	TypedArray<VectorN> get_vertex_positions_bind();
	TypedArray<VectorN> get_normal_values_bind();
	TypedArray<VectorM> get_texture_map_values_bind();
	virtual int get_dimension();

	GDVIRTUAL0R(PackedInt32Array, _get_edge_indices);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_vertex_positions);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_normal_values);
	GDVIRTUAL0R(TypedArray<VectorM>, _get_texture_map_values);
	GDVIRTUAL0R(bool, _validate_mesh_data);
	GDVIRTUAL0(_update_proxy_mesh_3d);
	GDVIRTUAL1(_validate_material_for_mesh, Ref<MaterialND>);
};
