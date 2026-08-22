#pragma once

#include "../../nodes/node_nd.h"
#include "sky/sky_material_nd.h"

class WorldEnvironmentND : public NodeND {
	GDCLASS(WorldEnvironmentND, NodeND);

	Ref<SkyMaterialND> _sky_material;

	bool _is_current = false;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	bool is_current() const;
	void set_current(const bool p_enabled);
	void clear_current(const bool p_enable_next = true);
	void make_current();

	Ref<SkyMaterialND> get_sky_material() const { return _sky_material; }
	void set_sky_material(const Ref<SkyMaterialND> &p_sky_material) { _sky_material = p_sky_material; }
};
