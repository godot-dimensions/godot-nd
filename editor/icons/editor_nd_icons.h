#pragma once

#include "../../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/image_texture.hpp>
#elif GODOT_MODULE
#include "scene/resources/image_texture.h"
#endif

Ref<ImageTexture> _generate_editor_nd_icon(const String &p_icon_name);
