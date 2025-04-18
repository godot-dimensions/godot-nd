#pragma once

#include "wire_mesh_nd.h"

class BoxWireMeshND : public WireMeshND {
	GDCLASS(BoxWireMeshND, WireMeshND);

	PackedInt32Array _edge_indices_cache;
	Vector<VectorN> _vertices_cache;
	VectorN _size;

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override { return true; }

public:
	VectorN get_half_extents() const;
	void set_half_extents(const VectorN &p_half_extents);

	VectorN get_size() const;
	void set_size(const VectorN &p_size);

	virtual PackedInt32Array get_edge_indices() override;
	virtual Vector<VectorN> get_vertices() override;

	virtual Ref<WireMeshND> to_wire_mesh() override;
};
