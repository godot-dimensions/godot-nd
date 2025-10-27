#include "material_nd.h"

#include "mesh_nd.h"

Color MaterialND::get_albedo_color_of_edge(const int64_t p_edge_index, const Ref<MeshND> &p_for_mesh) {
	if (!(_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY)) {
		// No need to allocate any memory for _edge_albedo_color_cache if the color array is not used.
		if (likely(_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR)) {
			return _albedo_color;
		}
		return Color(1, 1, 1, 1);
	}
	if (p_edge_index < _edge_albedo_color_cache.size()) {
		return _edge_albedo_color_cache[p_edge_index];
	}
	ERR_FAIL_COND_V_MSG(p_for_mesh.is_null(), _albedo_color, "MaterialND: Mesh is null.");
	const PackedInt32Array edge_indices = p_for_mesh->get_edge_indices();
	const int64_t edge_count = edge_indices.size() / 2;
	ERR_FAIL_COND_V_MSG(p_edge_index >= edge_count, _albedo_color, "MaterialND: Edge index out of bounds for mesh.");
	if (_albedo_source_flags & COLOR_SOURCE_FLAG_PER_EDGE) {
		ERR_FAIL_COND_V_MSG(edge_count > _albedo_color_array.size(), _albedo_color, "MaterialND: Mesh has more edges than the material's color array.");
		if (_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) {
			_edge_albedo_color_cache.resize(edge_count);
			for (int64_t i = 0; i < edge_count; i++) {
				_edge_albedo_color_cache.set(i, _albedo_color_array[i] * _albedo_color);
			}
		} else {
			_edge_albedo_color_cache = _albedo_color_array.duplicate();
		}
	} else if (_albedo_source_flags & COLOR_SOURCE_FLAG_PER_VERT) {
		_edge_albedo_color_cache.resize(edge_count);
		for (int64_t i = 0; i < edge_count; i++) {
			const int32_t first_vertex = edge_indices[i * 2];
			const int32_t second_vertex = edge_indices[i * 2 + 1];
			ERR_FAIL_INDEX_V_MSG(first_vertex, _albedo_color_array.size(), _albedo_color, "MaterialND: Cannot get vertex color of mesh because the first vertex index is out of bounds of the material's color array.");
			ERR_FAIL_INDEX_V_MSG(second_vertex, _albedo_color_array.size(), _albedo_color, "MaterialND: Cannot get vertex color of mesh because the second vertex index is out of bounds of the material's color array.");
			const Color per_edge_color = (_albedo_color_array[first_vertex] + _albedo_color_array[second_vertex]) * 0.5f;
			if (_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) {
				_edge_albedo_color_cache.set(i, per_edge_color * _albedo_color);
			} else {
				_edge_albedo_color_cache.set(i, per_edge_color);
			}
		}
	} else if (_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) {
		_edge_albedo_color_cache.resize(edge_count);
		_edge_albedo_color_cache.fill(_albedo_color);
	} else {
		_edge_albedo_color_cache.resize(edge_count);
		_edge_albedo_color_cache.fill(Color(1, 1, 1, 1));
	}
	return _albedo_color;
}

bool MaterialND::is_default_material() const {
	return _albedo_source_flags == COLOR_SOURCE_FLAG_SINGLE_COLOR && _albedo_color == Color(1, 1, 1, 1);
}

