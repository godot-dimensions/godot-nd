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
