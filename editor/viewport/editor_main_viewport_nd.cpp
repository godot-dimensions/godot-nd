#include "editor_main_viewport_nd.h"

#include "editor_camera_nd.h"
#include "editor_input_surface_nd.h"
#include "editor_transform_gizmo_nd.h"
#include "editor_viewport_rotation_nd.h"

#include "../../math/vector_nd.h"
#include "../../nodes/camera_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#elif GODOT_MODULE
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#endif

Vector2 EditorMainViewportND::_get_warped_mouse_motion(const Ref<InputEventMouseMotion> &p_ev_mouse_motion) const {
#if GODOT_MODULE
	if (bool(EDITOR_GET("editors/3d/navigation/warped_mouse_panning"))) {
		return Input::get_singleton()->warp_mouse_motion(p_ev_mouse_motion, _input_surface_nd->get_global_rect());
	}
#endif // GODOT_MODULE
	return p_ev_mouse_motion->get_relative();
}

Ref<TransformND> _ground_basis_rotation(const int p_dimension, const Vector2 &p_rotation_radians) {
	Ref<TransformND> ground_rot_dx = TransformND::from_rotation(p_dimension, 0, p_rotation_radians.x);
	Ref<TransformND> ground_rot_zd = TransformND::from_rotation(2, p_dimension, p_rotation_radians.y);
	return ground_rot_dx->compose_square(ground_rot_zd);
}

Ref<TransformND> EditorMainViewportND::_ground_rotation_input(const Ref<InputEventMouseMotion> &p_input_event, const Vector2 &p_rotation_radians) const {
	Input *input = Input::get_singleton();
	const bool cmd_or_ctrl = p_input_event->is_ctrl_pressed() || p_input_event->is_command_or_control_pressed();
	if (_dimension > 12 && input->is_physical_key_pressed(KEY_PERIOD)) {
		return _ground_basis_rotation(12, p_rotation_radians);
	} else if (_dimension > 11 && input->is_physical_key_pressed(KEY_COMMA)) {
		return _ground_basis_rotation(11, p_rotation_radians);
	} else if (_dimension > 10 && input->is_physical_key_pressed(KEY_M)) {
		return _ground_basis_rotation(10, p_rotation_radians);
	} else if (_dimension > 9 && input->is_physical_key_pressed(KEY_N)) {
		return _ground_basis_rotation(9, p_rotation_radians);
	} else if (_dimension > 8 && input->is_physical_key_pressed(KEY_B)) {
		return _ground_basis_rotation(8, p_rotation_radians);
	} else if (_dimension > 7 && input->is_physical_key_pressed(KEY_V)) {
		return _ground_basis_rotation(7, p_rotation_radians);
	} else if (_dimension > 6 && input->is_physical_key_pressed(KEY_C)) {
		return _ground_basis_rotation(6, p_rotation_radians);
	} else if (_dimension > 5 && input->is_physical_key_pressed(KEY_X)) {
		return _ground_basis_rotation(5, p_rotation_radians);
	} else if (_dimension > 4 && input->is_physical_key_pressed(KEY_Z)) {
		return _ground_basis_rotation(4, p_rotation_radians);
	} else if (_dimension > 3 && cmd_or_ctrl) {
		return _ground_basis_rotation(3, p_rotation_radians);
	} else if (_dimension > 2) {
		_editor_camera_nd->rotate_pitch_no_apply(-p_rotation_radians.y);
		return TransformND::from_rotation(0, 2, p_rotation_radians.x);
	}
	return Ref<TransformND>();
}

void EditorMainViewportND::_update_theme() {
	// Set the information label offsets.
	_information_label->set_offset(SIDE_LEFT, 6.0f * EDSCALE);
	_information_label->set_offset(SIDE_RIGHT, -6.0f * EDSCALE);
	_information_label->set_offset(SIDE_TOP, -40.0f * EDSCALE);
	_information_label->set_offset(SIDE_BOTTOM, -6.0f * EDSCALE);
}

