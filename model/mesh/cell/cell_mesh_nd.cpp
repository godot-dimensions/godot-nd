#include "cell_mesh_nd.h"

#include "../../../math/plane_nd.h"
#include "../../../math/vector_nd.h"
#include "array_cell_mesh_nd.h"
#include "cell_material_nd.h"

int64_t CellMeshND::_binomial_coefficient(const int64_t n, const int64_t k) {
	if (k < 0 || k > n) {
		return 0;
	}
	if (k == 0 || k == n) {
		return 1;
	}
	int64_t result = 1;
	for (int64_t i = 1; i <= k; i++) {
		result *= n - (k - i);
		result /= i;
	}
	return result;
}

// Internal helper function used only by `_generate_combinations`.
void CellMeshND::_generate_combinations_recursive(const PackedInt32Array &p_items, const int64_t p_count, const int64_t p_choose, const int64_t p_start, const int64_t p_depth, int &r_result_index, PackedInt32Array &r_current, Vector<PackedInt32Array> &r_result) {
	if (p_depth == p_choose) {
		r_result.set(r_result_index++, r_current);
		return;
	}
	for (int64_t i = p_start; i < p_count; i++) {
		r_current.set(p_depth, p_items[i]);
		_generate_combinations_recursive(p_items, p_count, p_choose, i + 1, p_depth + 1, r_result_index, r_current, r_result);
	}
}

Vector<PackedInt32Array> CellMeshND::_generate_combinations(const PackedInt32Array &p_items, int64_t p_choose) {
	Vector<PackedInt32Array> result;
	result.resize(_binomial_coefficient(p_items.size(), p_choose));
	int result_index = 0;
	if (p_choose > p_items.size() || p_choose < 0) {
		return result; // Invalid input, return empty.
	}
	PackedInt32Array current;
	current.resize(p_choose);
	_generate_combinations_recursive(p_items, p_items.size(), p_choose, 0, 0, result_index, current, result);
	return result;
}

// Find unique opposing faces of the cell that are not coplanar with the pivot.
Vector<PackedInt32Array> CellMeshND::_determine_opposing_faces(const Vector<VectorN> &p_vertices, const PackedInt32Array &p_poly_cell_indices_without_pivot, const int p_dimension, const int p_pivot_index, const Vector<VectorN> &p_poly_cell_normals, Vector<VectorN> &r_out_normals) {
	Vector<PackedInt32Array> combinations = _generate_combinations(p_poly_cell_indices_without_pivot, p_dimension);
	const VectorN pivot_vertex = p_vertices[p_pivot_index];
	Vector<PackedInt32Array> opposing_faces;
	for (const PackedInt32Array &combination : combinations) {
		Vector<VectorN> plane_points;
		plane_points.resize(combination.size() + p_poly_cell_normals.size());
		for (int64_t i = 0; i < combination.size(); i++) {
			plane_points.set(i, p_vertices[combination[i]]);
		}
		for (int64_t i = 0; i < p_poly_cell_normals.size(); i++) {
			plane_points.set(i + combination.size(), VectorND::add(plane_points[0], p_poly_cell_normals[i]));
		}
		const Ref<PlaneND> plane = PlaneND::from_points(plane_points);
		if (plane.is_null()) {
			continue; // Skip invalid planes.
		}
		const double pivot_distance = plane->distance_to(pivot_vertex);
		if (Math::is_zero_approx(pivot_distance)) {
			continue; // Skip coplanar faces.
		}
		const int pivot_sign = pivot_distance > 0.0 ? 1 : -1;
		bool cont = false;
		PackedInt32Array face = combination;
		for (int64_t i = 0; i < p_poly_cell_indices_without_pivot.size(); i++) {
			// Skip any indices that are already in the combination.
			if (combination.has(p_poly_cell_indices_without_pivot[i])) {
				continue;
			}
			const VectorN vertex = p_vertices[p_poly_cell_indices_without_pivot[i]];
			const double vertex_distance = plane->distance_to(vertex);
			// Any vertex that is coplanar with this simplex is part of a larger face.
			if (Math::is_zero_approx(vertex_distance)) {
				face.append(p_poly_cell_indices_without_pivot[i]);
				continue;
			}
			const int vertex_sign = vertex_distance > 0.0 ? 1 : -1;
			// If we find a vertex on the opposite side of the pivot, this face cuts through the cell.
			// We are only interested in faces on the surface of the cell, not the interior.
			if (pivot_sign != vertex_sign) {
				cont = true;
				break;
			}
		}
		if (cont) {
			continue;
		}
		face.sort();
		if (!opposing_faces.has(face)) {
			opposing_faces.append(face);
			r_out_normals.append(plane->get_normal());
		}
	}
	return opposing_faces;
}

