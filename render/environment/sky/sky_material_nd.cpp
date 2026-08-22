#include "sky_material_nd.h"

void SkyMaterialND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_energy_multiplier", "multiplier"), &SkyMaterialND::set_energy_multiplier);
	ClassDB::bind_method(D_METHOD("get_energy_multiplier"), &SkyMaterialND::get_energy_multiplier);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "energy_multiplier", PROPERTY_HINT_RANGE, "0.1,2,0.001,exp,or_greater,or_less"), "set_energy_multiplier", "get_energy_multiplier");
}
