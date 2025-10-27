#include "box_poly_mesh_nd.h"

#include "../../math/vector_nd.h"
#include "../cell/box_cell_mesh_nd.h"
#include "../wire/box_wire_mesh_nd.h"

void BoxPolyMeshND::_clear_caches() {
	_poly_cell_indices_cache.clear();
	_vertices_cache.clear();
	poly_mesh_clear_cache();
}

VectorN BoxPolyMeshND::get_half_extents() const {
	return VectorND::multiply_scalar(_size, 0.5f);
}

void BoxPolyMeshND::set_half_extents(const VectorN &p_half_extents) {
	_size = VectorND::multiply_scalar(p_half_extents, 2.0f);
	_clear_caches();
}

VectorN BoxPolyMeshND::get_size() const {
	return _size;
}

void BoxPolyMeshND::set_size(const VectorN &p_size) {
	_size = p_size;
	_clear_caches();
}

void BoxPolyMeshND::set_dimension(const int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "BoxPolyMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 10, "BoxPolyMeshND: Too many dimensions for cell-based box.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

Vector<Vector<PackedInt32Array>> BoxPolyMeshND::get_poly_cell_indices() {
}

Vector<VectorN> BoxPolyMeshND::get_poly_cell_vertices() {
	return get_vertices();
}

Vector<VectorN> BoxPolyMeshND::get_poly_cell_boundary_normals() {
}

Vector<Vector<VectorN>> BoxPolyMeshND::get_poly_cell_vertex_normals() {
}

Vector<Vector<VectorM>> BoxPolyMeshND::get_poly_cell_texture_map() {
	ERR_FAIL_V_MSG(Vector<Vector<VectorM>>(), "BoxPolyMeshND: Invalid cell texture map type.");
}

PackedInt32Array BoxPolyMeshND::get_edge_indices() {
}

Vector<VectorN> BoxPolyMeshND::get_vertices() {
	if (_vertices_cache.is_empty()) {
		const VectorN he = get_half_extents();
		const int dimension = _size.size();
		const int64_t vertex_count = 1 << dimension;
		_vertices_cache.resize(vertex_count);
		for (int64_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
			VectorN vertex;
			for (int dim_index = 0; dim_index < dimension; dim_index++) {
				const float coord = (vertex_index & (1 << dim_index)) ? he[dim_index] : -he[dim_index];
				vertex.set(dim_index, coord);
			}
			_vertices_cache.set(vertex_index, vertex);
		}
	}
	return _vertices_cache;
}

Ref<BoxPolyMeshND> BoxPolyMeshND::from_box_cell_mesh(const Ref<BoxCellMeshND> &p_cell_mesh) {
	Ref<BoxPolyMeshND> poly_mesh;
	poly_mesh.instantiate();
	poly_mesh->set_size(p_cell_mesh->get_size());
	poly_mesh->set_material(p_cell_mesh->get_material());
	return poly_mesh;
}

Ref<BoxPolyMeshND> BoxPolyMeshND::from_box_wire_mesh(const Ref<BoxWireMeshND> &p_wire_mesh) {
	Ref<BoxPolyMeshND> poly_mesh;
	poly_mesh.instantiate();
	poly_mesh->set_size(p_wire_mesh->get_size());
	poly_mesh->set_material(p_wire_mesh->get_material());
	return poly_mesh;
}

Ref<BoxCellMeshND> BoxPolyMeshND::to_box_cell_mesh() const {
	Ref<BoxCellMeshND> cell_mesh;
	cell_mesh.instantiate();
	cell_mesh->set_size(_size);
	cell_mesh->set_material(get_material());
	return cell_mesh;
}

Ref<BoxWireMeshND> BoxPolyMeshND::to_box_wire_mesh() const {
	Ref<BoxWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<CellMeshND> BoxPolyMeshND::to_cell_mesh() {
	return to_box_cell_mesh();
}

Ref<WireMeshND> BoxPolyMeshND::to_wire_mesh() {
	return to_box_wire_mesh();
}

void BoxPolyMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &BoxPolyMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,10,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_half_extents"), &BoxPolyMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &BoxPolyMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &BoxPolyMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &BoxPolyMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");

	ClassDB::bind_static_method("BoxPolyMeshND", D_METHOD("from_box_cell_mesh", "cell_mesh"), &BoxPolyMeshND::from_box_cell_mesh);
	ClassDB::bind_static_method("BoxPolyMeshND", D_METHOD("from_box_wire_mesh", "wire_mesh"), &BoxPolyMeshND::from_box_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_box_cell_mesh"), &BoxPolyMeshND::to_box_cell_mesh);
	ClassDB::bind_method(D_METHOD("to_box_wire_mesh"), &BoxPolyMeshND::to_box_wire_mesh);
}
