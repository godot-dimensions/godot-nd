#include "rendering_server_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#elif GODOT_MODULE
#include "scene/main/scene_tree.h"
#include "servers/rendering_server.h"
#endif

Ref<RenderingEngineND> RenderingServerND::_get_rendering_engine(const String &p_friendly_name) const {
	if (_rendering_engines.has(p_friendly_name)) {
		return _rendering_engines[p_friendly_name];
	}
	// Fallback to the first registered rendering engine. If the name is empty,
	// treat it as "auto" and do not print a warning. Else, print a warning.
	if (!p_friendly_name.is_empty()) {
		WARN_PRINT("Rendering engine '" + p_friendly_name + "' not registered. Using the first registered engine.");
	}
	return _rendering_engines.begin()->value;
}

TypedArray<MeshInstanceND> RenderingServerND::_get_visible_mesh_instances() const {
	TypedArray<MeshInstanceND> visible_mesh_instances;
	for (int i = 0; i < _mesh_instances.size(); i++) {
		MeshInstanceND *mesh_instance = (MeshInstanceND *)(Object *)_mesh_instances[i];
		if (mesh_instance->is_visible_in_tree()) {
			Ref<MeshND> mesh = mesh_instance->get_mesh();
			if (mesh.is_valid() && mesh->is_mesh_data_valid()) {
				visible_mesh_instances.append(mesh_instance);
			}
		}
	}
	return visible_mesh_instances;
}

void RenderingServerND::_render_frame() {
	for (const KeyValue<Viewport *, Vector<CameraND *>> &E : _viewport_cameras) {
		Viewport *viewport = E.key;
		const Vector<CameraND *> &cameras = E.value;
		if (cameras.is_empty()) {
			continue; // Viewport has no ND cameras.
		}
		CameraND *camera0 = cameras[0];
		if (!camera0->is_current()) {
			continue; // No ND cameras are currently rendering.
		}
		ERR_FAIL_COND_MSG(_rendering_engines.is_empty(), "No ND rendering engines registered. ND rendering will not occur.");
		Ref<RenderingEngineND> rendering_engine = _get_rendering_engine(camera0->get_rendering_engine());
		if (viewport->has_meta("last_rendering_engine_nd")) {
			Variant last_rendering_engine_variant = viewport->get_meta("last_rendering_engine_nd");
			if (last_rendering_engine_variant.get_type() == Variant::STRING) {
				const String last_rendering_engine_name = last_rendering_engine_variant;
				const String next_rendering_engine_name = rendering_engine->get_friendly_name();
				if (next_rendering_engine_name != last_rendering_engine_name) {
					Ref<RenderingEngineND> last_rendering_engine_nd = _get_rendering_engine(last_rendering_engine_name);
					last_rendering_engine_nd->cleanup_for_viewport_if_needed(viewport);
				}
			}
		}
		// Now that we have a rendering engine selected, set up its properties.
		rendering_engine->setup_for_viewport_if_needed(viewport);
		rendering_engine->set_camera(camera0);
		TypedArray<MeshInstanceND> visible_mesh_instances = _get_visible_mesh_instances();
		rendering_engine->set_mesh_instances(visible_mesh_instances);
		emit_signal("pre_render", camera0, viewport, rendering_engine);
		rendering_engine->calculate_relative_transforms();
		rendering_engine->render_frame();
	}
}

void RenderingServerND::register_camera(CameraND *p_camera) {
	ERR_FAIL_NULL(p_camera);
	Viewport *viewport = p_camera->get_viewport();
	ERR_FAIL_NULL(viewport);
	if (_viewport_cameras.has(viewport)) {
		Vector<CameraND *> &cameras = _viewport_cameras[viewport];
		ERR_FAIL_COND_MSG(cameras.has(p_camera), "CameraND is already registered to this Viewport.");
		cameras.append(p_camera);
		if (p_camera->is_current()) {
			RenderingServerND::make_camera_current(p_camera);
		}
		return;
	}
	// This is the first time a CameraND has been registered to this Viewport.
	Vector<CameraND *> cameras;
	cameras.append(p_camera);
	_viewport_cameras[viewport] = cameras;
	p_camera->make_current();
	// Is this also the first time any CameraND has been registered? If so, connect to the RenderingServer's frame signal.
	if (_is_render_frame_connected) {
		return;
	}
	RenderingServer *godot_rendering_server = RenderingServer::get_singleton();
	godot_rendering_server->connect(StringName("frame_pre_draw"), callable_mp(this, &RenderingServerND::_render_frame));
	_is_render_frame_connected = true;
}

