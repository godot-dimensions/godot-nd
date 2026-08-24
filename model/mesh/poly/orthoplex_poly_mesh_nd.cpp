#include "orthoplex_poly_mesh_nd.h"

#include "../../../math/vector_nd.h"
#include "../cell/orthoplex_cell_mesh_nd.h"
#include "../wire/orthoplex_wire_mesh_nd.h"

void OrthoplexPolyMeshND::_clear_caches() {
	_poly_cell_indices_cache.clear();
	_orthoplex_edge_indices_cache.clear();
	_boundary_normals_cache.clear();
	_vertex_normals_cache.clear();
	_texture_map_cache.clear();
	_vertices_cache.clear();
	poly_mesh_clear_cache();
}

VectorN OrthoplexPolyMeshND::get_half_extents() const {
	return VectorND::multiply_scalar(_size, 0.5f);
}

void OrthoplexPolyMeshND::set_half_extents(const VectorN &p_half_extents) {
	_size = VectorND::multiply_scalar(p_half_extents, 2.0f);
	_clear_caches();
}

VectorN OrthoplexPolyMeshND::get_size() const {
	return _size;
}

void OrthoplexPolyMeshND::set_size(const VectorN &p_size) {
	_size = p_size;
	_clear_caches();
}

void OrthoplexPolyMeshND::set_dimension(const int p_dimension) {
	ERR_FAIL_COND_MSG(p_dimension < 0, "OrthoplexPolyMeshND: Dimension must not be negative.");
	ERR_FAIL_COND_MSG(p_dimension > 10, "OrthoplexPolyMeshND: Too many dimensions for poly-based orthoplex.");
	set_size(VectorND::with_dimension(_size, p_dimension));
}

int64_t OrthoplexPolyMeshND::_vertex_set_key(const PackedInt32Array &p_vertex_indices) {
	// Assumes the vertex indices are sorted ascending, below 32, and at most 10 of them.
	int64_t key = 0;
	for (int64_t i = 0; i < p_vertex_indices.size(); i++) {
		key = (key << 6) | int64_t(p_vertex_indices[i] + 1);
	}
	return key;
}

