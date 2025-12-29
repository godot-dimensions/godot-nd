#pragma once

#include "cell_mesh_nd.h"

class OrthoplexWireMeshND;
class WireMeshND;

class OrthoplexCellMeshND : public CellMeshND {
	GDCLASS(OrthoplexCellMeshND, CellMeshND);

private:
	PackedInt32Array _cell_indices_cache;
	Vector<VectorN> _vertices_cache;

	VectorN _size;

	void _clear_caches();

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override { return true; }

public:
	VectorN get_half_extents() const;
	void set_half_extents(const VectorN &p_half_extents);

	VectorN get_size() const;
	void set_size(const VectorN &p_size);

	virtual PackedInt32Array get_simplex_cell_indices() override;
	virtual Vector<VectorN> get_vertices() override;
	virtual int get_dimension() override { return _size.size(); }
	void set_dimension(int p_dimension);

	static Ref<OrthoplexCellMeshND> from_orthoplex_wire_mesh(const Ref<OrthoplexWireMeshND> &p_wire_mesh);
	Ref<OrthoplexWireMeshND> to_orthoplex_wire_mesh() const;
	virtual Ref<WireMeshND> to_wire_mesh() override;
};
