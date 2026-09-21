#include "editor_import_plugin_off_poly_nd.h"

#include "../../../model/mesh/poly/array_poly_mesh_nd.h"
#include "../../../model/off/off_document_nd.h"

String EditorImportPluginOFFPolyND::GDEXTMOD_GET_IMPORTER_NAME() const {
	return "godot_nd.off_geometry_format.array_poly_mesh_nd";
}

String EditorImportPluginOFFPolyND::GDEXTMOD_GET_RESOURCE_TYPE() const {
	return "ArrayPolyMeshND";
}

String EditorImportPluginOFFPolyND::GDEXTMOD_GET_VISIBLE_NAME() const {
	return "ArrayPolyMeshND";
}

void EditorImportPluginOFFPolyND::_make_single_convex_volume(Ref<ArrayPolyMeshND> p_mesh) {
	const int dimension = p_mesh->get_dimension();
	Vector<Vector<PackedInt32Array>> poly_cell_indices = p_mesh->get_poly_cell_indices();
	ERR_FAIL_COND_MSG(dimension < 3 || poly_cell_indices.size() < dimension - 2, "EditorImportPluginOFFPolyND: Cannot make a single convex volume because the mesh has no boundary cells.");
	// Replace any existing hypervolume cells with one cell made of all boundary cells.
	poly_cell_indices.resize(dimension - 1);
	poly_cell_indices.set(dimension - 2, Vector<PackedInt32Array>{ p_mesh->make_single_cell_from_all_cells(dimension) });
	p_mesh->set_poly_cell_indices(poly_cell_indices);
	// Assign boundary normals pointing outward from the center of the mesh, fixing the cell orientations to match.
	p_mesh->calculate_boundary_normals(ArrayPolyMeshND::COMPUTE_NORMALS_MODE_FORCE_OUTWARD_FIX_CELL_ORIENTATION);
}

#if GDEXTENSION
TypedArray<Dictionary> EditorImportPluginOFFPolyND::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	TypedArray<Dictionary> options;
	Dictionary force_dimension;
	force_dimension["name"] = "force_dimension";
	force_dimension["type"] = Variant::INT;
	force_dimension["default_value"] = -1;
	options.append(force_dimension);
	Dictionary compute_normals_mode;
	compute_normals_mode["name"] = "compute_normals_mode";
	compute_normals_mode["type"] = Variant::INT;
	compute_normals_mode["default_value"] = ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY;
	compute_normals_mode["property_hint"] = PROPERTY_HINT_ENUM;
	compute_normals_mode["hint_string"] = "Cell Orientation Only,Force Outward and Fix Cell Orientation,Force Outward and Override Cell Orientation";
	options.append(compute_normals_mode);
	Dictionary double_sided;
	double_sided["name"] = "double_sided";
	double_sided["type"] = Variant::BOOL;
	double_sided["default_value"] = false;
	options.append(double_sided);
	Dictionary single_convex_volume;
	single_convex_volume["name"] = "single_convex_volume";
	single_convex_volume["type"] = Variant::BOOL;
	single_convex_volume["default_value"] = false;
	options.append(single_convex_volume);
	return options;
}

Error EditorImportPluginOFFPolyND::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const
#elif GODOT_MODULE
void EditorImportPluginOFFPolyND::get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "force_dimension"), -1));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "compute_normals_mode", PROPERTY_HINT_ENUM, "Cell Orientation Only,Force Outward and Fix Cell Orientation,Force Outward and Override Cell Orientation"), ArrayPolyMeshND::COMPUTE_NORMALS_MODE_CELL_ORIENTATION_ONLY));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "double_sided"), false));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "single_convex_volume"), false));
}

#if VERSION_HEX < 0x040400
Error EditorImportPluginOFFPolyND::import(const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata)
#else
Error EditorImportPluginOFFPolyND::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata)
#endif // VERSION_HEX
#endif // GDExtension or module.
{
	Ref<OFFDocumentND> off_doc = OFFDocumentND::import_load_from_file(p_source_file);
	ERR_FAIL_COND_V(off_doc.is_null(), ERR_FILE_CANT_OPEN);
	const int64_t force_dimension = p_options[StringName("force_dimension")];
	if (force_dimension >= 0) {
		off_doc->set_dimension(force_dimension);
	}
	Ref<ArrayPolyMeshND> poly_mesh = off_doc->import_generate_array_poly_mesh_nd();
	ERR_FAIL_COND_V(poly_mesh.is_null(), ERR_FILE_CORRUPT);
	poly_mesh->set_name(p_source_file.get_file());
	const int dimension = poly_mesh->get_dimension();
	const bool has_boundary_cells = dimension >= 3 && poly_mesh->get_poly_cell_indices().size() >= dimension - 2;
	if (has_boundary_cells) {
		poly_mesh->calculate_boundary_normals(ArrayPolyMeshND::ComputeNormalsMode(int(p_options["compute_normals_mode"])), false);
		if (bool(p_options["double_sided"])) {
			poly_mesh->make_double_sided(false);
		}
		if (bool(p_options["single_convex_volume"])) {
			_make_single_convex_volume(poly_mesh);
		}
	} else {
		if (bool(p_options["double_sided"])) {
			WARN_PRINT("EditorImportPluginOFFPolyND: Ignoring double_sided option because the imported OFF file does not contain any boundary cells for its dimension.");
		}
		if (bool(p_options["single_convex_volume"])) {
			WARN_PRINT("EditorImportPluginOFFPolyND: Ignoring single_convex_volume option because the imported OFF file does not contain any boundary cells for its dimension.");
		}
	}
	const String save_path_with_ext = p_save_path + String(".res");
#if GDEXTENSION
	Error err = ResourceSaver::get_singleton()->save(poly_mesh, save_path_with_ext);
#elif GODOT_MODULE
	Error err = ResourceSaver::save(poly_mesh, save_path_with_ext);
#endif
	return err;
}