// Procedurally generates the N-orthoplex's poly cell hierarchy. Every k-dimensional cell
// with k < N is a simplex made of k + 1 vertices on distinct axes, one vertex per axis,
// on either the positive or negative side. Vertex index 2i is the negative side of axis i
// and vertex index 2i + 1 is the positive side, matching get_vertices.
void OrthoplexPolyMeshND::_generate_poly_data() {
	_poly_cell_indices_cache.clear();
	_orthoplex_edge_indices_cache.clear();
	_boundary_normals_cache.clear();
	_vertex_normals_cache.clear();
	const int64_t dimension = _size.size();
	if (dimension < 1) {
		return;
	}
	if (dimension == 1) {
		// A 1D orthoplex is a line segment.
		_orthoplex_edge_indices_cache.append(0);
		_orthoplex_edge_indices_cache.append(1);
		return;
	}
	HashMap<int64_t, int32_t> prev_level_map;
	// Level k, for k up to N-1, enumerates every combination of k + 1 distinct axes and a
	// positive or negative sign for each. Level 1 is the edges, stored in the flat edge array.
	for (int64_t level_dimension = 1; level_dimension < dimension; level_dimension++) {
		const int64_t vertices_per_cell = level_dimension + 1;
		HashMap<int64_t, int32_t> level_map;
		Vector<PackedInt32Array> level_cells;
		const bool is_boundary_level = level_dimension == dimension - 1;
		// Enumerate axis combinations in ascending lexicographic order.
		PackedInt32Array axis_combo;
		axis_combo.resize(vertices_per_cell);
		for (int64_t i = 0; i < vertices_per_cell; i++) {
			axis_combo.set(i, (int32_t)i);
		}
		while (true) {
			const int64_t sign_combo_count = int64_t(1) << vertices_per_cell;
			for (int64_t sign_combo = 0; sign_combo < sign_combo_count; sign_combo++) {
				PackedInt32Array cell_vertices;
				cell_vertices.resize(vertices_per_cell);
				for (int64_t i = 0; i < vertices_per_cell; i++) {
					cell_vertices.set(i, axis_combo[i] * 2 + ((sign_combo >> i) & 1));
				}
				if (level_dimension == 1) {
					const int32_t edge_index = _orthoplex_edge_indices_cache.size() / 2;
					_orthoplex_edge_indices_cache.append(cell_vertices[0]);
					_orthoplex_edge_indices_cache.append(cell_vertices[1]);
					level_map[_vertex_set_key(cell_vertices)] = edge_index;
					continue;
				}
				PackedInt32Array cell;
				if (level_dimension == 2) {
					// 2D faces need their edges in a continuous closed loop order.
					cell.append(prev_level_map[_vertex_set_key(PackedInt32Array{ cell_vertices[0], cell_vertices[1] })]);
					cell.append(prev_level_map[_vertex_set_key(PackedInt32Array{ cell_vertices[1], cell_vertices[2] })]);
					cell.append(prev_level_map[_vertex_set_key(PackedInt32Array{ cell_vertices[0], cell_vertices[2] })]);
				} else {
					// Higher-dimensional simplex cells list their sub-simplexes, dropping one
					// vertex each. The first two members drop different vertices, so they
					// share a common ridge, encoding the orientation.
					for (int64_t drop_index = 0; drop_index < vertices_per_cell; drop_index++) {
						PackedInt32Array member_vertices;
						member_vertices.resize(vertices_per_cell - 1);
						int64_t member_position = 0;
						for (int64_t i = 0; i < vertices_per_cell; i++) {
							if (i == drop_index) {
								continue;
							}
							member_vertices.set(member_position, cell_vertices[i]);
							member_position++;
						}
						cell.append(prev_level_map[_vertex_set_key(member_vertices)]);
					}
				}
				level_map[_vertex_set_key(cell_vertices)] = (int32_t)level_cells.size();
				if (is_boundary_level) {
					// The boundary cell touches every axis, and its nominal normal is the
					// normalized diagonal of the signs, matching the sign of each vertex.
					VectorN normal;
					normal.resize(dimension);
					const double component = 1.0 / Math::sqrt((double)dimension);
					for (int64_t i = 0; i < dimension; i++) {
						normal.set(i, ((sign_combo >> i) & 1) ? component : -component);
					}
					_boundary_normals_cache.append(normal);
				}
				level_cells.append(cell);
			}
			// Advance to the next axis combination.
			int64_t position = vertices_per_cell - 1;
			while (position >= 0 && axis_combo[position] == dimension - (vertices_per_cell - position)) {
				position--;
			}
			if (position < 0) {
				break;
			}
			axis_combo.set(position, axis_combo[position] + 1);
			for (int64_t i = position + 1; i < vertices_per_cell; i++) {
				axis_combo.set(i, axis_combo[i - 1] + 1);
			}
		}
		if (level_dimension >= 2) {
			_poly_cell_indices_cache.append(level_cells);
		}
		prev_level_map = level_map;
	}
	if (dimension == 2) {
		// A 2D orthoplex is a single quadrilateral face made of the 4 edges in a closed loop.
		PackedInt32Array face;
		face.append(prev_level_map[_vertex_set_key(PackedInt32Array{ 0, 2 })]);
		face.append(prev_level_map[_vertex_set_key(PackedInt32Array{ 1, 2 })]);
		face.append(prev_level_map[_vertex_set_key(PackedInt32Array{ 1, 3 })]);
		face.append(prev_level_map[_vertex_set_key(PackedInt32Array{ 0, 3 })]);
		Vector<PackedInt32Array> face_level;
		face_level.append(face);
		_poly_cell_indices_cache.append(face_level);
		return;
	}
	// The whole N-orthoplex is a single volumetric cell made of all of the boundary cells.
	// The first two boundary cells differ only in the sign of the first axis, so they share
	// the common sub-simplex that drops the first axis.
	const int64_t boundary_cell_count = int64_t(1) << dimension;
	PackedInt32Array volumetric_cell;
	volumetric_cell.resize(boundary_cell_count);
	for (int64_t i = 0; i < boundary_cell_count; i++) {
		volumetric_cell.set(i, (int32_t)i);
	}
	Vector<PackedInt32Array> volumetric_level;
	volumetric_level.append(volumetric_cell);
	_poly_cell_indices_cache.append(volumetric_level);
	// Orient the boundary cells so that their orientation-derived normals point outward.
	_orient_cells_to_match_normals(_poly_cell_indices_cache, _orthoplex_edge_indices_cache, get_vertices(), _boundary_normals_cache, dimension - 3);
	// Flat shading vertex normals: every vertex instance of a cell uses the cell's normal.
	_vertex_normals_cache.resize(_boundary_normals_cache.size());
	for (int64_t cell_index = 0; cell_index < _boundary_normals_cache.size(); cell_index++) {
		Vector<VectorN> vertex_normals_for_cell;
		vertex_normals_for_cell.resize(dimension);
		for (int64_t vert_inst = 0; vert_inst < dimension; vert_inst++) {
			vertex_normals_for_cell.set(vert_inst, _boundary_normals_cache[cell_index]);
		}
		_vertex_normals_cache.set(cell_index, vertex_normals_for_cell);
	}
}

