#include "orthoplex_poly_mesh_nd.h"

#include "../../math/vector_nd.h"
#include "../cell/orthoplex_cell_mesh_nd.h"
#include "../wire/orthoplex_wire_mesh_nd.h"

void OrthoplexPolyMeshND::_clear_caches() {
	_poly_cell_indices_cache.clear();
	_vertices_cache.clear();
	poly_mesh_clear_cache();
}

VectorN OrthoplexPolyMeshND::get_half_extents() const {
	return VectorND::multiply_scalar(_size, 0.5f);
}

void OrthoplexPolyMeshND::set_half_extents(const VectorN &p_half_extents) {
	_size = VectorND::multiply_scalar(p_half_extents, 2.0f);
	_clear_caches();
}

VectorN OrthoplexPolyMeshND::get_size() const {
	return _size;
}

void OrthoplexPolyMeshND::set_size(const VectorN &p_size) {
	_size = p_size;
	_clear_caches();
}

void OrthoplexPolyMeshND::set_dimension(const int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "BoxPolyMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 10, "BoxPolyMeshND: Too many dimensions for cell-based box.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

Vector<Vector<PackedInt32Array>> OrthoplexPolyMeshND::get_poly_cell_indices() {
	return ORTHOPLEX_POLY_CELL_INDICES;
}

Vector<VectorN> OrthoplexPolyMeshND::get_poly_cell_vertices() {
	return get_vertices();
}

Vector<VectorN> OrthoplexPolyMeshND::get_poly_cell_boundary_normals() {
	return ORTHOPLEX_CELL_BOUNDARY_NORMALS;
}

Vector<Vector<VectorN>> OrthoplexPolyMeshND::get_poly_cell_vertex_normals() {
	return ORTHOPLEX_POLY_CELL_VERTEX_NORMALS;
}

Vector<Vector<VectorM>> OrthoplexPolyMeshND::get_poly_cell_texture_map() {
	return ORTHOPLEX_POLY_CELL_POLY_TEXTURE_MAP;
}

PackedInt32Array OrthoplexPolyMeshND::get_edge_indices() {
	return ORTHOPLEX_EDGE_INDICES;
}

Vector<VectorN> OrthoplexPolyMeshND::get_vertices() {
	if (_vertices_cache.is_empty()) {
		const VectorN he = get_half_extents();
		for (int i = 0; i < _size.size(); i++) {
			_vertices_cache.append(VectorND::value_on_axis_with_dimension(-he[i], i, _size.size()));
			_vertices_cache.append(VectorND::value_on_axis_with_dimension(+he[i], i, _size.size()));
		}
	}
	return _vertices_cache;
}

Ref<OrthoplexPolyMeshND> OrthoplexPolyMeshND::from_orthoplex_cell_mesh(const Ref<OrthoplexCellMeshND> &p_cell_mesh) {
	Ref<OrthoplexPolyMeshND> poly_mesh;
	poly_mesh.instantiate();
	poly_mesh->set_size(p_cell_mesh->get_size());
	poly_mesh->set_material(p_cell_mesh->get_material());
	return poly_mesh;
}

Ref<OrthoplexPolyMeshND> OrthoplexPolyMeshND::from_orthoplex_wire_mesh(const Ref<OrthoplexWireMeshND> &p_wire_mesh) {
	Ref<OrthoplexPolyMeshND> poly_mesh;
	poly_mesh.instantiate();
	poly_mesh->set_size(p_wire_mesh->get_size());
	poly_mesh->set_material(p_wire_mesh->get_material());
	return poly_mesh;
}

Ref<OrthoplexCellMeshND> OrthoplexPolyMeshND::to_orthoplex_cell_mesh() const {
	Ref<OrthoplexCellMeshND> cell_mesh;
	cell_mesh.instantiate();
	cell_mesh->set_size(_size);
	cell_mesh->set_material(get_material());
	return cell_mesh;
}

Ref<OrthoplexWireMeshND> OrthoplexPolyMeshND::to_orthoplex_wire_mesh() const {
	Ref<OrthoplexWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<CellMeshND> OrthoplexPolyMeshND::to_cell_mesh() {
	return to_orthoplex_cell_mesh();
}

Ref<WireMeshND> OrthoplexPolyMeshND::to_wire_mesh() {
	return to_orthoplex_wire_mesh();
}

void OrthoplexPolyMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &OrthoplexPolyMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,10,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_half_extents"), &OrthoplexPolyMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &OrthoplexPolyMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &OrthoplexPolyMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &OrthoplexPolyMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");

	ClassDB::bind_static_method("OrthoplexPolyMeshND", D_METHOD("from_orthoplex_cell_mesh", "cell_mesh"), &OrthoplexPolyMeshND::from_orthoplex_cell_mesh);
	ClassDB::bind_static_method("OrthoplexPolyMeshND", D_METHOD("from_orthoplex_wire_mesh", "wire_mesh"), &OrthoplexPolyMeshND::from_orthoplex_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_orthoplex_cell_mesh"), &OrthoplexPolyMeshND::to_orthoplex_cell_mesh);
	ClassDB::bind_method(D_METHOD("to_orthoplex_wire_mesh"), &OrthoplexPolyMeshND::to_orthoplex_wire_mesh);
}
