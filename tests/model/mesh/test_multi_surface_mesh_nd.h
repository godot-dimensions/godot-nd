#pragma once

#include "../../../math/transform_nd.h"
#include "../../../math/vector_nd.h"
#include "../../../model/mesh/cell/array_cell_mesh_nd.h"
#include "../../../model/mesh/cell/cell_material_nd.h"
#include "../../../model/mesh/multi_surface_mesh_nd.h"
#include "../../../model/mesh/poly/array_poly_mesh_nd.h"
#include "../../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../../model/mesh/wire/array_wire_mesh_nd.h"
#include "../../../model/mesh/wire/box_wire_mesh_nd.h"

#include "tests/test_macros.h"

namespace TestMultiSurfaceMeshND {
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

static Ref<BoxPolyMeshND> _make_box_poly(const int p_dimension, const double p_size) {
	Ref<BoxPolyMeshND> box;
	box.instantiate();
	box->set_size(VectorND::fill(p_dimension, p_size));
	return box;
}

static Ref<BoxWireMeshND> _make_box_wire(const int p_dimension, const double p_size) {
	Ref<BoxWireMeshND> box;
	box.instantiate();
	box->set_size(VectorND::fill(p_dimension, p_size));
	return box;
}

TEST_CASE("[MultiSurfaceMeshND] Transforming keeps every surface and makes them writable") {
	// A mix of non-array and array surfaces, since the non-array ones must be converted before they can be transformed.
	Ref<BoxPolyMeshND> box_poly = _make_box_poly(4, 1.0);
	box_poly->set_name("Poly");
	Ref<ArrayCellMeshND> array_cell = _make_box_poly(4, 2.0)->to_array_cell_mesh();
	array_cell->set_name("Cell");
	const Vector<VectorN> poly_vertices = box_poly->get_vertex_positions();
	const Vector<VectorN> cell_vertices = array_cell->get_vertex_positions();

	Ref<MultiSurfaceMeshND> mesh;
	mesh.instantiate();
	mesh->set_surface_meshes({ box_poly, array_cell });
	REQUIRE(mesh->is_mesh_data_valid());
	CHECK(mesh->get_dimension() == 4);
	const Ref<RectND> bounds_before = mesh->get_rect_bounds();

	// A quarter turn in the YZ plane followed by an offset, similar to the Z-up to Y-up conversion editor importers apply.
	const Ref<TransformND> transform = TransformND::from_position_rotation(VectorN{ 1, 2, 3, 4 }, 1, 2, Math_TAU / 4.0);
	mesh->transform_mesh(transform);
	CHECK(mesh->is_mesh_data_valid());
	// The bounds of the whole mesh must follow the transformed surfaces, always including the origin.
	Ref<RectND> expected_bounds = RectND::from_position_size(VectorND::zero(4), VectorND::zero(4));
	for (const Vector<VectorN> &original_vertices : { poly_vertices, cell_vertices }) {
		for (const VectorN &vertex : original_vertices) {
			expected_bounds->expand_self_to_point(transform->xform(vertex));
		}
	}
	CHECK_FALSE(VectorND::is_equal_approx(expected_bounds->get_end(), bounds_before->get_end()));
	CHECK_MESSAGE(VectorND::is_equal_approx(mesh->get_rect_bounds()->get_position(), expected_bounds->get_position()), "Transforming the surfaces should mark the multi-surface mesh's bounds dirty.");
	CHECK_MESSAGE(VectorND::is_equal_approx(mesh->get_rect_bounds()->get_end(), expected_bounds->get_end()), "Transforming the surfaces should mark the multi-surface mesh's bounds dirty.");
	const Vector<Ref<SingleSurfaceMeshND>> &surfaces = mesh->get_surface_meshes();
	REQUIRE(surfaces.size() == 2);
	// The box poly mesh was converted to an array mesh so it could be transformed, keeping its name.
	const Ref<ArrayPolyMeshND> transformed_poly = surfaces[0];
	REQUIRE(transformed_poly.is_valid());
	CHECK(transformed_poly->get_name() == "Poly");
	const Vector<VectorN> transformed_poly_vertices = transformed_poly->get_vertex_positions();
	REQUIRE(transformed_poly_vertices.size() == poly_vertices.size());
	for (int64_t i = 0; i < poly_vertices.size(); i++) {
		CHECK(VectorND::is_equal_approx(transformed_poly_vertices[i], transform->xform(poly_vertices[i])));
	}
	// The array cell mesh is transformed in place.
	const Ref<ArrayCellMeshND> transformed_cell = surfaces[1];
	REQUIRE(transformed_cell.is_valid());
	CHECK(transformed_cell == array_cell);
	CHECK(transformed_cell->get_name() == "Cell");
	const Vector<VectorN> transformed_cell_vertices = transformed_cell->get_vertex_positions();
	REQUIRE(transformed_cell_vertices.size() == cell_vertices.size());
	for (int64_t i = 0; i < cell_vertices.size(); i++) {
		CHECK(VectorND::is_equal_approx(transformed_cell_vertices[i], transform->xform(cell_vertices[i])));
	}
}

TEST_CASE("[MultiSurfaceMeshND] Surface changes propagate to the multi-surface mesh") {
	// A plain array mesh with no texture map, so its simplex indices can be replaced freely below.
	Ref<ArrayCellMeshND> array_cell;
	array_cell.instantiate();
	array_cell->set_vertex_positions({ VectorN{ 0, 0, 0, 0 }, VectorN{ 1, 0, 0, 0 }, VectorN{ 0, 1, 0, 0 }, VectorN{ 0, 0, 1, 0 } });
	array_cell->set_simplex_cell_vertex_indices({ 0, 1, 2, 3 });
	Ref<BoxPolyMeshND> box_poly = _make_box_poly(4, 1.0);
	Ref<MultiSurfaceMeshND> mesh;
	mesh.instantiate();
	mesh->set_surface_meshes({ array_cell, box_poly });
	REQUIRE(mesh->is_mesh_data_valid());
	const Ref<RectND> bounds_before = mesh->get_rect_bounds();

	SUBCASE("Transforming a surface directly only dirties the bounds and proxy mesh") {
		SIGNAL_WATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_WATCH(mesh.ptr(), PROXY_DIRTY);
		array_cell->transform_mesh(TransformND::from_position(VectorN{ 10, 0, 0, 0 }));
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		SIGNAL_UNWATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_UNWATCH(mesh.ptr(), PROXY_DIRTY);
		CHECK(mesh->is_mesh_data_valid());
		CHECK_MESSAGE(mesh->get_rect_bounds()->get_end()[0] == doctest::Approx(bounds_before->get_end()[0] + 10.0), "The multi-surface mesh's bounds must follow a surface transformed behind its back.");
	}

	SUBCASE("Resizing a primitive surface only dirties the bounds and proxy mesh") {
		SIGNAL_WATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_WATCH(mesh.ptr(), PROXY_DIRTY);
		box_poly->set_size(VectorND::fill(4, 4.0));
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(1));
		SIGNAL_UNWATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_UNWATCH(mesh.ptr(), PROXY_DIRTY);
		CHECK(VectorND::is_equal_approx(mesh->get_rect_bounds()->get_end(), VectorN{ 2, 2, 2, 2 }));
	}

