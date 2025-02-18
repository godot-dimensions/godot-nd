#include "material_nd.h"

Color MaterialND::get_albedo_color() const {
	return _albedo_color;
}

void MaterialND::set_albedo_color(const Color &p_albedo_color) {
	_albedo_color = p_albedo_color;
}

PackedColorArray MaterialND::get_albedo_color_array() const {
	return _albedo_color_array;
}

void MaterialND::set_albedo_color_array(const PackedColorArray &p_albedo_color_array) {
	_albedo_color_array = p_albedo_color_array;
}

void MaterialND::append_albedo_color(const Color &p_albedo_color) {
	_albedo_color_array.push_back(p_albedo_color);
}

void MaterialND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_albedo_color"), &MaterialND::get_albedo_color);
	ClassDB::bind_method(D_METHOD("set_albedo_color", "albedo_color"), &MaterialND::set_albedo_color);

	ClassDB::bind_method(D_METHOD("get_albedo_color_array"), &MaterialND::get_albedo_color_array);
	ClassDB::bind_method(D_METHOD("set_albedo_color_array", "albedo_color_array"), &MaterialND::set_albedo_color_array);
	ClassDB::bind_method(D_METHOD("append_albedo_color", "albedo_color"), &MaterialND::append_albedo_color);
}
