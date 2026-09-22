#include "wire_mesh_nd.h"

void WireMeshND::wire_mesh_clear_cache(const bool p_reset_validation) {
	_edge_positions_cache.clear();
	// The proxy mesh and rect bounds are also caches, so they are always marked dirty here.
	if (p_reset_validation) {
		reset_mesh_data_validation();
	} else {
		mark_mesh_bounds_and_proxy_mesh_3d_dirty();
	}
}

Vector<VectorN> WireMeshND::get_edge_positions() {
	if (_edge_positions_cache.is_empty()) {
		const PackedInt32Array edge_indices = get_edge_indices();
		const Vector<VectorN> vertex_positions = get_vertex_positions();
		const int32_t vertices_count = vertex_positions.size();
		for (const int32_t edge_index : edge_indices) {
			ERR_FAIL_INDEX_V(edge_index, vertices_count, _edge_positions_cache);
			_edge_positions_cache.append(vertex_positions[edge_index]);
		}
	}
	return _edge_positions_cache;
}

Ref<WireMaterialND> WireMeshND::_fallback_material;

Ref<MaterialND> WireMeshND::get_fallback_material() {
	return _fallback_material;
}

void WireMeshND::init_fallback_material() {
	_fallback_material.instantiate();
}

void WireMeshND::cleanup_fallback_material() {
	_fallback_material.unref();
}

void WireMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("wire_mesh_clear_cache", "reset_validation"), &WireMeshND::wire_mesh_clear_cache, DEFVAL(true));
}
