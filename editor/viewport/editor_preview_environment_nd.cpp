#include "editor_preview_environment_nd.h"

#include "../../render/environment/sky/plain_sky_material_nd.h"
#include "../../render/environment/world_environment_nd.h"

#ifdef TOOLS_ENABLED
#if GDEXTENSION
#include <godot_cpp/classes/editor_interface.hpp>
#elif GODOT_MODULE
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR < 5
#include "editor/scene_tree_dock.h"
#else
#include "editor/docks/scene_tree_dock.h"
#endif
#include "editor/editor_interface.h"
#endif
#endif // TOOLS_ENABLED

#if GDEXTENSION
#include <godot_cpp/classes/scene_tree.hpp>
#elif GODOT_MODULE
#include "scene/main/scene_tree.h"
#endif

void EditorPreviewEnvironmentND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			const Callable node_changed_callable = callable_mp(this, &EditorPreviewEnvironmentND::_on_scene_node_changed);
			if (!get_tree()->is_connected(StringName("node_added"), node_changed_callable)) {
				get_tree()->connect(StringName("node_added"), node_changed_callable);
				get_tree()->connect(StringName("node_removed"), node_changed_callable);
			}
			_update_theme();
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;
		case NOTIFICATION_READY: {
			// Children register with RenderingServerND after this node enters the tree.
			_update_environment();
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			_update_environment();
		} break;
	}
}

void EditorPreviewEnvironmentND::_on_environment_settings_pressed() {
	const Vector2 button_bottom = _environment_settings_button->get_screen_position() + _environment_settings_button->get_size();
	_environment_popup->reset_size();
	const float popup_half_width = _environment_popup->get_contents_minimum_size().x * 0.5f;
	_environment_popup->set_position(Vector2i(button_bottom - Vector2(popup_half_width, 0.0f)));
	_environment_popup->popup();
	_environment_popup->grab_focus();
}

void EditorPreviewEnvironmentND::_on_toggle_preview_changed(const bool p_toggled_ignored) {
	_update_environment();
	write_to_config_file();
}

void EditorPreviewEnvironmentND::_on_environment_color_changed(const Color &p_color_ignored) {
	apply_to_nodes();
	write_to_config_file();
}

void EditorPreviewEnvironmentND::_on_environment_energy_multiplier_changed(const double p_value_ignored) {
	apply_to_nodes();
	write_to_config_file();
}

void EditorPreviewEnvironmentND::_on_scene_node_changed(Node *p_node) {
	const bool world_environment_changed = p_node != _preview_world_environment && Object::cast_to<WorldEnvironmentND>(p_node) != nullptr;
	if (world_environment_changed) {
		// SceneTree's node_added/node_removed signals may run before the node's
		// enter/exit-tree notification has finished registering it with RenderingServerND.
		callable_mp(this, &EditorPreviewEnvironmentND::_update_environment).call_deferred(false);
	}
}

bool EditorPreviewEnvironmentND::_edited_scene_contains(const StringName &p_type) const {
	Node *edited_scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
	if (edited_scene_root == nullptr) {
		return false;
	}
	if (edited_scene_root->is_class(p_type)) {
		return true;
	}
	return !edited_scene_root->find_children("*", p_type, true, false).is_empty();
}