Vector<Vector<PackedInt32Array>> OrthoplexPolyMeshND::get_poly_cell_indices() {
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _poly_cell_indices_cache;
}

Vector<VectorN> OrthoplexPolyMeshND::get_poly_cell_vertices() {
	return get_vertices();
}

Vector<VectorN> OrthoplexPolyMeshND::get_poly_cell_boundary_normals() {
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _boundary_normals_cache;
}

Vector<Vector<VectorN>> OrthoplexPolyMeshND::get_poly_cell_vertex_normals() {
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _vertex_normals_cache;
}

// Procedurally generates the texture map for the orthoplex's boundary cells. The vertex of
// the positive last axis maps to the center of the texture space (0.5 on every axis), and
// the vertices of the other axes map to points offset by half a unit from the center on the
// corresponding texture axis, so the cells on the positive last axis side form a cross-polytope
// of simplexes around the center. The negative last axis vertex of each remaining cell maps to
// the center reflected over the hyperplane through the cell's other texture points, giving it
// many disconnected texture representations, one adjacent to each positive side cell.
void OrthoplexPolyMeshND::_generate_texture_map() {
	_texture_map_cache.clear();
	const int64_t dimension = _size.size();
	if (dimension < 3) {
		// Texture maps bind to boundary poly cells, which a 2D or lower orthoplex does not have.
		return;
	}
	if (_poly_cell_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	const int64_t texture_dimension = dimension - 1;
	const int64_t last_axis = dimension - 1;
	const Vector<PackedInt32Array> cell_vertex_indices = _get_vertex_indices_of_boundary_cells(_poly_cell_indices_cache, _orthoplex_edge_indices_cache, dimension - 3, false);
	const int64_t cell_count = cell_vertex_indices.size();
	ERR_FAIL_COND(_boundary_normals_cache.size() != cell_count);
	_texture_map_cache.resize(cell_count);
	for (int64_t cell_index = 0; cell_index < cell_count; cell_index++) {
		// The signs of the cell's vertices on each axis, read from its outward normal.
		const VectorN &cell_normal = _boundary_normals_cache[cell_index];
		const PackedInt32Array &cell_vertices = cell_vertex_indices[cell_index];
		Vector<VectorM> cell_texture_map;
		cell_texture_map.resize(cell_vertices.size());
		for (int64_t vertex_number = 0; vertex_number < cell_vertices.size(); vertex_number++) {
			// Vertex index 2i is the negative side of axis i, and 2i + 1 is the positive side.
			const int64_t vertex_index = cell_vertices[vertex_number];
			const int64_t vertex_axis = vertex_index / 2;
			const bool vertex_positive = vertex_index % 2 == 1;
			VectorM texcoord = VectorND::fill(texture_dimension, 0.5);
			if (vertex_axis == last_axis) {
				if (!vertex_positive) {
					// The center reflected over the hyperplane through the cell's other points.
					for (int64_t tex_axis = 0; tex_axis < texture_dimension; tex_axis++) {
						const double sign = cell_normal[tex_axis] > 0.0 ? 1.0 : -1.0;
						texcoord.set(tex_axis, 0.5 + sign / (double)texture_dimension);
					}
				}
			} else {
				texcoord.set(vertex_axis, vertex_positive ? 1.0 : 0.0);
			}
			cell_texture_map.set(vertex_number, texcoord);
		}
		_texture_map_cache.set(cell_index, cell_texture_map);
	}
}

Vector<Vector<VectorM>> OrthoplexPolyMeshND::get_poly_cell_texture_map() {
	if (_texture_map_cache.is_empty()) {
		_generate_texture_map();
	}
	return _texture_map_cache;
}

PackedInt32Array OrthoplexPolyMeshND::get_edge_indices() {
	if (_poly_cell_indices_cache.is_empty() && _orthoplex_edge_indices_cache.is_empty()) {
		_generate_poly_data();
	}
	return _orthoplex_edge_indices_cache;
}

Vector<VectorN> OrthoplexPolyMeshND::get_vertices() {
	if (_vertices_cache.is_empty()) {
		const VectorN he = get_half_extents();
		for (int i = 0; i < _size.size(); i++) {
			_vertices_cache.append(VectorND::value_on_axis_with_dimension(-he[i], i, _size.size()));
			_vertices_cache.append(VectorND::value_on_axis_with_dimension(+he[i], i, _size.size()));
		}
	}
	return _vertices_cache;
}

Ref<OrthoplexPolyMeshND> OrthoplexPolyMeshND::from_orthoplex_cell_mesh(const Ref<OrthoplexCellMeshND> &p_cell_mesh) {
	Ref<OrthoplexPolyMeshND> poly_mesh;
	poly_mesh.instantiate();
	poly_mesh->set_size(p_cell_mesh->get_size());
	poly_mesh->set_material(p_cell_mesh->get_material());
	return poly_mesh;
}

Ref<OrthoplexPolyMeshND> OrthoplexPolyMeshND::from_orthoplex_wire_mesh(const Ref<OrthoplexWireMeshND> &p_wire_mesh) {
	Ref<OrthoplexPolyMeshND> poly_mesh;
	poly_mesh.instantiate();
	poly_mesh->set_size(p_wire_mesh->get_size());
	poly_mesh->set_material(p_wire_mesh->get_material());
	return poly_mesh;
}

Ref<OrthoplexCellMeshND> OrthoplexPolyMeshND::to_orthoplex_cell_mesh() const {
	Ref<OrthoplexCellMeshND> cell_mesh;
	cell_mesh.instantiate();
	cell_mesh->set_size(_size);
	cell_mesh->set_material(get_material());
	return cell_mesh;
}

Ref<OrthoplexWireMeshND> OrthoplexPolyMeshND::to_orthoplex_wire_mesh() const {
	Ref<OrthoplexWireMeshND> wire_mesh;
	wire_mesh.instantiate();
	wire_mesh->set_size(_size);
	wire_mesh->set_material(get_material());
	return wire_mesh;
}

Ref<CellMeshND> OrthoplexPolyMeshND::to_cell_mesh() {
	return to_orthoplex_cell_mesh();
}

Ref<WireMeshND> OrthoplexPolyMeshND::to_wire_mesh() {
	return to_orthoplex_wire_mesh();
}

void OrthoplexPolyMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &OrthoplexPolyMeshND::set_dimension);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dimension", PROPERTY_HINT_RANGE, "0,10,1", PROPERTY_USAGE_EDITOR), "set_dimension", "get_dimension");

	ClassDB::bind_method(D_METHOD("get_half_extents"), &OrthoplexPolyMeshND::get_half_extents);
	ClassDB::bind_method(D_METHOD("set_half_extents", "half_extents"), &OrthoplexPolyMeshND::set_half_extents);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "half_extents", PROPERTY_HINT_NONE, "suffix:m", PROPERTY_USAGE_NONE), "set_half_extents", "get_half_extents");

	ClassDB::bind_method(D_METHOD("get_size"), &OrthoplexPolyMeshND::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &OrthoplexPolyMeshND::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");

	ClassDB::bind_static_method("OrthoplexPolyMeshND", D_METHOD("from_orthoplex_cell_mesh", "cell_mesh"), &OrthoplexPolyMeshND::from_orthoplex_cell_mesh);
	ClassDB::bind_static_method("OrthoplexPolyMeshND", D_METHOD("from_orthoplex_wire_mesh", "wire_mesh"), &OrthoplexPolyMeshND::from_orthoplex_wire_mesh);
	ClassDB::bind_method(D_METHOD("to_orthoplex_cell_mesh"), &OrthoplexPolyMeshND::to_orthoplex_cell_mesh);
	ClassDB::bind_method(D_METHOD("to_orthoplex_wire_mesh"), &OrthoplexPolyMeshND::to_orthoplex_wire_mesh);
}
