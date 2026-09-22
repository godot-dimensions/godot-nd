#pragma once

#include "../../nodes/node_nd.h"
#include "material_nd.h"
#include "mesh_nd.h"

class SingleSurfaceMeshND;

class MeshInstanceND : public NodeND {
	GDCLASS(MeshInstanceND, NodeND);

	Vector<Ref<MaterialND>> _material_overrides;
	Ref<MeshND> _mesh;

	static Ref<MaterialND> _get_valid_active_material_for_surface(const Ref<SingleSurfaceMeshND> &p_surface_mesh, Ref<MaterialND> p_material);

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void _validate_property(PropertyInfo &p_property) const;

public:
	Ref<MaterialND> get_active_material(const int p_surface_index = 0) const;

	Ref<MaterialND> get_material_override() const;
	void set_material_override(const Ref<MaterialND> &p_material_override);

	Vector<Ref<MaterialND>> get_material_overrides() const;
	void set_material_overrides(const Vector<Ref<MaterialND>> &p_material_overrides);

	TypedArray<MaterialND> get_material_overrides_bind() const;
	void set_material_overrides_bind(const TypedArray<MaterialND> &p_material_overrides);

	Ref<MeshND> get_mesh() const;
	void set_mesh(const Ref<MeshND> &p_mesh);

	virtual Ref<RectND> get_rect_bounds(const Ref<TransformND> &p_to_target) const override;
};