void MaterialND::merge_with(const Ref<MaterialND> &p_material, const int p_first_item_count, const int p_second_item_count) {
	const Color start_albedo_color = _albedo_color;
	const ColorSourceFlagsND start_albedo_source_flags = _albedo_source_flags;
	_albedo_source_flags = ColorSourceFlagsND(_albedo_source_flags | p_material->_albedo_source_flags);
	// Check if a single color is used, and if it's the same and we can keep it, or if we need to get rid of it.
	if (_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) {
		if (_albedo_color != p_material->_albedo_color) {
			// They don't use the same color, so we need to switch to using a color array.
			_albedo_source_flags = ColorSourceFlagsND(_albedo_source_flags & ~COLOR_SOURCE_FLAG_SINGLE_COLOR);
			if (!(_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY)) {
				_albedo_source_flags = ColorSourceFlagsND(_albedo_source_flags | COLOR_SOURCE_FLAG_USES_COLOR_ARRAY);
			}
		}
	}
	if (_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY) {
		const int albedo_color_array_end_size = p_first_item_count + p_second_item_count;
		_albedo_color_array.resize(albedo_color_array_end_size);
		// Convert existing color array data from this material if needed.
		if (!(start_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY)) {
			// Start does not use a color array, so we need to insert white. This may get multiplied by the start single color next.
			for (int i = 0; i < p_first_item_count; i++) {
				_albedo_color_array.set(i, Color(1, 1, 1, 1));
			}
		}
		if (!(_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) && (start_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR)) {
			// Start uses a single color, but destination does not, so we need to "bake" the start color into the array.
			for (int i = 0; i < p_first_item_count; i++) {
				_albedo_color_array.set(i, _albedo_color_array[i] * start_albedo_color);
			}
		}
		// Merge the other material's color array data.
		if (p_material->_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY) {
			// Other has a color array, so copy it over.
			ERR_FAIL_COND_MSG(p_material->_albedo_color_array.size() < p_second_item_count, "MaterialND.merge_with: Other material's color array has fewer than the declared amount of colors.");
			for (int i = 0; i < p_second_item_count; i++) {
				_albedo_color_array.set(p_first_item_count + i, p_material->_albedo_color_array[i]);
			}
		} else {
			// Other does not use a color array, so we need to insert white. This may get multiplied by the other single color next.
			for (int i = p_first_item_count; i < albedo_color_array_end_size; i++) {
				_albedo_color_array.set(i, Color(1, 1, 1, 1));
			}
		}
		if (!(_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) && (p_material->_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR)) {
			// Other uses a single color, but destination does not, so we need to "bake" the other color into the array.
			for (int i = p_first_item_count; i < albedo_color_array_end_size; i++) {
				_albedo_color_array.set(i, _albedo_color_array[i] * p_material->_albedo_color);
			}
		}
	}
}

void MaterialND::set_albedo_color(const Color &p_albedo_color) {
	_albedo_color = p_albedo_color;
	_edge_albedo_color_cache.clear();
}

void MaterialND::set_albedo_source_flags(const ColorSourceFlagsND p_albedo_source_flags) {
	_albedo_source_flags = p_albedo_source_flags;
	_edge_albedo_color_cache.clear();
}

void MaterialND::set_albedo_color_array(const PackedColorArray &p_albedo_color_array) {
	_albedo_color_array = p_albedo_color_array;
	_edge_albedo_color_cache.clear();
}

void MaterialND::append_albedo_color(const Color &p_albedo_color) {
	_albedo_color_array.push_back(p_albedo_color);
}

void MaterialND::resize_albedo_color_array(const int64_t p_size, const Color &p_fill_color) {
	const int64_t existing_size = _albedo_color_array.size();
	_albedo_color_array.resize(p_size);
	for (int64_t i = existing_size; i < p_size; i++) {
		_albedo_color_array.set(i, p_fill_color);
	}
}

void MaterialND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_albedo_color_of_edge", "edge_index", "for_mesh"), &MaterialND::get_albedo_color_of_edge);
	ClassDB::bind_method(D_METHOD("is_default_material"), &MaterialND::is_default_material);

	ClassDB::bind_method(D_METHOD("get_albedo_color"), &MaterialND::get_albedo_color);
	ClassDB::bind_method(D_METHOD("set_albedo_color", "albedo_color"), &MaterialND::set_albedo_color);

	ClassDB::bind_method(D_METHOD("get_albedo_source_flags"), &MaterialND::get_albedo_source_flags);
	ClassDB::bind_method(D_METHOD("set_albedo_source_flags", "albedo_source_flags"), &MaterialND::set_albedo_source_flags);

	ClassDB::bind_method(D_METHOD("get_albedo_color_array"), &MaterialND::get_albedo_color_array);
	ClassDB::bind_method(D_METHOD("set_albedo_color_array", "albedo_color_array"), &MaterialND::set_albedo_color_array);
	ClassDB::bind_method(D_METHOD("append_albedo_color", "albedo_color"), &MaterialND::append_albedo_color);
	ClassDB::bind_method(D_METHOD("resize_albedo_color_array", "size", "fill_color"), &MaterialND::resize_albedo_color_array, DEFVAL(Color(1, 1, 1, 1)));

	BIND_ENUM_CONSTANT(COLOR_SOURCE_FLAG_SINGLE_COLOR);
	BIND_ENUM_CONSTANT(COLOR_SOURCE_FLAG_PER_VERT);
	BIND_ENUM_CONSTANT(COLOR_SOURCE_FLAG_PER_EDGE);
	BIND_ENUM_CONSTANT(COLOR_SOURCE_FLAG_PER_CELL);
	BIND_ENUM_CONSTANT(COLOR_SOURCE_FLAG_CELL_TEXTURE_MAP);
	BIND_ENUM_CONSTANT(COLOR_SOURCE_FLAG_DIRECT_TEXTURE_MAP);
}