void EditorMainViewportND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			_information_label_auto_hide_time -= get_process_delta_time();
			if (_information_label_auto_hide_time <= 0.0) {
				_information_label->hide();
				set_process(false);
			}
		} break;
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;
	}
}

void EditorMainViewportND::focus_selected_nodes() {
	TypedArray<Node> selected_nodes = EditorInterface::get_singleton()->get_selection()->get_selected_nodes();
	VectorN position_sum;
	int position_count = 0;
	for (int i = 0; i < selected_nodes.size(); i++) {
		NodeND *node_nd = Object::cast_to<NodeND>(selected_nodes[i]);
		if (node_nd != nullptr) {
			position_sum = VectorND::add(position_sum, node_nd->get_global_position());
			position_count++;
		}
	}
	if (position_count > 0) {
		_editor_camera_nd->set_target_position(VectorND::divide_scalar(position_sum, position_count));
	}
}

Ref<TransformND> EditorMainViewportND::get_view_camera_transform() const {
	return _editor_camera_nd->get_transform();
}

void EditorMainViewportND::navigation_freelook(const Ref<InputEventMouseMotion> &p_input_event) {
	const Vector2 relative = _get_warped_mouse_motion(p_input_event);
	const double degrees_per_pixel = EDITOR_GET("editors/3d/freelook/freelook_sensitivity");
	const double radians_per_pixel = Math::deg_to_rad(degrees_per_pixel);
	const bool invert_y_axis = EDITOR_GET("editors/3d/navigation/invert_y_axis");
	Vector2 rotation_radians = relative * radians_per_pixel;
	if (invert_y_axis) {
		rotation_radians.y = -rotation_radians.y;
	}
	Ref<TransformND> ground_rot = _ground_rotation_input(p_input_event, rotation_radians);
	if (ground_rot.is_valid()) {
		_editor_camera_nd->freelook_rotate_ground_basis(ground_rot);
	}
	_viewport_rotation_nd->queue_redraw();
}

void EditorMainViewportND::navigation_orbit(const Ref<InputEventMouseMotion> &p_input_event) {
	Vector2 relative = _get_warped_mouse_motion(p_input_event);
	const double degrees_per_pixel = EDITOR_GET("editors/3d/navigation_feel/orbit_sensitivity");
	const double radians_per_pixel = Math::deg_to_rad(degrees_per_pixel);
	const bool invert_y_axis = EDITOR_GET("editors/3d/navigation/invert_y_axis");
	const bool invert_x_axis = EDITOR_GET("editors/3d/navigation/invert_x_axis");
	Vector2 rotation_radians = relative * radians_per_pixel;
	if (invert_x_axis) {
		rotation_radians.x = -rotation_radians.x;
	}
	if (invert_y_axis) {
		rotation_radians.y = -rotation_radians.y;
	}
	Ref<TransformND> ground_rot = _ground_rotation_input(p_input_event, rotation_radians);
	if (ground_rot.is_valid()) {
		_editor_camera_nd->orbit_rotate_ground_basis(ground_rot);
	}
	_viewport_rotation_nd->queue_redraw();
}

void EditorMainViewportND::navigation_pan(const Ref<InputEventMouseMotion> &p_input_event) {
	Vector2 relative = _get_warped_mouse_motion(p_input_event) / get_size().y;
	VectorN pan;
	if (p_input_event->is_ctrl_pressed() || p_input_event->is_command_or_control_pressed()) {
		pan = VectorN{ 0.0, 0.0, relative.y, relative.x };
	} else {
		pan = VectorN{ -relative.x, relative.y };
	}
	_editor_camera_nd->pan_camera(pan);
}

String _viewport_nd_format_number(const double p_number) {
	const int decimals = MAX(0, 3 - log10(p_number));
	String number_text = String::num(p_number, decimals);
	if (number_text.length() < 3 && !number_text.contains(".")) {
		number_text += ".0";
	}
	return number_text;
}