	SUBCASE("Changing a surface's data resets the validation of the whole mesh") {
		SIGNAL_WATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_WATCH(mesh.ptr(), PROXY_DIRTY);
		array_cell->set_simplex_cell_vertex_indices({ 0, 1, 2, 3 });
		SIGNAL_CHECK(VALIDATION_RESET, _emissions(1));
		// Once from the validation reset, and once more from the surface's own proxy dirty signal that follows it.
		SIGNAL_CHECK(PROXY_DIRTY, _emissions(2));
		SIGNAL_UNWATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_UNWATCH(mesh.ptr(), PROXY_DIRTY);
		CHECK(mesh->is_mesh_data_valid());
		array_cell->set_simplex_cell_vertex_indices({ 0, 1, 2, 99 });
		ERR_PRINT_OFF; // The surface is intentionally invalid now.
		CHECK_FALSE(mesh->is_mesh_data_valid());
		ERR_PRINT_ON;
	}

	SUBCASE("Replaced surfaces are disconnected") {
		mesh->set_surface_meshes({ box_poly });
		SIGNAL_WATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_WATCH(mesh.ptr(), PROXY_DIRTY);
		array_cell->transform_mesh(TransformND::from_position(VectorN{ 10, 0, 0, 0 }));
		array_cell->set_simplex_cell_vertex_indices({ 0, 1, 2, 3 });
		SIGNAL_CHECK_FALSE(VALIDATION_RESET);
		SIGNAL_CHECK_FALSE(PROXY_DIRTY);
		SIGNAL_UNWATCH(mesh.ptr(), VALIDATION_RESET);
		SIGNAL_UNWATCH(mesh.ptr(), PROXY_DIRTY);
	}
}

