#pragma once

#include "rendering_engine_nd.h"

#if GDEXTENSION
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#elif GODOT_MODULE
#include "core/templates/hash_set.h"
#endif

class WorldEnvironmentND;

class RenderingServerND : public Object {
	GDCLASS(RenderingServerND, Object);

	HashMap<String, Ref<RenderingEngineND>> _rendering_engines;
	HashMap<Viewport *, Vector<CameraND *>> _viewport_cameras;
	HashMap<Viewport *, Vector<WorldEnvironmentND *>> _viewport_world_environments;
	// For 3D, Godot has "World3D" which meshes are added to. Cameras in the same world can see the same meshes.
	// For ND, we will use a simpler approach, just have one global array of meshes which all cameras can see.
	// We could add a "WorldND" class in the future if we want to add this feature, but it's not necessary for now.
	Vector<MeshInstanceND *> _mesh_instances;

	HashSet<String> _warned_incompatible_rendering_engine_names;
	String _get_current_godot_rendering_method() const;
	Ref<RenderingEngineND> _select_rendering_engine(const String &p_friendly_name, const String &p_godot_rendering_method) const;
	PackedInt64Array _get_visible_mesh_instance_object_ids() const;
	bool _are_render_frame_and_process_frame_connected = false;
	void _render_frame();
	void _request_godot_redraw();

protected:
	static RenderingServerND *singleton;
	static void _bind_methods();

public:
	void register_camera(CameraND *p_camera);
	void unregister_camera(CameraND *p_camera);
	void make_camera_current(CameraND *p_camera);
	void clear_camera_current(CameraND *p_camera);
	CameraND *get_current_camera(Viewport *p_viewport) const;

	void register_world_environment(WorldEnvironmentND *p_world_environment);
	void unregister_world_environment(WorldEnvironmentND *p_world_environment);
	void make_world_environment_current(WorldEnvironmentND *p_world_environment);
	void clear_world_environment_current(WorldEnvironmentND *p_world_environment);
	WorldEnvironmentND *get_current_world_environment(Viewport *p_viewport) const;
	WorldEnvironmentND *get_current_world_environment_for_camera(CameraND *p_camera) const; // Internal use only, do not expose.

	void register_mesh_instance(MeshInstanceND *p_mesh_instance);
	void unregister_mesh_instance(MeshInstanceND *p_mesh_instance);

	void register_rendering_engine(const Ref<RenderingEngineND> &p_engine);
	void unregister_rendering_engine(const String &p_friendly_name);
	PackedStringArray get_rendering_engine_names() const;
	Ref<RenderingEngineND> get_rendering_engine_from_name(const String &p_friendly_name) const;

	static RenderingServerND *get_singleton() { return singleton; }
	RenderingServerND() { singleton = this; }
	~RenderingServerND();
};