void EditorMainViewportND::navigation_change_speed(const double p_speed_change) {
	const double speed_and_zoom = _editor_camera_nd->change_speed_and_zoom(p_speed_change);
	set_information_text("Speed: " + _viewport_nd_format_number(speed_and_zoom) + "m/s");
}

void EditorMainViewportND::navigation_change_zoom(const double p_zoom_change) {
	const double speed_and_zoom = _editor_camera_nd->change_speed_and_zoom(p_zoom_change);
	set_information_text("Zoom: " + _viewport_nd_format_number(speed_and_zoom) + "m");
}

void EditorMainViewportND::viewport_mouse_input(const Ref<InputEventMouse> &p_mouse_event) {
	const CameraND *camera = _editor_camera_nd->get_camera_readonly();
	const bool used_by_gizmo = _transform_gizmo_nd->gizmo_mouse_input(p_mouse_event, camera);
	if (used_by_gizmo) {
		return;
	}
	// TODO: Try to make a selection if the transform gizmo didn't use the mouse.
}

void EditorMainViewportND::set_dimension(const int p_dimension) {
	_dimension = p_dimension;
	_editor_camera_nd->set_camera_dimension(p_dimension);
	_viewport_rotation_nd->set_dimension(p_dimension);
}

void EditorMainViewportND::set_ground_view_axes(const int p_right, const int p_up, const int p_back) {
	_editor_camera_nd->set_ground_view_axes(p_right, p_up, p_back);
	_viewport_rotation_nd->queue_redraw();
}

void EditorMainViewportND::set_orthonormalized_axis_aligned() {
	_editor_camera_nd->set_orthonormalized_axis_aligned();
	_viewport_rotation_nd->queue_redraw();
}

void EditorMainViewportND::set_information_text(const String &p_text, const double p_auto_hide_time) {
	_information_label->set_text(p_text);
	_information_label->show();
	_information_label_auto_hide_time = p_auto_hide_time;
	set_process(true);
}

void EditorMainViewportND::set_orthogonal_view_plane(const int p_right, const int p_up) {
	_editor_camera_nd->set_orthogonal_view_plane(p_right, p_up);
	_viewport_rotation_nd->queue_redraw();
}

void EditorMainViewportND::setup(EditorMainScreenND *p_editor_main_screen, EditorTransformGizmoND *p_transform_gizmo_nd) {
	// Things that we should do in the constructor but can't in GDExtension
	// due to how GDExtension runs the constructor for each registered class.
	set_name(StringName("EditorMainViewportND"));
	set_clip_children_mode(Control::CLIP_CHILDREN_AND_DRAW);
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	// Set up the viewport container and camera.
	_sub_viewport_container = memnew(SubViewportContainer);
	_sub_viewport_container->set_clip_children_mode(Control::CLIP_CHILDREN_AND_DRAW);
	_sub_viewport_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	_sub_viewport_container->set_stretch(true);
	add_child(_sub_viewport_container);

	_sub_viewport = memnew(SubViewport);
	_sub_viewport_container->add_child(_sub_viewport);

	_information_label = memnew(Label);
	_information_label->set_anchors_and_offsets_preset(Control::PRESET_BOTTOM_WIDE);
	_information_label->set_vertical_alignment(VerticalAlignment::VERTICAL_ALIGNMENT_BOTTOM);
	_sub_viewport_container->add_child(_information_label);

	_editor_camera_nd = memnew(EditorCameraND);
	_editor_camera_nd->setup();
	_sub_viewport->add_child(_editor_camera_nd);

	// Set up the input surfaces.
	_input_surface_nd = memnew(EditorInputSurfaceND);
	_sub_viewport_container->add_child(_input_surface_nd);

	// Set up things with the arguments (not constructor things).
	_editor_main_screen = p_editor_main_screen;
	_input_surface_nd->setup(_editor_main_screen, this);
	_transform_gizmo_nd = p_transform_gizmo_nd;

	_viewport_rotation_nd = memnew(EditorViewportRotationND);
	_viewport_rotation_nd->setup(_editor_main_screen, this);
	_input_surface_nd->add_child(_viewport_rotation_nd);
}