TEST_CASE("[MultiSurfaceMeshND] Merging compatible surfaces combines surfaces sharing a material and keeps separate materials apart") {
	// Four boxes with the same name. Three share one per-vertex material, and the fourth has an equal but separate copy.
	Ref<CellMaterialND> shared_material;
	shared_material.instantiate();
	shared_material->set_albedo_source_flags(MaterialND::COLOR_SOURCE_FLAG_PER_VERT);
	const int64_t box_vertex_count = 16;
	PackedColorArray vertex_colors;
	for (int64_t vertex = 0; vertex < box_vertex_count; vertex++) {
		vertex_colors.append(Color(vertex / 16.0f, 0.5f, 0.25f));
	}
	shared_material->set_albedo_color_array(vertex_colors);
	const Ref<MaterialND> separate_material = shared_material->duplicate();
	Vector<Ref<SingleSurfaceMeshND>> surfaces;
	for (int i = 0; i < 4; i++) {
		Ref<ArrayPolyMeshND> array_box = _make_box_poly(4, 1.0)->to_array_poly_mesh();
		REQUIRE(array_box->get_poly_cell_vertex_positions().size() == box_vertex_count);
		array_box->set_name("Same");
		array_box->set_material(i < 3 ? Ref<MaterialND>(shared_material) : separate_material);
		surfaces.append(array_box);
	}
	Ref<MultiSurfaceMeshND> mesh;
	mesh.instantiate();
	mesh->set_surface_meshes(surfaces);
	mesh->merge_compatible_surfaces();
	CHECK(mesh->is_mesh_data_valid());
	const Vector<Ref<SingleSurfaceMeshND>> &merged = mesh->get_surface_meshes();
	REQUIRE_MESSAGE(merged.size() == 2, "The three surfaces sharing a material must merge into one, and the separate material must stay apart.");
	CHECK(merged[0] == surfaces[0]);
	CHECK(merged[1] == surfaces[3]);
	// The merged surface holds the geometry of all three boxes, with one color per vertex of each.
	CHECK(merged[0]->get_vertex_positions().size() == box_vertex_count * 3);
	const Ref<MaterialND> merged_material = merged[0]->get_material();
	REQUIRE(merged_material.is_valid());
	const PackedColorArray merged_colors = merged_material->get_albedo_color_array();
	REQUIRE(merged_colors.size() == box_vertex_count * 3);
	for (int64_t i = 0; i < merged_colors.size(); i++) {
		CHECK(merged_colors[i] == vertex_colors[i % box_vertex_count]);
	}
	// The separate material and its surface are untouched.
	CHECK(merged[1]->get_vertex_positions().size() == box_vertex_count);
	CHECK(merged[1]->get_material() == separate_material);
	CHECK(separate_material->get_albedo_color_array() == vertex_colors);
}

