#include "editor_viewport_rotation_nd.h"

#include "editor_main_screen_nd.h"
#include "editor_main_viewport_nd.h"

#include "../../math/vector_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#elif GODOT_MODULE
#include "scene/resources/font.h"
#endif

// How far apart the axis circles are from the center of the gizmo.
// Godot's 3D uses 80, but for ND we have more axes, so we need to space them out more.
constexpr float GIZMO_ND_BASE_SIZE = 100.0f;

void EditorViewportRotationND::_notification(int p_what) {
	if (_editor_main_screen == nullptr) {
		return;
	}
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (!is_connected(StringName("mouse_exited"), callable_mp(this, &EditorViewportRotationND::_on_mouse_exited))) {
				connect(StringName("mouse_exited"), callable_mp(this, &EditorViewportRotationND::_on_mouse_exited));
			}
			_update_theme();
		} break;
		case NOTIFICATION_DRAW: {
			_draw();
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;
	}
}

void EditorViewportRotationND::_draw() {
	const Vector2 center = get_size() / 2.0f;
	if (_focused_axis.axis_number > -3 || _orbiting_mouse_button_index != -1) {
		draw_circle(center, center.x, Color(0.5f, 0.5f, 0.5f, 0.25f), true, -1.0f, true);
	}
	Vector<Axis2D> axis_to_draw;
	_get_sorted_axis(center, axis_to_draw);
	for (int i = 0; i < axis_to_draw.size(); ++i) {
		Axis2D axis = axis_to_draw[i];
		if (axis.axis_type == AXIS_TYPE_LINE) {
			_draw_axis_line(axis, center);
		} else if (axis.axis_type == AXIS_TYPE_PLANE) {
			_draw_axis_plane(axis);
		} else {
			_draw_axis_circle(axis);
		}
	}
}

void EditorViewportRotationND::_draw_axis_circle(const Axis2D &p_axis) {
	const bool is_focused = _focused_axis.axis_number == p_axis.axis_number && _focused_axis.axis_type == p_axis.axis_type;
	const Color axis_color = p_axis.axis_number < 0 ? Color(0.6f, 0.6f, 0.6f) : _axis_colors[p_axis.axis_number % _axis_colors.size()];
	const float alpha = MIN(2.0f, p_axis.z_index + 2.0f);
	const Color color = is_focused ? Color(axis_color.lightened(0.75f), 1.0f) : Color(axis_color, alpha);
	const double axis_circle_radius = (8.0f + p_axis.z_index) * _editor_scale;
	if (p_axis.axis_type == AXIS_TYPE_CIRCLE_POSITIVE) {
		draw_circle(p_axis.screen_point, axis_circle_radius, color, true, -1.0f, true);
		// Draw the axis letter for the positive axes.
		const String axis_letter = VectorND::axis_letter(p_axis.axis_number);
		const Ref<Font> &font = get_theme_font(StringName("rotation_control"), StringName("EditorFonts"));
		const int font_size = get_theme_font_size(StringName("rotation_control_size"), StringName("EditorFonts"));
		const Size2 char_size = font->get_char_size(axis_letter[0], font_size);
		Vector2 char_offset = Vector2(-char_size.width / 2.0f, char_size.height * 0.25f);
		if (p_axis.axis_number < 0) {
			char_offset.y += 4.0f;
		}
		draw_char(font, p_axis.screen_point + char_offset, axis_letter, font_size, Color(0.0f, 0.0f, 0.0f, alpha * 0.6f));
	} else {
		// Draw an outline around the negative axes.
		draw_circle(p_axis.screen_point, axis_circle_radius, color, true, -1.0f, true);
		draw_circle(p_axis.screen_point, axis_circle_radius * 0.8f, color.darkened(0.4f), true, -1.0f, true);
	}
}

void EditorViewportRotationND::_draw_axis_line(const Axis2D &p_axis, const Vector2 &p_center) {
	const bool is_focused = _focused_axis.axis_number == p_axis.axis_number && _focused_axis.axis_type == AXIS_TYPE_CIRCLE_POSITIVE;
	const Color axis_color = _axis_colors[p_axis.axis_number % _axis_colors.size()];
	const float alpha = MIN(2.0f, p_axis.z_index + 2.0f);
	const Color color = is_focused ? Color(axis_color.lightened(0.75f), 1.0f) : Color(axis_color, alpha);
	draw_line(p_center, p_axis.screen_point, color, 1.5f * _editor_scale, true);
}

