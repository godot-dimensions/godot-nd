#include "editor_import_plugin_off_cell_nd.h"

#include "../../../model/off/off_document_nd.h"

String EditorImportPluginOFFCellND::GDEXTMOD_GET_IMPORTER_NAME() const {
	return "godot_nd.off_geometry_format.array_cell_mesh_nd";
}

String EditorImportPluginOFFCellND::GDEXTMOD_GET_RESOURCE_TYPE() const {
	return "ArrayCellMeshND";
}

String EditorImportPluginOFFCellND::GDEXTMOD_GET_VISIBLE_NAME() const {
	return "ArrayCellMeshND";
}

#if GDEXTENSION
TypedArray<Dictionary> EditorImportPluginOFFCellND::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	TypedArray<Dictionary> options;
	return options;
}

Error EditorImportPluginOFFCellND::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const {
	Ref<OFFDocumentND> off_doc = OFFDocumentND::load_from_file(p_source_file);
	ERR_FAIL_COND_V(off_doc.is_null(), ERR_FILE_CANT_OPEN);
	Ref<ArrayCellMeshND> cell_mesh = off_doc->generate_array_cell_mesh_nd();
	ERR_FAIL_COND_V(cell_mesh.is_null(), ERR_FILE_CORRUPT);
	cell_mesh->set_name(p_source_file.get_file());
	Error err = ResourceSaver::get_singleton()->save(cell_mesh, p_save_path + String(".res"));
	return err;
}
#elif GODOT_MODULE
void EditorImportPluginOFFCellND::get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const {}

#if VERSION_HEX < 0x040400
Error EditorImportPluginOFFCellND::import(const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata)
#else
Error EditorImportPluginOFFCellND::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata)
#endif
{
	Ref<OFFDocumentND> off_doc = OFFDocumentND::load_from_file(p_source_file);
	ERR_FAIL_COND_V(off_doc.is_null(), ERR_FILE_CANT_OPEN);
	Ref<ArrayCellMeshND> cell_mesh = off_doc->generate_array_cell_mesh_nd();
	ERR_FAIL_COND_V(cell_mesh.is_null(), ERR_FILE_CORRUPT);
	cell_mesh->set_name(p_source_file.get_file());
	Error err = ResourceSaver::save(cell_mesh, p_save_path + String(".res"));
	return err;
}
#endif
