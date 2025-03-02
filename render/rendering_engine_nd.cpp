#include "rendering_engine_nd.h"

#include <algorithm>
#include <tuple>
#include <vector>

void RenderingEngineND::calculate_relative_transforms() {
	const int mesh_count = _mesh_instances.size();
	_mesh_relative_transforms.resize(mesh_count);
	const Ref<TransformND> camera_inverse_transform = _camera->get_global_transform()->inverse();
	for (int i = 0; i < _mesh_instances.size(); i++) {
		const MeshInstanceND *mesh_instance = (const MeshInstanceND *)(const Object *)_mesh_instances[i];
		const Ref<TransformND> relative_transform = camera_inverse_transform->compose_square(mesh_instance->get_global_transform());
		_mesh_relative_transforms[i] = relative_transform;
	}
	_sort_meshes_by_relative_z();
}

void RenderingEngineND::_sort_meshes_by_relative_z() {
	// Can't use Godot's types to do this operation easily, so we'll use the standard library instead.
	std::vector<std::tuple<Variant, Variant>> combined;
	combined.reserve(_mesh_instances.size());
	for (int i = 0; i < _mesh_instances.size(); ++i) {
		combined.emplace_back(_mesh_instances[i], _mesh_relative_transforms[i]);
	}
	// Sort the vector of tuples based on the Z position.
	std::sort(combined.begin(), combined.end(), [](const auto &a, const auto &b) {
		const Ref<TransformND> a_transform = std::get<1>(a);
		const Ref<TransformND> b_transform = std::get<1>(b);
		const VectorN a_origin = a_transform->get_origin();
		const VectorN b_origin = b_transform->get_origin();
		const double a_z = a_origin.size() > 2 ? a_origin[2] : 0.0;
		const double b_z = b_origin.size() > 2 ? b_origin[2] : 0.0;
		return a_z < b_z;
	});
	// Unpack the sorted tuples back into the original arrays
	for (size_t i = 0; i < combined.size(); ++i) {
		_mesh_instances[i] = std::get<0>(combined[i]);
		_mesh_relative_transforms[i] = std::get<1>(combined[i]);
	}
}

Viewport *RenderingEngineND::get_viewport() const {
	return _viewport;
}

void RenderingEngineND::set_viewport(Viewport *p_viewport) {
	_viewport = p_viewport;
}

CameraND *RenderingEngineND::get_camera() const {
	return _camera;
}

void RenderingEngineND::set_camera(CameraND *p_camera) {
	_camera = p_camera;
}

TypedArray<MeshInstanceND> RenderingEngineND::get_mesh_instances() const {
	return _mesh_instances;
}

void RenderingEngineND::set_mesh_instances(TypedArray<MeshInstanceND> p_mesh_instances) {
	_mesh_instances = p_mesh_instances;
}

TypedArray<TransformND> RenderingEngineND::get_mesh_relative_transforms() const {
	return _mesh_relative_transforms;
}

void RenderingEngineND::set_mesh_relative_transforms(TypedArray<TransformND> p_mesh_relative_transforms) {
	_mesh_relative_transforms = p_mesh_relative_transforms;
}

bool RenderingEngineND::prefers_wireframe_meshes() {
	bool prefers_wireframe = false;
	GDVIRTUAL_CALL(_prefers_wireframe_meshes, prefers_wireframe);
	return prefers_wireframe;
}

void RenderingEngineND::setup_for_viewport_if_needed(Viewport *p_for_viewport) {
	_viewport = p_for_viewport;
	if (_setup_viewports.has(p_for_viewport)) {
		return;
	}
	_setup_viewports.append(p_for_viewport);
	setup_for_viewport();
}

void RenderingEngineND::setup_for_viewport() {
	GDVIRTUAL_CALL(_setup_for_viewport);
}

void RenderingEngineND::render_frame() {
	GDVIRTUAL_CALL(_render_frame);
}

void RenderingEngineND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_viewport"), &RenderingEngineND::get_viewport);
	ClassDB::bind_method(D_METHOD("set_viewport", "viewport"), &RenderingEngineND::set_viewport);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "viewport", PROPERTY_HINT_RESOURCE_TYPE, "Viewport"), "set_viewport", "get_viewport");

	ClassDB::bind_method(D_METHOD("get_camera"), &RenderingEngineND::get_camera);
	ClassDB::bind_method(D_METHOD("set_camera", "camera"), &RenderingEngineND::set_camera);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_RESOURCE_TYPE, "CameraND"), "set_camera", "get_camera");

	ClassDB::bind_method(D_METHOD("get_mesh_instances"), &RenderingEngineND::get_mesh_instances);
	ClassDB::bind_method(D_METHOD("set_mesh_instances", "mesh_instances"), &RenderingEngineND::set_mesh_instances);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "mesh_instances"), "set_mesh_instances", "get_mesh_instances");

	ClassDB::bind_method(D_METHOD("get_mesh_relative_transforms"), &RenderingEngineND::get_mesh_relative_transforms);
	ClassDB::bind_method(D_METHOD("set_mesh_relative_transforms", "mesh_relative_transforms"), &RenderingEngineND::set_mesh_relative_transforms);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "mesh_relative_transforms"), "set_mesh_relative_transforms", "get_mesh_relative_transforms");

	GDVIRTUAL_BIND(_prefers_wireframe_meshes);
	GDVIRTUAL_BIND(_setup_for_viewport);
	GDVIRTUAL_BIND(_render_frame);
}
