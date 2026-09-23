#include "poly_material_nd.h"

// This function also supports CellMeshND converted from PolyMeshND
// or otherwise have simplexes grouped by their starting vertex.
void PolyMaterialND::populate_albedo_color_array_for_poly_mesh(const Ref<CellMeshND> &p_poly_mesh) {
	ERR_FAIL_COND(p_poly_mesh.is_null());
	const int64_t poly_color_array_size = _poly_albedo_color_array.size();
	if (poly_color_array_size == 0) {
		return; // Nothing to do.
	}
	const PackedInt32Array simplexes = p_poly_mesh->get_simplex_cell_vertex_indices();
	const int64_t indices_per_simplex = p_poly_mesh->get_indices_per_simplex_cell();
	ERR_FAIL_COND(indices_per_simplex < 1);
	ERR_FAIL_COND(simplexes.size() < indices_per_simplex || simplexes.size() % indices_per_simplex != 0);
	const int64_t simplex_count = simplexes.size() / indices_per_simplex;
	_albedo_color_array.clear();
	Ref<PolyMeshND> poly_mesh = p_poly_mesh;
	if (poly_mesh.is_valid()) {
		// Poly meshes track which poly cell each simplex came from, so use that exact mapping.
		for (int64_t simplex_index = 0; simplex_index < simplex_count; simplex_index++) {
			const int32_t source_cell = poly_mesh->get_source_poly_cell_for_simplex_cell(simplex_index);
			ERR_FAIL_INDEX(source_cell, poly_color_array_size);
			_albedo_color_array.append(_poly_albedo_color_array[source_cell]);
		}
		return;
	}
	// Otherwise, assume the simplexes are grouped by their starting (pivot) vertex.
	_albedo_color_array.append(_poly_albedo_color_array[0]);
	int64_t color_index = 0;
	int32_t last_simplex_start = simplexes[0];
	for (int64_t simplex_start_index = indices_per_simplex; simplex_start_index < simplexes.size(); simplex_start_index += indices_per_simplex) {
		const int32_t simplex_start = simplexes[simplex_start_index];
		if (simplex_start != last_simplex_start) {
			last_simplex_start = simplex_start;
			color_index++;
			ERR_FAIL_INDEX(color_index, poly_color_array_size);
		}
		_albedo_color_array.append(_poly_albedo_color_array[color_index]);
	}
}

// For a PolyMaterialND, the merged items are the polyhedral boundary cells colored by `poly_albedo_color_array`,
// so callers must pass the boundary cell counts of the meshes rather than their vertex counts.
void PolyMaterialND::merge_with(const Ref<MaterialND> &p_material, const int p_first_item_count, const int p_second_item_count) {
	ERR_FAIL_COND_MSG(p_material.is_null(), "PolyMaterialND.merge_with: Cannot merge with a null material.");
	// MaterialND::merge_with merges `_albedo_color_array`, but for PolyMaterialND that array is only a
	// per-simplex cache derived from `_poly_albedo_color_array`, which holds the real per-cell colors.
	// Build a view of the other material whose color array is per-cell, and merge our per-cell array with it.
	ColorSourceFlagsND other_flags = p_material->get_albedo_source_flags();
	Color other_color = p_material->get_albedo_color();
	PackedColorArray other_cell_colors;
	const Ref<PolyMaterialND> other_poly_material = p_material;
	if (other_poly_material.is_valid()) {
		other_cell_colors = other_poly_material->get_poly_albedo_color_array();
	} else if (other_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY) {
		// The other material's colors are per-vertex, per-simplex, or per-edge. Without the mesh there
		// is no way to map those onto polyhedral cells, so the best we can do is keep its single color.
		WARN_PRINT("PolyMaterialND.merge_with: The other material's color array cannot be converted to per-cell colors, so it will be ignored. Merge with a PolyMaterialND to preserve per-cell colors.");
		other_flags = ColorSourceFlagsND(other_flags & ~COLOR_SOURCE_FLAG_USES_COLOR_ARRAY);
		if (!(other_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR)) {
			other_flags = ColorSourceFlagsND(other_flags | COLOR_SOURCE_FLAG_SINGLE_COLOR);
			other_color = Color(1, 1, 1, 1);
		}
	}
	Ref<MaterialND> other_per_cell_material;
	other_per_cell_material.instantiate();
	other_per_cell_material->set_albedo_source_flags(other_flags);
	other_per_cell_material->set_albedo_color(other_color);
	other_per_cell_material->set_albedo_color_array(other_cell_colors);
	// Let the base classes merge the per-cell arrays and update the albedo source, then move the result back.
	_albedo_color_array = _poly_albedo_color_array;
	CellMaterialND::merge_with(other_per_cell_material, p_first_item_count, p_second_item_count);
	_poly_albedo_color_array = _albedo_color_array;
	_albedo_color_array.clear();
	// The base class only knows that a color array is now used, not which items it colors, so it enables every
	// per-item flag. For a PolyMaterialND the array always colors the boundary cells, so pin the flags to that.
	if (_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY) {
		_albedo_source_flags = ColorSourceFlagsND((_albedo_source_flags & ~COLOR_SOURCE_FLAG_USES_COLOR_ARRAY) | COLOR_SOURCE_FLAG_PER_CELL);
	}
}

void PolyMaterialND::set_poly_albedo_color_array(const PackedColorArray &p_colors) {
	_poly_albedo_color_array = p_colors;
	_albedo_color_array.clear();
}

void PolyMaterialND::_validate_property(PropertyInfo &p_property) const {
	CellMaterialND::_validate_property(p_property);
	if (p_property.name == StringName("poly_albedo_color_array")) {
		p_property.usage = (_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY) ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_NONE;
	} else if (p_property.name == StringName("albedo_color_array")) {
		p_property.usage = PROPERTY_USAGE_NONE;
	}
}

void PolyMaterialND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_poly_albedo_color_array"), &PolyMaterialND::get_poly_albedo_color_array);
	ClassDB::bind_method(D_METHOD("set_poly_albedo_color_array", "colors"), &PolyMaterialND::set_poly_albedo_color_array);

	ClassDB::bind_method(D_METHOD("populate_albedo_color_array_for_poly_mesh", "poly_mesh"), &PolyMaterialND::populate_albedo_color_array_for_poly_mesh);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "poly_albedo_color_array"), "set_poly_albedo_color_array", "get_poly_albedo_color_array");
}