void CellMeshND::cell_mesh_clear_cache() {
	_cell_positions_cache.clear();
	_edge_positions_cache.clear();
	_edge_indices_cache.clear();
}

void CellMeshND::validate_material_for_mesh(const Ref<MaterialND> &p_material) {
	const int dimension = get_dimension();
	const MaterialND::ColorSourceFlagsND albedo_source = p_material->get_albedo_source_flags();
	if (albedo_source & MaterialND::COLOR_SOURCE_FLAG_PER_CELL) {
		const PackedInt32Array cell_indices = get_simplex_cell_indices();
		PackedColorArray color_array = p_material->get_albedo_color_array();
		const int64_t vertices_per_cell = dimension + 1;
		const int64_t cell_count = cell_indices.size() / vertices_per_cell;
		if (color_array.size() < cell_count) {
			p_material->resize_albedo_color_array(cell_count);
		}
	}
	MeshND::validate_material_for_mesh(p_material);
}

Ref<ArrayCellMeshND> CellMeshND::to_array_cell_mesh() {
	Ref<ArrayCellMeshND> array_mesh;
	array_mesh.instantiate();
	array_mesh->set_vertices(get_vertices());
	array_mesh->set_simplex_cell_indices(get_simplex_cell_indices());
	array_mesh->set_cell_boundary_normals(get_simplex_cell_boundary_normals());
	array_mesh->set_simplex_cell_vertex_normals(get_simplex_cell_vertex_normals());
	array_mesh->set_material(get_material());
	return array_mesh;
}

int CellMeshND::get_simplex_cell_count() {
	const int dimension = get_dimension();
	const PackedInt32Array cell_indices = get_simplex_cell_indices();
	ERR_FAIL_COND_V_MSG(cell_indices.size() % dimension != 0, -1, "CellMeshND: Cell indices size must be a multiple of the dimension.");
	return cell_indices.size() / dimension;
}

int CellMeshND::get_indices_per_simplex_cell() {
	return get_dimension();
}

PackedInt32Array CellMeshND::get_simplex_cell_indices() {
	PackedInt32Array indices;
	GDVIRTUAL_CALL(_get_simplex_cell_indices, indices);
	return indices;
}

Vector<VectorN> CellMeshND::get_simplex_cell_positions() {
	if (_cell_positions_cache.is_empty()) {
		const PackedInt32Array cell_indices = get_simplex_cell_indices();
		const Vector<VectorN> vertices = get_vertices();
		const int32_t vertices_count = vertices.size();
		for (const int cell_index : cell_indices) {
			ERR_FAIL_INDEX_V(cell_index, vertices_count, _cell_positions_cache);
			_cell_positions_cache.append(vertices[cell_index]);
		}
	}
	return _cell_positions_cache;
}

Vector<VectorN> CellMeshND::get_simplex_cell_boundary_normals() {
	TypedArray<VectorN> boundary_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_boundary_normals, boundary_normals_bind);
	Vector<VectorN> boundary_normals;
	boundary_normals.resize(boundary_normals_bind.size());
	for (int i = 0; i < boundary_normals_bind.size(); i++) {
		const VectorN cell_face_normal = boundary_normals_bind[i];
		boundary_normals.set(i, cell_face_normal);
	}
	return boundary_normals;
}

Vector<VectorN> CellMeshND::get_simplex_cell_vertex_normals() {
	TypedArray<VectorN> vertex_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_vertex_normals, vertex_normals_bind);
	Vector<VectorN> vertex_normals;
	vertex_normals.resize(vertex_normals_bind.size());
	for (int i = 0; i < vertex_normals_bind.size(); i++) {
		const VectorN cell_vertex_normal = vertex_normals_bind[i];
		vertex_normals.set(i, cell_vertex_normal);
	}
	return vertex_normals;
}

