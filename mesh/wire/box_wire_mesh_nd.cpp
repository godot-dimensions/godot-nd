#include "box_wire_mesh_nd.h"

#include "../../math/vector_nd.h"
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

PackedInt32Array BoxWireMeshND::get_edge_indices() {
	if (_edge_indices_cache.is_empty()) {
		const int dimension = _size.size();
		ERR_FAIL_COND_V_MSG(dimension > 30, _edge_indices_cache, "BoxWireMeshND: Too many dimensions for box edges.");
		const int64_t vertex_count = 1 << dimension;
		for (int i = 0; i < vertex_count; i++) {
			for (int j = 0; j < dimension; j++) {
				if ((i & (1 << j)) == 0) {
					const int other_vertex_index = i + (1 << j);
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
		const VectorN he = get_half_extents();
		ERR_FAIL_COND_V_MSG(he.size() > 30, _vertices_cache, "BoxWireMeshND: Too many dimensions for box vertices.");
		const int64_t vertex_count = 1 << he.size();
		_vertices_cache.resize(vertex_count);
		for (int64_t i = 0; i < vertex_count; i++) {
			VectorN vertex;
			vertex.resize(he.size());
			for (int64_t j = 0; j < he.size(); j++) {
				vertex.set(j, (i & (1 << j)) ? he[j] : -he[j]);
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
	ClassDB::bind_method(D_METHOD("get_half_extents"), &BoxWireMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &BoxWireMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &BoxWireMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &BoxWireMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");
}
