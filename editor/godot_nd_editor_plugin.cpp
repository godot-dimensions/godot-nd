#include "godot_nd_editor_plugin.h"

#include "../nodes/node_nd.h"
#include "icons/editor_nd_icons.h"

#if GDEXTENSION
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#elif GODOT_MODULE
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "scene/main/window.h"

#if VERSION_HEX < 0x040400
#define set_button_icon set_icon
#endif
#endif

Button *_find_button_by_text_nd(Node *p_start, const String &p_text) {
	Button *start_button = Object::cast_to<Button>(p_start);
	if (start_button != nullptr) {
		if (start_button->get_text() == p_text) {
			return start_button;
		}
	}
	for (int i = 0; i < p_start->get_child_count(); i++) {
		Button *button = _find_button_by_text_nd(p_start->get_child(i), p_text);
		if (button != nullptr) {
			return button;
		}
	}
	return nullptr;
}

void EditorCreateNDSceneButton::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		set_button_icon(_generate_editor_nd_icon("NodeND"));
	}
}

void GodotNDEditorPlugin::_move_nd_main_screen_tab_button() const {
	Control *editor = EditorInterface::get_singleton()->get_base_control();
	ERR_FAIL_NULL(editor);
	// Move ND button to the left of the "Script" button, to the right of the "3D" button.
	Node *button_asset_lib_tab = editor->find_child("AssetLib", true, false);
	ERR_FAIL_NULL(button_asset_lib_tab);
	Node *main_editor_button_hbox = button_asset_lib_tab->get_parent();
	Button *button_nd_tab = GET_NODE_TYPE(main_editor_button_hbox, Button, "ND");
	Button *button_script_tab = GET_NODE_TYPE(main_editor_button_hbox, Button, "Script");
	ERR_FAIL_NULL(button_nd_tab);
	ERR_FAIL_NULL(button_script_tab);
	main_editor_button_hbox->move_child(button_nd_tab, button_script_tab->get_index());
}

void GodotNDEditorPlugin::_inject_nd_scene_button() {
	Control *editor = EditorInterface::get_singleton()->get_base_control();
	ERR_FAIL_NULL(editor);
	// Add a "ND Scene" button above the "User Interface" button, below the "3D Scene" button.
	Button *button_other_node_scene = _find_button_by_text_nd(editor, "Other Node");
	ERR_FAIL_NULL(button_other_node_scene);
	Control *beginner_node_shortcuts = Object::cast_to<Control>(button_other_node_scene->get_parent()->get_child(0));
	Button *user_interface_scene = _find_button_by_text_nd(beginner_node_shortcuts, "User Interface");
	ERR_FAIL_NULL(user_interface_scene);
	// Now that we know where to insert the button, create it.
	EditorCreateNDSceneButton *button_nd = memnew(EditorCreateNDSceneButton);
	button_nd->set_text(TTR("ND Scene"));
	button_nd->set_button_icon(_generate_editor_nd_icon("NodeND"));
	button_nd->connect(StringName("pressed"), callable_mp(this, &GodotNDEditorPlugin::_create_nd_scene));
	beginner_node_shortcuts->add_child(button_nd);
	beginner_node_shortcuts->move_child(button_nd, user_interface_scene->get_index());
}

void GodotNDEditorPlugin::_create_nd_scene() {
	EditorInterface *editor_interface = EditorInterface::get_singleton();
	NodeND *new_node = memnew(NodeND);
	Node *editor_node = editor_interface->get_base_control()->get_parent();
	ERR_FAIL_NULL(editor_node);
	editor_node->call(StringName("set_edited_scene"), new_node);
	editor_interface->edit_node(new_node);
	EditorSelection *editor_selection = editor_interface->get_selection();
	editor_selection->clear();
	editor_selection->add_node(new_node);
}

void GodotNDEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_move_nd_main_screen_tab_button();
			call_deferred(StringName("_inject_nd_scene_button"));
		} break;
	}
}

#if GDEXTENSION
Ref<Texture2D> GodotNDEditorPlugin::GDEXTMOD_GET_PLUGIN_ICON() const
#elif GODOT_MODULE
const Ref<Texture2D> GodotNDEditorPlugin::GDEXTMOD_GET_PLUGIN_ICON() const
#endif
{
	return _generate_editor_nd_icon("ND");
}

bool GodotNDEditorPlugin::GDEXTMOD_HANDLES(Object *p_object) const {
	return Object::cast_to<NodeND>(p_object) != nullptr;
}

void GodotNDEditorPlugin::GDEXTMOD_MAKE_VISIBLE(bool p_visible) {
	_main_screen->set_visible(p_visible);
}

void GodotNDEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_inject_nd_scene_button"), &GodotNDEditorPlugin::_inject_nd_scene_button);
}

GodotNDEditorPlugin::GodotNDEditorPlugin() {
	set_name(StringName("GodotNDEditorPlugin"));
	Control *godot_editor_main_screen = EditorInterface::get_singleton()->get_editor_main_screen();
	if (godot_editor_main_screen->has_node(NodePath("EditorMainScreenND"))) {
		_main_screen = GET_NODE_TYPE(godot_editor_main_screen, EditorMainScreenND, "EditorMainScreenND");
	} else {
		_main_screen = memnew(EditorMainScreenND);
		_main_screen->setup(get_undo_redo());
		godot_editor_main_screen->add_child(_main_screen);
	}
}
