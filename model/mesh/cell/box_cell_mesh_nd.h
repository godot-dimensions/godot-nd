#pragma once

#include "cell_mesh_nd.h"

class BoxWireMeshND;
class WireMeshND;

class BoxCellMeshND : public CellMeshND {
	GDCLASS(BoxCellMeshND, CellMeshND);

private:
	PackedInt32Array _cell_indices_cache;
	Vector<VectorN> _vertices_cache;

	VectorN _size;
	bool _polytope_cells = false;

	void _clear_caches();
	static int64_t _factorial(const int64_t p_dimension);
	static void _write_gray_path(const int64_t p_dimension, const int64_t p_index, const int64_t *p_bit_order, PackedInt32Array &r_out);
	static void _generate_cell_indices_lex(const int64_t p_dimension, PackedInt32Array &r_out);

protected:
	static void _bind_methods();
	virtual bool validate_mesh_data() override { return true; }

public:
	VectorN get_half_extents() const;
	void set_half_extents(const VectorN &p_half_extents);

	VectorN get_size() const;
	void set_size(const VectorN &p_size);

	bool get_polytope_cells() const { return _polytope_cells; }
	void set_polytope_cells(const bool p_polytope_cells);

	virtual int get_cell_count() override;
	virtual int get_indices_per_cell() override;
	virtual PackedInt32Array get_cell_indices() override;
	virtual PackedInt32Array get_edge_indices() override;
	virtual Vector<VectorN> get_vertices() override;
	virtual int get_dimension() override { return _size.size(); }
	void set_dimension(int p_dimension);

	static Ref<BoxCellMeshND> from_box_wire_mesh(const Ref<BoxWireMeshND> &p_wire_mesh);
	Ref<BoxWireMeshND> to_box_wire_mesh() const;
	virtual Ref<WireMeshND> to_wire_mesh() override;
};
