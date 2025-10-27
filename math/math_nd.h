#pragma once

#include "../godot_nd_defines.h"

// There is no real uint4_t type, but this semantically means we only use the 4 least significant bits.
using uint4_t = uint8_t;

// Math functions for the ND module that don't fit in any other file.
// Prefer using GeometryND, VectorND, etc if those are appropriate.
class MathND : public Object {
	GDCLASS(MathND, Object);

protected:
	static MathND *singleton;
	static void _bind_methods();

public:
	static double float4_to_double(const uint4_t p_float4);
	static double float8_to_double(const uint8_t p_float8);
	static double float16_to_double(const uint16_t p_float16);
	static uint4_t double_to_float4(const double p_double);
	static uint8_t double_to_float8(const double p_double);
	static uint16_t double_to_float16(const double p_double);

	static int32_t find_common_int32(const PackedInt32Array &p_a, const PackedInt32Array &p_b, int64_t &r_a_index, int64_t &r_b_index);

	static MathND *get_singleton() { return singleton; }
	MathND() { singleton = this; }
	~MathND() { singleton = nullptr; }
};