void EditorViewportRotationND::_draw_axis_plane(const Axis2D &p_axis) {
	const bool is_focused = _focused_axis.axis_number == p_axis.axis_number && _focused_axis.axis_type == p_axis.axis_type && _focused_axis.secondary_axis_number == p_axis.secondary_axis_number;
	Color primary_color = _axis_colors[p_axis.axis_number % _axis_colors.size()];
	Color secondary_color = _axis_colors[p_axis.secondary_axis_number % _axis_colors.size()];
	if (is_focused) {
		primary_color = primary_color.lightened(0.75f);
		secondary_color = secondary_color.lightened(0.75f);
	} else {
		const float alpha = MIN(2.0f, p_axis.z_index + 2.0f);
		primary_color.a = alpha;
		secondary_color.a = alpha;
	}
	constexpr float QUARTER_TURN = Math_TAU / 4.0f;
	const float outer_radius = (4.0f + p_axis.z_index) * _editor_scale;
	_draw_filled_arc(p_axis.screen_point, outer_radius, p_axis.angle + QUARTER_TURN, p_axis.angle + QUARTER_TURN * 3.0f, primary_color);
	_draw_filled_arc(p_axis.screen_point, outer_radius, p_axis.angle - QUARTER_TURN, p_axis.angle + QUARTER_TURN, secondary_color);
	if (p_axis.z_index < 4.0f) {
		const float inner_radius = (3.0f + p_axis.z_index) * _editor_scale;
		_draw_filled_arc(p_axis.screen_point, inner_radius, p_axis.angle + QUARTER_TURN, p_axis.angle + QUARTER_TURN * 3.0f, primary_color.darkened(0.4f));
		_draw_filled_arc(p_axis.screen_point, inner_radius, p_axis.angle - QUARTER_TURN, p_axis.angle + QUARTER_TURN, secondary_color.darkened(0.4f));
	} else {
		// If the circle is big enough, draw letters.
		const String primary_letter = VectorND::axis_letter(p_axis.axis_number);
		const String secondary_letter = VectorND::axis_letter(p_axis.secondary_axis_number);
		const Ref<Font> &font = get_theme_font(StringName("rotation_control"), StringName("EditorFonts"));
		const int font_size = get_theme_font_size(StringName("rotation_control_size"), StringName("EditorFonts"));
		const Vector2 primary_char_size = font->get_char_size(primary_letter[0], font_size);
		const Vector2 secondary_char_size = font->get_char_size(secondary_letter[0], font_size);
		const Vector2 primary_char_offset = Vector2(Math::ceil(-5.5f * _editor_scale - 0.5f * primary_char_size.width), primary_char_size.height * 0.25f);
		const Vector2 secondary_char_offset = Vector2(Math::floor(5.5f * _editor_scale - 0.5f * secondary_char_size.width), secondary_char_size.height * 0.25f);
		draw_char(font, p_axis.screen_point + primary_char_offset, primary_letter, font_size, Color(0.0f, 0.0f, 0.0f, primary_color.a * 0.6f));
		draw_char(font, p_axis.screen_point + secondary_char_offset, secondary_letter, font_size, Color(0.0f, 0.0f, 0.0f, secondary_color.a * 0.6f));
	}
}

void EditorViewportRotationND::_draw_filled_arc(const Vector2 &p_center, double p_radius, double p_start_angle, double p_end_angle, const Color &p_color) {
	ERR_THREAD_GUARD;
	constexpr int ARC_POINTS = 16;
	const double angle_step = (p_end_angle - p_start_angle) / (ARC_POINTS - 1);
	PackedVector2Array points;
	points.resize(ARC_POINTS);
	for (int i = 0; i < ARC_POINTS; i++) {
		const double angle = p_start_angle + i * angle_step;
		points.set(i, p_center + Vector2(Math::cos(angle), Math::sin(angle)) * p_radius);
	}
	PackedColorArray colors = { p_color };
	draw_polygon(points, colors);
}

