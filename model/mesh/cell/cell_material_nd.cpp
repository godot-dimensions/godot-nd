#include "cell_material_nd.h"

#include "cell_mesh_nd.h"

Color CellMaterialND::get_albedo_color_of_edge(const int64_t p_edge_index, const Ref<MeshND> &p_for_mesh) {
	if (!(_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY)) {
		// No need to allocate any memory for _edge_albedo_color_cache if the color array is not used.
		if (_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) {
			return _albedo_color;
		}
		return Color(1, 1, 1, 1);
	}
	if (p_edge_index < _edge_albedo_color_cache.size()) {
		return _edge_albedo_color_cache[p_edge_index];
	}
	ERR_FAIL_COND_V_MSG(p_for_mesh.is_null(), _albedo_color, "CellMaterialND: Mesh is null.");
	const PackedInt32Array edge_indices = p_for_mesh->get_edge_indices();
	const int64_t edge_count = edge_indices.size() / 2;
	ERR_FAIL_COND_V_MSG(p_edge_index >= edge_count, _albedo_color, "CellMaterialND: Edge index out of bounds for mesh.");
	if (_albedo_source_flags & COLOR_SOURCE_FLAG_PER_CELL) {
		Ref<CellMeshND> cell_mesh = p_for_mesh;
		ERR_FAIL_COND_V_MSG(cell_mesh.is_null(), _albedo_color, "CellMaterialND: Mesh with per-cell colors is not a cell mesh.");
		_edge_albedo_color_cache.resize(edge_count);
		const PackedInt32Array cell_indices = cell_mesh->get_cell_indices();
		const int64_t vertices_per_cell = cell_mesh->get_dimension();
		for (int64_t i = 0; i < edge_count; i++) {
			const int32_t first_vertex_index = edge_indices[i * 2];
			const int32_t second_vertex_index = edge_indices[i * 2 + 1];
			Color sum_color = Color(0, 0, 0, 0);
			int color_amount = 0;
			int64_t find_from = 0;
			while (true) {
				find_from = cell_indices.find(first_vertex_index, find_from);
				if (find_from < 0) {
					break;
				}
				// Truncated integer division to get the cell index.
				const int64_t cell_index = find_from / vertices_per_cell;
				ERR_FAIL_INDEX_V_MSG(cell_index, _albedo_color_array.size(), _albedo_color, "CellMaterialND: Cell index out of bounds for material's color array.");
				find_from = cell_indices.find(second_vertex_index, cell_index * vertices_per_cell);
				if (find_from < 0) {
					break;
				}
				const int64_t next_cell_start = (cell_index + 1) * vertices_per_cell;
				if (find_from < next_cell_start) {
					// The second vertex is in the same cell as the first vertex, therefore this cell has this edge.
					sum_color += _albedo_color_array[cell_index];
					color_amount++;
				}
				find_from = next_cell_start;
			}
			if (color_amount == 0) {
				// No color found, use the single color as a fallback even if the single color flag is not set.
				sum_color = _albedo_color;
			} else {
				sum_color /= color_amount;
				if (_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) {
					sum_color *= _albedo_color;
				}
			}
			_edge_albedo_color_cache.set(i, sum_color);
		}
	}
	return MaterialND::get_albedo_color_of_edge(p_edge_index, p_for_mesh);
}

void CellMaterialND::_get_property_list(List<PropertyInfo> *p_list) const {
	for (List<PropertyInfo>::Element *E = p_list->front(); E; E = E->next()) {
		PropertyInfo &prop = E->get();
		if (prop.name == StringName("albedo_color")) {
			prop.usage = (_albedo_source_flags & COLOR_SOURCE_FLAG_SINGLE_COLOR) ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_NONE;
		} else if (prop.name == StringName("albedo_color_array")) {
			prop.usage = (_albedo_source_flags & COLOR_SOURCE_FLAG_USES_COLOR_ARRAY) ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_NONE;
		}
	}
}

CellMaterialND::CellMaterialND() {
	set_albedo_source_flags(COLOR_SOURCE_FLAG_SINGLE_COLOR);
}

void CellMaterialND::_bind_methods() {
	//ADD_GROUP("Albedo", "albedo_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "albedo_source", PROPERTY_HINT_ENUM, "Single Color:1,Per Vertex Only:2,Per Vertex and Single Color:3,Per Cell Only:8,Per Cell and Single Color:9,Cell Texture Map Only:16,Cell Texture Map and Single Color:17,Cell Texture Map and Per Vertex:18,Cell Texture Map and Per Vertex and Single Color:19,Cell Texture Map and Per Cell:24,Cell Texture Map and Per Cell and and Single Color:25"), "set_albedo_source_flags", "get_albedo_source_flags");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "albedo_color"), "set_albedo_color", "get_albedo_color");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "albedo_color_array"), "set_albedo_color_array", "get_albedo_color_array");
}
