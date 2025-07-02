#include "camera_nd.h"

#include "../math/vector_nd.h"
#include "../render/rendering_server_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/viewport.hpp>
#elif GODOT_MODULE
#include "scene/main/viewport.h"
#endif

void CameraND::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == StringName("rendering_engine")) {
		PackedStringArray rendering_engine_names = RenderingServerND::get_singleton()->get_rendering_engine_names();
		p_property.hint_string = String(",").join(rendering_engine_names);
	} else if (p_property.name == StringName("view_angle_type")) {
		if (_projection_type != PROJECTION_PERSPECTIVE) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("focal_length")) {
		if (_view_angle_type != VIEW_ANGLE_FOCAL_LENGTH || _projection_type != PROJECTION_PERSPECTIVE) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("field_of_view")) {
		if (_view_angle_type != VIEW_ANGLE_FIELD_OF_VIEW || _projection_type != PROJECTION_PERSPECTIVE) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("orthographic_size")) {
		if (_projection_type != PROJECTION_ORTHOGRAPHIC) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_color_negative")) {
		if (!(_perp_fade_mode & PERP_FADE_HUE_SHIFT)) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_color_positive")) {
		if (!(_perp_fade_mode & PERP_FADE_HUE_SHIFT)) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_distance")) {
		if (_perp_fade_mode == PERP_FADE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	} else if (p_property.name == StringName("perp_fade_slope")) {
		if (_perp_fade_mode == PERP_FADE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	}
}

void CameraND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			RenderingServerND::get_singleton()->register_camera(this);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			RenderingServerND::get_singleton()->unregister_camera(this);
		} break;
	}
}

bool CameraND::is_current() const {
	return _is_current;
}

void CameraND::set_current(bool p_enabled) {
	_is_current = p_enabled;
	if (is_inside_tree()) {
		if (p_enabled) {
			RenderingServerND::get_singleton()->make_camera_current(this);
		} else {
			RenderingServerND::get_singleton()->clear_camera_current(this);
		}
	}
}

void CameraND::clear_current(bool p_enable_next) {
	_is_current = false;
	if (p_enable_next && is_inside_tree()) {
		RenderingServerND::get_singleton()->clear_camera_current(this);
	}
}

void CameraND::make_current() {
	_is_current = true;
	if (is_inside_tree()) {
		RenderingServerND::get_singleton()->make_camera_current(this);
	}
}

void CameraND::set_depth_fade(const bool p_depth_fade) {
	_use_depth_fade = p_depth_fade;
}

bool CameraND::get_depth_fade() const {
	return _use_depth_fade;
}

void CameraND::set_depth_fade_start(const double p_depth_fade_start) {
	_depth_fade_start = p_depth_fade_start;
}

double CameraND::get_depth_fade_start() const {
	return _depth_fade_start;
}

bool CameraND::is_position_behind(const VectorN &p_global_position) const {
	const Ref<TransformND> global_xform = get_global_transform();
	const VectorN depth_vector = global_xform->get_basis_column_raw(2);
	if (depth_vector.is_empty()) {
		return false;
	}
	const VectorN global_offset = VectorND::subtract(p_global_position, global_xform->get_origin());
	return VectorND::dot(depth_vector, global_offset) > -_clip_near;
}

VectorN CameraND::viewport_to_world_ray_origin(const Vector2 &p_viewport_position) const {
	ERR_FAIL_COND_V_MSG(!is_inside_tree(), VectorN(), "CameraND is not inside the scene tree.");
	const Ref<TransformND> global_xform = get_global_transform();
	const VectorN global_pos = global_xform->get_origin();
	// Perspective cameras always have their ray origin at the camera's position.
	if (_projection_type == PROJECTION_PERSPECTIVE) {
		return global_pos;
	}
	// Orthographic cameras have ray origins offset by the orthographic size.
	const Vector2 viewport_size = get_viewport()->call(StringName("get_size"));
	const double pixel_size = _keep_aspect == KEEP_WIDTH ? viewport_size.x : viewport_size.y;
	const Vector2 scaled_position = (p_viewport_position * 2.0f - viewport_size) * (_orthographic_size / pixel_size);
	const VectorN x = VectorND::multiply_scalar(global_xform->get_basis_column(0), scaled_position.x);
	const VectorN y = VectorND::multiply_scalar(global_xform->get_basis_column(1), -scaled_position.y);
	return VectorND::add(global_pos, VectorND::add(x, y));
}

