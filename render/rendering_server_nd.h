#pragma once

#include "rendering_engine_nd.h"

#if GDEXTENSION
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#endif

class RenderingServerND : public Object {
	GDCLASS(RenderingServerND, Object);

	HashMap<String, Ref<RenderingEngineND>> _rendering_engines;
	HashMap<Viewport *, Vector<CameraND *>> _viewport_cameras;
	// For 3D, Godot has "World3D" which meshes are added to. Cameras in the same world can see the same meshes.
	// For ND, we will use a simpler approach, just have one global array of meshes which all cameras can see.
	// We could add a "WorldND" class in the future if we want to add this feature, but it's not necessary for now.
	Vector<MeshInstanceND *> _mesh_instances;

	Ref<RenderingEngineND> _get_rendering_engine(const String &p_name) const;
	TypedArray<MeshInstanceND> _get_visible_mesh_instances() const;
	bool _is_render_frame_connected = false;
	void _render_frame();

protected:
	static RenderingServerND *singleton;
	static void _bind_methods();

public:
	bool is_currently_preferring_wireframe_meshes(Viewport *p_viewport) const;

	void register_camera(CameraND *p_camera);
	void unregister_camera(CameraND *p_camera);
	void make_camera_current(CameraND *p_camera);
	void clear_camera_current(CameraND *p_camera);
	CameraND *get_current_camera(Viewport *p_viewport) const;

	void register_mesh_instance(MeshInstanceND *p_mesh_instance);
	void unregister_mesh_instance(MeshInstanceND *p_mesh_instance);

	void register_rendering_engine(const String &p_name, const Ref<RenderingEngineND> &p_engine);
	void unregister_rendering_engine(const String &p_name);
	PackedStringArray get_rendering_engine_names() const;

	static RenderingServerND *get_singleton() { return singleton; }
	RenderingServerND() { singleton = this; }
	~RenderingServerND() { singleton = nullptr; }
};
