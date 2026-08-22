#pragma once

#include "../../../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/resource.hpp>
#elif GODOT_MODULE
#include "core/io/resource.h"
#endif

class SkyMaterialND : public Resource {
	GDCLASS(SkyMaterialND, Resource);

	real_t _energy_multiplier = 1.0f;

protected:
	static void _bind_methods();

public:
	real_t get_energy_multiplier() const { return _energy_multiplier; }
	void set_energy_multiplier(const real_t p_energy_multiplier) { _energy_multiplier = p_energy_multiplier; }
};
