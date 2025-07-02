#pragma once

#include "editor_viewport_nd_defines.h"

#if GDEXTENSION
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#elif GODOT_MODULE
#include "core/io/config_file.h"
#include "editor/editor_inspector.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/spin_box.h"
#endif

class MarkerND;

// Main class for the ND editor main screen.
// Has a toolbar at the top and a display of the ND scene below it.
// Has an EditorMainViewportND, or up to 4 of them, for displaying the ND scene.
class EditorMainScreenND : public Control {
	GDCLASS(EditorMainScreenND, Control);

	// Defined for readability but not intended to be changed.
	static constexpr int _MAX_VIEWPORTS = 1;

public:
	// Ensure the MODE items are kept in sync with EditorTransformGizmoND::GizmoMode.
	enum ToolbarButton {
		TOOLBAR_BUTTON_MODE_SELECT, // 0
		TOOLBAR_BUTTON_MODE_MOVE, // 1
		TOOLBAR_BUTTON_MODE_ROTATE, // 2
		TOOLBAR_BUTTON_MODE_SCALE, // 3
		TOOLBAR_BUTTON_MODE_STRETCH, // 4
		TOOLBAR_BUTTON_MODE_MAX, // 5
		TOOLBAR_BUTTON_USE_LOCAL_ROTATION = TOOLBAR_BUTTON_MODE_MAX, // Still 5
		TOOLBAR_BUTTON_MAX
	};

	// Ensure the KEEP items are kept in sync with EditorTransformGizmoND::KeepMode.
	enum TransformSettingsItem {
		TRANSFORM_SETTING_KEEP_FREEFORM, // 0
		TRANSFORM_SETTING_KEEP_ORTHOGONAL, // 1
		TRANSFORM_SETTING_KEEP_CONFORMAL, // 2
		TRANSFORM_SETTING_KEEP_ORTHONORMAL, // 3
		TRANSFORM_SETTING_KEEP_MAX, // 4
	};

	enum LayoutItem {
		LAYOUT_ITEM_1_VIEWPORT, // 0
		LAYOUT_ITEM_2_VIEWPORTS_TOP_BOTTOM, // 1
		LAYOUT_ITEM_2_VIEWPORTS_LEFT_RIGHT, // 2
		LAYOUT_ITEM_3_VIEWPORTS_TOP_WIDE, // 3
		LAYOUT_ITEM_3_VIEWPORTS_RIGHT_TALL, // 4
		LAYOUT_ITEM_4_VIEWPORTS, // 5
		LAYOUT_ITEM_VIEWPORT_MAX, // 6
	};

	enum ViewItem {
		VIEW_ITEM_RENDERING_ENGINE,
		VIEW_ITEM_SHOW_ORIGIN_MARKER,
		VIEW_ITEM_CAMERA_SETTINGS,
	};

private:
	Button *_toolbar_buttons[TOOLBAR_BUTTON_MAX] = { nullptr };
	MenuButton *_transform_settings_menu = nullptr;
	MenuButton *_layout_menu = nullptr;
	MenuButton *_view_menu = nullptr;
	PopupMenu *_rendering_engine_menu_popup = nullptr;
	ConfirmationDialog *_camera_settings_dialog = nullptr;
	EditorInspector *_camera_settings_inspector = nullptr;
	EditorCameraSettingsND *_camera_settings = nullptr;
	Control *_editor_main_viewport_holder = nullptr;
	EditorMainViewportND *_editor_main_viewports[_MAX_VIEWPORTS] = { nullptr };
	HBoxContainer *_toolbar_hbox = nullptr;
	EditorTransformGizmoND *_transform_gizmo_nd = nullptr;
	MarkerND *_origin_marker = nullptr;
	Label *_dimensions_label = nullptr;
	Ref<ConfigFile> _nd_editor_config_file;
	String _nd_editor_config_file_path;

	PackedColorArray _axis_colors;
	double _information_label_auto_hide_time = 0.0;

	void _apply_nd_editor_settings();
	int _calculate_scene_dimension(Node *p_node) const;
	void _on_button_toggled(const bool p_toggled_on, const int p_option);
	void _on_selection_changed();
	void _on_transform_settings_menu_id_pressed(const int p_id);
	void _on_layout_menu_id_pressed(const int p_id);
	void _on_view_menu_id_pressed(const int p_id);
	void _on_rendering_engine_menu_id_pressed(const int p_id);
	void _update_rendering_engine_menu();
	void _update_theme();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	PackedColorArray get_axis_colors() const;
	void press_menu_item(const int p_option);
	void set_viewport_layout(const int8_t p_viewport_count, const Side p_dominant_side = SIDE_TOP);
	void update_dimension();

	void setup(EditorUndoRedoManager *p_undo_redo_manager);
	~EditorMainScreenND();
};
