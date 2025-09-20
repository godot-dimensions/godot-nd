#pragma once

#include "../model/mesh/mesh_instance_nd.h"
#include "../nodes/camera_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#elif GODOT_MODULE
#include "core/object/ref_counted.h"
#include "core/variant/typed_array.h"
#include "scene/main/viewport.h"
#endif

class RenderingEngineND : public RefCounted {
	GDCLASS(RenderingEngineND, RefCounted);

#if GDEXTENSION
	TypedArray<Viewport> _setup_viewports;
#elif GODOT_MODULE
	Vector<Viewport *> _setup_viewports;
#endif

	Viewport *_viewport = nullptr;
	CameraND *_camera = nullptr;
	TypedArray<MeshInstanceND> _mesh_instances;
	TypedArray<TransformND> _mesh_relative_transforms;

	void _sort_meshes_by_relative_z();

protected:
	static void _bind_methods();

public:
	void calculate_relative_transforms();

	Viewport *get_viewport() const;
	void set_viewport(Viewport *p_viewport);

	CameraND *get_camera() const;
	void set_camera(CameraND *p_camera);

	TypedArray<MeshInstanceND> get_mesh_instances() const;
	void set_mesh_instances(TypedArray<MeshInstanceND> p_mesh_instances);

	TypedArray<TransformND> get_mesh_relative_transforms() const;
	void set_mesh_relative_transforms(TypedArray<TransformND> p_mesh_relative_transforms);

	void setup_for_viewport_if_needed(Viewport *p_for_viewport);
	void cleanup_for_viewport_if_needed(Viewport *p_for_viewport);

	virtual String get_friendly_name() const;
	virtual void setup_for_viewport();
	virtual void cleanup_for_viewport();
	virtual void render_frame();

	GDVIRTUAL0RC(String, _get_friendly_name);
	GDVIRTUAL0(_setup_for_viewport);
	GDVIRTUAL0(_cleanup_for_viewport);
	GDVIRTUAL0(_render_frame);
};
