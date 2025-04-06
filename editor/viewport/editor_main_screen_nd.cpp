#include "editor_main_screen_nd.h"

#include "editor_camera_nd.h"
#include "editor_main_viewport_nd.h"
#include "editor_transform_gizmo_nd.h"
#include "editor_viewport_rotation_nd.h"

#include "../../math/vector_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/v_separator.hpp>

#if GODOT_VERSION < 0x040500
#define get_top_selected_nodes get_transformable_selected_nodes
#endif // GODOT_VERSION
#elif GODOT_MODULE
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"

#if VERSION_HEX < 0x040500
#define get_top_selected_nodes get_selected_node_list
#else
#define get_top_selected_nodes get_top_selected_node_list
#endif // VERSION_HEX
#endif

int EditorMainScreenND::_calculate_scene_dimension(Node *p_node) const {
	int dimension = 0;
	if (p_node) {
		NodeND *node_nd = Object::cast_to<NodeND>(p_node);
		if (node_nd) {
			dimension = node_nd->get_dimension();
		}
		const int child_count = p_node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			Node *child = p_node->get_child(i);
			int child_dimension = _calculate_scene_dimension(child);
			if (child_dimension > dimension) {
				dimension = child_dimension;
			}
		}
	}
	return dimension;
}

void EditorMainScreenND::_on_button_toggled(const bool p_toggled_on, const int p_option) {
	press_menu_item(p_option);
}

void EditorMainScreenND::_on_selection_changed() {
	update_dimension();
	// Update selection.
	EditorSelection *selection = EditorInterface::get_singleton()->get_selection();
	TypedArray<Node> top_selected_nodes;
#if GDEXTENSION
	top_selected_nodes = selection->get_top_selected_nodes();
#elif GODOT_MODULE
	List<Node *> &top_selected_node_list = selection->get_top_selected_nodes();
	for (Node *node : top_selected_node_list) {
		top_selected_nodes.push_back(node);
	}
#endif
	_transform_gizmo_nd->selected_nodes_changed(top_selected_nodes);
}

void EditorMainScreenND::_on_transform_settings_menu_id_pressed(const int p_id) {
	PopupMenu *transform_settings_menu_popup = _transform_settings_menu->get_popup();
	if (p_id < TRANSFORM_SETTING_KEEP_MAX) {
		for (int i = 0; i < TRANSFORM_SETTING_KEEP_MAX; i++) {
			transform_settings_menu_popup->set_item_checked(i, i == p_id);
		}
		EditorTransformGizmoND::KeepMode keep_mode = (EditorTransformGizmoND::KeepMode)p_id;
		if (keep_mode == EditorTransformGizmoND::KeepMode::ORTHONORMAL) {
			// Orthonormal can't scale, so if any of the scale buttons are selected, switch to select.
			if (_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->is_pressed() || _toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->is_pressed()) {
				press_menu_item(TOOLBAR_BUTTON_MODE_SELECT);
			}
			_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->set_disabled(true);
			_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->release_focus();
			_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->set_disabled(true);
			_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->release_focus();
		} else {
			_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->set_disabled(false);
			_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->set_disabled(false);
		}
		_transform_gizmo_nd->set_keep_mode(keep_mode);
	}
}

void EditorMainScreenND::_on_view_layout_menu_id_pressed(const int p_id) {
	PopupMenu *view_layout_menu_popup = _view_layout_menu->get_popup();
	if (p_id < VIEW_LAYOUT_ITEM_VIEWPORT_MAX) {
		for (int i = 0; i < VIEW_LAYOUT_ITEM_VIEWPORT_MAX; i++) {
			view_layout_menu_popup->set_item_checked(i, i == p_id);
		}
	}
	switch (p_id) {
		case VIEW_LAYOUT_ITEM_1_VIEWPORT: {
			set_viewport_layout(1);
		} break;
		case VIEW_LAYOUT_ITEM_2_VIEWPORTS_TOP_BOTTOM: {
			set_viewport_layout(2, Side::SIDE_TOP);
		} break;
		case VIEW_LAYOUT_ITEM_2_VIEWPORTS_LEFT_RIGHT: {
			set_viewport_layout(2, Side::SIDE_RIGHT);
		} break;
		case VIEW_LAYOUT_ITEM_3_VIEWPORTS_TOP_WIDE: {
			set_viewport_layout(3, Side::SIDE_TOP);
		} break;
		case VIEW_LAYOUT_ITEM_3_VIEWPORTS_RIGHT_TALL: {
			set_viewport_layout(3, Side::SIDE_RIGHT);
		} break;
		case VIEW_LAYOUT_ITEM_4_VIEWPORTS: {
			set_viewport_layout(4);
		} break;
	}
}

