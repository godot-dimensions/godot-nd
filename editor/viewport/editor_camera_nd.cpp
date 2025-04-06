#include "editor_camera_nd.h"

#include "../../math/vector_nd.h"
#include "../../nodes/camera_nd.h"

void EditorCameraND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			const double delta = (double)get_process_delta_time();
			// Interpolate camera Z position for the speed and zoom level.
			const double zoom_inertia = EDITOR_GET("editors/3d/navigation_feel/zoom_inertia");
			const VectorN camera_local_position = _camera->get_position();
			const double current_z = camera_local_position.size() > 2 ? camera_local_position[2] : _target_speed_and_zoom;
			const double target_z = _target_speed_and_zoom;
			const double new_z = Math::lerp(current_z, target_z, MIN((double)1.0, delta / zoom_inertia));
			// For speed, use the same logic as zoom, except we want to preserve the camera's global position during freelook.
			if (Input::get_singleton()->is_mouse_button_pressed(MOUSE_BUTTON_RIGHT)) {
				const VectorN camera_global_position = _camera->get_global_position();
				_camera->set_position(VectorN{ 0.0, 0.0, new_z });
				const VectorN offset = VectorND::subtract(camera_global_position, _camera->get_global_position());
				_target_transform->set_origin(VectorND::add(_target_transform->get_origin(), offset));
				_process_freelook_movement(delta);
			} else {
				// For the orbit camera, interpolate towards the target position.
				_camera->set_position(VectorN{ 0.0, 0.0, new_z });
				const double translation_inertia = EDITOR_GET("editors/3d/navigation_feel/translation_inertia");
				set_position(VectorND::lerp(get_position(), _target_transform->get_origin(), delta / translation_inertia));
			}
			_camera->set_orthographic_size(new_z);
		} break;
	}
}

void EditorCameraND::_process_freelook_movement(const double p_delta) {
	const Input *input = Input::get_singleton();
	VectorN local_movement = VectorN{
		double(input->is_physical_key_pressed(KEY_D) - input->is_physical_key_pressed(KEY_A)),
		double(input->is_physical_key_pressed(KEY_R) - input->is_physical_key_pressed(KEY_F)),
		double(input->is_physical_key_pressed(KEY_S) - input->is_physical_key_pressed(KEY_W)),
		double(input->is_physical_key_pressed(KEY_E) - input->is_physical_key_pressed(KEY_Q)),
		double(input->is_physical_key_pressed(KEY_T) - input->is_physical_key_pressed(KEY_Y)),
		double(input->is_physical_key_pressed(KEY_G) - input->is_physical_key_pressed(KEY_H)),
		double(input->is_physical_key_pressed(KEY_U) - input->is_physical_key_pressed(KEY_I)),
		double(input->is_physical_key_pressed(KEY_J) - input->is_physical_key_pressed(KEY_K)),
		double(input->is_physical_key_pressed(KEY_O) - input->is_physical_key_pressed(KEY_P)),
		double(input->is_physical_key_pressed(KEY_L) - input->is_physical_key_pressed(KEY_SEMICOLON)),
		double(input->is_physical_key_pressed(KEY_BRACKETLEFT) - input->is_physical_key_pressed(KEY_BRACKETRIGHT)),
	};
	local_movement = VectorND::with_dimension(local_movement, get_dimension());
	if (input->is_key_pressed(KEY_SHIFT)) {
		local_movement = VectorND::multiply_scalar(local_movement, 2.0);
	}
	pan_camera(VectorND::multiply_scalar(local_movement, p_delta));
}

void EditorCameraND::_update_camera_auto_orthographicness() {
	if (_is_explicit_orthographic || !_is_auto_orthographic) {
		return;
	}
	const Vector<VectorN> basis_columns = get_all_basis_columns();
	int screen_projected_axis_count = 0;
	for (int i = 0; i < basis_columns.size(); i++) {
		ERR_FAIL_COND(basis_columns[0].size() < i || basis_columns[1].size() < i);
		if (!Math::is_zero_approx(basis_columns[0][i]) || !Math::is_zero_approx(basis_columns[1][i])) {
			screen_projected_axis_count++;
		}
	}
	const bool should_be_orthographic = screen_projected_axis_count <= 2;
	if (should_be_orthographic) {
		_camera->set_projection_type(CameraND::PROJECTION_ORTHOGRAPHIC);
		_camera->set_near(-_camera->get_far());
	} else {
		_camera->set_projection_type(CameraND::PROJECTION_PERSPECTIVE);
		_camera->set_near(0.05);
	}
}

void EditorCameraND::_update_transform_with_pitch() {
	set_transform(_target_transform->compose_square(TransformND::from_rotation(1, 2, _pitch_angle)));
	_update_camera_auto_orthographicness();
}

const CameraND *EditorCameraND::get_camera_readonly() const {
	return _camera;
}

double EditorCameraND::change_speed_and_zoom(const double p_change) {
	const double min_distance = _camera->get_near() * 4.0;
	const double max_distance = _camera->get_far() * 0.25;
	if (unlikely(min_distance > max_distance)) {
		_target_speed_and_zoom = (min_distance + max_distance) * 0.5;
	} else {
		_target_speed_and_zoom = CLAMP(_target_speed_and_zoom * p_change, min_distance, max_distance);
	}
	return _target_speed_and_zoom;
}

