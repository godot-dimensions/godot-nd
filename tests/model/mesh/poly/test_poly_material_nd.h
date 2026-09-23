#pragma once

#include "../../../../math/transform_nd.h"
#include "../../../../math/vector_nd.h"
#include "../../../../model/mesh/cell/cell_material_nd.h"
#include "../../../../model/mesh/poly/array_poly_mesh_nd.h"
#include "../../../../model/mesh/poly/box_poly_mesh_nd.h"
#include "../../../../model/mesh/poly/poly_material_nd.h"

#include "tests/test_macros.h"

namespace TestPolyMaterialND {
constexpr MaterialND::ColorSourceFlagsND PER_CELL_ONLY = MaterialND::COLOR_SOURCE_FLAG_PER_CELL;
constexpr MaterialND::ColorSourceFlagsND PER_CELL_AND_SINGLE = MaterialND::ColorSourceFlagsND(MaterialND::COLOR_SOURCE_FLAG_PER_CELL | MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);
constexpr MaterialND::ColorSourceFlagsND PER_VERT_ONLY = MaterialND::COLOR_SOURCE_FLAG_PER_VERT;
constexpr MaterialND::ColorSourceFlagsND PER_VERT_AND_SINGLE = MaterialND::ColorSourceFlagsND(MaterialND::COLOR_SOURCE_FLAG_PER_VERT | MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);

inline Ref<PolyMaterialND> make_single_color_poly_material(const Color &p_color) {
	Ref<PolyMaterialND> material;
	material.instantiate();
	material->set_albedo_source_flags(MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);
	material->set_albedo_color(p_color);
	return material;
}

inline Ref<PolyMaterialND> make_per_cell_poly_material(const PackedColorArray &p_cell_colors) {
	Ref<PolyMaterialND> material;
	material.instantiate();
	material->set_albedo_source_flags(PER_CELL_ONLY);
	material->set_poly_albedo_color_array(p_cell_colors);
	return material;
}

// A 4D box has 8 boundary cells (cubes).
inline Ref<ArrayPolyMeshND> make_box_array_mesh() {
	Ref<BoxPolyMeshND> box;
	box.instantiate();
	box->set_size(VectorND::fill(4, 1.0));
	return box->to_array_poly_mesh();
}

TEST_CASE("[PolyMaterialND] Merge materials") {
	const Color red = Color(1, 0, 0);
	const Color green = Color(0, 1, 0);
	const Color blue = Color(0, 0, 1);
	const Color white = Color(1, 1, 1);

	SUBCASE("Merging two different single colors produces a per-cell color array") {
		Ref<PolyMaterialND> first = make_single_color_poly_material(red);
		Ref<PolyMaterialND> second = make_single_color_poly_material(blue);
		first->merge_with(second, 3, 2);
		CHECK(first->get_albedo_source_flags() == PER_CELL_ONLY);
		CHECK_MESSAGE((first->get_poly_albedo_color_array() == PackedColorArray{ red, red, red, blue, blue }), "The cell counts should decide how many of each single color end up in the per-cell array.");
		CHECK_MESSAGE(first->get_albedo_color_array().is_empty(), "The per-simplex array is a cache that should be left empty until populated for a mesh.");
		CHECK_MESSAGE(second->get_poly_albedo_color_array().is_empty(), "Merging must not modify the other material.");
		CHECK(second->get_albedo_source_flags() == MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);
	}

	SUBCASE("Merging the same single color keeps a single color") {
		Ref<PolyMaterialND> first = make_single_color_poly_material(red);
		Ref<PolyMaterialND> second = make_single_color_poly_material(red);
		first->merge_with(second, 3, 2);
		CHECK(first->get_albedo_source_flags() == MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);
		CHECK(first->get_albedo_color() == red);
		CHECK(first->get_poly_albedo_color_array().is_empty());
	}

	SUBCASE("Merging a per-cell array with a single color appends the single color") {
		Ref<PolyMaterialND> first = make_per_cell_poly_material(PackedColorArray{ red, green });
		Ref<PolyMaterialND> second = make_single_color_poly_material(blue);
		first->merge_with(second, 2, 1);
		CHECK(first->get_albedo_source_flags() == PER_CELL_ONLY);
		CHECK((first->get_poly_albedo_color_array() == PackedColorArray{ red, green, blue }));
	}

	SUBCASE("Merging a single color with a per-cell array prepends the single color") {
		Ref<PolyMaterialND> first = make_single_color_poly_material(blue);
		Ref<PolyMaterialND> second = make_per_cell_poly_material(PackedColorArray{ red, green });
		first->merge_with(second, 1, 2);
		CHECK(first->get_albedo_source_flags() == PER_CELL_ONLY);
		CHECK((first->get_poly_albedo_color_array() == PackedColorArray{ blue, red, green }));
	}

	SUBCASE("Merging two per-cell arrays concatenates them") {
		Ref<PolyMaterialND> first = make_per_cell_poly_material(PackedColorArray{ red, green });
		Ref<PolyMaterialND> second = make_per_cell_poly_material(PackedColorArray{ blue, white, red });
		first->merge_with(second, 2, 3);
		CHECK(first->get_albedo_source_flags() == PER_CELL_ONLY);
		CHECK((first->get_poly_albedo_color_array() == PackedColorArray{ red, green, blue, white, red }));
	}

	SUBCASE("Merging a per-cell and single color material bakes the single color when it cannot be kept") {
		const Color gray = Color(0.5, 0.5, 0.5);
		Ref<PolyMaterialND> first = make_per_cell_poly_material(PackedColorArray{ red, green });
		first->set_albedo_source_flags(PER_CELL_AND_SINGLE);
		first->set_albedo_color(gray);
		Ref<PolyMaterialND> second = make_single_color_poly_material(blue);
		first->merge_with(second, 2, 1);
		CHECK(first->get_albedo_source_flags() == PER_CELL_ONLY);
		CHECK((first->get_poly_albedo_color_array() == PackedColorArray{ red * gray, green * gray, blue }));
	}

	SUBCASE("Merging a per-cell and single color material keeps the single color when it matches") {
		const Color gray = Color(0.5, 0.5, 0.5);
		Ref<PolyMaterialND> first = make_per_cell_poly_material(PackedColorArray{ red, green });
		first->set_albedo_source_flags(PER_CELL_AND_SINGLE);
		first->set_albedo_color(gray);
		Ref<PolyMaterialND> second = make_single_color_poly_material(gray);
		first->merge_with(second, 2, 1);
		CHECK(first->get_albedo_source_flags() == PER_CELL_AND_SINGLE);
		CHECK(first->get_albedo_color() == gray);
		CHECK((first->get_poly_albedo_color_array() == PackedColorArray{ red, green, white }));
	}

	SUBCASE("Merging with a non-poly material's color array keeps only its single color") {
		Ref<PolyMaterialND> first = make_single_color_poly_material(red);
		Ref<CellMaterialND> second;
		second.instantiate();
		second->set_albedo_source_flags(PER_VERT_AND_SINGLE);
		second->set_albedo_color(blue);
		second->set_albedo_color_array(PackedColorArray{ green, green, green, green });
		ERR_PRINT_OFF;
		first->merge_with(second, 2, 1);
		ERR_PRINT_ON;
		CHECK(first->get_albedo_source_flags() == PER_CELL_ONLY);
		CHECK_MESSAGE((first->get_poly_albedo_color_array() == PackedColorArray{ red, red, blue }), "Per-vertex colors cannot be mapped to cells, so only the other material's single color is used.");
	}

	SUBCASE("Merging with a non-poly material's color array without a single color inserts white") {
		Ref<PolyMaterialND> first = make_single_color_poly_material(red);
		Ref<CellMaterialND> second;
		second.instantiate();
		second->set_albedo_source_flags(PER_VERT_ONLY);
		second->set_albedo_color_array(PackedColorArray{ green, green, green, green });
		ERR_PRINT_OFF;
		first->merge_with(second, 2, 1);
		ERR_PRINT_ON;
		CHECK(first->get_albedo_source_flags() == PER_CELL_ONLY);
		CHECK((first->get_poly_albedo_color_array() == PackedColorArray{ red, red, white }));
	}
}

TEST_CASE("[ArrayPolyMeshND] Merge meshes with poly materials") {
	const Color red = Color(1, 0, 0);
	const Color blue = Color(0, 0, 1);
	const Color white = Color(1, 1, 1);
	const Ref<TransformND> offset = TransformND::from_position(VectorN{ 10, 0, 0, 0 });

	SUBCASE("Merging two boxes with different single colors gives one color per boundary cell") {
		Ref<ArrayPolyMeshND> mesh = make_box_array_mesh();
		Ref<PolyMaterialND> red_material = make_single_color_poly_material(red);
		mesh->set_material(red_material);
		Ref<ArrayPolyMeshND> other = make_box_array_mesh();
		Ref<PolyMaterialND> blue_material = make_single_color_poly_material(blue);
		other->set_material(blue_material);
		mesh->merge_with(other, offset);
		const Ref<PolyMaterialND> merged_material = mesh->get_material();
		REQUIRE_MESSAGE(merged_material.is_valid(), "The merged material should still be a PolyMaterialND.");
		CHECK_MESSAGE(merged_material.ptr() != red_material.ptr(), "Merging should not mutate the original material, which may be shared with other meshes.");
		CHECK(red_material->get_albedo_source_flags() == MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);
		CHECK(red_material->get_poly_albedo_color_array().is_empty());
		CHECK(blue_material->get_albedo_source_flags() == MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);
		CHECK(merged_material->get_albedo_source_flags() == PER_CELL_ONLY);
		const PackedColorArray cell_colors = merged_material->get_poly_albedo_color_array();
		REQUIRE_MESSAGE(cell_colors.size() == 16, "Each box has 8 boundary cells, so the merged material should have 16 cell colors, not one per vertex.");
		for (int64_t i = 0; i < 8; i++) {
			CHECK(cell_colors[i] == red);
			CHECK(cell_colors[8 + i] == blue);
		}
		// The per-cell colors can be expanded to one color per simplex of the merged mesh.
		merged_material->populate_albedo_color_array_for_poly_mesh(mesh);
		const PackedColorArray simplex_colors = merged_material->get_albedo_color_array();
		const int64_t simplex_count = mesh->get_simplex_cell_vertex_indices().size() / 4;
		REQUIRE(simplex_count > 0);
		REQUIRE(simplex_colors.size() == simplex_count);
		CHECK(simplex_colors[0] == red);
		CHECK(simplex_colors[simplex_count - 1] == blue);
	}

	SUBCASE("Merging into a mesh without a material creates a poly material with white for the existing cells") {
		Ref<ArrayPolyMeshND> mesh = make_box_array_mesh();
		Ref<ArrayPolyMeshND> other = make_box_array_mesh();
		PackedColorArray other_cell_colors;
		for (int i = 0; i < 8; i++) {
			other_cell_colors.append(Color(i / 8.0f, 0, 1));
		}
		Ref<PolyMaterialND> other_material = make_per_cell_poly_material(other_cell_colors);
		other->set_material(other_material);
		mesh->merge_with(other, offset);
		const Ref<PolyMaterialND> merged_material = mesh->get_material();
		REQUIRE_MESSAGE(merged_material.is_valid(), "The new material should be a PolyMaterialND to hold the per-cell colors.");
		CHECK(merged_material.ptr() != other_material.ptr());
		CHECK_MESSAGE(merged_material->get_albedo_source_flags() == PER_CELL_AND_SINGLE, "Both materials have a white single color, so it can be kept alongside the per-cell array.");
		CHECK(merged_material->get_albedo_color() == white);
		const PackedColorArray cell_colors = merged_material->get_poly_albedo_color_array();
		REQUIRE(cell_colors.size() == 16);
		for (int64_t i = 0; i < 8; i++) {
			CHECK(cell_colors[i] == white);
			CHECK(cell_colors[8 + i] == other_cell_colors[i]);
		}
		CHECK_MESSAGE((other_material->get_poly_albedo_color_array() == other_cell_colors), "Merging must not modify the other mesh's material.");
	}

	SUBCASE("Merging into a mesh without a material shares a single color material") {
		Ref<ArrayPolyMeshND> mesh = make_box_array_mesh();
		Ref<ArrayPolyMeshND> other = make_box_array_mesh();
		Ref<PolyMaterialND> other_material = make_single_color_poly_material(blue);
		other->set_material(other_material);
		mesh->merge_with(other, offset);
		CHECK_MESSAGE(mesh->get_material().ptr() == other_material.ptr(), "A single color needs no per-cell data, so the material can be shared as-is.");
	}
}
} // namespace TestPolyMaterialND