EditorViewportRotationND::Axis2D EditorViewportRotationND::_make_plane_axis(const Ref<TransformND> &p_basis, const int p_right, const int p_up, const Vector2 &p_center, const double p_radius) {
	Axis2D ret;
	ret.axis_type = AXIS_TYPE_PLANE;
	ret.axis_number = p_right;
	ret.secondary_axis_number = p_up;
	const Vector3 right_vec3 = VectorND::to_3d(p_basis->get_basis_column(p_right));
	const Vector3 up_vec3 = VectorND::to_3d(p_basis->get_basis_column(p_up));
	const Vector3 axis_3d = (right_vec3 + up_vec3).normalized();
	ret.screen_point = p_center + Vector2(axis_3d.x, -axis_3d.y) * p_radius;
	ret.angle = Vector2(right_vec3.x, -right_vec3.y).angle_to_point(Vector2(up_vec3.x, -up_vec3.y));
	ret.z_index = axis_3d[2];
	return ret;
}

void EditorViewportRotationND::_get_sorted_axis(const Vector2 &p_center, Vector<Axis2D> &r_axis) {
	const Vector2 center = get_size() / 2.0f;
	const double radius = get_size().x / 2.0f - 10.0f * _editor_scale;
	const Ref<TransformND> camera_transform = _editor_main_viewport->get_view_camera_transform();
	const Ref<TransformND> camera_transform_transposed = camera_transform->inverse_basis_transposed();
	// Add axes in each direction.
	int screen_aligned_axis_index = -1;
	for (int i = 0; i < _dimension; i++) {
		const Vector3 axis_3d = VectorND::to_3d(camera_transform_transposed->get_basis_column(i));
		const Vector2 axis_screen_position = Vector2(axis_3d.x, -axis_3d.y) * radius;

		if (axis_screen_position.is_zero_approx()) {
			// Special case when the axis is aligned with the camera.
			if (screen_aligned_axis_index == -1) {
				Axis2D axis;
				axis.axis_type = AXIS_TYPE_CIRCLE_POSITIVE;
				axis.axis_number = i;
				axis.screen_point = center;
				screen_aligned_axis_index = r_axis.size();
				r_axis.push_back(axis);
			} else {
				Axis2D axis = r_axis[screen_aligned_axis_index];
				axis.axis_number = -1;
				r_axis.set(screen_aligned_axis_index, axis);
			}
		} else {
			Axis2D pos_axis;
			pos_axis.axis_type = AXIS_TYPE_CIRCLE_POSITIVE;
			pos_axis.axis_number = i;
			pos_axis.screen_point = center + axis_screen_position;
			pos_axis.z_index = axis_3d.z;
			r_axis.push_back(pos_axis);

			Axis2D line_axis;
			line_axis.axis_type = AXIS_TYPE_LINE;
			line_axis.axis_number = i;
			line_axis.screen_point = center + axis_screen_position;
			// Ensure the lines draw behind their connected circles.
			line_axis.z_index = MIN(axis_3d.z, 0.0f) - (float)CMP_EPSILON;
			r_axis.push_back(line_axis);

			Axis2D neg_axis;
			neg_axis.axis_type = AXIS_TYPE_CIRCLE_NEGATIVE;
			neg_axis.axis_number = i;
			neg_axis.screen_point = center - axis_screen_position;
			neg_axis.z_index = -axis_3d.z;
			r_axis.push_back(neg_axis);
		}
	}
	// Add orthogonal planes. Disabled to avoid cluttering the UI.
	//r_axis.append(_make_plane_axis(camera_transform_transposed, 0, 1, center, radius));
	//r_axis.append(_make_plane_axis(camera_transform_transposed, 2, 1, center, radius));
	//r_axis.append(_make_plane_axis(camera_transform_transposed, 0, 2, center, radius));
	//r_axis.append(_make_plane_axis(camera_transform_transposed, 0, 3, center, radius));
	//r_axis.append(_make_plane_axis(camera_transform_transposed, 3, 1, center, radius));
	//r_axis.append(_make_plane_axis(camera_transform_transposed, 2, 3, center, radius));
	// Sort the axes by z_index.
	r_axis.sort_custom<Axis2DCompare>();
}

void EditorViewportRotationND::_on_mouse_exited() {
	_focused_axis.axis_number = -3;
	queue_redraw();
}

