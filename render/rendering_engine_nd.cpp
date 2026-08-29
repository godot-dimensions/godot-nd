#include "rendering_engine_nd.h"

#include <algorithm>
#include <tuple>
#include <vector>

void RenderingEngineND::calculate_relative_transforms() {
	ERR_FAIL_NULL(_camera);
	const int mesh_count = _mesh_instance_object_ids.size();
	_mesh_relative_transforms.resize(mesh_count);
	const Ref<TransformND> camera_inverse_transform = _camera->get_global_transform()->inverse();
	for (int64_t i = 0; i < _mesh_instance_object_ids.size(); i++) {
		const ObjectID mesh_instance_object_id = (ObjectID)_mesh_instance_object_ids[i];
		const MeshInstanceND *mesh_instance = Object::cast_to<const MeshInstanceND>(ObjectDB::get_instance(mesh_instance_object_id));
		ERR_CONTINUE(mesh_instance == nullptr);
		const Ref<TransformND> relative_transform = camera_inverse_transform->compose_square(mesh_instance->get_global_transform());
		_mesh_relative_transforms[i] = relative_transform;
	}
	_sort_meshes_by_relative_z();
}

void RenderingEngineND::_sort_meshes_by_relative_z() {
	// Can't use Godot's types to do this operation easily, so we'll use the standard library instead.
	std::vector<std::tuple<Variant, Variant>> combined;
	const int64_t mesh_count = _mesh_instance_object_ids.size();
	combined.reserve(mesh_count);
	for (int64_t i = 0; i < mesh_count; ++i) {
		combined.emplace_back(_mesh_instance_object_ids[i], _mesh_relative_transforms[i]);
	}
	// Sort the vector of tuples based on the Z position.
	std::sort(combined.begin(), combined.end(), [](const auto &a, const auto &b) {
		const Ref<TransformND> a_transform = std::get<1>(a);
		const Ref<TransformND> b_transform = std::get<1>(b);
		if (a_transform.is_null()) {
			return b_transform.is_valid();
		}
		if (b_transform.is_null()) {
			return false;
		}
		const VectorN a_origin = a_transform->get_origin();
		const VectorN b_origin = b_transform->get_origin();
		const double a_z = a_origin.size() > 2 ? a_origin[2] : 0.0;
		const double b_z = b_origin.size() > 2 ? b_origin[2] : 0.0;
		return a_z < b_z;
	});
	// Unpack the sorted tuples back into the original arrays
	for (size_t i = 0; i < combined.size(); ++i) {
		_mesh_instance_object_ids.set(i, std::get<0>(combined[i]));
		_mesh_relative_transforms[i] = std::get<1>(combined[i]);
	}
}

void RenderingEngineND::set_viewport(Viewport *p_viewport) {
	_viewport = p_viewport;
}

void RenderingEngineND::set_camera(CameraND *p_camera) {
	_camera = p_camera;
}

void RenderingEngineND::set_mesh_instance_object_ids(PackedInt64Array p_mesh_instance_object_ids) {
	_mesh_instance_object_ids = p_mesh_instance_object_ids;
}

String RenderingEngineND::get_friendly_name() const {
	String friendly_name;
	GDVIRTUAL_CALL(_get_friendly_name, friendly_name);
	return friendly_name;
}

bool RenderingEngineND::requires_transparent_background() const {
	bool requires_transparent_background = false;
	GDVIRTUAL_CALL(_requires_transparent_background, requires_transparent_background);
	return requires_transparent_background;
}

bool RenderingEngineND::supports_godot_rendering_method(const String &p_godot_rendering_method) const {
	bool supports_godot_rendering_method = true;
	GDVIRTUAL_CALL(_supports_godot_rendering_method, p_godot_rendering_method, supports_godot_rendering_method);
	return supports_godot_rendering_method;
}

void RenderingEngineND::setup_for_viewport_if_needed(Viewport *p_for_viewport) {
	_viewport = p_for_viewport;
	// Every time, regardless of being already setup, make sure the viewport has a transparent background if the rendering engine requires it.
	// Note that the cleanup explicitly excludes restoring any previous setting, because something else may have changed it after this did.
	if (requires_transparent_background() && !p_for_viewport->has_transparent_background()) {
		p_for_viewport->set_transparent_background(true);
	}
	if (_setup_viewports.has(p_for_viewport)) {
		return;
	}
	p_for_viewport->set_meta("last_rendering_engine_name_nd", get_friendly_name());
	_setup_viewports.append(p_for_viewport);
	setup_for_viewport();
}

void RenderingEngineND::setup_for_viewport() {
	GDVIRTUAL_CALL(_setup_for_viewport);
}

void RenderingEngineND::cleanup_for_viewport_if_needed(Viewport *p_for_viewport) {
	_viewport = p_for_viewport;
	if (!_setup_viewports.has(p_for_viewport)) {
		return;
	}
	_setup_viewports.erase(p_for_viewport);
	p_for_viewport->remove_meta("last_rendering_engine_name_nd");
	cleanup_for_viewport();
}

void RenderingEngineND::cleanup_for_viewport() {
	GDVIRTUAL_CALL(_cleanup_for_viewport);
}

void RenderingEngineND::render_frame() {
	GDVIRTUAL_CALL(_render_frame);
}

void RenderingEngineND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_friendly_name"), &RenderingEngineND::get_friendly_name);
	ClassDB::bind_method(D_METHOD("requires_transparent_background"), &RenderingEngineND::requires_transparent_background);
	ClassDB::bind_method(D_METHOD("supports_godot_rendering_method", "godot_rendering_method"), &RenderingEngineND::supports_godot_rendering_method);

	ClassDB::bind_method(D_METHOD("get_viewport"), &RenderingEngineND::get_viewport);
	ClassDB::bind_method(D_METHOD("get_camera"), &RenderingEngineND::get_camera);

	ClassDB::bind_method(D_METHOD("get_mesh_instance_object_ids"), &RenderingEngineND::get_mesh_instance_object_ids);
	ClassDB::bind_method(D_METHOD("get_mesh_relative_transforms"), &RenderingEngineND::get_mesh_relative_transforms);

	GDVIRTUAL_BIND(_get_friendly_name);
	GDVIRTUAL_BIND(_requires_transparent_background);
	GDVIRTUAL_BIND(_supports_godot_rendering_method, "godot_rendering_method");
	GDVIRTUAL_BIND(_setup_for_viewport);
	GDVIRTUAL_BIND(_cleanup_for_viewport);
	GDVIRTUAL_BIND(_render_frame);
}
