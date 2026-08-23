#include "world_environment_nd.h"

#include "../rendering_server_nd.h"

void WorldEnvironmentND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			RenderingServerND::get_singleton()->register_world_environment(this);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			// The singleton is already gone if the module was uninitialized first.
			RenderingServerND *rendering_server = RenderingServerND::get_singleton();
			if (rendering_server != nullptr) {
				rendering_server->unregister_world_environment(this);
			}
		} break;
	}
}

bool WorldEnvironmentND::is_current() const {
	return _is_current;
}

void WorldEnvironmentND::set_current(const bool p_enabled) {
	_is_current = p_enabled;
	if (is_inside_tree()) {
		if (p_enabled) {
			RenderingServerND::get_singleton()->make_world_environment_current(this);
		} else {
			RenderingServerND::get_singleton()->clear_world_environment_current(this);
		}
	}
}

void WorldEnvironmentND::clear_current(const bool p_enable_next) {
	_is_current = false;
	if (p_enable_next && is_inside_tree()) {
		RenderingServerND::get_singleton()->clear_world_environment_current(this);
	}
}

void WorldEnvironmentND::make_current() {
	_is_current = true;
	if (is_inside_tree()) {
		RenderingServerND::get_singleton()->make_world_environment_current(this);
	}
}

void WorldEnvironmentND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_current"), &WorldEnvironmentND::is_current);
	ClassDB::bind_method(D_METHOD("set_current", "enabled"), &WorldEnvironmentND::set_current);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "current"), "set_current", "is_current");
	ClassDB::bind_method(D_METHOD("clear_current", "enable_next"), &WorldEnvironmentND::clear_current);
	ClassDB::bind_method(D_METHOD("make_current"), &WorldEnvironmentND::make_current);

	ClassDB::bind_method(D_METHOD("get_sky_material"), &WorldEnvironmentND::get_sky_material);
	ClassDB::bind_method(D_METHOD("set_sky_material", "sky_material"), &WorldEnvironmentND::set_sky_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sky_material", PROPERTY_HINT_RESOURCE_TYPE, "SkyMaterialND"), "set_sky_material", "get_sky_material");
}
