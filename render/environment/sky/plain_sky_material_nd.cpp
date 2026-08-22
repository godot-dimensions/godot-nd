#include "plain_sky_material_nd.h"

void PlainSkyMaterialND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_color", "color"), &PlainSkyMaterialND::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &PlainSkyMaterialND::get_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color", PROPERTY_HINT_COLOR_NO_ALPHA), "set_color", "get_color");
}

PlainSkyMaterialND::PlainSkyMaterialND() {
	set_color(Color(0.0f, 0.0f, 0.0f));
	set_energy_multiplier(1.0f);
}
