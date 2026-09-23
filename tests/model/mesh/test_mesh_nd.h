#pragma once

#include "../../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../../model/mesh/cell/cell_material_nd.h"
#include "../../../model/mesh/poly/array_poly_mesh_nd.h"
#include "../../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../../model/mesh/wire/array_wire_mesh_nd.h"
#include "../../../model/mesh/wire/box_wire_mesh_nd.h"
#include "../../../model/mesh/wire/wire_material_nd.h"

#include "tests/test_macros.h"

namespace TestMeshND {
TEST_CASE("[MeshND] Rect bounds include the local origin") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 1, 1, 1, 1 }, VectorN{ 3, 3, 3, 3 } }));
	// Even though every vertex is offset from the origin, the bounds must still include it.
	const Ref<RectND> bounds = mesh->get_rect_bounds();
	CHECK_MESSAGE(VectorND::is_equal_exact(bounds->get_position(), VectorN{ 0, 0, 0, 0 }), "MeshND get_rect_bounds should always include the mesh's local origin.");
	CHECK_MESSAGE(VectorND::is_equal_exact(bounds->get_end(), VectorN{ 3, 3, 3, 3 }), "MeshND get_rect_bounds should still reach every vertex.");
}

TEST_CASE("[ArrayWireMeshND] Bounds cache invalidation on merge") {
	Ref<ArrayWireMeshND> mesh1;
	mesh1.instantiate();
	mesh1->set_vertex_positions(Vector<VectorN>({ VectorN{ -1, -1, -1, -1 }, VectorN{ 1, 1, 1, 1 } }));
	mesh1->append_edge_indices(0, 1);

	Ref<RectND> bounds1 = mesh1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds1->get_position(), VectorN{ -1, -1, -1, -1 }));
	CHECK(VectorND::is_equal_exact(bounds1->get_size(), VectorN{ 2, 2, 2, 2 }));

	// Create a second mesh with vertices outside the first mesh's bounds.
	Ref<ArrayWireMeshND> mesh2;
	mesh2.instantiate();
	mesh2->set_vertex_positions(Vector<VectorN>({ VectorN{ 5, 5, 5, 5 }, VectorN{ 10, 10, 10, 10 } }));
	mesh2->append_edge_indices(0, 1);

	mesh1->merge_with(mesh2, TransformND::identity_transform(4));

	// Bounds should expand to include the merged vertices.
	Ref<RectND> bounds_after_merge = mesh1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_position(), VectorN{ -1, -1, -1, -1 }));
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_end(), VectorN{ 10, 10, 10, 10 }));
}

TEST_CASE("[ArrayCellMeshND] Bounds cache invalidation on merge") {
	Ref<ArrayCellMeshND> cell1;
	cell1.instantiate();
	cell1->set_vertex_positions(Vector<VectorN>({ VectorN{ 0, 0, 0 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 0, 1 } }));
	// In 3D, simplex boundary cells are triangles, not tetrahedra.
	const PackedInt32Array triangles = { 0, 1, 2, 0, 1, 3, 0, 2, 3, 1, 2, 3 };
	cell1->set_simplex_cell_vertex_indices(triangles);
	REQUIRE(cell1->is_mesh_data_valid());

	Ref<RectND> bounds1 = cell1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds1->get_position(), VectorN{ 0, 0, 0 }));
	CHECK(VectorND::is_equal_exact(bounds1->get_end(), VectorN{ 1, 1, 1 }));

	// Create a second cell mesh with vertices outside the first mesh's bounds.
	Ref<ArrayCellMeshND> cell2;
	cell2.instantiate();
	cell2->set_vertex_positions(Vector<VectorN>({ VectorN{ 5, 5, 5 }, VectorN{ 6, 5, 5 }, VectorN{ 5, 6, 5 }, VectorN{ 5, 5, 6 } }));
	cell2->set_simplex_cell_vertex_indices(triangles);
	REQUIRE(cell2->is_mesh_data_valid());

	cell1->merge_with(cell2, TransformND::identity_transform(3));

	// Bounds should expand to include the merged vertices.
	Ref<RectND> bounds_after_merge = cell1->get_rect_bounds();
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_position(), VectorN{ 0, 0, 0 }));
	CHECK(VectorND::is_equal_exact(bounds_after_merge->get_end(), VectorN{ 6, 6, 6 }));
}

