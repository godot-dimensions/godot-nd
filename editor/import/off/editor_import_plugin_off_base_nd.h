#pragma once

#include "../editor_import_plugin_base_nd.h"

class EditorImportPluginOFFBaseND : public EditorImportPluginBaseND {
	GDCLASS(EditorImportPluginOFFBaseND, EditorImportPluginBaseND);

protected:
	static void _bind_methods() {}

public:
	virtual int GDEXTMOD_GET_IMPORT_ORDER() const override { return 0; }
#if GDEXTENSION
	virtual PackedStringArray _get_recognized_extensions() const override {
		return PackedStringArray{ "off" };
	}
#elif GODOT_MODULE
	virtual void get_recognized_extensions(List<String> *p_extensions) const override {
		p_extensions->push_back("off");
	}
#endif
};