TypedArray<VectorN> CellMeshND::get_simplex_cell_boundary_normals_bind() {
	TypedArray<VectorN> boundary_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_boundary_normals, boundary_normals_bind);
	if (!boundary_normals_bind.is_empty()) {
		return boundary_normals_bind;
	}
	const Vector<VectorN> boundary_normals = get_simplex_cell_boundary_normals();
	boundary_normals_bind.resize(boundary_normals.size());
	for (int i = 0; i < boundary_normals.size(); i++) {
		const VectorN &cell_face_normal = boundary_normals[i];
		boundary_normals_bind[i] = cell_face_normal;
	}
	return boundary_normals_bind;
}

TypedArray<VectorN> CellMeshND::get_simplex_cell_vertex_normals_bind() {
	TypedArray<VectorN> vertex_normals_bind;
	GDVIRTUAL_CALL(_get_simplex_cell_vertex_normals, vertex_normals_bind);
	if (!vertex_normals_bind.is_empty()) {
		return vertex_normals_bind;
	}
	const Vector<VectorN> vertex_normals = get_simplex_cell_vertex_normals();
	vertex_normals_bind.resize(vertex_normals.size());
	for (int i = 0; i < vertex_normals.size(); i++) {
		const VectorN &cell_vertex_normal = vertex_normals[i];
		vertex_normals_bind[i] = cell_vertex_normal;
	}
	return vertex_normals_bind;
}

TypedArray<VectorN> CellMeshND::get_simplex_cell_positions_bind() {
	TypedArray<VectorN> cell_positions_bind;
	const Vector<VectorN> cell_positions = get_simplex_cell_positions();
	cell_positions_bind.resize(cell_positions.size());
	for (int i = 0; i < cell_positions.size(); i++) {
		const VectorN &cell_position = cell_positions[i];
		cell_positions_bind[i] = cell_position;
	}
	return cell_positions_bind;
}

// Recursive function that decomposes a polytope cell into simplexes.
// Each simplex will be a set of p_dimension indices in the returned array,
// therefore the returned array will be a multiple of p_dimension.
// The algorithm is as follows:
// 1. Pick a pivot index from the cell indices (must be different from the last pivot).
// 2. Find unique opposing faces of the cell that are not coplanar with the pivot.
// 3. For each face, recursively call this function with the face as the new cell.
// 4. Each of the returned simplexes will have the pivot index prepended to it.
// This function has atrocious time complexity, so avoid using it on large cells or at runtime.
Vector<PackedInt32Array> CellMeshND::decompose_polytope_cell_into_simplexes(const Vector<VectorN> &p_vertices, const PackedInt32Array &p_poly_cell_indices, const int p_dimension, const int p_last_pivot, const Vector<VectorN> &p_poly_cell_normals) {
	Vector<PackedInt32Array> simplexes;
	if (p_poly_cell_indices.size() < 2) {
		return simplexes; // No simplexes can be formed.
	}
	PackedInt32Array without_pivot = p_poly_cell_indices;
	int pivot_item;
	if (p_poly_cell_indices[0] == p_last_pivot) {
		pivot_item = p_poly_cell_indices[1];
		without_pivot.remove_at(1);
	} else {
		pivot_item = p_poly_cell_indices[0];
		without_pivot.remove_at(0);
	}
	Vector<VectorN> out_normals;
	Vector<PackedInt32Array> opposing_faces = _determine_opposing_faces(p_vertices, without_pivot, p_dimension, pivot_item, p_poly_cell_normals, out_normals);
	for (int i = 0; i < opposing_faces.size(); i++) {
		PackedInt32Array face = opposing_faces[i];
		if (face.size() == p_dimension) {
			// This is a flat simplex of the next lowest dimension, just add the pivot.
			face.insert(0, pivot_item);
			simplexes.append(face);
			continue;
		}
		// This face is not a simplex, so we need to recurse.
		Vector<VectorN> boundary_normals = p_poly_cell_normals;
		boundary_normals.append(out_normals[i]);
		Vector<PackedInt32Array> lower_simplexes = decompose_polytope_cell_into_simplexes(p_vertices, face, p_dimension - 1, pivot_item, boundary_normals);
		for (PackedInt32Array &lower_simplex : lower_simplexes) {
			lower_simplex.insert(0, pivot_item);
			simplexes.append(lower_simplex);
		}
	}
	return simplexes;
}

