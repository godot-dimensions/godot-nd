#pragma once

#include "poly_mesh_nd.h"

class BoxCellMeshND;
class BoxWireMeshND;
class WireMeshND;

class BoxPolyMeshND : public PolyMeshND {
	GDCLASS(BoxPolyMeshND, PolyMeshND);

private:
	Vector<Vector<PackedInt32Array>> _poly_cell_indices_cache;
	PackedInt32Array _box_edge_indices_cache;
	Vector<VectorN> _boundary_normals_cache;
	Vector<Vector<VectorN>> _vertex_normals_cache;
	Vector<VectorN> _vertices_cache;

	VectorN _size;

	void _clear_caches();
	void _generate_poly_data();

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override { return true; }
	virtual bool _validate_poly_mesh_data_only() override { return true; }

public:
	VectorN get_half_extents() const;
	void set_half_extents(const VectorN &p_half_extents);

	VectorN get_size() const;
	void set_size(const VectorN &p_size);

	virtual int get_dimension() override { return _size.size(); }
	void set_dimension(const int p_dimension);

	virtual Vector<Vector<PackedInt32Array>> get_poly_cell_indices() override;
	virtual Vector<VectorN> get_poly_cell_vertices() override;
	virtual Vector<VectorN> get_poly_cell_boundary_normals() override;
	virtual Vector<Vector<VectorN>> get_poly_cell_vertex_normals() override;
	virtual Vector<Vector<VectorM>> get_poly_cell_texture_map() override;

	virtual PackedInt32Array get_edge_indices() override;
	virtual Vector<VectorN> get_vertices() override;

	static Ref<BoxPolyMeshND> from_box_cell_mesh(const Ref<BoxCellMeshND> &p_cell_mesh);
	static Ref<BoxPolyMeshND> from_box_wire_mesh(const Ref<BoxWireMeshND> &p_wire_mesh);
	Ref<BoxCellMeshND> to_box_cell_mesh() const;
	Ref<BoxWireMeshND> to_box_wire_mesh() const;
	virtual Ref<CellMeshND> to_cell_mesh() override;
	virtual Ref<WireMeshND> to_wire_mesh() override;
};
