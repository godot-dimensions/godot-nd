#include "register_types.h"

#if GDEXTENSION
#include <godot_cpp/classes/engine.hpp>
#elif GODOT_MODULE
#include "core/config/engine.h"
#endif

// General.
#include "math/rect_nd.h"
#include "math/transform_nd.h"
#include "math/vector_nd.h"
#include "nodes/camera_nd.h"
#include "nodes/node_nd.h"

// Virtual classes.
#include "mesh/material_nd.h"
#include "mesh/mesh_nd.h"
#include "mesh/wire/wire_mesh_nd.h"

// Mesh.
#include "mesh/mesh_instance_nd.h"
#include "mesh/wire/array_wire_mesh_nd.h"
#include "mesh/wire/box_wire_mesh_nd.h"
#include "mesh/wire/orthoplex_wire_mesh_nd.h"
#include "mesh/wire/wire_material_nd.h"

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
		GDREGISTER_CLASS(RectND);
		GDREGISTER_CLASS(TransformND);
		GDREGISTER_CLASS(VectorND);
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// General.
		GDREGISTER_CLASS(NodeND);
		GDREGISTER_CLASS(CameraND);
		add_godot_singleton("VectorND", memnew(VectorND));
		// Virtual classes.
		GDREGISTER_CLASS(MaterialND);
		GDREGISTER_CLASS(MeshND);
		GDREGISTER_CLASS(WireMeshND);
		// Mesh.
		GDREGISTER_CLASS(ArrayWireMeshND);
		GDREGISTER_CLASS(BoxWireMeshND);
		GDREGISTER_CLASS(MeshInstanceND);
		GDREGISTER_CLASS(OrthoplexWireMeshND);
		GDREGISTER_CLASS(WireMaterialND);
	}
}

void uninitialize_nd_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		remove_godot_singleton("VectorND");
		memdelete(VectorND::get_singleton());
	}
}
