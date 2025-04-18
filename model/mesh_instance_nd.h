#pragma once

#include "../nodes/node_nd.h"
#include "mesh_nd.h"

class MeshInstanceND : public NodeND {
	GDCLASS(MeshInstanceND, NodeND);

	Ref<MaterialND> _material_override;
	Ref<MeshND> _mesh;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	Ref<MaterialND> get_active_material() const;

	Ref<MaterialND> get_material_override() const;
	void set_material_override(const Ref<MaterialND> &p_material_override);

	Ref<MeshND> get_mesh() const;
	void set_mesh(const Ref<MeshND> &p_mesh);

	virtual Ref<RectND> get_rect_bounds(const Ref<TransformND> &p_inv_relative_to) const override;
};