void EditorViewportRotationND::_process_click(int p_index, Vector2 p_position, bool p_pressed) {
	if (_orbiting_mouse_button_index != -1 && _orbiting_mouse_button_index != p_index) {
		return;
	}
	if (p_pressed) {
		if (p_position.distance_to(get_size() / 2.0f) < get_size().x / 2.0f) {
			_orbiting_mouse_button_index = p_index;
		}
	} else {
		if (_focused_axis.axis_number > -2) {
			if (_focused_axis.secondary_axis_number > -1) {
				_editor_main_viewport->set_orthogonal_view_plane(int(_focused_axis.axis_number), int(_focused_axis.secondary_axis_number));
			} else {
				_editor_main_viewport->set_orthonormalized_axis_aligned();
			}
			_update_focus();
		}
		_orbiting_mouse_button_index = -1;
		if (Input::get_singleton()->get_mouse_mode() == Input::MOUSE_MODE_CAPTURED) {
			Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
			Input::get_singleton()->warp_mouse(_orbiting_mouse_start);
		}
	}
	queue_redraw();
}

void EditorViewportRotationND::_process_drag(Ref<InputEventWithModifiers> p_event, int p_index, Vector2 p_position) {
	if (_orbiting_mouse_button_index == p_index) {
		if (Input::get_singleton()->get_mouse_mode() == Input::MOUSE_MODE_VISIBLE) {
			Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
			_orbiting_mouse_start = p_position;
		}
		_editor_main_viewport->navigation_orbit(p_event);
		_focused_axis.axis_number = -2;
	} else {
		_update_focus();
	}
	queue_redraw();
}

void EditorViewportRotationND::_update_focus() {
	const Vector2 center = get_size() / 2.0f;
	const Vector2 mouse_pos = get_local_mouse_position();
	const int original_focus = _focused_axis.axis_number;
	_focused_axis = Axis2D();
	_focused_axis.z_index = -10.0f;
	if (mouse_pos.distance_to(center) < center.x) {
		_focused_axis.axis_number = -2;
	}
	Vector<Axis2D> axes;
	_get_sorted_axis(center, axes);
	for (int i = 0; i < axes.size(); i++) {
		const Axis2D &axis = axes[i];
		if (axis.z_index > _focused_axis.z_index && mouse_pos.distance_to(axis.screen_point) < 8.0f * _editor_scale) {
			_focused_axis = axis;
		}
	}

	if (_focused_axis.axis_number != original_focus) {
		queue_redraw();
	}
}

void EditorViewportRotationND::_update_theme() {
	_editor_scale = EDSCALE;
	const double scaled_size = GIZMO_ND_BASE_SIZE * _editor_scale;
	set_custom_minimum_size(Vector2(scaled_size, scaled_size));
	set_offset(SIDE_RIGHT, -0.1f * scaled_size);
	set_offset(SIDE_BOTTOM, 1.1f * scaled_size);
	set_offset(SIDE_LEFT, -1.1f * scaled_size);
	set_offset(SIDE_TOP, 0.1f * scaled_size);
	_axis_colors = _editor_main_screen->get_axis_colors();
	queue_redraw();
}

void EditorViewportRotationND::GDEXTMOD_GUI_INPUT(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	// Mouse events
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
		_process_click(100, mb->get_position(), mb->is_pressed());
	}

	const Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		_process_drag(mm, 100, mm->get_global_position());
	}

	// Touch events
	const Ref<InputEventScreenTouch> screen_touch = p_event;
	if (screen_touch.is_valid()) {
		_process_click(screen_touch->get_index(), screen_touch->get_position(), screen_touch->is_pressed());
	}

	const Ref<InputEventScreenDrag> screen_drag = p_event;
	if (screen_drag.is_valid()) {
		_process_drag(screen_drag, screen_drag->get_index(), screen_drag->get_position());
	}
}

void EditorViewportRotationND::set_dimension(const int p_dimension) {
	_dimension = p_dimension;
	queue_redraw();
}

void EditorViewportRotationND::setup(EditorMainScreenND *p_editor_main_screen, EditorMainViewportND *p_editor_main_viewport) {
	// Things that we should do in the constructor but can't in GDExtension
	// due to how GDExtension runs the constructor for each registered class.
	set_name(StringName("EditorViewportRotationND"));
	set_anchors_and_offsets_preset(Control::PRESET_TOP_RIGHT);

	// Set up things with the arguments (not constructor things).
	_editor_main_screen = p_editor_main_screen;
	_editor_main_viewport = p_editor_main_viewport;
}