void EditorPreviewEnvironmentND::_add_environment_to_scene() {
	_environment_popup->hide();
	Node *edited_scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
#if GODOT_MODULE
	if (edited_scene_root == nullptr) {
		SceneTreeDock::get_singleton()->add_root_node(memnew(NodeND));
		edited_scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
	}
#endif
	ERR_FAIL_NULL_MSG(edited_scene_root, "A scene root is required before adding the preview environment to the scene.");
	ERR_FAIL_NULL(_undo_redo);
	WorldEnvironmentND *new_environment = Object::cast_to<WorldEnvironmentND>(_preview_world_environment->duplicate());
	ERR_FAIL_NULL(new_environment);
	new_environment->set_current(false);
	Ref<SkyMaterialND> preview_sky_material = _preview_world_environment->get_sky_material();
	if (preview_sky_material.is_valid()) {
		Ref<SkyMaterialND> duplicated_sky_material = preview_sky_material->duplicate();
		new_environment->set_sky_material(duplicated_sky_material);
	}
	new_environment->set_name(StringName("WorldEnvironmentND"));
	_undo_redo->create_action(TTR("Add Preview Environment to Scene"));
	_undo_redo->add_do_method(edited_scene_root, StringName("add_child"), new_environment, true);
	_undo_redo->add_do_method(edited_scene_root, StringName("move_child"), new_environment, 0);
	_undo_redo->add_do_method(new_environment, StringName("set_owner"), edited_scene_root);
	_undo_redo->add_undo_method(edited_scene_root, StringName("remove_child"), new_environment);
	_undo_redo->add_do_reference(new_environment);
	_undo_redo->commit_action();
}

void EditorPreviewEnvironmentND::_reset_environment() {
	_environment_single_color->set_pick_color(Color(0.0f, 0.0f, 0.0f));
	_environment_energy_multiplier->set_value_no_signal(1.0);
	apply_to_nodes();
	write_to_config_file();
}

void EditorPreviewEnvironmentND::apply_to_nodes() const {
	ERR_FAIL_NULL(_preview_world_environment);
	Ref<PlainSkyMaterialND> plain_sky_material = _preview_world_environment->get_sky_material();
	if (plain_sky_material.is_null()) {
		plain_sky_material.instantiate();
		_preview_world_environment->set_sky_material(plain_sky_material);
	}
	plain_sky_material->set_color(_environment_single_color->get_pick_color());
	plain_sky_material->set_energy_multiplier(_environment_energy_multiplier->get_value());
}

void EditorPreviewEnvironmentND::_update_environment(const bool p_toggled_ignored) {
	if (_preview_world_environment == nullptr) {
		return;
	}
	Node *edited_scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
	TypedArray<Node> scene_world_environments;
	if (edited_scene_root != nullptr) {
		if (Object::cast_to<WorldEnvironmentND>(edited_scene_root) != nullptr) {
			scene_world_environments.append(edited_scene_root);
		}
		scene_world_environments.append_array(edited_scene_root->find_children("*", "WorldEnvironmentND", true, false));
	}
	const bool scene_has_world_environment = !scene_world_environments.is_empty();
	_toggle_preview_environment_button->set_disabled(scene_has_world_environment);
	_environment_settings_disabled_label->set_visible(scene_has_world_environment);
	_environment_properties_vbox->set_visible(!scene_has_world_environment);
	const bool preview_environment_enabled = is_visible_in_tree() && _toggle_preview_environment_button->is_pressed() && !scene_has_world_environment;
	if (preview_environment_enabled) {
		_preview_world_environment->make_current();
	} else {
		_preview_world_environment->clear_current();
		bool scene_environment_is_current = false;
		for (int i = 0; i < scene_world_environments.size(); i++) {
			WorldEnvironmentND *world_environment = Object::cast_to<WorldEnvironmentND>(scene_world_environments[i]);
			CRASH_COND(world_environment == nullptr);
			scene_environment_is_current |= world_environment->is_current();
		}
		if (!scene_environment_is_current && scene_has_world_environment) {
			WorldEnvironmentND *first_world_environment = Object::cast_to<WorldEnvironmentND>(scene_world_environments[0]);
			CRASH_COND(first_world_environment == nullptr);
			first_world_environment->make_current();
		}
	}
}

void EditorPreviewEnvironmentND::_update_theme() {
	// Set icons.
	_toggle_preview_environment_button->set_button_icon(get_editor_theme_icon(StringName("PreviewEnvironment")));
	_environment_settings_button->set_button_icon(get_editor_theme_icon(StringName("GuiTabMenuHl")));
	// Set the minimum height of the color picker.
	const Size2 min_color_size = Size2(100.0f, 30.0f) * EDSCALE;
	_environment_single_color->set_custom_minimum_size(min_color_size);
}

