#pragma once

#include "../single_surface_mesh_nd.h"

class WireMeshND : public SingleSurfaceMeshND {
	GDCLASS(WireMeshND, SingleSurfaceMeshND);

protected:
	static void _bind_methods();
	Vector<VectorN> _edge_positions_cache;

public:
	void wire_mesh_clear_cache(const bool p_reset_validation = true);
	virtual Vector<VectorN> get_edge_positions() override;
};
