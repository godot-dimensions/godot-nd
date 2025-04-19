#include "cell_material_nd.h"

CellMaterialND::CellColorSourceND CellMaterialND::get_albedo_source() const {
	return _albedo_source;
}

void CellMaterialND::set_albedo_source(const CellColorSourceND p_albedo_source) {
	_albedo_source = p_albedo_source;
	notify_property_list_changed();
}

void CellMaterialND::_get_property_list(List<PropertyInfo> *p_list) const {
	for (List<PropertyInfo>::Element *E = p_list->front(); E; E = E->next()) {
		PropertyInfo &prop = E->get();
		if (prop.name == StringName("albedo_color")) {
			prop.usage = (_albedo_source & CELL_COLOR_SOURCE_SINGLE_COLOR) ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_NONE;
		} else if (prop.name == StringName("albedo_color_array")) {
			prop.usage = (_albedo_source & CELL_COLOR_SOURCE_USES_COLOR_ARRAY) ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_NONE;
		}
	}
}

CellMaterialND::CellMaterialND() {
	set_albedo_source(CELL_COLOR_SOURCE_SINGLE_COLOR);
}

void CellMaterialND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_albedo_source"), &CellMaterialND::get_albedo_source);
	ClassDB::bind_method(D_METHOD("set_albedo_source", "albedo_source"), &CellMaterialND::set_albedo_source);

	//ADD_GROUP("Albedo", "albedo_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "albedo_source", PROPERTY_HINT_ENUM, "Single Color,Per Vertex Only,Per Cell Only,Cell UVW Only,TextureND Only,Per Vertex and Single Color,Per Cell and Single Color,Cell UVW and Single Color,TextureND and Single Color"), "set_albedo_source", "get_albedo_source");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "albedo_color"), "set_albedo_color", "get_albedo_color");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "albedo_color_array"), "set_albedo_color_array", "get_albedo_color_array");

	BIND_ENUM_CONSTANT(CELL_COLOR_SOURCE_SINGLE_COLOR);
	BIND_ENUM_CONSTANT(CELL_COLOR_SOURCE_PER_VERT_ONLY);
	BIND_ENUM_CONSTANT(CELL_COLOR_SOURCE_PER_CELL_ONLY);
	BIND_ENUM_CONSTANT(CELL_COLOR_SOURCE_CELL_TEXTURE_MAP_ONLY);
	BIND_ENUM_CONSTANT(CELL_COLOR_SOURCE_PER_VERT_AND_SINGLE);
	BIND_ENUM_CONSTANT(CELL_COLOR_SOURCE_PER_CELL_AND_SINGLE);
	BIND_ENUM_CONSTANT(CELL_COLOR_SOURCE_CELL_TEXTURE_MAP_AND_SINGLE);
}
