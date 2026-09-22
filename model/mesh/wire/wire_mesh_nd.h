#pragma once

#include "../single_surface_mesh_nd.h"
#include "wire_material_nd.h"

class WireMeshND : public SingleSurfaceMeshND {
	GDCLASS(WireMeshND, SingleSurfaceMeshND);

protected:
	static void _bind_methods();
	Vector<VectorN> _edge_positions_cache;

public:
	void wire_mesh_clear_cache(const bool p_reset_validation = true);
	virtual Vector<VectorN> get_edge_positions() override;

	Ref<MaterialND> get_fallback_material() override;
	static void init_fallback_material();
	static void cleanup_fallback_material();

private:
	static Ref<WireMaterialND> _fallback_material;
};
