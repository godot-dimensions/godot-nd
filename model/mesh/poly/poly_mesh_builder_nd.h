#pragma once

#include "array_poly_mesh_nd.h"

// Static helper class for ND poly mesh building functions.
class PolyMeshBuilderND : public Object {
	GDCLASS(PolyMeshBuilderND, Object);

protected:
	static PolyMeshBuilderND *singleton;
	static void _bind_methods();

public:
	// These functions create new meshes from the given data.
	static Ref<ArrayPolyMeshND> convert_mesh_3d_to_nd_faces_only(const Ref<ArrayMesh> &p_mesh_3d, const int p_which_surface = -1, const bool p_deduplicate = true);
	static Ref<ArrayPolyMeshND> extrude_linear(const Ref<ArrayPolyMeshND> &p_input_mesh, const VectorN &p_extrusion_vector = VectorN());

	// In-place adjustments to the given mesh.
	static void make_boundary_normals_topologically_consistent(const Ref<ArrayPolyMeshND> &p_mesh_nd, const PackedInt32Array &p_authoritative);

	static PolyMeshBuilderND *get_singleton() { return singleton; }
	PolyMeshBuilderND() { singleton = this; }
	~PolyMeshBuilderND() { singleton = nullptr; }
};
