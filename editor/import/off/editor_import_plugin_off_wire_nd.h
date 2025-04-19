#pragma once

#include "editor_import_plugin_off_base_nd.h"

class EditorImportPluginOFFWireND : public EditorImportPluginOFFBaseND {
	GDCLASS(EditorImportPluginOFFWireND, EditorImportPluginOFFBaseND);

protected:
	static void _bind_methods() {}

public:
	virtual String GDEXTMOD_GET_IMPORTER_NAME() const override;
	virtual String GDEXTMOD_GET_RESOURCE_TYPE() const override;
	virtual String GDEXTMOD_GET_VISIBLE_NAME() const override;
	virtual float GDEXTMOD_GET_PRIORITY() const override { return 4.5f; }
#if GDEXTENSION
	virtual TypedArray<Dictionary> _get_import_options(const String &p_path, int32_t p_preset_index) const override;
	virtual Error _import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const override;
#elif GODOT_MODULE
	virtual void get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const override;
#if VERSION_HEX < 0x040400
	virtual Error import(const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata = nullptr) override;
#else
	virtual Error import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata = nullptr) override;
#endif
#endif
};
