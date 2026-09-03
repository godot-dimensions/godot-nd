#pragma once

#include "poly_mesh_nd.h"

class BoxWireMeshND;
class WireMeshND;

class BoxPolyMeshND : public PolyMeshND {
	GDCLASS(BoxPolyMeshND, PolyMeshND);

public:
	enum BoxPolyTextureMap {
		BOX_POLY_TEXTURE_MAP_CROSS_ISLAND,
		BOX_POLY_TEXTURE_MAP_FILL_EACH_SIDE,
		BOX_POLY_TEXTURE_MAP_LONG_CROSS,
	};

private:
	Vector<Vector<PackedInt32Array>> _poly_cell_indices_cache;
	PackedInt32Array _box_edge_indices_cache;
	// The boundary normals also serve as the normal values, since each boundary cell's
	// vertex normals are flat shading normals that all point along the cell's normal.
	Vector<VectorN> _boundary_normals_cache;
	Vector<PackedInt32Array> _normal_indices_cache;
	Vector<VectorM> _texture_map_values_cache;
	Vector<PackedInt32Array> _texture_map_indices_cache;
	Vector<VectorN> _vertices_cache;

	VectorN _size;
	BoxPolyTextureMap _poly_texture_map = BOX_POLY_TEXTURE_MAP_CROSS_ISLAND;

	void _clear_caches();
	void _generate_poly_data();
	void _generate_texture_map();

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

	BoxPolyTextureMap get_poly_texture_map() const { return _poly_texture_map; }
	void set_poly_texture_map(const BoxPolyTextureMap p_map);

	virtual Vector<Vector<PackedInt32Array>> get_poly_cell_indices() override;
	virtual Vector<VectorN> get_poly_cell_vertex_positions() override;
	virtual Vector<VectorN> get_poly_cell_boundary_normals() override;
	virtual Vector<VectorN> get_poly_cell_normal_values() override;
	virtual Vector<VectorM> get_poly_cell_texture_map_values() override;
	virtual Vector<PackedInt32Array> get_poly_cell_normal_indices() override;
	virtual Vector<PackedInt32Array> get_poly_cell_texture_map_indices() override;

	virtual PackedInt32Array get_edge_indices() override;
	virtual Vector<VectorN> get_vertex_positions() override;

	static Ref<BoxPolyMeshND> from_box_wire_mesh(const Ref<BoxWireMeshND> &p_wire_mesh);
	Ref<BoxWireMeshND> to_box_wire_mesh() const;
	virtual Ref<WireMeshND> to_wire_mesh() override;
};

VARIANT_ENUM_CAST(BoxPolyMeshND::BoxPolyTextureMap);