VectorN CameraND::viewport_to_world_ray_direction(const Vector2 &p_viewport_position) const {
	ERR_FAIL_COND_V_MSG(!is_inside_tree(), VectorN(), "CameraND is not inside the scene tree.");
	const Ref<TransformND> global_xform = get_global_transform();
	// 0D, 1D, or 2D camera transforms can just directly map viewport positions to world positions.
	// As such, the camera does not really have a ray direction, so return an empty vector.
	if (global_xform->get_basis_column_count() < 3) {
		return VectorN();
	}
	const VectorN depth_vector = global_xform->get_basis_column(2);
	// Orthographic cameras always have their ray direction pointing straight down the negative Z-axis.
	if (_projection_type == PROJECTION_ORTHOGRAPHIC) {
		return VectorND::negate(depth_vector);
	}
	// Perspective cameras have ray directions pointing more to the side when near the sides of the viewport.
	const Vector2 viewport_size = get_viewport()->call(StringName("get_size"));
	const double pixel_size = _keep_aspect == KEEP_WIDTH ? viewport_size.x : viewport_size.y;
	const Vector2 scaled_position = (p_viewport_position * 2.0f - viewport_size) / pixel_size;
	const VectorN x = VectorND::multiply_scalar(global_xform->get_basis_column(0), scaled_position.x);
	const VectorN y = VectorND::multiply_scalar(global_xform->get_basis_column(1), -scaled_position.y);
	const VectorN z = VectorND::multiply_scalar(depth_vector, -_focal_length);
	const VectorN ray_direction = VectorND::add(VectorND::add(x, y), z);
	return VectorND::normalized(ray_direction);
}

Vector2 CameraND::world_to_viewport_local_normal(const VectorN &p_local_position, const bool p_force_orthographic) const {
	const double x = p_local_position.size() > 0 ? p_local_position[0] : 0.0;
	const double y = p_local_position.size() > 1 ? p_local_position[1] : 0.0;
	if (p_force_orthographic || _projection_type == CameraND::PROJECTION_ORTHOGRAPHIC) {
		return Vector2(x, -y) / _orthographic_size;
	}
	const double z = p_local_position.size() > 2 ? p_local_position[2] : 0.0;
	// -Z is forward, so this will "flip" X and Y. We need Y flipped anyway, but X needs to be flipped back.
	return Vector2(-x, y) * (_focal_length / z);
}

Vector2 CameraND::world_to_viewport(const VectorN &p_global_position) const {
	const Ref<TransformND> global_xform = get_global_transform();
	const VectorN local_position = global_xform->xform_transposed(p_global_position);
	const Vector2 viewport_size = get_viewport()->call(StringName("get_size"));
	const double pixel_size = _keep_aspect == KEEP_WIDTH ? viewport_size.x : viewport_size.y;
	const bool force_orthographic = global_xform->get_basis_column_count() < 3;
	const Vector2 projected = world_to_viewport_local_normal(local_position, force_orthographic);
	return (projected * pixel_size + viewport_size) * 0.5f;
}

String CameraND::get_rendering_engine() const {
	return _rendering_engine;
}

void CameraND::set_rendering_engine(const String &p_rendering_engine) {
	_rendering_engine = p_rendering_engine;
}

CameraND::KeepAspect CameraND::get_keep_aspect() const {
	return _keep_aspect;
}

void CameraND::set_keep_aspect(const KeepAspect p_keep_aspect) {
	_keep_aspect = p_keep_aspect;
}

CameraND::ProjectionType CameraND::get_projection_type() const {
	return _projection_type;
}

void CameraND::set_projection_type(const ProjectionType p_projection_type_nd) {
	_projection_type = p_projection_type_nd;
	notify_property_list_changed();
}

CameraND::ViewAngleType CameraND::get_view_angle_type() const {
	return _view_angle_type;
}

void CameraND::set_view_angle_type(const ViewAngleType p_view_angle_type) {
	_view_angle_type = p_view_angle_type;
	notify_property_list_changed();
}

double CameraND::get_focal_length() const {
	return _focal_length;
}

void CameraND::set_focal_length(const double p_focal_length_nd) {
	_focal_length = p_focal_length_nd;
}

double CameraND::get_field_of_view() const {
	return Math_PI - 2.0 * Math::atan(_focal_length);
}

void CameraND::set_field_of_view(const double p_field_of_view_nd) {
	_focal_length = Math::tan((Math_PI - p_field_of_view_nd) * 0.5);
}

