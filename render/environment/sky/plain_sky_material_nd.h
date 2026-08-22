#pragma once

#include "sky_material_nd.h"

class PlainSkyMaterialND : public SkyMaterialND {
	GDCLASS(PlainSkyMaterialND, SkyMaterialND);

	Color _color;

protected:
	static void _bind_methods();

public:
	Color get_color() const { return _color; }
	void set_color(const Color &p_color) { _color = p_color; }

	PlainSkyMaterialND();
};
