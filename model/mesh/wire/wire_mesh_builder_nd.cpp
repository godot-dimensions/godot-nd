#include "wire_mesh_builder_nd.h"

#include "../../../math/vector_nd.h"

Ref<ArrayWireMeshND> WireMeshBuilderND::extrude_linear(const Ref<ArrayWireMeshND> &p_input_mesh, const VectorN &p_extrusion_vector) {
	Ref<ArrayWireMeshND> ret;
	ret.instantiate();
	ERR_FAIL_COND_V_MSG(p_input_mesh.is_null() || !p_input_mesh->is_mesh_data_valid(), ret, "Input mesh is not valid, so extrusion cannot be performed.");
	// Extract and copy a bunch of data from the input mesh.
	// Start by copying the input mesh's data into the output mesh twice,
	// offset by the extrusion vector in both negative and positive directions.
	ret = p_input_mesh->duplicate();
	Ref<TransformND> offset_transform;
	offset_transform.instantiate();
	offset_transform->set_origin(VectorND::negate(p_extrusion_vector));
	ret->transform_vertices(offset_transform);
	offset_transform->set_origin(p_extrusion_vector);
	ret->merge_with(p_input_mesh, offset_transform);
	// Form new edges between the vertices of the two copies of the input mesh.
	{
		const Vector<VectorN> &input_vertices = p_input_mesh->get_vertex_positions();
		PackedInt32Array edge_indices = ret->get_edge_indices();
		int64_t edge_indices_iter = edge_indices.size();
		const int32_t input_vertex_count = (int32_t)input_vertices.size();
		edge_indices.resize(edge_indices.size() + input_vertex_count * 2);
		for (int input_vertex_index = 0; input_vertex_index < input_vertex_count; input_vertex_index++) {
			edge_indices.set(edge_indices_iter, input_vertex_index);
			edge_indices.set(edge_indices_iter + 1, input_vertex_index + input_vertex_count);
			edge_indices_iter += 2;
		}
		ret->set_edge_indices(edge_indices);
	}
	return ret;
}

WireMeshBuilderND *WireMeshBuilderND::singleton = nullptr;

void WireMeshBuilderND::_bind_methods() {
	ClassDB::bind_static_method("WireMeshBuilderND", D_METHOD("extrude_linear", "input_mesh", "extrusion_vector"), &WireMeshBuilderND::extrude_linear);
}
