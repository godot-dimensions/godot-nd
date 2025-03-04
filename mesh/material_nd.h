#pragma once

#include "../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#elif GODOT_MODULE
#include "core/io/resource.h"
#endif

class MaterialND : public Resource {
	GDCLASS(MaterialND, Resource);

protected:
	static void _bind_methods();

	Color _albedo_color = Color(1, 1, 1, 1);
	PackedColorArray _albedo_color_array;

public:
	Color get_albedo_color() const;
	void set_albedo_color(const Color &p_albedo_color);

	PackedColorArray get_albedo_color_array() const;
	void set_albedo_color_array(const PackedColorArray &p_albedo_color_array);
	void append_albedo_color(const Color &p_albedo_color);
	void resize_albedo_color_array(const int64_t p_size, const Color &p_fill_color = Color(1, 1, 1, 1));
};