void EditorCameraND::pan_camera(const VectorN &p_pan_amount) {
	const VectorN camera_position = _camera->get_position();
	const double zoom_dist = camera_position.size() > 2 ? camera_position[2] : 0.0;
	const VectorN local_pan_offset = VectorND::multiply_scalar(p_pan_amount, 2.0 * zoom_dist);
	_target_transform->translate_global(get_transform()->xform_basis(local_pan_offset));
	_update_transform_with_pitch();
}

void EditorCameraND::freelook_rotate_ground_basis(const Ref<TransformND> &p_ground_basis) {
	const VectorN camera_position = _camera->get_global_position();
	_target_transform = _target_transform->compose_square(p_ground_basis);
	_update_transform_with_pitch();
	// Move the camera origin to keep the CameraND in the same position.
	const VectorN target_pos = VectorND::add(_target_transform->get_origin(), VectorND::subtract(camera_position, _camera->get_global_position()));
	_target_transform->set_origin(target_pos);
	_update_transform_with_pitch();
}

void EditorCameraND::orbit_rotate_ground_basis(const Ref<TransformND> &p_ground_basis) {
	_target_transform = _target_transform->compose_square(p_ground_basis);
	_update_transform_with_pitch();
}

// Meant to be called immediately before `freelook_rotate_ground_basis` or `orbit_rotate_ground_basis`.
void EditorCameraND::rotate_pitch_no_apply(const double p_pitch_angle) {
	_pitch_angle = CLAMP(_pitch_angle + p_pitch_angle, -1.57, 1.57);
}

void EditorCameraND::set_camera_dimension(const int p_dimension) {
	const VectorN position = VectorND::with_dimension(_target_transform->get_origin(), p_dimension);
	if (p_dimension < 3) {
		_pitch_angle = 0.0;
		_target_transform = TransformND::identity_basis(p_dimension);
	} else {
		_target_transform = _target_transform->with_dimension(p_dimension)->orthonormalized();
		// If the orthonormalization fails or results in a flip, reset to the default.
		if (_target_transform->determinant() < 0.999999) {
			set_ground_view_axes(0, 1, 2);
		}
	}
	_target_transform->set_origin(position);
	_update_transform_with_pitch();
}

void EditorCameraND::set_ground_view_axes(const int p_right, const int p_up, const int p_back, const double p_yaw_angle, const double p_pitch_angle) {
	ERR_FAIL_COND(p_right == p_up || p_right == p_back || p_up == p_back);
	ERR_FAIL_COND(p_right != 0 && p_right < 3);
	ERR_FAIL_COND(p_up != 1 && p_up < 3);
	ERR_FAIL_COND(p_back != 2 && p_back < 3);
	_pitch_angle = p_pitch_angle;
	_target_transform = TransformND::from_rotation(2, 0, p_yaw_angle);
	if (p_right != 0) {
		_target_transform = TransformND::from_swap_rotation(0, p_right)->compose_square(_target_transform);
	}
	if (p_up != 1) {
		_target_transform = TransformND::from_swap_rotation(1, p_up)->compose_square(_target_transform);
	}
	if (p_back != 2) {
		_target_transform = TransformND::from_swap_rotation(2, p_back)->compose_square(_target_transform);
	}
	_update_transform_with_pitch();
}

void EditorCameraND::set_orthonormalized_axis_aligned(const double p_yaw_angle, const double p_pitch_angle) {
	_pitch_angle = p_pitch_angle;
	Ref<TransformND> ortho = get_transform()->orthonormalized_axis_aligned();
	_target_transform = ortho->compose_square(TransformND::from_rotation(2, 0, p_yaw_angle));
	_update_transform_with_pitch();
}

void EditorCameraND::set_orthogonal_view_plane(const int p_right, const int p_up) {
	ERR_FAIL_COND(p_right == p_up);
	ERR_FAIL_COND(p_right != 0 && p_right < 2);
	ERR_FAIL_COND(p_up != 1 && p_up < 2);
	if (p_right != 0) {
		if (p_up != 1) {
			_target_transform = TransformND::from_swap_rotation(1, p_up)->compose_square(TransformND::from_swap_rotation(0, p_right));
		} else {
			_target_transform = TransformND::from_swap_rotation(0, p_right);
		}
	} else {
		if (p_up != 1) {
			_target_transform = TransformND::from_swap_rotation(1, p_up);
		} else {
			_target_transform = TransformND::identity_basis(2);
		}
	}
	if (_is_auto_orthographic) {
		_camera->set_projection_type(CameraND::PROJECTION_ORTHOGRAPHIC);
		_camera->set_near(-_camera->get_far());
	}
	_pitch_angle = 0.0;
	set_transform(_target_transform);
}

void EditorCameraND::set_target_position(const VectorN &p_position) {
	_target_transform->set_origin(p_position);
}

void EditorCameraND::setup() {
	set_name(StringName("EditorCameraND"));
	_camera = memnew(CameraND);
	_camera->set_name(StringName("CameraND"));
	_camera->set_position(VectorN{ 0.0, 0.0, 4.0 });
	add_child(_camera);

	set_ground_view_axes(0, 1, 2);
	set_process(true);
}
