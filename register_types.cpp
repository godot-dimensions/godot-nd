#include "register_types.h"

#if GDEXTENSION
#include <godot_cpp/classes/engine.hpp>
#elif GODOT_MODULE
#include "core/config/engine.h"
#ifdef TOOLS_ENABLED
#include "editor/plugins/editor_plugin.h"
#include "editor/themes/editor_color_map.h"
#endif // TOOLS_ENABLED
#endif

// General.
#include "math/geometry_nd.h"
#include "math/plane_nd.h"
#include "math/rect_nd.h"
#include "math/transform_nd.h"
#include "math/vector_nd.h"
#include "nodes/camera_nd.h"
#include "nodes/marker_nd.h"
#include "nodes/node_nd.h"

// Virtual classes.
#include "model/cell/cell_mesh_nd.h"
#include "model/material_nd.h"
#include "model/mesh_nd.h"
#include "model/wire/wire_mesh_nd.h"

// Model.
#include "model/cell/array_cell_mesh_nd.h"
#include "model/cell/box_cell_mesh_nd.h"
#include "model/cell/cell_material_nd.h"
#include "model/cell/orthoplex_cell_mesh_nd.h"
#include "model/mesh_instance_nd.h"
#include "model/off/off_document_nd.h"
#include "model/wire/array_wire_mesh_nd.h"
#include "model/wire/box_wire_mesh_nd.h"
#include "model/wire/orthoplex_wire_mesh_nd.h"
#include "model/wire/wire_material_nd.h"

// Render.
#include "render/rendering_engine_nd.h"
#include "render/rendering_server_nd.h"
#include "render/wireframe_canvas/wireframe_canvas_rendering_engine_nd.h"
#include "render/wireframe_canvas/wireframe_render_canvas_nd.h"

#if GDEXTENSION
// GDExtension has a nervous breakdown whenever singleton or casted classes are not registered.
// We don't need to register these in principle, and we don't need it for a module, just for GDExtension.
#include "render/wireframe_canvas/wireframe_canvas_rendering_engine_nd.h"
#include "render/wireframe_canvas/wireframe_render_canvas_nd.h"
#ifdef TOOLS_ENABLED
#include "editor/godot_nd_editor_plugin.h"
#include "editor/viewport/editor_camera_nd.h"
#include "editor/viewport/editor_camera_settings_nd.h"
#include "editor/viewport/editor_input_surface_nd.h"
#include "editor/viewport/editor_main_screen_nd.h"
#include "editor/viewport/editor_main_viewport_nd.h"
#include "editor/viewport/editor_transform_gizmo_nd.h"
#include "editor/viewport/editor_viewport_rotation_nd.h"
#endif // TOOLS_ENABLED
#endif // GDEXTENSION

#ifdef TOOLS_ENABLED
#include "editor/godot_nd_editor_plugin.h"
#endif // TOOLS_ENABLED

inline void add_godot_singleton(const StringName &p_singleton_name, Object *p_object) {
#if GDEXTENSION
	Engine::get_singleton()->register_singleton(p_singleton_name, p_object);
#elif GODOT_MODULE
	Engine::get_singleton()->add_singleton(Engine::Singleton(p_singleton_name, p_object));
#endif
}

inline void remove_godot_singleton(const StringName &p_singleton_name) {
#if GDEXTENSION
	Engine::get_singleton()->unregister_singleton(p_singleton_name);
#elif GODOT_MODULE
	Engine::get_singleton()->remove_singleton(p_singleton_name);
#endif
}