void EditorMainScreenND::_update_theme() {
	// Set the toolbar offsets. Note that these numbers are designed to match Godot's 2D and 3D editor toolbars.
	_toolbar_hbox->set_offset(Side::SIDE_LEFT, 4.0f * EDSCALE);
	_toolbar_hbox->set_offset(Side::SIDE_RIGHT, -4.0f * EDSCALE);
	_toolbar_hbox->set_offset(Side::SIDE_TOP, 0.0f);
	_toolbar_hbox->set_offset(Side::SIDE_BOTTOM, 29.5f * EDSCALE);
	_editor_main_viewport_holder->set_offset(Side::SIDE_TOP, 33.0f * EDSCALE);
	// Set icons.
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT]->set_button_icon(get_editor_theme_icon(StringName("ToolSelect")));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_MOVE]->set_button_icon(get_editor_theme_icon(StringName("ToolMove")));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_ROTATE]->set_button_icon(get_editor_theme_icon(StringName("ToolRotate")));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->set_button_icon(get_editor_theme_icon(StringName("ToolScale")));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->set_button_icon(get_editor_theme_icon(StringName("AnimationAutoFitBezier")));
	_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION]->set_button_icon(get_editor_theme_icon(StringName("Object")));
	// Set the axis colors.
	if (_axis_colors.is_empty()) {
		_axis_colors.resize(192);
		for (int i = 4; i < 192; i++) {
			_axis_colors.set(i, VectorND::axis_color(i));
		}
	}
	_axis_colors.set(0, get_theme_color(StringName("axis_x_color"), StringName("Editor")));
	_axis_colors.set(1, get_theme_color(StringName("axis_y_color"), StringName("Editor")));
	_axis_colors.set(2, get_theme_color(StringName("axis_z_color"), StringName("Editor")));
	_axis_colors.set(3, Color(0.9f, 0.75f, 0.1f)); // W axis color.
	_transform_gizmo_nd->set_axis_colors(_axis_colors);
}

void EditorMainScreenND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;
	}
}

PackedColorArray EditorMainScreenND::get_axis_colors() const {
	return _axis_colors;
}

void EditorMainScreenND::press_menu_item(const int p_option) {
	switch (p_option) {
		case TOOLBAR_BUTTON_MODE_SELECT:
		case TOOLBAR_BUTTON_MODE_MOVE:
		case TOOLBAR_BUTTON_MODE_ROTATE:
		case TOOLBAR_BUTTON_MODE_SCALE:
		case TOOLBAR_BUTTON_MODE_STRETCH: {
			for (int i = 0; i < TOOLBAR_BUTTON_MODE_MAX; i++) {
				_toolbar_buttons[i]->set_pressed(i == p_option);
			}
			_transform_gizmo_nd->set_gizmo_mode(EditorTransformGizmoND::GizmoMode(p_option));
		} break;
		case TOOLBAR_BUTTON_USE_LOCAL_ROTATION: {
			_transform_gizmo_nd->set_use_local_rotation(_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION]->is_pressed());
		} break;
	}
}

