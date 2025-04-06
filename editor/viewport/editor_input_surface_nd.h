#pragma once

#include "editor_viewport_nd_defines.h"

// Grabs input events and sends them to the main screen.
// This sits as a layer on top of most of the viewport, except the rotation gizmo.
class EditorInputSurfaceND : public Control {
	GDCLASS(EditorInputSurfaceND, Control);

	EditorMainScreenND *_editor_main_screen = nullptr;
	EditorMainViewportND *_editor_main_viewport = nullptr;

public:
	static void _bind_methods() {}
	virtual void GDEXTMOD_GUI_INPUT(const Ref<InputEvent> &p_event) override;

	void setup(EditorMainScreenND *p_editor_main_screen, EditorMainViewportND *p_editor_main_viewport);
};
