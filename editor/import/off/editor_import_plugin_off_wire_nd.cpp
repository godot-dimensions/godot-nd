#include "editor_import_plugin_off_wire_nd.h"

#include "../../../model/off/off_document_nd.h"

String EditorImportPluginOFFWireND::GDEXTMOD_GET_IMPORTER_NAME() const {
	return "godot_nd.off_geometry_format.array_wire_mesh_nd";
}

String EditorImportPluginOFFWireND::GDEXTMOD_GET_RESOURCE_TYPE() const {
	return "ArrayWireMeshND";
}

String EditorImportPluginOFFWireND::GDEXTMOD_GET_VISIBLE_NAME() const {
	return "ArrayWireMeshND";
}

#if GDEXTENSION
TypedArray<Dictionary> EditorImportPluginOFFWireND::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	TypedArray<Dictionary> options;
	Dictionary force_dimension;
	force_dimension["name"] = "force_dimension";
	force_dimension["type"] = Variant::INT;
	force_dimension["default_value"] = -1;
	options.append(force_dimension);
	Dictionary deduplicate_edges;
	deduplicate_edges["name"] = "deduplicate_edges";
	deduplicate_edges["type"] = Variant::BOOL;
	deduplicate_edges["default_value"] = true;
	options.append(deduplicate_edges);
	return options;
}

Error EditorImportPluginOFFWireND::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const
#elif GODOT_MODULE
void EditorImportPluginOFFWireND::get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "force_dimension"), -1));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "deduplicate_edges"), true));
}

#if VERSION_HEX < 0x040400
Error EditorImportPluginOFFWireND::import(const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata)
#else
Error EditorImportPluginOFFWireND::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata)
#endif // VERSION_HEX
#endif // GDExtension or module.
{
	Ref<OFFDocumentND> off_doc = OFFDocumentND::import_load_from_file(p_source_file);
	ERR_FAIL_COND_V(off_doc.is_null(), ERR_FILE_CANT_OPEN);
	const int64_t force_dimension = p_options[StringName("force_dimension")];
	if (force_dimension >= 0) {
		off_doc->set_dimension(force_dimension);
	}
	Ref<ArrayWireMeshND> wire_mesh = off_doc->import_generate_wire_mesh_nd(p_options[StringName("deduplicate_edges")]);
	ERR_FAIL_COND_V(wire_mesh.is_null(), ERR_FILE_CORRUPT);
	wire_mesh->set_name(p_source_file.get_file());
	const String save_path_with_ext = p_save_path + String(".res");
#if GDEXTENSION
	Error err = ResourceSaver::get_singleton()->save(wire_mesh, save_path_with_ext);
#elif GODOT_MODULE
	Error err = ResourceSaver::save(wire_mesh, save_path_with_ext);
#endif
	return err;
}