void RenderingServerND::unregister_camera(CameraND *p_camera) {
	ERR_FAIL_NULL(p_camera);
	Viewport *viewport = p_camera->get_viewport();
	Vector<CameraND *> &cameras = _viewport_cameras[viewport];
	ERR_FAIL_COND(!cameras.has(p_camera));
	if (p_camera == cameras[0]) {
		p_camera->clear_current();
	}
	cameras.erase(p_camera);
}

void RenderingServerND::make_camera_current(CameraND *p_camera) {
	ERR_FAIL_NULL(p_camera);
	if (unlikely(!p_camera->is_current())) {
		// Will set the camera's variable and call RenderingServerND::make_camera_current again.
		p_camera->make_current();
		return;
	}
	Viewport *viewport = p_camera->get_viewport();
	Vector<CameraND *> &cameras = _viewport_cameras[viewport];
	ERR_FAIL_COND(!cameras.has(p_camera));
	CameraND *camera0 = cameras[0];
	if (p_camera != camera0) {
		if (camera0->is_current()) {
			camera0->clear_current(false);
		}
		cameras.erase(p_camera);
		cameras.insert(0, p_camera);
	}
}

void RenderingServerND::clear_camera_current(CameraND *p_camera) {
	ERR_FAIL_NULL(p_camera);
	if (unlikely(p_camera->is_current())) {
		p_camera->clear_current(false);
	}
	Viewport *viewport = p_camera->get_viewport();
	Vector<CameraND *> &cameras = _viewport_cameras[viewport];
	ERR_FAIL_COND(!cameras.has(p_camera));
	if (p_camera == cameras[0] && cameras.size() > 1) {
		cameras.remove_at(0);
		cameras.append(p_camera);
		CameraND *camera0 = cameras[0];
		camera0->make_current();
	}
}

CameraND *RenderingServerND::get_current_camera(Viewport *p_viewport) const {
	if (!_viewport_cameras.has(p_viewport)) {
		return nullptr;
	}
	const Vector<CameraND *> &cameras = _viewport_cameras[p_viewport];
	if (cameras.is_empty()) {
		return nullptr;
	}
	CameraND *camera0 = cameras[0];
	if (camera0->is_current()) {
		return camera0;
	}
	return nullptr;
}

void RenderingServerND::register_mesh_instance(MeshInstanceND *p_mesh_instance) {
	_mesh_instances.append(p_mesh_instance);
}

void RenderingServerND::unregister_mesh_instance(MeshInstanceND *p_mesh_instance) {
	_mesh_instances.erase(p_mesh_instance);
}

void RenderingServerND::register_rendering_engine(const Ref<RenderingEngineND> &p_engine) {
	const String friendly_name = p_engine->get_friendly_name();
	if (_rendering_engines.has(friendly_name)) {
		WARN_PRINT("Rendering engine '" + friendly_name + "' already registered. The existing engine will be replaced.");
	}
	_rendering_engines[friendly_name] = p_engine;
}

void RenderingServerND::unregister_rendering_engine(const String &p_friendly_name) {
	_rendering_engines.erase(p_friendly_name);
}

PackedStringArray RenderingServerND::get_rendering_engine_names() const {
	// HashMap doesn't have a keys() method, so we have to do this manually.
	PackedStringArray engine_names;
	if (_rendering_engines.is_empty()) {
		return engine_names;
	}
	engine_names.resize(_rendering_engines.size());
	int i = 0;
	for (const KeyValue<String, Ref<RenderingEngineND>> &E : _rendering_engines) {
		engine_names.set(i, String(E.key));
		i++;
	}
	return engine_names;
}

RenderingServerND *RenderingServerND::singleton = nullptr;

void RenderingServerND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_current_camera", "viewport"), &RenderingServerND::get_current_camera);
	ClassDB::bind_method(D_METHOD("register_rendering_engine", "engine"), &RenderingServerND::register_rendering_engine);
	ClassDB::bind_method(D_METHOD("unregister_rendering_engine", "name"), &RenderingServerND::unregister_rendering_engine);
	ClassDB::bind_method(D_METHOD("get_rendering_engine_names"), &RenderingServerND::get_rendering_engine_names);

	ADD_SIGNAL(MethodInfo("pre_render", PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_RESOURCE_TYPE, "CameraND"), PropertyInfo(Variant::OBJECT, "viewport", PROPERTY_HINT_RESOURCE_TYPE, "Viewport"), PropertyInfo(Variant::OBJECT, "rendering_engine", PROPERTY_HINT_RESOURCE_TYPE, "RenderingEngineND")));
}
