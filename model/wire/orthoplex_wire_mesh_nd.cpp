#include "orthoplex_wire_mesh_nd.h"

#include "../../math/vector_nd.h"

VectorN OrthoplexWireMeshND::get_half_extents() const {
	return VectorND::multiply_scalar(_size, 0.5);
}

void OrthoplexWireMeshND::set_half_extents(const VectorN &p_half_extents) {
	set_size(VectorND::multiply_scalar(p_half_extents, 2.0));
}

VectorN OrthoplexWireMeshND::get_size() const {
	return _size;
}

void OrthoplexWireMeshND::set_size(const VectorN &p_size) {
	if (p_size != _size) {
		if (p_size.size() != _size.size()) {
			_edge_indices_cache.clear();
		}
		_size = p_size;
		_vertices_cache.clear();
		wire_mesh_clear_cache();
	}
}

void OrthoplexWireMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "OrthoplexWireMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 1000, "OrthoplexWireMeshND: Too many dimensions for orthoplex.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

PackedInt32Array OrthoplexWireMeshND::get_edge_indices() {
	if (_edge_indices_cache.is_empty()) {
		const int dimension = _size.size();
		ERR_FAIL_COND_V_MSG(dimension > 1000, _edge_indices_cache, "OrthoplexWireMeshND: Too many dimensions for orthoplex.");
		const int vertex_count = 2 * dimension;
		_edge_indices_cache.resize(2 * vertex_count * (dimension - 1));
		int index = 0;
		for (int start_vertex = 0; start_vertex < vertex_count; start_vertex += 2) {
			for (int end_vertex = start_vertex + 2; end_vertex < vertex_count; end_vertex += 2) {
				_edge_indices_cache.set(index++, start_vertex);
				_edge_indices_cache.set(index++, end_vertex);
				_edge_indices_cache.set(index++, start_vertex);
				_edge_indices_cache.set(index++, end_vertex + 1);
				_edge_indices_cache.set(index++, start_vertex + 1);
				_edge_indices_cache.set(index++, end_vertex);
				_edge_indices_cache.set(index++, start_vertex + 1);
				_edge_indices_cache.set(index++, end_vertex + 1);
			}
		}
	}
	return _edge_indices_cache;
}

Vector<VectorN> OrthoplexWireMeshND::get_vertices() {
	if (_vertices_cache.is_empty()) {
		const int dimension = _size.size();
		ERR_FAIL_COND_V_MSG(dimension > 10000, _vertices_cache, "OrthoplexWireMeshND: Too many dimensions for orthoplex.");
		const VectorN he = get_half_extents();
		_vertices_cache.resize(dimension * 2);
		for (int i = 0; i < dimension; i++) {
			VectorN positive;
			positive.resize(dimension);
			positive.set(i, he[i]);
			_vertices_cache.set(i * 2, positive);
			VectorN negative;
			negative.resize(dimension);
			negative.set(i, -he[i]);
			_vertices_cache.set(i * 2 + 1, negative);
		}
	}
	return _vertices_cache;
}

Ref<WireMeshND> OrthoplexWireMeshND::to_wire_mesh() {
	Ref<OrthoplexWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

void OrthoplexWireMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &OrthoplexWireMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,1000,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_half_extents"), &OrthoplexWireMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &OrthoplexWireMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &OrthoplexWireMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &OrthoplexWireMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");
}
