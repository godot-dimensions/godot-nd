#include "editor_camera_settings_nd.h"

void EditorCameraSettingsND::set_view_angle_type(const CameraND::ViewAngleType p_view_angle_type) {
	_view_angle_type = p_view_angle_type;
	notify_property_list_changed();
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::set_focal_length(const double p_focal_length) {
	_focal_length = p_focal_length;
	apply_to_cameras();
	write_to_config_file();
}

double EditorCameraSettingsND::get_field_of_view() const {
	return Math_PI - 2.0 * Math::atan(_focal_length);
}

void EditorCameraSettingsND::set_field_of_view(const double p_field_of_view) {
	_focal_length = Math::tan((Math_PI - p_field_of_view) * 0.5);
	apply_to_cameras();
	write_to_config_file();
}

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

void EditorCameraSettingsND::set_rendering_engine_name(const String &p_rendering_engine_name) {
	_rendering_engine_name = p_rendering_engine_name;
	notify_property_list_changed();
	apply_to_cameras();
	write_to_config_file();
}

void EditorCameraSettingsND::apply_to_cameras() const {
	TypedArray<Node> cameras = _ancestor_of_cameras->find_children("*", "CameraND", true, false);
	for (int i = 0; i < cameras.size(); i++) {
		CameraND *camera = Object::cast_to<CameraND>(cameras[i]);
		CRASH_COND(camera == nullptr);
		camera->set_view_angle_type(_view_angle_type);
		camera->set_focal_length(_focal_length);
		camera->set_clip_near(_clip_near);
		camera->set_clip_far(_clip_far);
		camera->set_perp_fade_mode(_perp_fade_mode);
		camera->set_perp_fade_distance(_perp_fade_distance);
		camera->set_perp_fade_slope(_perp_fade_slope);
		camera->set_rendering_engine_name(_rendering_engine_name);
	}
}

void EditorCameraSettingsND::setup(Node *p_ancestor_of_cameras, Ref<ConfigFile> &p_config_file, const String &p_config_file_path) {
	_ancestor_of_cameras = p_ancestor_of_cameras;
	_nd_editor_config_file = p_config_file;
	_nd_editor_config_file_path = p_config_file_path;
	_view_angle_type = (CameraND::ViewAngleType)(int)p_config_file->get_value("camera", "view_angle_type", _view_angle_type);
	_focal_length = p_config_file->get_value("camera", "focal_length", _focal_length);
	_clip_near = p_config_file->get_value("camera", "clip_near", _clip_near);
	_clip_far = p_config_file->get_value("camera", "clip_far", _clip_far);
	_perp_fade_mode = (CameraND::PerpFadeMode)(int)p_config_file->get_value("camera", "perp_fade_mode", _perp_fade_mode);
	_perp_fade_distance = p_config_file->get_value("camera", "perp_fade_distance", _perp_fade_distance);
	_perp_fade_slope = p_config_file->get_value("camera", "perp_fade_slope", _perp_fade_slope);
	// Keep this in sync with `EditorMainScreenND::_update_rendering_engine_menu()`.
	_rendering_engine_name = p_config_file->get_value("camera", "rendering_engine_name", _rendering_engine_name);
	apply_to_cameras();
}

void EditorCameraSettingsND::write_to_config_file() const {
	if (_nd_editor_config_file->has_section("camera")) {
		_nd_editor_config_file->erase_section("camera");
	}
	if (_view_angle_type != CameraND::VIEW_ANGLE_FOCAL_LENGTH) {
		_nd_editor_config_file->set_value("camera", "view_angle_type", (int)_view_angle_type);
	}
	if (!Math::is_equal_approx(_focal_length, 1.25)) {
		_nd_editor_config_file->set_value("camera", "focal_length", _focal_length);
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
	if (!_rendering_engine_name.is_empty()) {
		_nd_editor_config_file->set_value("camera", "rendering_engine_name", _rendering_engine_name);
	}
	_nd_editor_config_file->save(_nd_editor_config_file_path);
}

void EditorCameraSettingsND::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == StringName("focal_length")) {
		if (_view_angle_type != CameraND::VIEW_ANGLE_FOCAL_LENGTH) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("field_of_view")) {
		if (_view_angle_type != CameraND::VIEW_ANGLE_FIELD_OF_VIEW) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("clip_far")) {
		if (_rendering_engine_name == "Wireframe Canvas") {
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
	ClassDB::bind_method(D_METHOD("get_view_angle_type"), &EditorCameraSettingsND::get_view_angle_type);
	ClassDB::bind_method(D_METHOD("set_view_angle_type", "view_angle_type"), &EditorCameraSettingsND::set_view_angle_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "view_angle_type", PROPERTY_HINT_ENUM, "Focal Length,Field of View"), "set_view_angle_type", "get_view_angle_type");

	ClassDB::bind_method(D_METHOD("get_focal_length"), &EditorCameraSettingsND::get_focal_length);
	ClassDB::bind_method(D_METHOD("set_focal_length", "focal_length"), &EditorCameraSettingsND::set_focal_length);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "focal_length", PROPERTY_HINT_NONE, "suffix:m"), "set_focal_length", "get_focal_length");

	ClassDB::bind_method(D_METHOD("get_field_of_view"), &EditorCameraSettingsND::get_field_of_view);
	ClassDB::bind_method(D_METHOD("set_field_of_view", "field_of_view"), &EditorCameraSettingsND::set_field_of_view);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "field_of_view", PROPERTY_HINT_RANGE, "1,179,0.1,radians_as_degrees"), "set_field_of_view", "get_field_of_view");

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
