#pragma once

#include "../../math/transform_nd.h"
#include "mesh_nd.h"

class SingleSurfaceMeshND;

class MultiSurfaceMeshND : public MeshND {
	GDCLASS(MultiSurfaceMeshND, MeshND);

	// The array of surface meshes contained within this MultiSurfaceMeshND.
	// Stored as `Vector<>` to guarantee the only changes to the array happen in `set_surface_meshes`.
	// Using `TypedArray<>` would allow external modifications, so we only use it in the bindings.
	Vector<Ref<SingleSurfaceMeshND>> _surface_meshes;
	// For each surface, the index of its first surface in the proxy 3D mesh, or -1 if it added none.
	// Rebuilt by `append_proxy_mesh_surfaces_3d`, so it matches the proxy mesh that was last generated.
	PackedInt32Array _proxy_surface_indices_3d;

	static bool _can_merge_surfaces_except_considering_type(const Ref<SingleSurfaceMeshND> &p_surface, const Ref<SingleSurfaceMeshND> &p_merged_surface, const Ref<MaterialND> &p_merged_surface_original_material);
	void _ensure_all_surfaces_are_writable();
	void _on_surface_mesh_data_validation_reset();
	void _on_surface_proxy_mesh_3d_marked_dirty();

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override;

public:
	virtual Ref<RectND> get_rect_bounds() override;
	virtual int get_dimension() override;

	const Vector<Ref<SingleSurfaceMeshND>> &get_surface_meshes() const { return _surface_meshes; }
	void set_surface_meshes(const Vector<Ref<SingleSurfaceMeshND>> &p_surface_meshes);

	TypedArray<SingleSurfaceMeshND> get_surface_meshes_bind() const;
	void set_surface_meshes_bind(const TypedArray<SingleSurfaceMeshND> &p_surface_meshes);

	virtual void append_proxy_mesh_surfaces_3d(const Ref<ArrayMesh> &p_proxy_mesh_3d) override;
	virtual int get_proxy_surface_index_3d(const int p_surface_index_nd) const override;
	virtual void validate_material_for_mesh(const Ref<MaterialND> &p_material) override;

	void merge_compatible_surfaces();
	void merge_with(const Ref<MeshND> &p_other, const Ref<TransformND> &p_transform = Ref<TransformND>());
	void transform_mesh(const Ref<TransformND> &p_transform);
};