TEST_CASE("[MeshND] Proxy mesh 3D is lazily created and cached") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_name("TestMesh");
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 } }));

	const Ref<ArrayMesh> proxy_mesh_3d = mesh->get_proxy_mesh_3d();
	CHECK_MESSAGE(proxy_mesh_3d.is_valid(), "MeshND get_proxy_mesh_3d should lazily instantiate the proxy mesh.");
	CHECK_MESSAGE(proxy_mesh_3d->get_name() == "TestMesh Proxy Mesh 3D", "MeshND get_proxy_mesh_3d should name the mesh after the resource it came from.");

	// The same instance must be handed out on subsequent calls, both while clean and after being marked dirty.
	CHECK_MESSAGE(mesh->get_proxy_mesh_3d() == proxy_mesh_3d, "MeshND get_proxy_mesh_3d should return the same instance while the cache is clean.");
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 2, 2, 2, 2 }, VectorN{ 3, 3, 3, 3 } }));
	CHECK_MESSAGE(mesh->get_proxy_mesh_3d() == proxy_mesh_3d, "MeshND get_proxy_mesh_3d should reuse the same instance after being marked dirty.");
}

TEST_CASE("[MeshND] Bounds cache persists across multiple accesses") {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ -2, -2, -2, -2 }, VectorN{ 3, 3, 3, 3 } }));

	// Access bounds multiple times - should use the cached value.
	Ref<RectND> bounds1 = mesh->get_rect_bounds();
	Ref<RectND> bounds2 = mesh->get_rect_bounds();
	Ref<RectND> bounds3 = mesh->get_rect_bounds();

	CHECK(bounds1 == bounds2);
	CHECK(bounds2 == bounds3);
	CHECK(VectorND::is_equal_exact(bounds1->get_position(), VectorN{ -2, -2, -2, -2 }));
	CHECK(VectorND::is_equal_exact(bounds1->get_end(), VectorN{ 3, 3, 3, 3 }));

	// Changing the vertices must invalidate the cache and produce a new Ref.
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 1, 1, 1 } }));
	Ref<RectND> bounds_after_change = mesh->get_rect_bounds();
	CHECK(bounds_after_change != bounds1);
	CHECK(VectorND::is_equal_exact(bounds_after_change->get_end(), VectorN{ 1, 1, 1, 1 }));
}

TEST_CASE("[MeshND] Array mesh validators share the first-vertex dimension contract") {
	for (const int dimension : { 0, 3, 4, 5 }) {
		CAPTURE(dimension);
		Ref<ArrayCellMeshND> cell;
		cell.instantiate();
		Ref<ArrayWireMeshND> wire;
		wire.instantiate();
		Ref<ArrayPolyMeshND> poly;
		poly.instantiate();
		const Vector<Ref<SingleSurfaceMeshND>> meshes = { cell, wire, poly };
		const Vector<VectorN> positions = { VectorND::fill(dimension, 1.0), VectorN(), dimension == 0 ? VectorN() : VectorN{ 2.0 } };
		cell->set_vertex_positions(positions);
		wire->set_vertex_positions(positions);
		poly->set_poly_cell_vertex_positions(positions);
		for (const Ref<SingleSurfaceMeshND> &mesh : meshes) {
			CHECK(mesh->get_dimension() == dimension);
			CHECK(mesh->is_mesh_data_valid());
			CHECK(mesh->get_vertex_positions() == positions);
		}
		Vector<VectorN> invalid = positions;
		invalid.set(1, VectorND::zero(dimension + 1));
		cell->set_vertex_positions(invalid);
		wire->set_vertex_positions(invalid);
		poly->set_poly_cell_vertex_positions(invalid);
		for (const Ref<SingleSurfaceMeshND> &mesh : meshes) {
			CHECK(mesh->get_dimension() == dimension);
			ERR_PRINT_OFF;
			CHECK_FALSE(mesh->is_mesh_data_valid());
			ERR_PRINT_ON;
		}
	}
}

static const char *VALIDATION_RESET = "mesh_data_validation_reset";
static const char *PROXY_DIRTY = "proxy_mesh_3d_marked_dirty";

// Both signals have no arguments, so each emission is recorded as an empty argument list.
static Array _emissions(const int p_count) {
	Array emissions;
	for (int i = 0; i < p_count; i++) {
		emissions.push_back(Array());
	}
	return emissions;
}

