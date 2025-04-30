#pragma once

#include "../material_nd.h"

class CellMaterialND : public MaterialND {
	GDCLASS(CellMaterialND, MaterialND);

protected:
	static void _bind_methods();
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	virtual Color get_albedo_color_of_edge(const int64_t p_edge_index, const Ref<MeshND> &p_for_mesh) override;

	CellMaterialND();
};
