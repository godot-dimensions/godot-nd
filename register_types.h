#pragma once

#include "godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/core/class_db.hpp>
#elif GODOT_MODULE
#include "modules/register_module_types.h"
#else
#error "Must build as Godot GDExtension or Godot module."
#endif

void initialize_nd_module(ModuleInitializationLevel p_level);
void uninitialize_nd_module(ModuleInitializationLevel p_level);
