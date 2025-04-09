#pragma once

#include "../../godot_nd_defines.h"
#include "../../nodes/node_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>

#define EDITOR_GET(m_name) EditorInterface::get_singleton()->get_editor_settings()->get_setting(m_name)
#define EDSCALE EditorInterface::get_singleton()->get_editor_scale()
#define ERR_THREAD_GUARD
#define TTR(m_text) m_text
#define get_editor_theme_icon(m_name) get_theme_icon(m_name, StringName("EditorIcons"))
#elif GODOT_MODULE
#include "core/input/input.h"
#include "editor/editor_settings.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/control.h"

#if VERSION_HEX < 0x040400
#define set_button_icon set_icon
#endif

#define KEY_A Key::A
#define KEY_B Key::B
#define KEY_C Key::C
#define KEY_D Key::D
#define KEY_E Key::E
#define KEY_F Key::F
#define KEY_G Key::G
#define KEY_H Key::H
#define KEY_I Key::I
#define KEY_J Key::J
#define KEY_K Key::K
#define KEY_L Key::L
#define KEY_M Key::M
#define KEY_N Key::N
#define KEY_O Key::O
#define KEY_P Key::P
#define KEY_Q Key::Q
#define KEY_R Key::R
#define KEY_S Key::S
#define KEY_T Key::T
#define KEY_U Key::U
#define KEY_V Key::V
#define KEY_W Key::W
#define KEY_X Key::X
#define KEY_Y Key::Y
#define KEY_Z Key::Z
#define KEY_BRACKETLEFT Key::BRACKETLEFT
#define KEY_BRACKETRIGHT Key::BRACKETRIGHT
#define KEY_COMMA Key::COMMA
#define KEY_PERIOD Key::PERIOD
#define KEY_SEMICOLON Key::SEMICOLON
#define KEY_SHIFT Key::SHIFT

#define MOUSE_BUTTON_MASK_MIDDLE MouseButtonMask::MIDDLE
#define MOUSE_BUTTON_MASK_RIGHT MouseButtonMask::RIGHT
#define MOUSE_BUTTON_RIGHT MouseButton::RIGHT
#define MOUSE_BUTTON_WHEEL_UP MouseButton::WHEEL_UP
#define MOUSE_BUTTON_WHEEL_DOWN MouseButton::WHEEL_DOWN
#endif

class EditorCameraND;
class EditorInputSurfaceND;
class EditorMainScreenND;
class EditorMainViewportND;
class EditorTransformGizmoND;
class EditorViewportRotationND;
