#pragma once

#include "array_wire_mesh_nd.h"

// Static helper class for ND wire mesh building functions.
class WireMeshBuilderND : public Object {
	GDCLASS(WireMeshBuilderND, Object);

protected:
	static WireMeshBuilderND *singleton;
	static void _bind_methods();

public:
	// These functions create new meshes from the given data.
	static Ref<ArrayWireMeshND> extrude_linear(const Ref<ArrayWireMeshND> &p_input_mesh, const VectorN &p_extrusion_vector = VectorN());

	static WireMeshBuilderND *get_singleton() { return singleton; }
	WireMeshBuilderND() { singleton = this; }
	~WireMeshBuilderND() { singleton = nullptr; }
};