void EditorPreviewEnvironmentND::setup(EditorMainScreenND *p_editor_main_screen, EditorUndoRedoManager *p_undo_redo, const Ref<ConfigFile> &p_config_file, const String &p_config_file_path) {
	set_name(StringName("EditorPreviewEnvironmentND"));
	_editor_main_screen = p_editor_main_screen;
	_undo_redo = p_undo_redo;
	_nd_editor_config_file = p_config_file;
	_nd_editor_config_file_path = p_config_file_path;

	_toggle_preview_environment_button = memnew(Button);
	_toggle_preview_environment_button->set_toggle_mode(true);
	_toggle_preview_environment_button->set_theme_type_variation("FlatButton");
	_toggle_preview_environment_button->set_tooltip_text(TTR("Toggle preview environment.\nIf a WorldEnvironmentND node is added to the scene, preview environment is disabled."));
	_toggle_preview_environment_button->connect(StringName("toggled"), callable_mp(this, &EditorPreviewEnvironmentND::_on_toggle_preview_changed));
	_toggle_preview_environment_button->set_pressed_no_signal(p_config_file->get_value("preview_environment", "environment_enabled", true));
	add_child(_toggle_preview_environment_button);

	_environment_settings_button = memnew(Button);
	_environment_settings_button->set_theme_type_variation("FlatButton");
	_environment_settings_button->set_tooltip_text(TTR("Edit Environment settings."));
	_environment_settings_button->connect(StringName("pressed"), callable_mp(this, &EditorPreviewEnvironmentND::_on_environment_settings_pressed));
	add_child(_environment_settings_button);

	// Note: Most of the below code is AI generated, with the main instruction to the AI
	// being to implement a settings popup in the style of Godot's 3D environment settings.

	// Set up the preview Environment settings popup.
	_environment_popup = memnew(PopupPanel);
	add_child(_environment_popup);
	HBoxContainer *environment_hbox = memnew(HBoxContainer);
	environment_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	environment_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
	environment_hbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	_environment_popup->add_child(environment_hbox);
	constexpr float MIN_COLUMN_HEIGHT = 130.0f;

	_preview_world_environment = memnew(WorldEnvironmentND);
	_preview_world_environment->set_name(StringName("PreviewWorldEnvironmentND"));
	add_child(_preview_world_environment);

	_environment_column_vbox = memnew(VBoxContainer);
	_environment_column_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	_environment_column_vbox->set_custom_minimum_size(Size2(200.0f, MIN_COLUMN_HEIGHT) * EDSCALE);
	environment_hbox->add_child(_environment_column_vbox);
	Label *environment_title = memnew(Label);
	environment_title->set_theme_type_variation("HeaderMedium");
	environment_title->set_text(TTR("Preview Environment"));
	environment_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	_environment_column_vbox->add_child(environment_title);

	_environment_settings_disabled_label = memnew(Label);
	// Translations may alter the placement of line breaks, keeping the lines at a limited width.
	_environment_settings_disabled_label->set_text(TTR("Disabled because a\nWorldEnvironmentND\nnode exists in the scene."));
	_environment_settings_disabled_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	_environment_settings_disabled_label->set_h_size_flags(SIZE_EXPAND_FILL);
	_environment_settings_disabled_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	_environment_settings_disabled_label->set_visible(false);
	_environment_column_vbox->add_child(_environment_settings_disabled_label);

	_environment_properties_vbox = memnew(VBoxContainer);
	_environment_properties_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	_environment_column_vbox->add_child(_environment_properties_vbox);

	_environment_single_color_label = memnew(Label);
	_environment_single_color_label->set_text(TTR("Single Color"));
	_environment_properties_vbox->add_child(_environment_single_color_label);
	_environment_single_color = memnew(ColorPickerButton);
	_environment_single_color->set_h_size_flags(SIZE_EXPAND_FILL);
	_environment_single_color->set_edit_alpha(false);
	_environment_single_color->set_pick_color(p_config_file->get_value("preview_environment", "single_color", Color(0.0f, 0.0f, 0.0f)));
	_environment_single_color->connect(StringName("color_changed"), callable_mp(this, &EditorPreviewEnvironmentND::_on_environment_color_changed));
	_environment_properties_vbox->add_child(_environment_single_color);

	Label *environment_energy_multiplier_label = memnew(Label);
	environment_energy_multiplier_label->set_text(TTR("Energy Multiplier"));
	_environment_properties_vbox->add_child(environment_energy_multiplier_label);
	_environment_energy_multiplier = memnew(EditorSpinSlider);
	_environment_energy_multiplier->set_min(0.1);
	_environment_energy_multiplier->set_max(2.0);
	_environment_energy_multiplier->set_step(0.01);
	_environment_energy_multiplier->set_exp_ratio(true);
	_environment_energy_multiplier->set_allow_greater(true);
	_environment_energy_multiplier->set_allow_lesser(true);
	_environment_energy_multiplier->set_value(p_config_file->get_value("preview_environment", "energy_multiplier", 1.0));
	_environment_energy_multiplier->connect(StringName("value_changed"), callable_mp(this, &EditorPreviewEnvironmentND::_on_environment_energy_multiplier_changed));
	_environment_properties_vbox->add_child(_environment_energy_multiplier);

	_environment_properties_vbox->add_spacer(false)->set_v_size_flags(SIZE_EXPAND_FILL);
	HBoxContainer *environment_action_hbox = memnew(HBoxContainer);
	_environment_properties_vbox->add_child(environment_action_hbox);
	Button *environment_reset_button = memnew(Button);
	environment_reset_button->set_text(TTR("Reset"));
	environment_reset_button->set_h_size_flags(SIZE_EXPAND_FILL);
	environment_reset_button->connect(StringName("pressed"), callable_mp(this, &EditorPreviewEnvironmentND::_reset_environment));
	environment_action_hbox->add_child(environment_reset_button);
	Button *environment_add_to_scene_button = memnew(Button);
	environment_add_to_scene_button->set_text(TTR("Add Environment to Scene"));
	environment_add_to_scene_button->set_tooltip_text(TTR("Adds a WorldEnvironmentND node matching the preview environment settings to the current scene."));
	environment_add_to_scene_button->set_h_size_flags(SIZE_EXPAND_FILL);
	environment_add_to_scene_button->connect(StringName("pressed"), callable_mp(this, &EditorPreviewEnvironmentND::_add_environment_to_scene));
	environment_action_hbox->add_child(environment_add_to_scene_button);

	apply_to_nodes();
	_update_environment();
}

void EditorPreviewEnvironmentND::write_to_config_file() const {
	ERR_FAIL_COND(_nd_editor_config_file.is_null());
	if (_nd_editor_config_file->has_section("preview_environment")) {
		_nd_editor_config_file->erase_section("preview_environment");
	}
	if (!_toggle_preview_environment_button->is_pressed()) {
		_nd_editor_config_file->set_value("preview_environment", "environment_enabled", false);
	}
	if (!_environment_single_color->get_pick_color().is_equal_approx(Color(0.0f, 0.0f, 0.0f))) {
		_nd_editor_config_file->set_value("preview_environment", "single_color", _environment_single_color->get_pick_color());
	}
	if (!Math::is_equal_approx(_environment_energy_multiplier->get_value(), 1.0)) {
		_nd_editor_config_file->set_value("preview_environment", "energy_multiplier", _environment_energy_multiplier->get_value());
	}
	_nd_editor_config_file->save(_nd_editor_config_file_path);
}