static void _watch_signals(MeshND *p_mesh) {
	SIGNAL_WATCH(p_mesh, VALIDATION_RESET);
	SIGNAL_WATCH(p_mesh, PROXY_DIRTY);
}

static void _unwatch_signals(MeshND *p_mesh) {
	SIGNAL_UNWATCH(p_mesh, VALIDATION_RESET);
	SIGNAL_UNWATCH(p_mesh, PROXY_DIRTY);
}

static Ref<ArrayWireMeshND> _make_wire_mesh() {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 0, 0, 0 } }));
	mesh->set_edge_indices({ 0, 1 });
	return mesh;
}

static Ref<ArrayCellMeshND> _make_cell_mesh() {
	Ref<ArrayCellMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 0, 0, 0 }, VectorN{ 1, 0, 0 }, VectorN{ 0, 1, 0 }, VectorN{ 0, 0, 1 } }));
	mesh->set_simplex_cell_vertex_indices({ 0, 1, 2, 0, 1, 3, 0, 2, 3, 1, 2, 3 });
	return mesh;
}

static Ref<ArrayPolyMeshND> _make_poly_mesh() {
	Ref<BoxPolyMeshND> box;
	box.instantiate();
	box->set_size(VectorN{ 1, 1, 1, 1 });
	return box->to_array_poly_mesh();
}

static TypedArray<VectorN> _to_typed_array(const Vector<VectorN> &p_vectors) {
	TypedArray<VectorN> typed_array;
	for (const VectorN &vector : p_vectors) {
		typed_array.push_back(vector);
	}
	return typed_array;
}

static PackedStringArray signal_order;
static void _record_validation_reset() {
	signal_order.push_back(VALIDATION_RESET);
}
static void _record_proxy_dirty() {
	signal_order.push_back(PROXY_DIRTY);
}

TEST_CASE("[MeshND] Resetting validation also marks the proxy mesh dirty") {
	Ref<ArrayWireMeshND> mesh = _make_wire_mesh();
	_watch_signals(mesh.ptr());
	mesh->reset_mesh_data_validation();
	SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
	SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
	_unwatch_signals(mesh.ptr());
}

TEST_CASE("[MeshND] Validation reset is emitted before the proxy mesh is marked dirty") {
	Ref<ArrayWireMeshND> mesh = _make_wire_mesh();
	signal_order.clear();
	mesh->connect(VALIDATION_RESET, callable_mp_static(&_record_validation_reset));
	mesh->connect(PROXY_DIRTY, callable_mp_static(&_record_proxy_dirty));
	mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 0, 0, 0, 0 }, VectorN{ 2, 0, 0, 0 } }));
	CHECK_MESSAGE(signal_order == PackedStringArray({ VALIDATION_RESET, PROXY_DIRTY }), "Listeners that care about validity should be notified before listeners that only rebuild proxy meshes.");
	mesh->disconnect(VALIDATION_RESET, callable_mp_static(&_record_validation_reset));
	mesh->disconnect(PROXY_DIRTY, callable_mp_static(&_record_proxy_dirty));
}

TEST_CASE("[MeshND] Structural changes emit both signals exactly once") {
	SUBCASE("ArrayWireMeshND") {
		Ref<ArrayWireMeshND> mesh = _make_wire_mesh();
		_watch_signals(mesh.ptr());
		mesh->set_vertex_positions(Vector<VectorN>({ VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 0, 0, 0 }, VectorN{ 0, 1, 0, 0 } }));
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		mesh->append_edge_indices(1, 2);
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		mesh->append_vertex(VectorN{ 0, 0, 1, 0 });
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		_unwatch_signals(mesh.ptr());
	}
	SUBCASE("ArrayCellMeshND") {
		Ref<ArrayCellMeshND> mesh = _make_cell_mesh();
		_watch_signals(mesh.ptr());
		mesh->set_simplex_cell_vertex_indices({ 0, 2, 1, 0, 1, 3, 0, 2, 3, 1, 2, 3 });
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		mesh->set_normal_values(Vector<VectorN>({ VectorN{ 0, 0, 1 } }));
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		mesh->append_vertex(VectorN{ 1, 1, 1 });
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		_unwatch_signals(mesh.ptr());
	}
	SUBCASE("ArrayPolyMeshND") {
		Ref<ArrayPolyMeshND> mesh = _make_poly_mesh();
		_watch_signals(mesh.ptr());
		mesh->set_poly_cell_vertex_positions(mesh->get_poly_cell_vertex_positions());
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		mesh->set_poly_cell_normal_values(mesh->get_poly_cell_normal_values());
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		mesh->append_vertex(VectorN{ 5, 5, 5, 5 });
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		_unwatch_signals(mesh.ptr());
	}
}

