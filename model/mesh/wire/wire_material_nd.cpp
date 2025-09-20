#include "wire_material_nd.h"

#include "../mesh_nd.h"

MaterialND::ColorSourceFlagsND WireMaterialND::_wire_source_to_flags(const WireColorSourceND p_wire_source) {
	switch (p_wire_source) {
		case WireMaterialND::WIRE_COLOR_SOURCE_SINGLE_COLOR:
			return MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR;
		case WireMaterialND::WIRE_COLOR_SOURCE_PER_EDGE_ONLY:
			return MaterialND::COLOR_SOURCE_FLAG_PER_EDGE;
		case WireMaterialND::WIRE_COLOR_SOURCE_PER_EDGE_AND_SINGLE:
			return MaterialND::ColorSourceFlagsND(MaterialND::COLOR_SOURCE_FLAG_PER_EDGE | MaterialND::COLOR_SOURCE_FLAG_SINGLE_COLOR);
	}
	return MaterialND::COLOR_SOURCE_FLAG_NONE;
}

void WireMaterialND::set_albedo_source(const WireColorSourceND p_albedo_source) {
	_albedo_source = p_albedo_source;
	set_albedo_source_flags(_wire_source_to_flags(p_albedo_source));
	notify_property_list_changed();
}

real_t WireMaterialND::get_line_thickness() const {
	return _line_thickness;
}

void WireMaterialND::set_line_thickness(const real_t p_line_thickness) {
	_line_thickness = p_line_thickness;
}

void WireMaterialND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_line_thickness"), &WireMaterialND::get_line_thickness);
	ClassDB::bind_method(D_METHOD("set_line_thickness", "line_thickness"), &WireMaterialND::set_line_thickness);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "line_thickness"), "set_line_thickness", "get_line_thickness");

	ClassDB::bind_method(D_METHOD("get_albedo_source"), &WireMaterialND::get_albedo_source);
	ClassDB::bind_method(D_METHOD("set_albedo_source", "albedo_source"), &WireMaterialND::set_albedo_source);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "albedo_source", PROPERTY_HINT_ENUM, "Single Color,Per Edge Only,Per Edge and Single Color"), "set_albedo_source", "get_albedo_source");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "albedo_color"), "set_albedo_color", "get_albedo_color");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "albedo_color_array"), "set_albedo_color_array", "get_albedo_color_array");

	BIND_ENUM_CONSTANT(WIRE_COLOR_SOURCE_SINGLE_COLOR);
	BIND_ENUM_CONSTANT(WIRE_COLOR_SOURCE_PER_EDGE_ONLY);
	BIND_ENUM_CONSTANT(WIRE_COLOR_SOURCE_PER_EDGE_AND_SINGLE);
}

void WireMaterialND::_get_property_list(List<PropertyInfo> *p_list) const {
	for (List<PropertyInfo>::Element *E = p_list->front(); E; E = E->next()) {
		PropertyInfo &prop = E->get();
		if (prop.name == StringName("albedo_color")) {
			prop.usage = (_albedo_source == WIRE_COLOR_SOURCE_PER_EDGE_ONLY) ? PROPERTY_USAGE_NONE : PROPERTY_USAGE_DEFAULT;
		} else if (prop.name == StringName("albedo_color_array")) {
			prop.usage = (_albedo_source == WIRE_COLOR_SOURCE_SINGLE_COLOR) ? PROPERTY_USAGE_NONE : PROPERTY_USAGE_DEFAULT;
		}
	}
	MaterialND::_get_property_list(p_list);
}