TEST_CASE("[MultiSurfaceMeshND] Self merge appends transformed copies of the original surfaces once") {
	const Ref<ArrayPolyMeshND> array_poly = _make_box_poly(4, 1.0)->to_array_poly_mesh();
	const Ref<BoxWireMeshND> box_wire = _make_box_wire(4, 1.0);
	const Vector<Ref<SingleSurfaceMeshND>> originals = { array_poly, box_wire };
	const Vector<Vector<VectorN>> original_vertices = { array_poly->get_vertex_positions(), box_wire->get_vertex_positions() };
	Ref<MultiSurfaceMeshND> mesh;
	mesh.instantiate();
	mesh->set_surface_meshes({ array_poly, Ref<SingleSurfaceMeshND>(), box_wire });
	const Ref<TransformND> transform = TransformND::from_position(VectorN{ 3, 4, 5, 6 });
	mesh->merge_with(mesh, transform);
	CHECK(mesh->is_mesh_data_valid());
	const Vector<Ref<SingleSurfaceMeshND>> &surfaces = mesh->get_surface_meshes();
	REQUIRE(surfaces.size() == 5);
	CHECK(surfaces[0] == array_poly);
	CHECK(surfaces[1].is_null());
	CHECK(surfaces[2] == box_wire);
	for (int i = 0; i < originals.size(); i++) {
		const Ref<SingleSurfaceMeshND> copy = surfaces[3 + i];
		REQUIRE(copy.is_valid());
		CHECK(copy != originals[i]);
		CHECK(VectorND::array_is_equal_exact(originals[i]->get_vertex_positions(), original_vertices[i]));
		const Vector<VectorN> copied_vertices = copy->get_vertex_positions();
		REQUIRE(copied_vertices.size() == original_vertices[i].size());
		for (int64_t vertex_index = 0; vertex_index < copied_vertices.size(); vertex_index++) {
			CHECK(VectorND::is_equal_approx(copied_vertices[vertex_index], transform->xform(original_vertices[i][vertex_index])));
		}
	}
}

TEST_CASE("[MultiSurfaceMeshND] Compatible merges preserve invalid sources and destinations") {
	Ref<SingleSurfaceMeshND> valid;
	Ref<SingleSurfaceMeshND> invalid;
	SUBCASE("Poly surfaces") {
		const Ref<BoxPolyMeshND> box = _make_box_poly(4, 1.0);
		valid = box->to_array_poly_mesh();
		const Ref<ArrayPolyMeshND> invalid_poly = box->to_array_poly_mesh();
		invalid_poly->set_poly_cell_vertex_positions(Vector<VectorN>());
		invalid = invalid_poly;
	}
	SUBCASE("Cell surfaces") {
		const Ref<BoxPolyMeshND> box = _make_box_poly(4, 1.0);
		valid = box->to_array_cell_mesh();
		const Ref<ArrayCellMeshND> invalid_cell = box->to_array_cell_mesh();
		invalid_cell->set_vertex_positions(Vector<VectorN>());
		invalid = invalid_cell;
	}
	SUBCASE("Wire surfaces") {
		const Ref<BoxWireMeshND> box = _make_box_wire(4, 1.0);
		valid = box->to_array_wire_mesh();
		const Ref<ArrayWireMeshND> invalid_wire = box->to_array_wire_mesh();
		invalid_wire->set_vertex_positions(Vector<VectorN>());
		invalid = invalid_wire;
	}
	REQUIRE(valid.is_valid());
	REQUIRE(invalid.is_valid());
	REQUIRE(valid->is_mesh_data_valid());
	const Vector<VectorN> valid_vertices = valid->get_vertex_positions();
	for (const bool invalid_first : { false, true }) {
		CAPTURE(invalid_first);
		const Vector<Ref<SingleSurfaceMeshND>> originals = invalid_first ? Vector<Ref<SingleSurfaceMeshND>>{ invalid, valid } : Vector<Ref<SingleSurfaceMeshND>>{ valid, invalid };
		Ref<MultiSurfaceMeshND> mesh;
		mesh.instantiate();
		mesh->set_surface_meshes(originals);
		ERR_PRINT_OFF;
		mesh->merge_compatible_surfaces();
		ERR_PRINT_ON;
		CHECK(mesh->get_surface_meshes() == originals);
		CHECK(valid->is_mesh_data_valid());
		CHECK(VectorND::array_is_equal_exact(valid->get_vertex_positions(), valid_vertices));
	}
}

TEST_CASE("[MultiSurfaceMeshND] Transforming an empty mesh does nothing") {
	Ref<MultiSurfaceMeshND> mesh;
	mesh.instantiate();
	mesh->transform_mesh(TransformND::from_position(VectorN{ 1, 0, 0, 0 }));
	CHECK(mesh->get_surface_meshes().is_empty());
	CHECK(mesh->is_mesh_data_valid());
	CHECK(mesh->get_dimension() == 0);
}
} // namespace TestMultiSurfaceMeshND
