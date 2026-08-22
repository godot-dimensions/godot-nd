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
	bool _is_cross_section_mesh_dirty = true;
	bool _is_rect_bounds_dirty = true;

protected:
	// Slightly under the 32-bit integer limit to avoid overflows.
	static constexpr int64_t MAX_VERTICES = 2147483640;
	Ref<ArrayMesh> _cross_section_mesh;

	static void _bind_methods();
	virtual bool validate_mesh_data();
	// Call when the mesh is modified to indicate that
	// the 3D mesh used for cross-section rendering needs to be updated.
	void mark_cross_section_mesh_dirty() { _is_cross_section_mesh_dirty = true; }
	void mark_mesh_bounds_and_cross_section_dirty() {
		_is_cross_section_mesh_dirty = true;
		_is_rect_bounds_dirty = true;
	}
	// Called when the cross-section mesh is requested and the cross-section mesh has been marked dirty.
	// Update the mesh referenced by _cross_section_mesh to match the current state of the mesh.
	virtual void update_cross_section_mesh();

public:
	static PackedInt32Array deduplicate_edge_indices(const PackedInt32Array &p_items);
	bool has_edge_indices(int p_first, int p_second);

	bool is_mesh_data_valid();
	void reset_mesh_data_validation();
	virtual void validate_material_for_mesh(const Ref<MaterialND> &p_material);

	Ref<ArrayWireMeshND> to_array_wire_mesh();
	virtual Ref<WireMeshND> to_wire_mesh();

	Ref<RectND> get_rect_bounds();
	// Returns a reference to the mesh used for cross-section rendering.
	Ref<ArrayMesh> get_cross_section_mesh();

	Ref<MaterialND> get_material() const;
	void set_material(const Ref<MaterialND> &p_material);

	virtual PackedInt32Array get_edge_indices();
	virtual Vector<VectorN> get_edge_positions();
	TypedArray<VectorN> get_edge_positions_bind();
	virtual Vector<VectorN> get_vertices();
	TypedArray<VectorN> get_vertices_bind();
	virtual int get_dimension();

	GDVIRTUAL0R(PackedInt32Array, _get_edge_indices);
	GDVIRTUAL0R(TypedArray<VectorN>, _get_vertices);
	GDVIRTUAL0R(bool, _validate_mesh_data);
	GDVIRTUAL0(_update_cross_section_mesh);
	GDVIRTUAL1(_validate_material_for_mesh, Ref<MaterialND>);
};