void initialize_nd_module(ModuleInitializationLevel p_level) {
	// Note: Classes MUST be registered in inheritance order.
	// When the inheritance doesn't matter, alphabetical order is used.
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
		GDREGISTER_CLASS(GeometryND);
		GDREGISTER_CLASS(PlaneND);
		GDREGISTER_CLASS(RectND);
		GDREGISTER_CLASS(TransformND);
		GDREGISTER_CLASS(VectorND);
		// Render.
		GDREGISTER_VIRTUAL_CLASS(RenderingEngineND);
		GDREGISTER_CLASS(RenderingServerND);
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// General.
		GDREGISTER_CLASS(NodeND);
		GDREGISTER_CLASS(CameraND);
		add_godot_singleton("GeometryND", memnew(GeometryND));
		add_godot_singleton("VectorND", memnew(VectorND));
		// Virtual classes.
		GDREGISTER_CLASS(MaterialND);
		GDREGISTER_CLASS(MeshND);
		GDREGISTER_CLASS(CellMeshND);
		GDREGISTER_CLASS(WireMeshND);
		// Model.
		GDREGISTER_CLASS(ArrayCellMeshND);
		GDREGISTER_CLASS(ArrayWireMeshND);
		GDREGISTER_CLASS(BoxCellMeshND);
		GDREGISTER_CLASS(BoxWireMeshND);
		GDREGISTER_CLASS(MeshInstanceND);
		GDREGISTER_CLASS(OFFDocumentND);
		GDREGISTER_CLASS(OrthoplexCellMeshND);
		GDREGISTER_CLASS(OrthoplexWireMeshND);
		GDREGISTER_CLASS(CellMaterialND);
		GDREGISTER_CLASS(WireMaterialND);
		// Depends on mesh.
		GDREGISTER_CLASS(MarkerND);
		// Render.
#if GDEXTENSION
		GDREGISTER_CLASS(WireframeRenderCanvasND);
		GDREGISTER_CLASS(WireframeCanvasRenderingEngineND);
#endif // GDEXTENSION
		RenderingServerND *rendering_server = memnew(RenderingServerND);
		rendering_server->register_rendering_engine(memnew(WireframeCanvasRenderingEngineND));
		add_godot_singleton("RenderingServerND", rendering_server);
#ifdef TOOLS_ENABLED
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
#ifdef GDEXTENSION
		GDREGISTER_CLASS(EditorCameraND);
		GDREGISTER_CLASS(EditorCameraSettingsND);
		GDREGISTER_CLASS(EditorCreateNDSceneButton);
		GDREGISTER_CLASS(EditorImportPluginBaseND);
		GDREGISTER_CLASS(EditorImportPluginOFFBaseND);
		GDREGISTER_CLASS(EditorImportPluginOFFCellND);
		GDREGISTER_CLASS(EditorImportPluginOFFSceneND);
		GDREGISTER_CLASS(EditorImportPluginOFFWireND);
		GDREGISTER_CLASS(EditorInputSurfaceND);
		GDREGISTER_CLASS(EditorMainScreenND);
		GDREGISTER_CLASS(EditorMainViewportND);
		GDREGISTER_CLASS(EditorTransformGizmoND);
		GDREGISTER_CLASS(EditorViewportRotationND);
		GDREGISTER_CLASS(GodotNDEditorPlugin);
#elif GODOT_MODULE
		EditorColorMap::add_conversion_color_pair("fff6a2", "ccc055");
		EditorColorMap::add_conversion_color_pair("fe5", "ba0");
		EditorColorMap::add_conversion_color_pair("fe7", "ba2");
		EditorColorMap::add_conversion_color_pair("fe9", "ba4");
		EditorColorMap::add_conversion_color_pair("fd0", "a90");
		EditorColorMap::add_conversion_color_pair("fd3", "a93");
		EditorColorMap::add_conversion_color_pair("dc3", "870");
		EditorColorMap::add_conversion_color_pair("ba3", "665d11");
#endif // GDEXTENSION or GODOT_MODULE
		EditorPlugins::add_by_type<GodotNDEditorPlugin>();
#endif // TOOLS_ENABLED
	}
}

void uninitialize_nd_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		remove_godot_singleton("GeometryND");
		remove_godot_singleton("RenderingServerND");
		remove_godot_singleton("VectorND");
		memdelete(GeometryND::get_singleton());
		memdelete(RenderingServerND::get_singleton());
		memdelete(VectorND::get_singleton());
	}
}
