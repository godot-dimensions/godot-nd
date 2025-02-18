#pragma once

#include "../mesh_nd.h"

class WireMeshND : public MeshND {
	GDCLASS(WireMeshND, MeshND);

protected:
	static void _bind_methods();
	Vector<VectorN> _edge_positions_cache;

public:
	void wire_mesh_clear_cache();
	virtual Vector<VectorN> get_edge_positions() override;
};
