#pragma once

#include "../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../model/mesh/cell/box_cell_mesh_nd.h"
#include "../../model/mesh/cell/orthoplex_cell_mesh_nd.h"
#include "tests/test_macros.h"

namespace TestCellMeshND {
TEST_CASE("[CellMeshND] Decompose Box Polytope Cell into Simplexes") {
	// 0D case.
	Vector<VectorN> vertices;
	PackedInt32Array cell_indices;
	Vector<PackedInt32Array> decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 0, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 0);
	// 1D case.
	vertices.append(VectorN{ 0 });
	vertices.append(VectorN{ 1 });
	cell_indices = { 0, 1 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 1, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 1);
	CHECK(decomposed[0] == PackedInt32Array{ 0, 1 });
	// 2D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0 });
	vertices.append(VectorN{ 1, 0 });
	vertices.append(VectorN{ 0, 1 });
	vertices.append(VectorN{ 1, 1 });
	cell_indices = { 0, 1, 2, 3 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 2, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 2);
	// The exact values it returns are not important, so long as they do not overlap and cover the cell.
	CHECK(decomposed[0] == PackedInt32Array{ 0, 1, 3 });
	CHECK(decomposed[1] == PackedInt32Array{ 0, 2, 3 });
	// 3D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0, 0 });
	vertices.append(VectorN{ 1, 0, 0 });
	vertices.append(VectorN{ 0, 1, 0 });
	vertices.append(VectorN{ 1, 1, 0 });
	vertices.append(VectorN{ 0, 0, 1 });
	vertices.append(VectorN{ 1, 0, 1 });
	vertices.append(VectorN{ 0, 1, 1 });
	vertices.append(VectorN{ 1, 1, 1 });
	cell_indices = { 0, 1, 2, 3, 4, 5, 6, 7 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 3, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 6);
	// 4D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0, 0, 0 });
	vertices.append(VectorN{ 1, 0, 0, 0 });
	vertices.append(VectorN{ 0, 1, 0, 0 });
	vertices.append(VectorN{ 1, 1, 0, 0 });
	vertices.append(VectorN{ 0, 0, 1, 0 });
	vertices.append(VectorN{ 1, 0, 1, 0 });
	vertices.append(VectorN{ 0, 1, 1, 0 });
	vertices.append(VectorN{ 1, 1, 1, 0 });
	vertices.append(VectorN{ 0, 0, 0, 1 });
	vertices.append(VectorN{ 1, 0, 0, 1 });
	vertices.append(VectorN{ 0, 1, 0, 1 });
	vertices.append(VectorN{ 1, 1, 0, 1 });
	vertices.append(VectorN{ 0, 0, 1, 1 });
	vertices.append(VectorN{ 1, 0, 1, 1 });
	vertices.append(VectorN{ 0, 1, 1, 1 });
	vertices.append(VectorN{ 1, 1, 1, 1 });
	cell_indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 4, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 24);
	// 5D case.
	vertices.clear();
	vertices.append(VectorN{ 0, 0, 0, 0, 0 });
	vertices.append(VectorN{ 1, 0, 0, 0, 0 });
	vertices.append(VectorN{ 0, 1, 0, 0, 0 });
	vertices.append(VectorN{ 1, 1, 0, 0, 0 });
	vertices.append(VectorN{ 0, 0, 1, 0, 0 });
	vertices.append(VectorN{ 1, 0, 1, 0, 0 });
	vertices.append(VectorN{ 0, 1, 1, 0, 0 });
	vertices.append(VectorN{ 1, 1, 1, 0, 0 });
	vertices.append(VectorN{ 0, 0, 0, 1, 0 });
	vertices.append(VectorN{ 1, 0, 0, 1, 0 });
	vertices.append(VectorN{ 0, 1, 0, 1, 0 });
	vertices.append(VectorN{ 1, 1, 0, 1, 0 });
	vertices.append(VectorN{ 0, 0, 1, 1, 0 });
	vertices.append(VectorN{ 1, 0, 1, 1, 0 });
	vertices.append(VectorN{ 0, 1, 1, 1, 0 });
	vertices.append(VectorN{ 1, 1, 1, 1, 0 });
	vertices.append(VectorN{ 0, 0, 0, 0, 1 });
	vertices.append(VectorN{ 1, 0, 0, 0, 1 });
	vertices.append(VectorN{ 0, 1, 0, 0, 1 });
	vertices.append(VectorN{ 1, 1, 0, 0, 1 });
	vertices.append(VectorN{ 0, 0, 1, 0, 1 });
	vertices.append(VectorN{ 1, 0, 1, 0, 1 });
	vertices.append(VectorN{ 0, 1, 1, 0, 1 });
	vertices.append(VectorN{ 1, 1, 1, 0, 1 });
	vertices.append(VectorN{ 0, 0, 0, 1, 1 });
	vertices.append(VectorN{ 1, 0, 0, 1, 1 });
	vertices.append(VectorN{ 0, 1, 0, 1, 1 });
	vertices.append(VectorN{ 1, 1, 0, 1, 1 });
	vertices.append(VectorN{ 0, 0, 1, 1, 1 });
	vertices.append(VectorN{ 1, 0, 1, 1, 1 });
	vertices.append(VectorN{ 0, 1, 1, 1, 1 });
	vertices.append(VectorN{ 1, 1, 1, 1, 1 });
	cell_indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
	decomposed = CellMeshND::decompose_polytope_cell_into_simplexes(vertices, cell_indices, 5, -1, Vector<VectorN>());
	CHECK(decomposed.size() == 120);
	// Note: Due to the nature of polytopes, this algorithm has O(2^n) complexity.
	// A test with 6D would take many minutes to run... so is omitted for sanity.
	// I am sure a better algorithm exists, but this technically works for now.
}
} // namespace TestCellMeshND