double CameraND::get_orthographic_size() const {
	return _orthographic_size;
}

void CameraND::set_orthographic_size(const double p_orthographic_size) {
	_orthographic_size = p_orthographic_size;
}

double CameraND::get_clip_near() const {
	return _clip_near;
}

void CameraND::set_clip_near(const double p_clip_near) {
	_clip_near = p_clip_near;
}

double CameraND::get_clip_far() const {
	return _clip_far;
}

void CameraND::set_clip_far(const double p_clip_far) {
	_clip_far = p_clip_far;
}

CameraND::PerpFadeMode CameraND::get_perp_fade_mode() const {
	return _perp_fade_mode;
}

void CameraND::set_perp_fade_mode(const PerpFadeMode p_perp_fade_mode) {
	_perp_fade_mode = p_perp_fade_mode;
	notify_property_list_changed();
}

double CameraND::get_perp_fade_distance() const {
	return _perp_fade_distance;
}

void CameraND::set_perp_fade_distance(const double p_perp_fade_distance) {
	_perp_fade_distance = p_perp_fade_distance;
}

double CameraND::get_perp_fade_slope() const {
	return _perp_fade_slope;
}

void CameraND::set_perp_fade_slope(const double p_perp_fade_slope) {
	_perp_fade_slope = p_perp_fade_slope;
}

