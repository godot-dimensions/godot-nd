#pragma once

#include "../material_nd.h"

class WireMaterialND : public MaterialND {
	GDCLASS(WireMaterialND, MaterialND);

public:
	// TODO: Switch to BitField in a future Godot version https://github.com/godotengine/godot/pull/95916
	enum WireColorSource {
		WIRE_COLOR_SOURCE_SINGLE_COLOR,
		WIRE_COLOR_SOURCE_PER_EDGE_ONLY,
		WIRE_COLOR_SOURCE_PER_EDGE_AND_SINGLE,
	};

private:
	WireColorSource _albedo_source = WIRE_COLOR_SOURCE_SINGLE_COLOR;
	real_t _line_thickness = 0.0f;

protected:
	static void _bind_methods();
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	WireColorSource get_albedo_source() const;
	void set_albedo_source(const WireColorSource p_albedo_source);

	real_t get_line_thickness() const;
	void set_line_thickness(const real_t p_line_thickness);
};

VARIANT_ENUM_CAST(WireMaterialND::WireColorSource);