TEST_CASE("[MeshND] Transforming the mesh marks the proxy dirty without resetting validation") {
	SUBCASE("ArrayWireMeshND") {
		Ref<ArrayWireMeshND> mesh = _make_wire_mesh();
		REQUIRE(mesh->is_mesh_data_valid());
		const double end_x_before = mesh->get_rect_bounds()->get_end()[0];
		_watch_signals(mesh.ptr());
		mesh->transform_mesh(TransformND::from_position(VectorN{ 10, 0, 0, 0 }));
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		CHECK_MESSAGE(mesh->get_rect_bounds()->get_end()[0] == doctest::Approx(end_x_before + 10.0), "Transforming the mesh should mark the rect bounds dirty.");
		_unwatch_signals(mesh.ptr());
	}
	SUBCASE("ArrayCellMeshND") {
		Ref<ArrayCellMeshND> mesh = _make_cell_mesh();
		REQUIRE(mesh->is_mesh_data_valid());
		const double end_x_before = mesh->get_rect_bounds()->get_end()[0];
		_watch_signals(mesh.ptr());
		mesh->transform_mesh(TransformND::from_position(VectorN{ 10, 0, 0 }));
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		CHECK_MESSAGE(mesh->get_rect_bounds()->get_end()[0] == doctest::Approx(end_x_before + 10.0), "Transforming the mesh should mark the rect bounds dirty.");
		_unwatch_signals(mesh.ptr());
	}
	SUBCASE("ArrayPolyMeshND") {
		Ref<ArrayPolyMeshND> mesh = _make_poly_mesh();
		REQUIRE(mesh->is_mesh_data_valid());
		const double end_x_before = mesh->get_rect_bounds()->get_end()[0];
		_watch_signals(mesh.ptr());
		mesh->transform_mesh(TransformND::from_position(VectorN{ 10, 0, 0, 0 }));
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		CHECK_MESSAGE(mesh->get_rect_bounds()->get_end()[0] == doctest::Approx(end_x_before + 10.0), "Transforming the mesh should mark the rect bounds dirty.");
		CHECK_MESSAGE(mesh->is_poly_mesh_data_valid(), "Transforming the mesh should not reset the poly mesh validation either.");
		_unwatch_signals(mesh.ptr());
	}
}

TEST_CASE("[MeshND] Appending only duplicate vertices emits nothing") {
	SUBCASE("ArrayWireMeshND") {
		Ref<ArrayWireMeshND> mesh = _make_wire_mesh();
		_watch_signals(mesh.ptr());
		mesh->append_vertices(mesh->get_vertex_positions(), true);
		mesh->append_vertices(Vector<VectorN>(), true);
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK_FALSE(PROXY_DIRTY);
		_unwatch_signals(mesh.ptr());
	}
	SUBCASE("ArrayCellMeshND") {
		Ref<ArrayCellMeshND> mesh = _make_cell_mesh();
		_watch_signals(mesh.ptr());
		mesh->append_vertices(mesh->get_vertex_positions(), true);
		mesh->append_vertices(Vector<VectorN>(), true);
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK_FALSE(PROXY_DIRTY);
		_unwatch_signals(mesh.ptr());
	}
	SUBCASE("ArrayPolyMeshND") {
		Ref<ArrayPolyMeshND> mesh = _make_poly_mesh();
		_watch_signals(mesh.ptr());
		mesh->append_vertices(_to_typed_array(mesh->get_poly_cell_vertex_positions()), true);
		mesh->append_vertices(TypedArray<VectorN>(), true);
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK_FALSE(PROXY_DIRTY);
		_unwatch_signals(mesh.ptr());
	}
}

TEST_CASE("[ArrayWireMeshND] Appending a vertex updates the rect bounds") {
	Ref<ArrayWireMeshND> mesh = _make_wire_mesh();
	CHECK(VectorND::is_equal_exact(mesh->get_rect_bounds()->get_end(), VectorN{ 1, 0, 0, 0 }));
	mesh->append_vertex(VectorN{ 0, 5, 0, 0 });
	CHECK_MESSAGE(VectorND::is_equal_exact(mesh->get_rect_bounds()->get_end(), VectorN{ 1, 5, 0, 0 }), "A vertex without any edges still counts towards the rect bounds.");
}

