#pragma once
// This file should be included before any other files.

// Uncomment one of these to help IDEs detect the build mode.
// The build system already defines one of these, so keep them
// commented out when committing.
#ifndef GDEXTENSION
//#define GDEXTENSION 1
#endif // GDEXTENSION

#ifndef GODOT_MODULE
//#define GODOT_MODULE 1
#endif // GODOT_MODULE

#if GDEXTENSION
// Extremely common classes used by most files. Customize for your extension as needed.
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/version.hpp>
#include <godot_cpp/variant/string.hpp>
#define GDEXTMOD_GUI_INPUT _gui_input
#define GET_NODE_TYPE(m_parent, m_type, m_path) m_parent->get_node<m_type>(NodePath(m_path))
#define MODULE_OVERRIDE
// Including the namespace helps make GDExtension code more similar to module code.
using namespace godot;
#elif GODOT_MODULE
#include "core/object/class_db.h"
#include "core/string/ustring.h"
#include "core/version.h"
#define GDEXTMOD_GUI_INPUT gui_input
#define GET_NODE_TYPE(m_parent, m_type, m_path) Object::cast_to<m_type>(m_parent->get_node(NodePath(m_path)))
#define MODULE_OVERRIDE override

#ifndef GODOT_VERSION_MAJOR
// Prior to Godot 4.5, the Godot version macros were just "VERSION_*" which did not match the godot-cpp API.
// See https://github.com/godotengine/godot/pull/103557
#define GODOT_VERSION_MAJOR VERSION_MAJOR
#define GODOT_VERSION_MINOR VERSION_MINOR
#define GODOT_VERSION_PATCH VERSION_PATCH
#endif

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR < 6
// Prior to Godot 4.6, the internal API of free_rid in RenderingServer and other servers did not match the exposed API.
// See https://github.com/godotengine/godot/pull/107139
#define free_rid free
#endif

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR > 4
// In Godot 4.5 and later, Math_TAU was replaced with Math::TAU, and a few other things also moved to the Math namespace.
#define ABS Math::abs
#define Math_E Math::E
#define Math_PI Math::PI
#define Math_SQRT12 Math::SQRT12
#define Math_SQRT2 Math::SQRT2
#define Math_TAU Math::TAU
#endif

#define MOUSE_BUTTON_LEFT MouseButton::LEFT
#else
#error "Must build as Godot GDExtension or Godot module."
#endif

#include <limits>

#ifndef Math_INF
#define Math_INF std::numeric_limits<double>::infinity()
#endif // Math_INF

#ifndef Math_NAN
#define Math_NAN std::numeric_limits<double>::quiet_NaN()
#endif // Math_INF

#ifndef _NO_DISCARD_
#define _NO_DISCARD_ [[nodiscard]]
#endif // _NO_DISCARD_

#define VectorN PackedFloat64Array