void EditorMainScreenND::set_viewport_layout(const int8_t p_viewport_count, const Side p_dominant_side) {
	ERR_FAIL_COND(p_viewport_count > _MAX_VIEWPORTS);
	for (int i = 0; i < p_viewport_count; i++) {
		if (_editor_main_viewports[i] == nullptr) {
			_editor_main_viewports[i] = memnew(EditorMainViewportND);
			_editor_main_viewports[i]->set_name(StringName("EditorMainViewportND_" + itos(i)));
			_editor_main_viewports[i]->setup(this, _transform_gizmo_nd);
			_editor_main_viewports[i]->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
			_editor_main_viewport_holder->add_child(_editor_main_viewports[i]);
		}
	}
	for (int i = p_viewport_count; i < _MAX_VIEWPORTS; i++) {
		if (_editor_main_viewports[i] != nullptr) {
			// Viewports are really computationally expensive, so we should delete them when not in use.
			_editor_main_viewports[i]->queue_free();
			_editor_main_viewports[i] = nullptr;
		}
	}
}

void EditorMainScreenND::update_dimension() {
	Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
	const int dimension = _calculate_scene_dimension(scene_root);
	_dimensions_label->set_text(vformat(TTR("Scene Dimension: %d"), dimension));
	for (int i = 0; i < _MAX_VIEWPORTS; i++) {
		if (_editor_main_viewports[i]) {
			_editor_main_viewports[i]->set_dimension(dimension);
		}
	}
	_transform_gizmo_nd->set_gizmo_dimension(dimension);
}