TEST_CASE("[MeshND] Primitive size changes mark bounds and proxy dirty without resetting validation") {
	SUBCASE("BoxWireMeshND") {
		Ref<BoxWireMeshND> mesh;
		mesh.instantiate();
		mesh->set_size(VectorN{ 1, 1, 1, 1 });
		REQUIRE(mesh->is_mesh_data_valid());
		_watch_signals(mesh.ptr());
		mesh->set_size(VectorN{ 2, 4, 6, 8 });
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		CHECK(VectorND::is_equal_exact(mesh->get_rect_bounds()->get_end(), VectorN{ 1, 2, 3, 4 }));
		_unwatch_signals(mesh.ptr());
	}
	SUBCASE("BoxPolyMeshND") {
		Ref<BoxPolyMeshND> mesh;
		mesh.instantiate();
		mesh->set_size(VectorN{ 1, 1, 1, 1 });
		REQUIRE(mesh->is_mesh_data_valid());
		_watch_signals(mesh.ptr());
		mesh->set_size(VectorN{ 2, 4, 6, 8 });
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		CHECK(VectorND::is_equal_exact(mesh->get_rect_bounds()->get_end(), VectorN{ 1, 2, 3, 4 }));
		_unwatch_signals(mesh.ptr());
	}
}
TEST_CASE("[SingleSurfaceMeshND] Array conversions keep the name and metadata") {
	Ref<BoxPolyMeshND> box_poly;
	box_poly.instantiate();
	box_poly->set_size(VectorND::fill(4, 1.0));
	box_poly->set_name("PolyNamed");
	box_poly->set_meta("source_file", "box.off");
	const Ref<ArrayPolyMeshND> array_poly = box_poly->to_array_poly_mesh();
	CHECK(array_poly->get_name() == "PolyNamed");
	CHECK(array_poly->get_meta("source_file") == Variant("box.off"));
	const Ref<ArrayCellMeshND> cell_from_poly = box_poly->to_array_cell_mesh();
	CHECK(cell_from_poly->get_name() == "PolyNamed");
	CHECK(cell_from_poly->get_meta("source_file") == Variant("box.off"));
	const Ref<ArrayWireMeshND> wire_from_cell = cell_from_poly->to_array_wire_mesh();
	CHECK(wire_from_cell->get_name() == "PolyNamed");
	CHECK(wire_from_cell->get_meta("source_file") == Variant("box.off"));

	Ref<BoxWireMeshND> box_wire;
	box_wire.instantiate();
	box_wire->set_size(VectorND::fill(4, 1.0));
	box_wire->set_name("WireNamed");
	box_wire->set_meta("source_file", "box_wire.off");
	const Ref<ArrayWireMeshND> array_wire = box_wire->to_array_wire_mesh();
	CHECK(array_wire->get_name() == "WireNamed");
	CHECK(array_wire->get_meta("source_file") == Variant("box_wire.off"));
}
TEST_CASE("[SingleSurfaceMeshND] Built-in mesh types share typed fallback materials") {
	Ref<ArrayWireMeshND> wire;
	wire.instantiate();
	Ref<BoxWireMeshND> box_wire;
	box_wire.instantiate();
	const Ref<WireMaterialND> wire_fallback = wire->get_fallback_material();
	REQUIRE(wire_fallback.is_valid());
	CHECK(box_wire->get_fallback_material() == wire_fallback);

	Ref<ArrayCellMeshND> cell;
	cell.instantiate();
	Ref<ArrayPolyMeshND> poly;
	poly.instantiate();
	const Ref<CellMaterialND> cell_fallback = cell->get_fallback_material();
	REQUIRE(cell_fallback.is_valid());
	CHECK_MESSAGE(poly->get_fallback_material() == cell_fallback, "Poly meshes are cell meshes, so they share the cell fallback material.");
	// Compare as Object pointers because the two Ref types are unrelated, so Ref's comparison operators would need an impossible cast.
	CHECK(static_cast<const Object *>(cell_fallback.ptr()) != static_cast<const Object *>(wire_fallback.ptr()));
}
} // namespace TestMeshND
