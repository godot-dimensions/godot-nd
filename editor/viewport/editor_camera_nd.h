#pragma once

#include "editor_viewport_nd_defines.h"

class CameraND;

// Node that handles navigating around the ND editor viewport.
// It can pan, zoom, orbit, and freelook just like the 3D editor camera.
// Not actually a camera itself, but has one as a child node.
class EditorCameraND : public NodeND {
	GDCLASS(EditorCameraND, NodeND);

	CameraND *_camera = nullptr;
	Ref<TransformND> _target_transform;
	double _pitch_angle = 0.0; // Will be set in the constructor.

	double _positive_clip_near = 0.05;
	double _target_speed_and_zoom = 4.0;
	int _zoom_failed_attempts_count = 0;
	bool _is_auto_orthographic = true;
	bool _is_explicit_orthographic = false;

	void _process_freelook_movement(const double p_delta);
	void _update_camera_auto_orthographicness();
	void _update_transform_with_pitch();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	const CameraND *get_camera_readonly() const;
	double change_speed_and_zoom(const double p_change);
	void pan_camera(const VectorN &p_pan_amount);
	void freelook_rotate_ground_basis(const Ref<TransformND> &p_ground_basis);
	void orbit_rotate_ground_basis(const Ref<TransformND> &p_ground_basis);
	void rotate_pitch_no_apply(const double p_pitch_angle);
	void set_camera_dimension(const int p_dimension);
	void set_ground_view_axes(const int p_right, const int p_up, const int p_back, const double p_yaw_angle = 0.5f, const double p_pitch_angle = -0.5f);
	void set_orthonormalized_axis_aligned(const double p_yaw_angle = 0.5f, const double p_pitch_angle = -0.5f);
	void set_target_position(const VectorN &p_position);
	void set_orthogonal_view_plane(const int p_right, const int p_up);

	void setup();
};
