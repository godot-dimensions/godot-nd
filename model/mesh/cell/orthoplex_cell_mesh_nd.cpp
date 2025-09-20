#include "orthoplex_cell_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "../wire/orthoplex_wire_mesh_nd.h"

void OrthoplexCellMeshND::_clear_caches() {
	_cell_indices_cache.clear();
	_vertices_cache.clear();
	cell_mesh_clear_cache();
}

VectorN OrthoplexCellMeshND::get_half_extents() const {
	return VectorND::multiply_scalar(_size, 0.5);
}

void OrthoplexCellMeshND::set_half_extents(const VectorN &p_half_extents) {
	set_dimension(p_half_extents.size());
	_size = VectorND::multiply_scalar(p_half_extents, 2.0);
	_clear_caches();
}

VectorN OrthoplexCellMeshND::get_size() const {
	return _size;
}

void OrthoplexCellMeshND::set_size(const VectorN &p_size) {
	set_dimension(p_size.size());
	_size = p_size;
	_clear_caches();
}

void OrthoplexCellMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "OrthoplexCellMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 30, "OrthoplexCellMeshND: Too many dimensions for orthoplex cells.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

PackedInt32Array OrthoplexCellMeshND::get_cell_indices() {
	if (_cell_indices_cache.is_empty()) {
		const uint64_t dimension = _size.size();
		ERR_FAIL_COND_V_MSG(dimension > 30, _cell_indices_cache, "OrthoplexCellMeshND: Too many dimensions for orthoplex cells.");
		const uint64_t cell_count = uint64_t(1) << dimension;
		_cell_indices_cache.resize(cell_count * dimension);
		int index = 0;
		for (uint64_t i = 0; i < cell_count; i++) {
			for (uint64_t j = 0; j < dimension; j++) {
				const int parity = (i & (uint64_t(1) << j)) ? 1 : 0;
				_cell_indices_cache.set(index++, j * 2 + parity);
			}
		}
		CRASH_COND(index != _cell_indices_cache.size());
	}
	return _cell_indices_cache;
}

Vector<VectorN> OrthoplexCellMeshND::get_vertices() {
	if (_vertices_cache.is_empty()) {
		const VectorN he = get_half_extents();
		_vertices_cache.resize(2 * _size.size());
		for (int i = 0; i < _size.size(); i++) {
			VectorN positive;
			positive.resize(_size.size());
			positive.set(i, he[i]);
			_vertices_cache.set(i * 2, positive);
			VectorN negative;
			negative.resize(_size.size());
			negative.set(i, -he[i]);
			_vertices_cache.set(i * 2 + 1, negative);
		}
	}
	return _vertices_cache;
}

Ref<OrthoplexCellMeshND> OrthoplexCellMeshND::from_orthoplex_wire_mesh(const Ref<OrthoplexWireMeshND> &p_wire_mesh) {
	Ref<OrthoplexCellMeshND> cell_mesh;
	cell_mesh.instantiate();
	cell_mesh->set_size(p_wire_mesh->get_size());
	cell_mesh->set_material(p_wire_mesh->get_material());
	return cell_mesh;
}

Ref<OrthoplexWireMeshND> OrthoplexCellMeshND::to_orthoplex_wire_mesh() const {
	Ref<OrthoplexWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<WireMeshND> OrthoplexCellMeshND::to_wire_mesh() {
	return to_orthoplex_wire_mesh();
}

void OrthoplexCellMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &OrthoplexCellMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,30,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_half_extents"), &OrthoplexCellMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &OrthoplexCellMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &OrthoplexCellMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &OrthoplexCellMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");

	ClassDB::bind_static_method("OrthoplexCellMeshND", D_METHOD("from_orthoplex_wire_mesh", "wire_mesh"), &OrthoplexCellMeshND::from_orthoplex_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_orthoplex_wire_mesh"), &OrthoplexCellMeshND::to_orthoplex_wire_mesh);
}
