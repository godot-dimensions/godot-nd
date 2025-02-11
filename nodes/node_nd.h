#pragma once

#include "../godot_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/node.hpp>
#elif GODOT_MODULE
#include "scene/main/node.h"
#endif

class NodeND : public Node {
	GDCLASS(NodeND, Node);

protected:
	static void _bind_methods();
};