PackedInt32Array CellMeshND::calculate_edge_indices_from_simplex_cell_indices(const PackedInt32Array &p_simplex_cell_indices, const int p_dimension, const bool p_deduplicate) {
	PackedInt32Array edge_indices;
	ERR_FAIL_COND_V_MSG(p_dimension < 1, edge_indices, "CellMeshND: Dimension must be greater than 0.");
	ERR_FAIL_COND_V_MSG(p_simplex_cell_indices.size() % p_dimension != 0, edge_indices, "CellMeshND: Simplex cell indices size must be a multiple of the dimension.");
	const int cell_count = p_simplex_cell_indices.size() / p_dimension;
	// The number of edges is the triangular number of the dimension per cell.
	const int edge_index_count = cell_count * (p_dimension * (p_dimension - 1));
	edge_indices.resize(edge_index_count);
	int edge_index = 0;
	for (int cell_index = 0; cell_index < cell_count; cell_index++) {
		const int cell_start = cell_index * p_dimension;
		for (int i = 0; i < p_dimension; i++) {
			for (int j = i + 1; j < p_dimension; j++) {
				edge_indices.set(edge_index++, p_simplex_cell_indices[cell_start + i]);
				edge_indices.set(edge_index++, p_simplex_cell_indices[cell_start + j]);
			}
		}
	}
	CRASH_COND(edge_index != edge_index_count);
	if (p_deduplicate) {
		edge_indices = deduplicate_edge_indices(edge_indices);
	}
	return edge_indices;
}

PackedInt32Array CellMeshND::get_edge_indices() {
	const int dimension = get_dimension();
	if (_edge_indices_cache.is_empty()) {
		_edge_indices_cache = calculate_edge_indices_from_simplex_cell_indices(get_simplex_cell_indices(), dimension, true);
	}
	return _edge_indices_cache;
}

Vector<VectorN> CellMeshND::get_edge_positions() {
	if (_edge_positions_cache.is_empty()) {
		const PackedInt32Array edge_indices = get_edge_indices();
		const Vector<VectorN> vertices = get_vertices();
		const int32_t vertices_count = vertices.size();
		for (const int edge_index : edge_indices) {
			ERR_FAIL_INDEX_V(edge_index, vertices_count, _edge_positions_cache);
			_edge_positions_cache.append(vertices[edge_index]);
		}
	}
	return _edge_positions_cache;
}

void CellMeshND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("cell_mesh_clear_cache"), &CellMeshND::cell_mesh_clear_cache);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_count"), &CellMeshND::get_simplex_cell_count);
	ClassDB::bind_method(D_METHOD("get_indices_per_simplex_cell"), &CellMeshND::get_indices_per_simplex_cell);
	ClassDB::bind_method(D_METHOD("to_array_cell_mesh"), &CellMeshND::to_array_cell_mesh);

	ClassDB::bind_static_method("CellMeshND", D_METHOD("calculate_edge_indices_from_simplex_cell_indices", "simplex_cell_indices", "dimension", "deduplicate"), &CellMeshND::calculate_edge_indices_from_simplex_cell_indices);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_indices"), &CellMeshND::get_simplex_cell_indices);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_boundary_normals"), &CellMeshND::get_simplex_cell_boundary_normals_bind);
	ClassDB::bind_method(D_METHOD("get_simplex_cell_vertex_normals"), &CellMeshND::get_simplex_cell_vertex_normals_bind);

	GDVIRTUAL_BIND(_get_simplex_cell_indices);
	GDVIRTUAL_BIND(_get_simplex_cell_boundary_normals);
	GDVIRTUAL_BIND(_get_simplex_cell_vertex_normals);
}
