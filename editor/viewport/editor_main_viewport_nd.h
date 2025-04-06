#pragma once

#include "editor_viewport_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#elif GODOT_MODULE
#include "scene/gui/label.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/viewport.h"
#endif

// Class for the ND editor viewport, which there may be up to 4 of.
// Uses EditorCameraND, EditorInputSurfaceND, EditorViewportRotationND,
// and lots of other classes to handle the ND editor viewport.
class EditorMainViewportND : public Control {
	GDCLASS(EditorMainViewportND, Control);

public:
	// Keep the first items in sync with EditorTransformGizmoND.
	enum ToolbarButton {
		TOOLBAR_BUTTON_SELECT, // 0
		TOOLBAR_BUTTON_MOVE, // 1
		TOOLBAR_BUTTON_ROTATE, // 2
		TOOLBAR_BUTTON_SCALE, // 3
		TOOLBAR_BUTTON_MODE_MAX, // 4
		TOOLBAR_BUTTON_USE_LOCAL_TRANSFORM = TOOLBAR_BUTTON_MODE_MAX, // Still 4
		TOOLBAR_BUTTON_MAX
	};

private:
	SubViewportContainer *_sub_viewport_container = nullptr;
	SubViewport *_sub_viewport = nullptr;
	Label *_information_label = nullptr;
	EditorCameraND *_editor_camera_nd = nullptr;
	EditorInputSurfaceND *_input_surface_nd = nullptr;
	EditorMainScreenND *_editor_main_screen = nullptr;
	EditorTransformGizmoND *_transform_gizmo_nd = nullptr;
	EditorViewportRotationND *_viewport_rotation_nd = nullptr;

	PackedColorArray _axis_colors;
	int _dimension = 0;
	double _information_label_auto_hide_time = 0.0;

	Vector2 _get_warped_mouse_motion(const Ref<InputEventMouseMotion> &p_ev_mouse_motion) const;
	Ref<TransformND> _ground_rotation_input(const Ref<InputEventMouseMotion> &p_input_event, const Vector2 &p_rotation_radians) const;
	void _on_button_toggled(const bool p_toggled_on, const int p_option);
	void _update_theme();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	void focus_selected_nodes();
	Ref<TransformND> get_view_camera_transform() const;
	void navigation_freelook(const Ref<InputEventMouseMotion> &p_input_event);
	void navigation_orbit(const Ref<InputEventMouseMotion> &p_input_event);
	void navigation_pan(const Ref<InputEventMouseMotion> &p_input_event);
	void navigation_change_speed(const double p_speed_change);
	void navigation_change_zoom(const double p_zoom_change);
	void viewport_mouse_input(const Ref<InputEventMouse> &p_mouse_event);

	void set_dimension(const int p_dimension);
	void set_ground_view_axes(const int p_right, const int p_up, const int p_back);
	void set_orthonormalized_axis_aligned();
	void set_information_text(const String &p_text, const double p_auto_hide_time = 1.5);
	void set_orthogonal_view_plane(const int p_right, const int p_up);

	void setup(EditorMainScreenND *p_editor_main_screen, EditorTransformGizmoND *p_transform_gizmo_nd);
};
