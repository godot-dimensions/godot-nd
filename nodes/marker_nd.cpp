#include "marker_nd.h"

#include "../math/vector_nd.h"
#include "../model/wire/array_wire_mesh_nd.h"
#include "../model/wire/wire_material_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/engine.hpp>
#elif GODOT_MODULE
#include "core/config/engine.h"
#endif

void MarkerND::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (!Engine::get_singleton()->is_editor_hint())
#endif // TOOLS_ENABLED
			{
				switch (_runtime_behavior) {
					case RUNTIME_BEHAVIOR_SHOW:
						break;
					case RUNTIME_BEHAVIOR_HIDE:
						set_visible(false);
						return;
					case RUNTIME_BEHAVIOR_DELETE:
						queue_free();
						return;
				}
			}
			generate_marker_mesh();
		} break;
	}
	MeshInstanceND::_notification(p_what);
}

void MarkerND::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name != StringName("marker_extents") &&
			p_property.name != StringName("runtime_behavior") &&
			p_property.name != StringName("darken_negative_amount") &&
			p_property.name != StringName("subdivisions") &&
			p_property.name != StringName("dimension") &&
			p_property.name != StringName("editor_description")) {
		p_property.usage = PROPERTY_USAGE_NONE;
	}
}

PackedStringArray MarkerND::GDEXTMOD_GET_CONFIGURATION_WARNINGS() const {
	PackedStringArray warnings;
	if (_runtime_behavior == RUNTIME_BEHAVIOR_DELETE) {
		if (get_child_count() != 0) {
			warnings.append("MarkerND will be deleted at runtime, but it has children. This may cause unexpected behavior.");
		}
	}
	return warnings;
}

void MarkerND::set_marker_extents(const real_t p_marker_extents) {
	_marker_extents = p_marker_extents;
	generate_marker_mesh();
}

void MarkerND::set_darken_negative_amount(const float p_darken_negative_amount) {
	_darken_negative_amount = p_darken_negative_amount;
	generate_marker_mesh();
}

void MarkerND::set_subdivisions(const int p_subdivisions) {
	ERR_FAIL_COND(p_subdivisions < 1);
	ERR_FAIL_COND(p_subdivisions > 63);
	_subdivisions = p_subdivisions;
	generate_marker_mesh();
}

void MarkerND::generate_marker_mesh() {
	int dimension = get_dimension();
	// Splitting the line helps with precision issues when zooming.
	const int EDGES_PER_DIRECTION = _subdivisions;
	const int DIRECTIONS = dimension * 2; // Twice the number of axes.
	const int DIRECTION_INDICES = DIRECTIONS * 2;
	const int LAST_EDGES = EDGES_PER_DIRECTION - 1;
	const int LAST_VERTEX_INDEX = EDGES_PER_DIRECTION * DIRECTIONS;
	// Build the mesh data arrays for the marker.
	Vector<VectorN> vertices;
	vertices.resize(LAST_VERTEX_INDEX + 1);
	int vertex_index = 0;
	for (int edge = 0; edge < EDGES_PER_DIRECTION; edge++) {
		const real_t extent = _marker_extents / (1ull << uint64_t(edge));
		for (int axis = 0; axis < dimension; axis++) {
			VectorN vertex = VectorND::zero(dimension);
			vertex.set(axis, extent);
			vertices.set(vertex_index++, vertex);
			vertex.set(axis, -extent);
			vertices.set(vertex_index++, vertex);
		}
	}
	vertices.set(LAST_VERTEX_INDEX, VectorND::zero(dimension));
	PackedInt32Array edge_indices;
	edge_indices.resize(EDGES_PER_DIRECTION * DIRECTION_INDICES);
	int edge_index = 0;
	for (int edge = 0; edge < EDGES_PER_DIRECTION; edge++) {
		for (int dir = 0; dir < DIRECTIONS; dir++) {
			edge_indices.set(edge_index++, dir + DIRECTIONS * edge);
			if (edge == LAST_EDGES) {
				edge_indices.set(edge_index++, LAST_VERTEX_INDEX);
			} else {
				edge_indices.set(edge_index++, dir + DIRECTIONS * (edge + 1));
			}
		}
	}
	PackedColorArray albedo_colors;
	albedo_colors.resize(EDGES_PER_DIRECTION * DIRECTIONS);
	// MarkerND can be used at runtime, so it can't grab colors from the editor theme.
	PackedColorArray axis_colors;
	PackedColorArray neg_colors;
	for (int dim = 0; dim < dimension; dim++) {
		const Color axis_color = VectorND::axis_color(dim);
		axis_colors.push_back(axis_color);
		neg_colors.push_back(axis_color.darkened(_darken_negative_amount));
	}
	int color_index = 0;
	for (int edge = 0; edge < EDGES_PER_DIRECTION; edge++) {
		for (int dim = 0; dim < dimension; dim++) {
			albedo_colors.set(color_index++, axis_colors[dim]);
			albedo_colors.set(color_index++, neg_colors[dim]);
		}
	}
	// Set up the mesh and material with the generated data.
	Ref<WireMaterialND> material;
	material.instantiate();
	material->set_albedo_source(WireMaterialND::WIRE_COLOR_SOURCE_PER_EDGE_ONLY);
	material->set_albedo_color_array(albedo_colors);
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertices(vertices);
	mesh->set_edge_indices(edge_indices);
	mesh->set_material(material);
	set_mesh(mesh);
}

MarkerND::MarkerND() {
	connect("dimension_changed", callable_mp(this, &MarkerND::generate_marker_mesh));
}

void MarkerND::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_marker_extents"), &MarkerND::get_marker_extents);
	ClassDB::bind_method(D_METHOD("set_marker_extents", "extents"), &MarkerND::set_marker_extents);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "marker_extents"), "set_marker_extents", "get_marker_extents");

	ClassDB::bind_method(D_METHOD("get_darken_negative_amount"), &MarkerND::get_darken_negative_amount);
	ClassDB::bind_method(D_METHOD("set_darken_negative_amount", "darken_amount"), &MarkerND::set_darken_negative_amount);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "darken_negative_amount", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_darken_negative_amount", "get_darken_negative_amount");

	ClassDB::bind_method(D_METHOD("get_runtime_behavior"), &MarkerND::get_runtime_behavior);
	ClassDB::bind_method(D_METHOD("set_runtime_behavior", "behavior"), &MarkerND::set_runtime_behavior);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "runtime_behavior", PROPERTY_HINT_ENUM, "Show,Hide,Delete"), "set_runtime_behavior", "get_runtime_behavior");

	ClassDB::bind_method(D_METHOD("get_subdivisions"), &MarkerND::get_subdivisions);
	ClassDB::bind_method(D_METHOD("set_subdivisions", "subdivisions"), &MarkerND::set_subdivisions);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdivisions", PROPERTY_HINT_RANGE, "1,32,1"), "set_subdivisions", "get_subdivisions");

	BIND_ENUM_CONSTANT(RUNTIME_BEHAVIOR_SHOW);
	BIND_ENUM_CONSTANT(RUNTIME_BEHAVIOR_HIDE);
	BIND_ENUM_CONSTANT(RUNTIME_BEHAVIOR_DELETE);
}
