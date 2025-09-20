#include "box_wire_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "array_wire_mesh_nd.h"

VectorN BoxWireMeshND::get_half_extents() const {
	return VectorND::multiply_scalar(_size, 0.5);
}

void BoxWireMeshND::set_half_extents(const VectorN &p_half_extents) {
	set_size(VectorND::multiply_scalar(p_half_extents, 2.0));
}

VectorN BoxWireMeshND::get_size() const {
	return _size;
}

void BoxWireMeshND::set_size(const VectorN &p_size) {
	if (p_size != _size) {
		if (p_size.size() != _size.size()) {
			_edge_indices_cache.clear();
		}
		_size = p_size;
		_vertices_cache.clear();
		wire_mesh_clear_cache();
	}
}

void BoxWireMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "BoxWireMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 30, "BoxWireMeshND: Too many dimensions for wireframe box.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

PackedInt32Array BoxWireMeshND::get_edge_indices() {
	if (_edge_indices_cache.is_empty()) {
		const uint64_t dimension = _size.size();
		ERR_FAIL_COND_V_MSG(dimension > 30, _edge_indices_cache, "BoxWireMeshND: Too many dimensions for box edges.");
		const uint64_t vertex_count = uint64_t(1) << dimension;
		for (uint64_t i = 0; i < vertex_count; i++) {
			for (uint64_t j = 0; j < dimension; j++) {
				if ((i & (uint64_t(1) << j)) == 0) {
					const uint64_t other_vertex_index = i + (uint64_t(1) << j);
					_edge_indices_cache.append(i);
					_edge_indices_cache.append(other_vertex_index);
				}
			}
		}
	}
	return _edge_indices_cache;
}

Vector<VectorN> BoxWireMeshND::get_vertices() {
	if (_vertices_cache.is_empty()) {
		const uint64_t dimension = _size.size();
		ERR_FAIL_COND_V_MSG(dimension > 30, _vertices_cache, "BoxWireMeshND: Too many dimensions for box vertices.");
		const uint64_t vertex_count = uint64_t(1) << dimension;
		const VectorN he = get_half_extents();
		_vertices_cache.resize(vertex_count);
		for (uint64_t i = 0; i < vertex_count; i++) {
			VectorN vertex;
			vertex.resize(he.size());
			for (uint64_t j = 0; j < dimension; j++) {
				vertex.set(j, (i & (uint64_t(1) << j)) ? he[j] : -he[j]);
			}
			_vertices_cache.set(i, vertex);
		}
	}
	return _vertices_cache;
}

Ref<WireMeshND> BoxWireMeshND::to_wire_mesh() {
	Ref<BoxWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

void BoxWireMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &BoxWireMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,30,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_half_extents"), &BoxWireMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &BoxWireMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &BoxWireMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &BoxWireMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");
}
