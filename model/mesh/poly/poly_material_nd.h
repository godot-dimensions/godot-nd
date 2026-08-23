#pragma once

#include "../cell/cell_material_nd.h"
#include "poly_mesh_nd.h"

class PolyMaterialND : public CellMaterialND {
	GDCLASS(PolyMaterialND, CellMaterialND);

	PackedColorArray _poly_albedo_color_array;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	void populate_albedo_color_array_for_poly_mesh(const Ref<CellMeshND> &p_poly_mesh);

	PackedColorArray get_poly_albedo_color_array() const { return _poly_albedo_color_array; }
	void set_poly_albedo_color_array(const PackedColorArray &p_colors);

	PolyMaterialND() {}
};
