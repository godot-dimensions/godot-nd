#pragma once

#include "editor_viewport_nd_defines.h"

// Editor viewport rotation navigation widget (the thing in the top right corner).
// Shows the current view rotation and allows the user to rotate the view.
// Users can drag to spin like a ball, or click on an axis to make that perpendicular to the view.
class EditorViewportRotationND : public Control {
	GDCLASS(EditorViewportRotationND, Control);

	static constexpr int MOUSE_SENTINEL_INDEX = 100; // Distinguish from touch events.

	enum HitType2D {
		HIT_TYPE_NONE,
		HIT_TYPE_BACKGROUND,
		HIT_TYPE_AXIS_CIRCLE_POSITIVE,
		HIT_TYPE_AXIS_CIRCLE_NEGATIVE,
		HIT_TYPE_AXIS_LINE,
		HIT_TYPE_PLANE,
	};

	struct HitTarget2D {
		Vector2 screen_point = Vector2();
		float z_index = 0.0f;
		float angle = 0.0f; // Only used by the PLANE hit type.
		HitType2D hit_type = HIT_TYPE_NONE;
		int primary_axis_number = -1; // 0 to 3 for XYZW, more for other axes, or -1 for none and asterisk "*".
		int secondary_axis_number = -1; // 0 to 3 for XYZW, more for other axes. Only used by the PLANE hit type.
	};

	struct HitTarget2DCompare {
		_FORCE_INLINE_ bool operator()(const HitTarget2D &p_left, const HitTarget2D &p_right) const {
			return p_left.z_index < p_right.z_index;
		}
	};

	EditorMainScreenND *_editor_main_screen = nullptr;
	EditorMainViewportND *_editor_main_viewport = nullptr;
	PackedColorArray _axis_colors;
	HitTarget2D _focused_target = HitTarget2D();
	Vector2i _orbiting_mouse_start = Vector2i();
	int _dimension = -1;
	int _orbiting_mouse_button_index = -1;
	double _editor_scale = 1.0f;

	void _draw_axis_circle(const HitTarget2D &p_axis);
	void _draw_axis_line(const HitTarget2D &p_axis, const Vector2 &p_center);
	void _draw_plane_semicircles(const HitTarget2D &p_axis);
	void _draw_filled_arc(const Vector2 &p_center, const double p_radius, const double p_start_angle, const double p_end_angle, const Color &p_color);
	void _get_sorted_axis(const Vector2 &p_center, Vector<HitTarget2D> &r_targets);
	void _get_sorted_axis_screen_aligned(const Ref<TransformND> &p_basis, const Vector2 &p_center, const double p_radius, const int p_right_index, const int p_up_index, Vector<HitTarget2D> &r_targets);
	HitTarget2D _make_plane_axis(const Ref<TransformND> &p_basis, const int p_a, const int p_b, const Vector2 &p_center, const double p_radius);
	void _on_mouse_exited();
	void _process_click(int p_index, Vector2 p_position, bool p_pressed);
	void _process_drag(Ref<InputEvent> p_event, int p_index, Vector2 p_position);
	void _update_focus();
	void _update_theme();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
#if GDEXTENSION
	void _draw() override;
#elif GODOT_MODULE
	void _draw();
#endif

	virtual void GDEXTMOD_GUI_INPUT(const Ref<InputEvent> &p_event) override;

	void set_dimension(int p_dimension);
	void setup(EditorMainScreenND *p_editor_main_screen, EditorMainViewportND *p_editor_main_viewport);
};
