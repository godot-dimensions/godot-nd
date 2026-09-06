#pragma once

#include "../../godot_nd_defines.h"

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
	enum ColorSourceFlagsND : uint16_t {
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

	enum TextureTransformMode : uint8_t {
		TEXTURE_TRANSFORM_MODE_NONE,
		TEXTURE_TRANSFORM_MODE_ALL_CHANNELS,
		TEXTURE_TRANSFORM_MODE_PER_CHANNEL,
	};

protected:
	static void _bind_methods();

	// Shared properties.
	VectorM _texture_map_offset;
	VectorM _texture_map_scale = VectorM({ 1.0 }); // A single value represents uniform scaling.
	// The texture scale and offset properties are only used when the texture transform mode is not NONE.
	TextureTransformMode _texture_transform_mode = TEXTURE_TRANSFORM_MODE_NONE;

	// Albedo.
	PackedColorArray _edge_albedo_color_cache;
	PackedColorArray _albedo_color_array;
	VectorM _albedo_texture_map_offset;
	VectorM _albedo_texture_map_scale = VectorM({ 1.0 }); // A single value represents uniform scaling.
	Color _albedo_color = Color(1, 1, 1, 1);
	ColorSourceFlagsND _albedo_source_flags = COLOR_SOURCE_FLAG_SINGLE_COLOR;

public:
	// Common functions.
	virtual Color get_albedo_color_of_edge(const int64_t p_edge_index, const Ref<MeshND> &p_for_mesh);
	bool is_default_material() const;
	virtual void merge_with(const Ref<MaterialND> &p_material, const int p_first_item_count, const int p_second_item_count);

	// Shared properties.
	TextureTransformMode get_texture_transform_mode() const { return _texture_transform_mode; }
	void set_texture_transform_mode(const TextureTransformMode p_texture_transform_mode);

	VectorM get_texture_map_offset() const { return _texture_map_offset; }
	void set_texture_map_offset(const VectorM &p_texture_map_offset);
	VectorM get_texture_map_scale() const { return _texture_map_scale; }
	void set_texture_map_scale(const VectorM &p_texture_map_scale);

	// Albedo.
	Color get_albedo_color() const { return _albedo_color; }
	void set_albedo_color(const Color &p_albedo_color);

	ColorSourceFlagsND get_albedo_source_flags() const { return _albedo_source_flags; }
	void set_albedo_source_flags(const ColorSourceFlagsND p_albedo_source_flags);

	PackedColorArray get_albedo_color_array() const { return _albedo_color_array; }
	void set_albedo_color_array(const PackedColorArray &p_albedo_color_array);
	void append_albedo_color(const Color &p_albedo_color);
	void resize_albedo_color_array(const int64_t p_size, const Color &p_fill_color = Color(1, 1, 1, 1));

	VectorM get_albedo_texture_map_offset() const { return _albedo_texture_map_offset; }
	void set_albedo_texture_map_offset(const VectorM &p_albedo_texture_map_offset);
	VectorM get_albedo_texture_map_scale() const { return _albedo_texture_map_scale; }
	void set_albedo_texture_map_scale(const VectorM &p_albedo_texture_map_scale);

	// Resolves the texture transform mode against the albedo channel's own offset/scale,
	// falling back to the shared "all channels" values or the identity as appropriate.
	VectorM get_effective_albedo_texture_map_offset() const;
	VectorM get_effective_albedo_texture_map_scale() const;
};

VARIANT_ENUM_CAST(MaterialND::ColorSourceFlagsND);
VARIANT_ENUM_CAST(MaterialND::TextureTransformMode);
