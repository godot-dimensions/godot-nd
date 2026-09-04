#include "box_poly_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "../wire/box_wire_mesh_nd.h"

void BoxPolyMeshND::_clear_caches() {
	_poly_cell_indices_cache.clear();
	_box_edge_indices_cache.clear();
	_boundary_normals_cache.clear();
	_vertex_normals_cache.clear();
	_texture_map_cache.clear();
	_vertices_cache.clear();
	poly_mesh_clear_cache();
}

void BoxPolyMeshND::set_poly_texture_map(const BoxPolyTextureMap p_map) {
	_poly_texture_map = p_map;
	// The position caches can be kept, but the texture map and poly caches need clearing.
	_texture_map_cache.clear();
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
	ERR_FAIL_COND_MSG(p_dimension > 10, "BoxPolyMeshND: Too many dimensions for poly-based box.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

// Procedurally generates the N-cube's poly cell hierarchy. Each k-dimensional cell of the
// N-cube is identified by a set of k "free" axes and a fixed +/- bit for each other axis.
// Vertex indices are bitmasks where bit i set means the +X side of axis i, matching get_vertex_positions.
void BoxPolyMeshND::_generate_poly_data() {
	_poly_cell_indices_cache.clear();
	_box_edge_indices_cache.clear();
	_boundary_normals_cache.clear();
	_vertex_normals_cache.clear();
	const int64_t dimension = _size.size();
	if (dimension < 1) {
		return;
	}
	// Cells are identified by keys packing the free axis mask and the fixed vertex bits.
	HashMap<int64_t, int32_t> prev_level_map;
	// Level k == 1: Edges, along one free axis, with all other axes fixed.
	for (int64_t free_axis = 0; free_axis < dimension; free_axis++) {
		const int64_t free_mask = int64_t(1) << free_axis;
		// Enumerate every combination of fixed bits over the non-free axes.
		PackedInt32Array non_free_axes;
		for (int64_t axis = 0; axis < dimension; axis++) {
			if (axis != free_axis) {
				non_free_axes.append((int32_t)axis);
			}
		}
		const int64_t combo_count = int64_t(1) << non_free_axes.size();
		for (int64_t combo = 0; combo < combo_count; combo++) {
			int64_t fixed_bits = 0;
			for (int64_t i = 0; i < non_free_axes.size(); i++) {
				if (combo & (int64_t(1) << i)) {
					fixed_bits |= int64_t(1) << non_free_axes[i];
				}
			}
			const int32_t edge_index = _box_edge_indices_cache.size() / 2;
			_box_edge_indices_cache.append((int32_t)fixed_bits);
			_box_edge_indices_cache.append((int32_t)(fixed_bits | free_mask));
			prev_level_map[(free_mask << 32) | fixed_bits] = edge_index;
		}
	}
	// Levels k == 2 and up: Cells made of the cells of the level below.
	for (int64_t level_dimension = 2; level_dimension <= dimension; level_dimension++) {
		HashMap<int64_t, int32_t> level_map;
		Vector<PackedInt32Array> level_cells;
		const bool is_boundary_level = level_dimension == dimension - 1;
		for (int64_t free_mask = 0; free_mask < (int64_t(1) << dimension); free_mask++) {
			int64_t free_axis_count = 0;
			PackedInt32Array free_axes;
			PackedInt32Array non_free_axes;
			for (int64_t axis = 0; axis < dimension; axis++) {
				if (free_mask & (int64_t(1) << axis)) {
					free_axis_count++;
					free_axes.append((int32_t)axis);
				} else {
					non_free_axes.append((int32_t)axis);
				}
			}
			if (free_axis_count != level_dimension) {
				continue;
			}
			const int64_t combo_count = int64_t(1) << non_free_axes.size();
			for (int64_t combo = 0; combo < combo_count; combo++) {
				int64_t fixed_bits = 0;
				for (int64_t i = 0; i < non_free_axes.size(); i++) {
					if (combo & (int64_t(1) << i)) {
						fixed_bits |= int64_t(1) << non_free_axes[i];
					}
				}
				PackedInt32Array cell;
				if (level_dimension == 2) {
					// 2D faces need their edges in a continuous closed loop order.
					const int64_t axis_a_mask = int64_t(1) << free_axes[0];
					const int64_t axis_b_mask = int64_t(1) << free_axes[1];
					cell.append(prev_level_map[(axis_a_mask << 32) | fixed_bits]);
					cell.append(prev_level_map[(axis_b_mask << 32) | (fixed_bits | axis_a_mask)]);
					cell.append(prev_level_map[(axis_a_mask << 32) | (fixed_bits | axis_b_mask)]);
					cell.append(prev_level_map[(axis_b_mask << 32) | fixed_bits]);
				} else {
					// Higher-dimensional cells list their negative-side members, then their
					// positive-side members. The first two members drop different axes on the
					// negative side, so they share a common ridge, encoding the orientation.
					for (int64_t i = 0; i < free_axes.size(); i++) {
						const int64_t member_free_mask = free_mask & ~(int64_t(1) << free_axes[i]);
						cell.append(prev_level_map[(member_free_mask << 32) | fixed_bits]);
					}
					for (int64_t i = 0; i < free_axes.size(); i++) {
						const int64_t member_free_mask = free_mask & ~(int64_t(1) << free_axes[i]);
						cell.append(prev_level_map[(member_free_mask << 32) | (fixed_bits | (int64_t(1) << free_axes[i]))]);
					}
				}
				level_map[(free_mask << 32) | fixed_bits] = (int32_t)level_cells.size();
				if (is_boundary_level) {
					// The boundary cell fixes exactly one axis, and its normal points along it.
					const int64_t fixed_axis = non_free_axes[0];
					const double sign = (fixed_bits & (int64_t(1) << fixed_axis)) ? 1.0 : -1.0;
					_boundary_normals_cache.append(VectorND::value_on_axis_with_dimension(sign, fixed_axis, dimension));
				}
				level_cells.append(cell);
			}
		}
		_poly_cell_indices_cache.append(level_cells);
		prev_level_map = level_map;
	}
	// Orient the boundary cells so that their orientation-derived normals point outward.
	if (dimension >= 3) {
		_orient_cells_to_match_normals(_poly_cell_indices_cache, _box_edge_indices_cache, get_vertex_positions(), _boundary_normals_cache, dimension - 3);
		// Flat shading vertex normals: every vertex instance of a cell uses the cell's normal.
		const int64_t boundary_cell_vertex_count = int64_t(1) << (dimension - 1);
		_vertex_normals_cache.resize(_boundary_normals_cache.size());
		for (int64_t cell_index = 0; cell_index < _boundary_normals_cache.size(); cell_index++) {
			Vector<VectorN> vertex_normals_for_cell;
			vertex_normals_for_cell.resize(boundary_cell_vertex_count);
			for (int64_t vert_inst = 0; vert_inst < boundary_cell_vertex_count; vert_inst++) {
				vertex_normals_for_cell.set(vert_inst, _boundary_normals_cache[cell_index]);
			}
			_vertex_normals_cache.set(cell_index, vertex_normals_for_cell);
		}
	}
}

Vector<Vector<PackedInt32Array>> BoxPolyMeshND::get_poly_cell_indices() {
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _poly_cell_indices_cache;
}

Vector<VectorN> BoxPolyMeshND::get_poly_cell_vertex_positions() {
	return get_vertex_positions();
}

Vector<VectorN> BoxPolyMeshND::get_poly_cell_boundary_normals() {
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _boundary_normals_cache;
}

Vector<Vector<VectorN>> BoxPolyMeshND::get_poly_cell_vertex_normals() {
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _vertex_normals_cache;
}

// Procedurally generates the texture map for the box's boundary cells, by unfolding the
// N-cube's boundary into a cross shape in the (N-1)-dimensional texture space, like the
// classic cross unfolding of a cube or tesseract. The cell facing the positive last axis
// sits in the center of the cross, the cells of the other axes tile around it, unfolded
// so that shared vertices with the center cell have identical texture coordinates, and
// the cell facing the negative last axis is a special case placed depending on the mode.
void BoxPolyMeshND::_generate_texture_map() {
	_texture_map_cache.clear();
	const int64_t dimension = _size.size();
	if (dimension < 3) {
		// Texture maps bind to boundary poly cells, which a 2D or lower box does not have.
		return;
	}
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	const int64_t texture_dimension = dimension - 1;
	const int64_t last_axis = dimension - 1;
	// The ranges of the center box per texture axis, and of the negative last axis cell.
	// The side cells tile next to the center box with the same size as the center box.
	VectorM center_lo;
	VectorM center_hi;
	VectorM negative_lo;
	VectorM negative_hi;
	if (_poly_texture_map == BOX_POLY_TEXTURE_MAP_LONG_CROSS) {
		center_lo = VectorND::fill(texture_dimension, 1.0 / 3.0);
		center_hi = VectorND::fill(texture_dimension, 2.0 / 3.0);
		negative_lo = VectorND::fill(texture_dimension, 1.0 / 3.0);
		negative_hi = VectorND::fill(texture_dimension, 2.0 / 3.0);
		center_lo.set(0, 0.25);
		center_hi.set(0, 0.5);
		negative_lo.set(0, 0.75);
		negative_hi.set(0, 1.0);
	} else {
		center_lo = VectorND::fill(texture_dimension, 0.3);
		center_hi = VectorND::fill(texture_dimension, 0.6);
		negative_lo = VectorND::fill(texture_dimension, 0.7);
		negative_hi = VectorND::fill(texture_dimension, 1.0);
	}
	const Vector<PackedInt32Array> cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices_cache, _box_edge_indices_cache, dimension - 3, false);
	const int64_t cell_count = cell_vertex_indices.size();
	ERR_FAIL_COND(_boundary_normals_cache.size() != cell_count);
	_texture_map_cache.resize(cell_count);
	for (int64_t cell_index = 0; cell_index < cell_count; cell_index++) {
		// Determine which axis this cell faces from its outward normal.
		const VectorN &cell_normal = _boundary_normals_cache[cell_index];
		int64_t cell_axis = -1;
		bool cell_positive = false;
		for (int64_t axis = 0; axis < dimension; axis++) {
			if (cell_normal[axis] != 0.0) {
				cell_axis = axis;
				cell_positive = cell_normal[axis] > 0.0;
				break;
			}
		}
		ERR_FAIL_COND(cell_axis == -1);
		const PackedInt32Array &cell_vertices = cell_vertex_indices[cell_index];
		Vector<VectorM> cell_texture_map;
		cell_texture_map.resize(cell_vertices.size());
		for (int64_t vertex_number = 0; vertex_number < cell_vertices.size(); vertex_number++) {
			// Vertex indices are bitmasks where bit i set means the positive side of axis i.
			const int64_t vertex_bits = cell_vertices[vertex_number];
			const bool last_bit = (vertex_bits >> last_axis) & 1;
			VectorM texcoord = VectorND::fill(texture_dimension, 0.0);
			if (_poly_texture_map == BOX_POLY_TEXTURE_MAP_FILL_EACH_SIDE) {
				// Each cell fills the whole texture space, with the same orientations as the
				// cross unfolding: side cells carry the last world axis on the texture axis
				// they face, and the negative last cell is mirrored on every axis.
				for (int64_t tex_axis = 0; tex_axis < texture_dimension; tex_axis++) {
					const bool bit = (vertex_bits >> tex_axis) & 1;
					if (cell_axis == last_axis) {
						texcoord.set(tex_axis, bit == cell_positive ? 1.0 : 0.0);
					} else if (tex_axis == cell_axis) {
						texcoord.set(tex_axis, last_bit == cell_positive ? 0.0 : 1.0);
					} else {
						texcoord.set(tex_axis, bit ? 1.0 : 0.0);
					}
				}
			} else if (cell_axis == last_axis) {
				if (cell_positive) {
					// The positive last axis cell is the center box of the cross.
					for (int64_t tex_axis = 0; tex_axis < texture_dimension; tex_axis++) {
						const bool bit = (vertex_bits >> tex_axis) & 1;
						texcoord.set(tex_axis, bit ? center_hi[tex_axis] : center_lo[tex_axis]);
					}
				} else {
					// The negative last axis cell is a special case, mirrored on the first axis.
					// For the cross and island mode this is a disconnected island in the corner,
					// while for the long cross mode this connects to the positive first axis cell.
					for (int64_t tex_axis = 0; tex_axis < texture_dimension; tex_axis++) {
						const bool bit = (vertex_bits >> tex_axis) & 1;
						if (tex_axis == 0) {
							texcoord.set(tex_axis, bit ? negative_lo[tex_axis] : negative_hi[tex_axis]);
						} else {
							texcoord.set(tex_axis, bit ? negative_hi[tex_axis] : negative_lo[tex_axis]);
						}
					}
				}
			} else {
				// Side cells tile next to the center box, unfolded across the shared face, so
				// the last world axis maps onto the texture axis that the cell faces, pointing
				// away from the center box.
				for (int64_t tex_axis = 0; tex_axis < texture_dimension; tex_axis++) {
					const bool bit = (vertex_bits >> tex_axis) & 1;
					if (tex_axis == cell_axis) {
						const double tile_size = center_hi[tex_axis] - center_lo[tex_axis];
						if (cell_positive) {
							texcoord.set(tex_axis, last_bit ? center_hi[tex_axis] : center_hi[tex_axis] + tile_size);
						} else {
							texcoord.set(tex_axis, last_bit ? center_lo[tex_axis] : center_lo[tex_axis] - tile_size);
						}
					} else {
						texcoord.set(tex_axis, bit ? center_hi[tex_axis] : center_lo[tex_axis]);
					}
				}
			}
			cell_texture_map.set(vertex_number, texcoord);
		}
		_texture_map_cache.set(cell_index, cell_texture_map);
	}
}

Vector<Vector<VectorM>> BoxPolyMeshND::get_poly_cell_texture_map() {
	if (_texture_map_cache.is_empty()) {
		_generate_texture_map();
	}
	return _texture_map_cache;
}

PackedInt32Array BoxPolyMeshND::get_edge_indices() {
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _box_edge_indices_cache;
}

Vector<VectorN> BoxPolyMeshND::get_vertex_positions() {
	if (_vertices_cache.is_empty()) {
		const VectorN he = get_half_extents();
		const int64_t dimension = _size.size();
		const int64_t vertex_count = int64_t(1) << dimension;
		_vertices_cache.resize(vertex_count);
		for (int64_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
			VectorN vertex;
			vertex.resize(dimension);
			for (int64_t dim_index = 0; dim_index < dimension; dim_index++) {
				const double coord = (vertex_index & (int64_t(1) << dim_index)) ? he[dim_index] : -he[dim_index];
				vertex.set(dim_index, coord);
			}
			_vertices_cache.set(vertex_index, vertex);
		}
	}
	return _vertices_cache;
}

Ref<BoxPolyMeshND> BoxPolyMeshND::from_box_wire_mesh(const Ref<BoxWireMeshND> &p_wire_mesh) {
	Ref<BoxPolyMeshND> poly_mesh;
	poly_mesh.instantiate();
	poly_mesh->set_size(p_wire_mesh->get_size());
	poly_mesh->set_material(p_wire_mesh->get_material());
	return poly_mesh;
}

Ref<BoxWireMeshND> BoxPolyMeshND::to_box_wire_mesh() const {
	Ref<BoxWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
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

	ClassDB::bind_method(D_METHOD("get_poly_texture_map"), &BoxPolyMeshND::get_poly_texture_map);
	ClassDB::bind_method(D_METHOD("set_poly_texture_map", "texture_map"), &BoxPolyMeshND::set_poly_texture_map);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "poly_texture_map", PROPERTY_HINT_ENUM, "Cross and Island,Fill Each Side,Long Cross"), "set_poly_texture_map", "get_poly_texture_map");

	ClassDB::bind_static_method("BoxPolyMeshND", D_METHOD("from_box_wire_mesh", "wire_mesh"), &BoxPolyMeshND::from_box_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_box_wire_mesh"), &BoxPolyMeshND::to_box_wire_mesh);

	BIND_ENUM_CONSTANT(BOX_POLY_TEXTURE_MAP_CROSS_ISLAND);
	BIND_ENUM_CONSTANT(BOX_POLY_TEXTURE_MAP_FILL_EACH_SIDE);
	BIND_ENUM_CONSTANT(BOX_POLY_TEXTURE_MAP_LONG_CROSS);
}
