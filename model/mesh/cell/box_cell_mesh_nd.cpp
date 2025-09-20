#include "box_cell_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "../wire/box_wire_mesh_nd.h"

#include <algorithm>

void BoxCellMeshND::_clear_caches() {
	_cell_indices_cache.clear();
	_vertices_cache.clear();
	cell_mesh_clear_cache();
}

VectorN BoxCellMeshND::get_half_extents() const {
	return VectorND::multiply_scalar(_size, 0.5);
}

void BoxCellMeshND::set_half_extents(const VectorN &p_half_extents) {
	ERR_FAIL_COND_MSG(p_half_extents.size() > 10, "BoxCellMeshND: Too many dimensions for cell-based box.");
	_size = VectorND::multiply_scalar(p_half_extents, 2.0);
	_clear_caches();
}

VectorN BoxCellMeshND::get_size() const {
	return _size;
}

void BoxCellMeshND::set_size(const VectorN &p_size) {
	ERR_FAIL_COND_MSG(p_size.size() > 10, "BoxCellMeshND: Too many dimensions for cell-based box.");
	_size = p_size;
	_clear_caches();
}

void BoxCellMeshND::set_dimension(int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "BoxCellMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 10, "BoxCellMeshND: Too many dimensions for cell-based box.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

void BoxCellMeshND::set_polytope_cells(const bool p_polytope_cells) {
	_polytope_cells = p_polytope_cells;
	_clear_caches();
}

int64_t BoxCellMeshND::_factorial(const int64_t p_dimension) {
	int64_t result = 1;
	for (int64_t i = 2; i <= p_dimension; i++) {
		result *= i;
	}
	return result;
}

void BoxCellMeshND::_write_gray_path(const int64_t p_dimension, const int64_t p_index, const int64_t *p_bit_order, PackedInt32Array &r_out) {
	const int64_t offset = p_index * (p_dimension + 1);
	int64_t value = 0;
	r_out.set(offset, value);
	for (int64_t i = 0; i < p_dimension; ++i) {
		value ^= (int64_t(1) << int64_t(p_bit_order[i]));
		r_out.set(offset + i + 1, value);
	}
}

void BoxCellMeshND::_generate_cell_indices_lex(const int64_t p_dimension, PackedInt32Array &r_out) {
	int64_t bits[30];
	for (int64_t i = 0; i < p_dimension; ++i) {
		bits[i] = i;
	}

	uint64_t index = 0;
	do {
		_write_gray_path(p_dimension, index++, bits, r_out);
	} while (std::next_permutation(bits, bits + p_dimension));
}

int BoxCellMeshND::get_cell_count() {
	return _factorial(get_dimension());
}

int BoxCellMeshND::get_indices_per_cell() {
	return get_dimension() + 1;
}

PackedInt32Array BoxCellMeshND::get_cell_indices() {
	if (_cell_indices_cache.is_empty()) {
		const int64_t dimension = _size.size();
		if (dimension == 0) {
			return _cell_indices_cache;
		}
		ERR_FAIL_COND_V_MSG(dimension > 10, _cell_indices_cache, "BoxCellMeshND: Too many dimensions for box cells.");
		// Note: This algorithm completely fills the middle of the box.
		_cell_indices_cache.resize(get_cell_count() * get_indices_per_cell());
		_generate_cell_indices_lex(dimension, _cell_indices_cache);
	}
	return _cell_indices_cache;
}

PackedInt32Array BoxCellMeshND::get_edge_indices() {
	const uint64_t dimension = _size.size();
	if (_edge_indices_cache.is_empty()) {
		if (_polytope_cells) {
			ERR_FAIL_COND_V_MSG(dimension > 30, _edge_indices_cache, "BoxCellMeshND: Too many dimensions for box edges.");
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
		} else {
			_edge_indices_cache = calculate_edge_indices_from_cell_indices(get_cell_indices(), dimension + 1, true);
		}
	}
	return _edge_indices_cache;
}

Vector<VectorN> BoxCellMeshND::get_vertices() {
	if (_vertices_cache.is_empty()) {
		const uint64_t dimension = _size.size();
		ERR_FAIL_COND_V_MSG(dimension > 30, _vertices_cache, "BoxCellMeshND: Too many dimensions for box vertices.");
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

Ref<BoxCellMeshND> BoxCellMeshND::from_box_wire_mesh(const Ref<BoxWireMeshND> &p_wire_mesh) {
	Ref<BoxCellMeshND> cell_mesh;
	cell_mesh.instantiate();
	cell_mesh->set_size(p_wire_mesh->get_size());
	cell_mesh->set_material(p_wire_mesh->get_material());
	return cell_mesh;
}

Ref<BoxWireMeshND> BoxCellMeshND::to_box_wire_mesh() const {
	Ref<BoxWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<WireMeshND> BoxCellMeshND::to_wire_mesh() {
	return to_box_wire_mesh();
}

void BoxCellMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &BoxCellMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,10,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_half_extents"), &BoxCellMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &BoxCellMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &BoxCellMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &BoxCellMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");

	ClassDB::bind_method(D_METHOD("get_polytope_cells"), &BoxCellMeshND::get_polytope_cells);
	ClassDB::bind_method(D_METHOD("set_polytope_cells", "polytope_cells"), &BoxCellMeshND::set_polytope_cells);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "polytope_cells"), "set_polytope_cells", "get_polytope_cells");

	ClassDB::bind_static_method("BoxCellMeshND", D_METHOD("from_box_wire_mesh", "wire_mesh"), &BoxCellMeshND::from_box_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_box_wire_mesh"), &BoxCellMeshND::to_box_wire_mesh);
}
