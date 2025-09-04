#pragma once

#include "editor_viewport_nd_defines.h"

#include "../../nodes/camera_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/config_file.hpp>
#elif GODOT_MODULE
#include "core/io/config_file.h"
#endif

class EditorCameraSettingsND : public Object {
	GDCLASS(EditorCameraSettingsND, Object);

	Node *_ancestor_of_cameras = nullptr;

	double _clip_near = 0.05;
	double _clip_far = 4000.0;

	CameraND::PerpFadeMode _perp_fade_mode = CameraND::PERP_FADE_TRANSPARENCY;
	double _perp_fade_distance = 5.0;
	double _perp_fade_slope = 1.0;

	Ref<ConfigFile> _nd_editor_config_file;
	String _nd_editor_config_file_path = "";
	String _rendering_engine = "";

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	double get_clip_near() const { return _clip_near; }
	void set_clip_near(const double p_clip_near);

	double get_clip_far() const { return _clip_far; }
	void set_clip_far(const double p_clip_far);

	CameraND::PerpFadeMode get_perp_fade_mode() const { return _perp_fade_mode; }
	void set_perp_fade_mode(const CameraND::PerpFadeMode p_perp_fade_mode);

	double get_perp_fade_distance() const { return _perp_fade_distance; }
	void set_perp_fade_distance(const double p_perp_fade_distance);

	double get_perp_fade_slope() const { return _perp_fade_slope; }
	void set_perp_fade_slope(const double p_perp_fade_slope);

	// Only a setter because the source of truth for this should be the rendering engine menu.
	void set_rendering_engine(const String &p_rendering_engine);

	void apply_to_cameras() const;
	void setup(Node *p_ancestor_of_cameras, Ref<ConfigFile> &p_config_file, const String &p_config_file_path);
	void write_to_config_file() const;
};
