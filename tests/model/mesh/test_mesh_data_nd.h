#pragma once

#include "../../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../../model/mesh/poly/array_poly_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestMeshDataND {
// Keep dense expectations readable while exercising the indexed production API.
inline Vector<VectorN> sample_values(const PackedInt32Array &p_indices, const Vector<VectorN> &p_values) {
	Vector<VectorN> sampled;
	for (const int32_t index : p_indices) {
		REQUIRE(index >= 0);
		REQUIRE(index < p_values.size());
		sampled.append(p_values[index]);
	}
	return sampled;
}

inline Vector<Vector<VectorN>> sample_cells(const Vector<PackedInt32Array> &p_indices, const Vector<VectorN> &p_values) {
	Vector<Vector<VectorN>> sampled;
	for (const PackedInt32Array &indices : p_indices) {
		sampled.append(sample_values(indices, p_values));
	}
	return sampled;
}

inline Vector<VectorN> get_simplex_normals(const Ref<CellMeshND> &p_mesh) {
	const PackedInt32Array indices = p_mesh->get_simplex_cell_normal_indices();
	return sample_values(indices, p_mesh->get_normal_values());
}

inline Vector<VectorM> get_simplex_texture_map(const Ref<CellMeshND> &p_mesh) {
	const PackedInt32Array indices = p_mesh->get_simplex_cell_texture_map_indices();
	return sample_values(indices, p_mesh->get_texture_map_values());
}

inline PackedInt32Array sequential_indices(const int64_t p_count) {
	PackedInt32Array indices;
	for (int64_t i = 0; i < p_count; i++) {
		indices.append(i);
	}
	return indices;
}

inline void set_simplex_normals(const Ref<ArrayCellMeshND> &p_mesh, const Vector<VectorN> &p_values) {
	p_mesh->set_normal_values(p_values);
	p_mesh->set_simplex_cell_normal_indices(sequential_indices(p_values.size()));
}

inline void set_simplex_texture_map(const Ref<ArrayCellMeshND> &p_mesh, const Vector<VectorM> &p_values) {
	p_mesh->set_texture_map_values(p_values);
	p_mesh->set_simplex_cell_texture_map_indices(sequential_indices(p_values.size()));
}

inline Vector<Vector<VectorN>> get_poly_normals(const Ref<PolyMeshND> &p_mesh) {
	return sample_cells(p_mesh->get_poly_cell_normal_indices(), p_mesh->get_poly_cell_normal_values());
}

inline Vector<Vector<VectorM>> get_poly_texture_map(const Ref<PolyMeshND> &p_mesh) {
	return sample_cells(p_mesh->get_poly_cell_texture_map_indices(), p_mesh->get_poly_cell_texture_map_values());
}

using DenseBindings = HashMap<Vector2i, Vector<Vector<VectorN>>>;

inline DenseBindings get_all_poly_bindings(const Ref<ArrayPolyMeshND> &p_mesh, const bool p_texture) {
	const HashMap<Vector2i, Vector<PackedInt32Array>> bindings = p_texture ? p_mesh->get_all_poly_cell_texture_map_indices() : p_mesh->get_all_poly_cell_normal_indices();
	const Vector<VectorN> values = p_texture ? p_mesh->get_poly_cell_texture_map_values() : p_mesh->get_poly_cell_normal_values();
	DenseBindings dense;
	for (const KeyValue<Vector2i, Vector<PackedInt32Array>> &binding : bindings) {
		dense.insert(binding.key, sample_cells(binding.value, values));
	}
	return dense;
}

inline DenseBindings get_all_poly_normals(const Ref<ArrayPolyMeshND> &p_mesh) {
	return get_all_poly_bindings(p_mesh, false);
}

inline DenseBindings get_all_poly_texture_maps(const Ref<ArrayPolyMeshND> &p_mesh) {
	return get_all_poly_bindings(p_mesh, true);
}

inline void set_all_poly_bindings(const Ref<ArrayPolyMeshND> &p_mesh, const DenseBindings &p_dense, const bool p_texture) {
	Vector<VectorN> values;
	HashMap<Vector2i, Vector<PackedInt32Array>> bindings;
	for (const KeyValue<Vector2i, Vector<Vector<VectorN>>> &binding : p_dense) {
		Vector<PackedInt32Array> cells;
		for (const Vector<VectorN> &cell : binding.value) {
			PackedInt32Array indices;
			for (const VectorN &value : cell) {
				indices.append(values.size());
				values.append(value);
			}
			cells.append(indices);
		}
		// Preserve explicitly empty keys and malformed shapes used by validator tests.
		bindings.insert(binding.key, cells);
	}
	if (p_texture) {
		p_mesh->set_poly_cell_texture_map_values(values);
		p_mesh->set_all_poly_cell_texture_map_indices(bindings);
	} else {
		p_mesh->set_poly_cell_normal_values(values);
		p_mesh->set_all_poly_cell_normal_indices(bindings);
	}
}

inline void set_all_poly_normals(const Ref<ArrayPolyMeshND> &p_mesh, const DenseBindings &p_dense) {
	set_all_poly_bindings(p_mesh, p_dense, false);
}

inline void set_all_poly_texture_maps(const Ref<ArrayPolyMeshND> &p_mesh, const DenseBindings &p_dense) {
	set_all_poly_bindings(p_mesh, p_dense, true);
}

inline void set_poly_normals(const Ref<ArrayPolyMeshND> &p_mesh, const Vector<Vector<VectorN>> &p_dense) {
	DenseBindings bindings = get_all_poly_normals(p_mesh);
	const Vector2i key(p_mesh->get_dimension() - 1, 0);
	if (p_dense.is_empty()) {
		bindings.erase(key);
	} else {
		bindings.insert(key, p_dense);
	}
	set_all_poly_normals(p_mesh, bindings);
}

inline void set_poly_texture_map(const Ref<ArrayPolyMeshND> &p_mesh, const Vector<Vector<VectorM>> &p_dense) {
	DenseBindings bindings = get_all_poly_texture_maps(p_mesh);
	const Vector2i key(p_mesh->get_dimension() - 1, 0);
	if (p_dense.is_empty()) {
		bindings.erase(key);
	} else {
		bindings.insert(key, p_dense);
	}
	set_all_poly_texture_maps(p_mesh, bindings);
}
} // namespace TestMeshDataND