void EditorMainScreenND::setup(EditorUndoRedoManager *p_undo_redo_manager) {
	// Things that we should do in the constructor but can't in GDExtension
	// due to how GDExtension runs the constructor for each registered class.
	set_name(StringName("EditorMainScreenND"));
	set_visible(false);
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	// Set up the toolbar and tool buttons.
	_toolbar_hbox = memnew(HBoxContainer);
	_toolbar_hbox->set_anchors_and_offsets_preset(Control::PRESET_TOP_WIDE);

	add_child(_toolbar_hbox);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT] = memnew(Button);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT]->set_toggle_mode(true);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT]->set_theme_type_variation("FlatButton");
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT]->set_pressed(true);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT]->set_tooltip_text(TTR("(Q) Select nodes and manipulate their position."));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT]->connect(StringName("pressed"), callable_mp(this, &EditorMainScreenND::press_menu_item).bind(TOOLBAR_BUTTON_MODE_SELECT));
	_toolbar_hbox->add_child(_toolbar_buttons[TOOLBAR_BUTTON_MODE_SELECT]);
	_toolbar_hbox->add_child(memnew(VSeparator));

	_toolbar_buttons[TOOLBAR_BUTTON_MODE_MOVE] = memnew(Button);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_MOVE]->set_toggle_mode(true);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_MOVE]->set_theme_type_variation("FlatButton");
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_MOVE]->set_tooltip_text(TTR("(W) Move selected node, changing its position."));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_MOVE]->connect(StringName("pressed"), callable_mp(this, &EditorMainScreenND::press_menu_item).bind(TOOLBAR_BUTTON_MODE_MOVE));
	_toolbar_hbox->add_child(_toolbar_buttons[TOOLBAR_BUTTON_MODE_MOVE]);

	_toolbar_buttons[TOOLBAR_BUTTON_MODE_ROTATE] = memnew(Button);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_ROTATE]->set_toggle_mode(true);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_ROTATE]->set_theme_type_variation("FlatButton");
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_ROTATE]->set_tooltip_text(TTR("(E) Rotate selected node."));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_ROTATE]->connect(StringName("pressed"), callable_mp(this, &EditorMainScreenND::press_menu_item).bind(TOOLBAR_BUTTON_MODE_ROTATE));
	_toolbar_hbox->add_child(_toolbar_buttons[TOOLBAR_BUTTON_MODE_ROTATE]);

	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE] = memnew(Button);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->set_toggle_mode(true);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->set_theme_type_variation("FlatButton");
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->set_tooltip_text(TTR("(R) Scale selected node."));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]->connect(StringName("pressed"), callable_mp(this, &EditorMainScreenND::press_menu_item).bind(TOOLBAR_BUTTON_MODE_SCALE));
	_toolbar_hbox->add_child(_toolbar_buttons[TOOLBAR_BUTTON_MODE_SCALE]);

	_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH] = memnew(Button);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->set_toggle_mode(true);
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->set_theme_type_variation("FlatButton");
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->set_tooltip_text(TTR("Stretch selected node."));
	_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]->connect(StringName("pressed"), callable_mp(this, &EditorMainScreenND::press_menu_item).bind(TOOLBAR_BUTTON_MODE_STRETCH));
	_toolbar_hbox->add_child(_toolbar_buttons[TOOLBAR_BUTTON_MODE_STRETCH]);
	_toolbar_hbox->add_child(memnew(VSeparator));

	_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION] = memnew(Button);
	_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION]->set_toggle_mode(true);
	_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION]->set_theme_type_variation("FlatButton");
	_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION]->set_tooltip_text(TTR("(T) If pressed, use the object's local rotation for the gizmo. Else, transform in global space."));
	_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION]->connect("toggled", callable_mp(this, &EditorMainScreenND::_on_button_toggled).bind(TOOLBAR_BUTTON_USE_LOCAL_ROTATION));
	_toolbar_hbox->add_child(_toolbar_buttons[TOOLBAR_BUTTON_USE_LOCAL_ROTATION]);

	// All viewports share one gizmo.
	_transform_gizmo_nd = memnew(EditorTransformGizmoND);
	add_child(_transform_gizmo_nd);

	// Set up the viewports.
	_editor_main_viewport_holder = memnew(Control);
	_editor_main_viewport_holder->set_name(StringName("EditorMainViewportHolder"));
	_editor_main_viewport_holder->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	_editor_main_viewport_holder->set_offset(Side::SIDE_TOP, 33.0f * EDSCALE);
	add_child(_editor_main_viewport_holder);
	set_viewport_layout(1);

	// Set up the Transform settings menu in the toolbar.
	_transform_settings_menu = memnew(MenuButton);
	_transform_settings_menu->set_flat(false);
	_transform_settings_menu->set_theme_type_variation("FlatMenuButton");
	_transform_settings_menu->set_text(TTR("Transform"));
	_transform_settings_menu->set_tooltip_text(TTR("Change the transform gizmo settings for the ND editor."));
	_toolbar_hbox->add_child(_transform_settings_menu);

	PopupMenu *transform_settings_menu_popup = _transform_settings_menu->get_popup();
	transform_settings_menu_popup->add_radio_check_item(TTR("Freeform (allow skew/shear)"), TRANSFORM_SETTING_KEEP_FREEFORM);
	transform_settings_menu_popup->add_radio_check_item(TTR("Keep Orthogonal (fix skew/shear)"), TRANSFORM_SETTING_KEEP_ORTHOGONAL);
	transform_settings_menu_popup->add_radio_check_item(TTR("Keep Conformal (uniform scale)"), TRANSFORM_SETTING_KEEP_CONFORMAL);
	transform_settings_menu_popup->add_radio_check_item(TTR("Keep Orthonormal (no scale)"), TRANSFORM_SETTING_KEEP_ORTHONORMAL);
	transform_settings_menu_popup->set_item_checked((int)EditorTransformGizmoND::KeepMode::FREEFORM, true);
	transform_settings_menu_popup->connect(StringName("id_pressed"), callable_mp(this, &EditorMainScreenND::_on_transform_settings_menu_id_pressed));

	Control *spacer = memnew(Control);
	spacer->set_h_size_flags(SIZE_EXPAND_FILL);
	_toolbar_hbox->add_child(spacer);

	_dimensions_label = memnew(Label);
	_dimensions_label->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	_dimensions_label->set_name(StringName("DimensionsLabel"));
	_dimensions_label->set_text(vformat(TTR("Scene Dimension: %d"), -1));
	_dimensions_label->set_tooltip_text(TTR("Dimension of the editor camera, determined by the highest dimension of all nodes in the scene.\nTo refresh, select any node in the scene tree hierarchy."));
	_toolbar_hbox->add_child(_dimensions_label);

	EditorInterface::get_singleton()->get_selection()->connect(StringName("selection_changed"), callable_mp(this, &EditorMainScreenND::_on_selection_changed));

	// Set up things with the arguments (not constructor things).
	_transform_gizmo_nd->setup(this, p_undo_redo_manager);
}
