#pragma once

#include "../material_nd.h"

class CellMaterialND : public MaterialND {
	GDCLASS(CellMaterialND, MaterialND);

public:
	enum CellColorSourceND {
		CELL_COLOR_SOURCE_SINGLE_COLOR = 1,
		CELL_COLOR_SOURCE_PER_VERT_ONLY = 2,
		CELL_COLOR_SOURCE_PER_CELL_ONLY = 4,
		CELL_COLOR_SOURCE_CELL_TEXTURE_MAP_ONLY = 8,
		CELL_COLOR_SOURCE_PER_VERT_AND_SINGLE = 3,
		CELL_COLOR_SOURCE_PER_CELL_AND_SINGLE = 5,
		CELL_COLOR_SOURCE_CELL_TEXTURE_MAP_AND_SINGLE = 9,
		CELL_COLOR_SOURCE_USES_COLOR_ARRAY = 6,
	};

private:
	CellColorSourceND _albedo_source = CELL_COLOR_SOURCE_SINGLE_COLOR;

protected:
	static void _bind_methods();
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	CellColorSourceND get_albedo_source() const;
	void set_albedo_source(const CellColorSourceND p_albedo_source);

	CellMaterialND();
};

VARIANT_ENUM_CAST(CellMaterialND::CellColorSourceND);
