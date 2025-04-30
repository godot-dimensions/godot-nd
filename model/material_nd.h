#pragma once

#include "../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#elif GODOT_MODULE
#include "core/io/resource.h"
#endif

class MeshND;

class MaterialND : public Resource {
	GDCLASS(MaterialND, Resource);

public:
	// TODO: Switch to BitField in a future Godot version https://github.com/godotengine/godot/pull/95916
	// Many of these values are not used in the current implementation but are reserved for future use.
	enum ColorSourceFlagsND {
		COLOR_SOURCE_FLAG_NONE = 0, // Use for errors.
		COLOR_SOURCE_FLAG_SINGLE_COLOR = 1 << 0, // Uniform coloring.
		COLOR_SOURCE_FLAG_PER_VERT = 1 << 1, // 0D element coloring, cannot be used with per-edge or per-cell.
		COLOR_SOURCE_FLAG_PER_EDGE = 1 << 2, // 1D element coloring, cannot be used with per-vert or per-cell.
		COLOR_SOURCE_FLAG_PER_CELL = 1 << 3, // ND element coloring, cannot be used with per-vert or per-edge.
		COLOR_SOURCE_FLAG_CELL_TEXTURE_MAP = 1 << 4, // ND simplex cell texture mapping to (N-1)-dimensional faces (like 2D texture for a 3D mesh).
		COLOR_SOURCE_FLAG_DIRECT_TEXTURE_MAP = 1 << 5, // ND space direct vertex position texture mapping to ND (like 3D texture for a 3D mesh).
		COLOR_SOURCE_FLAG_USES_COLOR_ARRAY = COLOR_SOURCE_FLAG_PER_VERT | COLOR_SOURCE_FLAG_PER_EDGE | COLOR_SOURCE_FLAG_PER_CELL,
		COLOR_SOURCE_FLAG_USES_TEXTURE = COLOR_SOURCE_FLAG_CELL_TEXTURE_MAP | COLOR_SOURCE_FLAG_DIRECT_TEXTURE_MAP,
	};

protected:
	static void _bind_methods();

	PackedColorArray _edge_albedo_color_cache;
	PackedColorArray _albedo_color_array;
	Color _albedo_color = Color(1, 1, 1, 1);
	ColorSourceFlagsND _albedo_source_flags = COLOR_SOURCE_FLAG_SINGLE_COLOR;

public:
	virtual Color get_albedo_color_of_edge(const int64_t p_edge_index, const Ref<MeshND> &p_for_mesh);
	bool is_default_material() const;

	Color get_albedo_color() const { return _albedo_color; }
	void set_albedo_color(const Color &p_albedo_color);

	ColorSourceFlagsND get_albedo_source_flags() const { return _albedo_source_flags; }
	void set_albedo_source_flags(const ColorSourceFlagsND p_albedo_source_flags);

	PackedColorArray get_albedo_color_array() const { return _albedo_color_array; }
	void set_albedo_color_array(const PackedColorArray &p_albedo_color_array);
	void append_albedo_color(const Color &p_albedo_color);
	void resize_albedo_color_array(const int64_t p_size, const Color &p_fill_color = Color(1, 1, 1, 1));
};

VARIANT_ENUM_CAST(MaterialND::ColorSourceFlagsND);
