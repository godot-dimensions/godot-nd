#pragma once

#include "../../nodes/camera_nd.h"
#include "editor_viewport_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/color_picker_button.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/editor_spin_slider.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#elif GODOT_MODULE
#include "core/io/config_file.h"
#include "editor/gui/editor_spin_slider.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/label.h"
#include "scene/gui/popup.h"
#endif

class WorldEnvironmentND;

class EditorPreviewEnvironmentND : public HBoxContainer {
	GDCLASS(EditorPreviewEnvironmentND, HBoxContainer);

	EditorMainScreenND *_editor_main_screen = nullptr;
	EditorUndoRedoManager *_undo_redo = nullptr;
	Ref<ConfigFile> _nd_editor_config_file;
	String _nd_editor_config_file_path;

	// Nodes in the scene.
	WorldEnvironmentND *_preview_world_environment = nullptr;

	// Buttons in the toolbar.
	Button *_toggle_preview_environment_button = nullptr;
	Button *_environment_settings_button = nullptr;

	// The popup panel and its members.
	PopupPanel *_environment_popup = nullptr;

	VBoxContainer *_environment_column_vbox = nullptr;
	Label *_environment_settings_disabled_label = nullptr;
	VBoxContainer *_environment_properties_vbox = nullptr;
	Label *_environment_single_color_label = nullptr;
	ColorPickerButton *_environment_single_color = nullptr;
	EditorSpinSlider *_environment_energy_multiplier = nullptr;

	void _on_environment_settings_pressed();
	void _on_toggle_preview_changed(const bool p_toggled_ignored);
	void _on_environment_color_changed(const Color &p_color_ignored);
	void _on_environment_energy_multiplier_changed(const double p_value_ignored);
	void _on_scene_node_changed(Node *p_node);
	void _add_environment_to_scene();
	void _reset_environment();
	bool _edited_scene_contains(const StringName &p_type) const;
	void _update_environment(const bool p_toggled_ignored = false);
	void _update_theme();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	void apply_to_nodes() const;
	void setup(EditorMainScreenND *p_editor_main_screen, EditorUndoRedoManager *p_undo_redo, const Ref<ConfigFile> &p_config_file, const String &p_config_file_path);
	void write_to_config_file() const;
};