void CameraND::_bind_methods() {
	// Be sure to keep the relevant properties in sync with EditorCameraSettingsND.
	ClassDB::bind_method(D_METHOD("is_current"), &CameraND::is_current);
	ClassDB::bind_method(D_METHOD("set_current", "enabled"), &CameraND::set_current);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "current"), "set_current", "is_current");
	ClassDB::bind_method(D_METHOD("clear_current", "enable_next"), &CameraND::clear_current);
	ClassDB::bind_method(D_METHOD("make_current"), &CameraND::make_current);

	ClassDB::bind_method(D_METHOD("is_position_behind", "global_position"), &CameraND::is_position_behind);
	ClassDB::bind_method(D_METHOD("viewport_to_world_ray_origin", "viewport_position"), &CameraND::viewport_to_world_ray_origin);
	ClassDB::bind_method(D_METHOD("viewport_to_world_ray_direction", "viewport_position"), &CameraND::viewport_to_world_ray_direction);
	ClassDB::bind_method(D_METHOD("world_to_viewport_local_normal", "local_position", "force_orthographic"), &CameraND::world_to_viewport_local_normal, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("world_to_viewport", "global_position"), &CameraND::world_to_viewport);

	ClassDB::bind_method(D_METHOD("get_rendering_engine"), &CameraND::get_rendering_engine);
	ClassDB::bind_method(D_METHOD("set_rendering_engine", "rendering_engine"), &CameraND::set_rendering_engine);
	// The property hint string will be filled out when _validate_property is called.
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "rendering_engine", PROPERTY_HINT_ENUM, ""), "set_rendering_engine", "get_rendering_engine");

	ClassDB::bind_method(D_METHOD("get_keep_aspect"), &CameraND::get_keep_aspect);
	ClassDB::bind_method(D_METHOD("set_keep_aspect", "keep_aspect"), &CameraND::set_keep_aspect);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "keep_aspect", PROPERTY_HINT_ENUM, "Keep Width,Keep Height"), "set_keep_aspect", "get_keep_aspect");

	ClassDB::bind_method(D_METHOD("get_projection_type"), &CameraND::get_projection_type);
	ClassDB::bind_method(D_METHOD("set_projection_type", "projection_type"), &CameraND::set_projection_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "projection_type", PROPERTY_HINT_ENUM, "Orthographic,Perspective"), "set_projection_type", "get_projection_type");

	ClassDB::bind_method(D_METHOD("get_view_angle_type"), &CameraND::get_view_angle_type);
	ClassDB::bind_method(D_METHOD("set_view_angle_type", "view_angle_type"), &CameraND::set_view_angle_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "view_angle_type", PROPERTY_HINT_ENUM, "Focal Length,Field of View"), "set_view_angle_type", "get_view_angle_type");

	ClassDB::bind_method(D_METHOD("get_focal_length"), &CameraND::get_focal_length);
	ClassDB::bind_method(D_METHOD("set_focal_length", "focal_length"), &CameraND::set_focal_length);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "focal_length", PROPERTY_HINT_NONE, "suffix:m"), "set_focal_length", "get_focal_length");

	ClassDB::bind_method(D_METHOD("get_field_of_view"), &CameraND::get_field_of_view);
	ClassDB::bind_method(D_METHOD("set_field_of_view", "field_of_view"), &CameraND::set_field_of_view);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "field_of_view", PROPERTY_HINT_RANGE, "1,179,0.1,radians_as_degrees"), "set_field_of_view", "get_field_of_view");

	ClassDB::bind_method(D_METHOD("get_orthographic_size"), &CameraND::get_orthographic_size);
	ClassDB::bind_method(D_METHOD("set_orthographic_size", "orthographic_size"), &CameraND::set_orthographic_size);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "orthographic_size", PROPERTY_HINT_NONE, "suffix:m"), "set_orthographic_size", "get_orthographic_size");

	ClassDB::bind_method(D_METHOD("get_clip_near"), &CameraND::get_clip_near);
	ClassDB::bind_method(D_METHOD("set_clip_near", "clip_near"), &CameraND::set_clip_near);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "clip_near", PROPERTY_HINT_RANGE, "0.001,1000,0.001,or_less,or_greater,exp,suffix:m"), "set_clip_near", "get_clip_near");

	ClassDB::bind_method(D_METHOD("get_clip_far"), &CameraND::get_clip_far);
	ClassDB::bind_method(D_METHOD("set_clip_far", "clip_far"), &CameraND::set_clip_far);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "clip_far", PROPERTY_HINT_RANGE, "0.01,4000,0.01,or_less,or_greater,exp,suffix:m"), "set_clip_far", "get_clip_far");

	ClassDB::bind_method(D_METHOD("get_perp_fade_mode"), &CameraND::get_perp_fade_mode);
	ClassDB::bind_method(D_METHOD("set_perp_fade_mode", "perp_fade_mode"), &CameraND::set_perp_fade_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "perp_fade_mode", PROPERTY_HINT_ENUM, "Disabled,Transparency,Hue Shift,Transparency + Hue Shift"), "set_perp_fade_mode", "get_perp_fade_mode");

	ClassDB::bind_method(D_METHOD("get_perp_fade_distance"), &CameraND::get_perp_fade_distance);
	ClassDB::bind_method(D_METHOD("set_perp_fade_distance", "perp_fade_distance"), &CameraND::set_perp_fade_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "perp_fade_distance", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater,or_less,suffix:m"), "set_perp_fade_distance", "get_perp_fade_distance");

	ClassDB::bind_method(D_METHOD("get_perp_fade_slope"), &CameraND::get_perp_fade_slope);
	ClassDB::bind_method(D_METHOD("set_perp_fade_slope", "perp_fade_slope"), &CameraND::set_perp_fade_slope);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "perp_fade_slope", PROPERTY_HINT_RANGE, "0.001,10,0.001,or_greater,or_less"), "set_perp_fade_slope", "get_perp_fade_slope");

	ClassDB::bind_method(D_METHOD("get_depth_fade"), &CameraND::get_depth_fade);
	ClassDB::bind_method(D_METHOD("set_depth_fade", "depth_fade"), &CameraND::set_depth_fade);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "depth_fade"), "set_depth_fade", "get_depth_fade");

	ClassDB::bind_method(D_METHOD("get_depth_fade_start"), &CameraND::get_depth_fade_start);
	ClassDB::bind_method(D_METHOD("set_depth_fade_start", "depth_fade_start"), &CameraND::set_depth_fade_start);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "depth_fade_start", PROPERTY_HINT_RANGE, "5.0,100,0.01,or_greater,or_less"), "set_depth_fade_start", "get_depth_fade_start");

	BIND_ENUM_CONSTANT(KEEP_WIDTH);
	BIND_ENUM_CONSTANT(KEEP_HEIGHT);

	BIND_ENUM_CONSTANT(PROJECTION_ORTHOGRAPHIC);
	BIND_ENUM_CONSTANT(PROJECTION_PERSPECTIVE);

	BIND_ENUM_CONSTANT(VIEW_ANGLE_FOCAL_LENGTH);
	BIND_ENUM_CONSTANT(VIEW_ANGLE_FIELD_OF_VIEW);

	BIND_ENUM_CONSTANT(PERP_FADE_DISABLED);
	BIND_ENUM_CONSTANT(PERP_FADE_TRANSPARENCY);
	BIND_ENUM_CONSTANT(PERP_FADE_HUE_SHIFT);
	BIND_ENUM_CONSTANT(PERP_FADE_TRANSPARENCY_HUE_SHIFT);
}
