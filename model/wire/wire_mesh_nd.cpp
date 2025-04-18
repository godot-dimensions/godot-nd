#include "wire_mesh_nd.h"

void WireMeshND::wire_mesh_clear_cache() {
	_edge_positions_cache.clear();
}

Vector<VectorN> WireMeshND::get_edge_positions() {
	if (_edge_positions_cache.is_empty()) {
		const PackedInt32Array edge_indices = get_edge_indices();
		const Vector<VectorN> vertices = get_vertices();
		for (const int32_t edge_index : edge_indices) {
			_edge_positions_cache.append(vertices[edge_index]);
		}
	}
	return _edge_positions_cache;
}

void WireMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("wire_mesh_clear_cache"), &WireMeshND::wire_mesh_clear_cache);
}
