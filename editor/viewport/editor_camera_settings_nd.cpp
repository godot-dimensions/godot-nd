#include "editor_camera_settings_nd.h"

void EditorCameraSettingsND::set_clip_near(const double p_clip_near) {
	_clip_near = p_clip_near;
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::set_clip_far(const double p_clip_far) {
	_clip_far = p_clip_far;
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::set_perp_fade_mode(const CameraND::PerpFadeMode p_perp_fade_mode) {
	_perp_fade_mode = p_perp_fade_mode;
	notify_property_list_changed();
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::set_perp_fade_distance(const double p_perp_fade_distance) {
	_perp_fade_distance = p_perp_fade_distance;
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::set_perp_fade_slope(const double p_perp_fade_slope) {
	_perp_fade_slope = p_perp_fade_slope;
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::set_rendering_engine(const String &p_rendering_engine) {
	_rendering_engine = p_rendering_engine;
	notify_property_list_changed();
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::apply_to_cameras() const {
	TypedArray<Node> cameras = _ancestor_of_cameras->find_children("*", "CameraND", true, false);
	for (int i = 0; i < cameras.size(); i++) {
		CameraND *camera = Object::cast_to<CameraND>(cameras[i]);
		CRASH_COND(camera == nullptr);
		camera->set_clip_near(_clip_near);
		camera->set_clip_far(_clip_far);
		camera->set_perp_fade_mode(_perp_fade_mode);
		camera->set_perp_fade_distance(_perp_fade_distance);
		camera->set_perp_fade_slope(_perp_fade_slope);
		camera->set_rendering_engine(_rendering_engine);
	}
}

void EditorCameraSettingsND::setup(Node *p_ancestor_of_cameras, Ref<ConfigFile> &p_config_file, const String &p_config_file_path) {
	_ancestor_of_cameras = p_ancestor_of_cameras;
	_nd_editor_config_file = p_config_file;
	_nd_editor_config_file_path = p_config_file_path;
	_clip_near = p_config_file->get_value("camera", "clip_near", _clip_near);
	_clip_far = p_config_file->get_value("camera", "clip_far", _clip_far);
	_perp_fade_mode = (CameraND::PerpFadeMode)(int)p_config_file->get_value("camera", "perp_fade_mode", _perp_fade_mode);
	_perp_fade_distance = p_config_file->get_value("camera", "perp_fade_distance", _perp_fade_distance);
	_perp_fade_slope = p_config_file->get_value("camera", "perp_fade_slope", _perp_fade_slope);
	_rendering_engine = p_config_file->get_value("camera", "rendering_engine", _rendering_engine);
	apply_to_cameras();
}

void EditorCameraSettingsND::write_to_config_file() const {
	if (_nd_editor_config_file->has_section("camera")) {
		_nd_editor_config_file->erase_section("camera");
	}
	if (!Math::is_equal_approx(_clip_near, 0.05)) {
		_nd_editor_config_file->set_value("camera", "clip_near", _clip_near);
	}
	if (!Math::is_equal_approx(_clip_far, 4000.0)) {
		_nd_editor_config_file->set_value("camera", "clip_far", _clip_far);
	}
	if (_perp_fade_mode != CameraND::PERP_FADE_TRANSPARENCY) {
		_nd_editor_config_file->set_value("camera", "perp_fade_mode", (int)_perp_fade_mode);
	}
	if (!Math::is_equal_approx(_perp_fade_distance, 5.0)) {
		_nd_editor_config_file->set_value("camera", "perp_fade_distance", _perp_fade_distance);
	}
	if (!Math::is_equal_approx(_perp_fade_slope, 1.0)) {
		_nd_editor_config_file->set_value("camera", "perp_fade_slope", _perp_fade_slope);
	}
	if (!_rendering_engine.is_empty()) {
		_nd_editor_config_file->set_value("camera", "rendering_engine", _rendering_engine);
	}
	_nd_editor_config_file->save(_nd_editor_config_file_path);
}

void EditorCameraSettingsND::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == StringName("clip_far")) {
		if (_rendering_engine == "Wireframe Canvas") {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_color_negative")) {
		if (!(_perp_fade_mode & CameraND::PERP_FADE_HUE_SHIFT)) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_color_positive")) {
		if (!(_perp_fade_mode & CameraND::PERP_FADE_HUE_SHIFT)) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_distance")) {
		if (_perp_fade_mode == CameraND::PERP_FADE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_slope")) {
		if (_perp_fade_mode == CameraND::PERP_FADE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	}
}

void EditorCameraSettingsND::_bind_methods() {
	// These are copies of the CameraND properties relevant for the editor camera.
	// Be sure to keep these in sync with CameraND.
	ClassDB::bind_method(D_METHOD("get_clip_near"), &EditorCameraSettingsND::get_clip_near);
	ClassDB::bind_method(D_METHOD("set_clip_near", "clip_near"), &EditorCameraSettingsND::set_clip_near);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "clip_near", PROPERTY_HINT_RANGE, "0.001,1000,0.001,or_greater,exp,suffix:m"), "set_clip_near", "get_clip_near");

	ClassDB::bind_method(D_METHOD("get_clip_far"), &EditorCameraSettingsND::get_clip_far);
	ClassDB::bind_method(D_METHOD("set_clip_far", "clip_far"), &EditorCameraSettingsND::set_clip_far);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "clip_far", PROPERTY_HINT_RANGE, "0.01,4000,0.01,or_greater,exp,suffix:m"), "set_clip_far", "get_clip_far");

	ClassDB::bind_method(D_METHOD("get_perp_fade_mode"), &EditorCameraSettingsND::get_perp_fade_mode);
	ClassDB::bind_method(D_METHOD("set_perp_fade_mode", "perp_fade_mode"), &EditorCameraSettingsND::set_perp_fade_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "perp_fade_mode", PROPERTY_HINT_ENUM, "Disabled,Transparency,Hue Shift,Transparency + Hue Shift"), "set_perp_fade_mode", "get_perp_fade_mode");

	ClassDB::bind_method(D_METHOD("get_perp_fade_distance"), &EditorCameraSettingsND::get_perp_fade_distance);
	ClassDB::bind_method(D_METHOD("set_perp_fade_distance", "perp_fade_distance"), &EditorCameraSettingsND::set_perp_fade_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "perp_fade_distance", PROPERTY_HINT_RANGE, "0.01,10,0.001,or_greater,exp,suffix:m"), "set_perp_fade_distance", "get_perp_fade_distance");

	ClassDB::bind_method(D_METHOD("get_perp_fade_slope"), &EditorCameraSettingsND::get_perp_fade_slope);
	ClassDB::bind_method(D_METHOD("set_perp_fade_slope", "perp_fade_slope"), &EditorCameraSettingsND::set_perp_fade_slope);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "perp_fade_slope", PROPERTY_HINT_RANGE, "0.01,10,0.001,or_less,or_greater,exp"), "set_perp_fade_slope", "get_perp_fade_slope");
}
