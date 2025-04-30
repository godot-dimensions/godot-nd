#include "editor_transform_gizmo_nd.h"

#include "editor_main_screen_nd.h"

#include "../../math/geometry_nd.h"
#include "../../math/plane_nd.h"
#include "../../math/vector_nd.h"
#include "../../model/wire/array_wire_mesh_nd.h"
#include "../../model/wire/box_wire_mesh_nd.h"
#include "../../model/wire/wire_material_nd.h"
#include "../../render/rendering_server_nd.h"

#if GDEXTENSION
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#elif GODOT_MODULE
#include "editor/editor_inspector.h"
#include "editor/editor_interface.h"
#endif

constexpr double LINE_THICKNESS_ND = 3.0;
constexpr int MOVE_ARROW_SUBDIVISIONS_ND = 4;
constexpr int ROTATION_RING_SEGMENTS_ND = 64; // Should be a multiple of 8 for correct coloration.
constexpr int SCALE_BOX_SUBDIVISIONS_ND = 3;
constexpr int PLANE_EDGES_ND = 6; // Must match the data in _make_plane_wire_mesh_nd.
constexpr double PLANE_OFFSET_ND = 0.3;
constexpr double PLANE_RADIUS_ND = 0.08;

static_assert(MOVE_ARROW_SUBDIVISIONS_ND > 1);
static_assert(ROTATION_RING_SEGMENTS_ND % 8 == 0);
static_assert(SCALE_BOX_SUBDIVISIONS_ND > 1);

Ref<WireMaterialND> _make_single_color_wire_material_nd(const Color &p_color) {
	Ref<WireMaterialND> mat;
	mat.instantiate();
	mat->set_line_thickness(LINE_THICKNESS_ND);
	mat->set_albedo_source(WireMaterialND::WIRE_COLOR_SOURCE_SINGLE_COLOR);
	mat->set_albedo_color(p_color);
	return mat;
}

Ref<WireMaterialND> _make_plane_material_nd(const Color &p_first_color, const Color &p_second_color) {
	Ref<WireMaterialND> mat;
	mat.instantiate();
	mat->set_line_thickness(LINE_THICKNESS_ND);
	mat->set_albedo_source(WireMaterialND::WIRE_COLOR_SOURCE_PER_EDGE_ONLY);
	PackedColorArray colors;
	for (int i = 0; i < PLANE_EDGES_ND; i++) {
		if (i < PLANE_EDGES_ND / 2) {
			colors.push_back(p_first_color);
		} else {
			colors.push_back(p_second_color);
		}
	}
	mat->set_albedo_color_array(colors);
	return mat;
}

Ref<WireMaterialND> _make_rotation_ring_material_nd(const Color &p_first_color, const Color &p_second_color) {
	Ref<WireMaterialND> mat;
	mat.instantiate();
	mat->set_albedo_source(WireMaterialND::WIRE_COLOR_SOURCE_PER_EDGE_ONLY);
	PackedColorArray colors;
	for (int i = 0; i < ROTATION_RING_SEGMENTS_ND; i++) {
		if (i < ROTATION_RING_SEGMENTS_ND / 8) {
			colors.push_back(p_first_color);
		} else if (i < ROTATION_RING_SEGMENTS_ND * 3 / 8) {
			colors.push_back(p_second_color);
		} else if (i < ROTATION_RING_SEGMENTS_ND * 5 / 8) {
			colors.push_back(p_first_color);
		} else if (i < ROTATION_RING_SEGMENTS_ND * 7 / 8) {
			colors.push_back(p_second_color);
		} else {
			colors.push_back(p_first_color);
		}
	}
	mat->set_albedo_color_array(colors);
	return mat;
}

Ref<ArrayWireMeshND> _make_move_arrow_wire_mesh_nd() {
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	Vector<VectorN> vertices = { VectorN{ 0.0 }, VectorN{ 1.0 } };
	PackedInt32Array edge_indices = { 0, 1 };
	mesh->set_vertices(vertices.duplicate());
	mesh->set_edge_indices(edge_indices);
	return mesh;
}

Ref<ArrayWireMeshND> _make_rotation_ring_wire_mesh_nd() {
	Vector<VectorN> vertices;
	PackedInt32Array edge_indices;
	vertices.resize(ROTATION_RING_SEGMENTS_ND);
	edge_indices.resize(ROTATION_RING_SEGMENTS_ND * 2);
	vertices.set(0, VectorN{ 1.0, 0.0 });
	edge_indices.set(0, 0);
	for (int i = 1; i < ROTATION_RING_SEGMENTS_ND; i++) {
		const double angle = Math_TAU * i / ROTATION_RING_SEGMENTS_ND;
		const double x = Math::cos(angle);
		const double y = Math::sin(angle);
		vertices.set(i, VectorN{ x, y });
		edge_indices.set(i * 2 - 1, i);
		edge_indices.set(i * 2, i);
	}
	edge_indices.set(ROTATION_RING_SEGMENTS_ND * 2 - 1, 0);
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertices(vertices);
	mesh->set_edge_indices(edge_indices);
	return mesh;
}

Ref<ArrayWireMeshND> _make_plane_wire_mesh_nd() {
	// Must match `constexpr int PLANE_SEGMENTS_ND`.
	Vector<VectorN> vertices = {
		VectorN{ -PLANE_RADIUS_ND * 0.9, -PLANE_RADIUS_ND, 0.0, 0.0 }, // First triangle lower left.
		VectorN{ PLANE_RADIUS_ND, -PLANE_RADIUS_ND, 0.0, 0.0 }, // First triangle lower right.
		VectorN{ PLANE_RADIUS_ND, PLANE_RADIUS_ND * 0.9, 0.0, 0.0 }, // First triangle upper right.
		VectorN{ -PLANE_RADIUS_ND, -PLANE_RADIUS_ND * 0.9, 0.0, 0.0 }, // Second triangle lower left.
		VectorN{ PLANE_RADIUS_ND * 0.9, PLANE_RADIUS_ND, 0.0, 0.0 }, // Second triangle upper right.
		VectorN{ -PLANE_RADIUS_ND, PLANE_RADIUS_ND, 0.0, 0.0 }, // Second triangle upper left.
	};
	PackedInt32Array edge_indices = { 0, 1, 0, 2, 1, 2, 3, 4, 3, 5, 4, 5 };
	Ref<ArrayWireMeshND> mesh;
	mesh.instantiate();
	mesh->set_vertices(vertices);
	mesh->set_edge_indices(edge_indices);
	return mesh;
}

MeshInstanceND *EditorTransformGizmoND::_make_mesh_instance(const StringName &p_name, const Ref<ArrayWireMeshND> &p_mesh, const Ref<WireMaterialND> &p_material) {
	MeshInstanceND *mesh_instance = memnew(MeshInstanceND);
	mesh_instance->set_name(p_name);
	mesh_instance->set_mesh(p_mesh);
	mesh_instance->set_material_override(p_material);
	mesh_instance->set_meta(StringName("original_material"), p_material);
	_mesh_holder->add_child(mesh_instance);
	return mesh_instance;
}

Ref<TransformND> _realign_xy_to_axes(const int p_x, const int p_y) {
	if (p_x == 0) {
		if (p_y == 1) {
			return TransformND::identity_basis(2);
		} else {
			return TransformND::from_swap_rotation(1, p_y);
		}
	} else if (p_y == 1) {
		return TransformND::from_swap_rotation(0, p_x);
	}
	return TransformND::from_swap_rotation(0, p_x)->compose_square(TransformND::from_swap_rotation(1, p_y));
}

int _triangular_number(const int p_n) {
	return p_n * (p_n + 1) / 2;
}

int _plane_index_in_triangular_number(const int p_i, const int p_j, const int p_dimension) {
	return p_i * (2 * p_dimension - p_i - 1) / 2 + (p_j - p_i - 1);
}

void EditorTransformGizmoND::_generate_gizmo_meshes() {
	const int dimension = get_dimension();
	Vector<Ref<WireMaterialND>> axis_materials;
	axis_materials.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		Color axis_color = _axis_colors[i % _axis_colors.size()];
		axis_materials.set(i, _make_single_color_wire_material_nd(axis_color));
	}
	// Create the move arrow meshes.
	Ref<ArrayWireMeshND> move_arrow_mesh = _make_move_arrow_wire_mesh_nd();
	Vector<MeshInstanceND *> &move_arrow_meshes = _meshes[TRANSFORM_MOVE_AXIS];
	move_arrow_meshes.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const StringName name = StringName("MoveArrow" + String::num_int64(i));
		MeshInstanceND *mesh_instance = _make_mesh_instance(name, move_arrow_mesh, axis_materials[i]);
		if (i != 0) {
			mesh_instance->set_transform(TransformND::from_swap_rotation(0, i));
		}
		move_arrow_meshes.set(i, mesh_instance);
	}
	// Create the plane meshes.
	Ref<ArrayWireMeshND> plane_mesh = _make_plane_wire_mesh_nd();
	Vector<MeshInstanceND *> &plane_meshes = _meshes[TRANSFORM_MOVE_PLANE];
	plane_meshes.resize(_triangular_number(dimension - 1));
	for (int i = 0; i < dimension; i++) {
		for (int j = i + 1; j < dimension; j++) {
			const StringName name = StringName("Plane" + String::num_int64(i) + "_" + String::num_int64(j));
			Ref<WireMaterialND> plane_material = _make_plane_material_nd(axis_materials[i]->get_albedo_color(), axis_materials[j]->get_albedo_color());
			MeshInstanceND *mesh_instance = _make_mesh_instance(name, plane_mesh, plane_material);
			Ref<TransformND> mesh_transform = _realign_xy_to_axes(i, j);
			VectorN mesh_pos = VectorND::value_on_axis_with_dimension(PLANE_OFFSET_ND, i, dimension);
			mesh_pos.set(j, PLANE_OFFSET_ND);
			mesh_transform->set_origin(mesh_pos);
			mesh_instance->set_transform(mesh_transform);
			plane_meshes.set(_plane_index_in_triangular_number(i, j, dimension), mesh_instance);
		}
	}
	// Create the rotation ring meshes.
	Ref<ArrayWireMeshND> rotation_ring_mesh = _make_rotation_ring_wire_mesh_nd();
	Vector<MeshInstanceND *> &rotation_ring_meshes = _meshes[TRANSFORM_ROTATE];
	rotation_ring_meshes.resize(_triangular_number(dimension - 1));
	for (int i = 0; i < dimension; i++) {
		for (int j = i + 1; j < dimension; j++) {
			const StringName name = StringName("RotationRing" + String::num_int64(i) + "_" + String::num_int64(j));
			Ref<WireMaterialND> rotation_ring_material = _make_rotation_ring_material_nd(axis_materials[i]->get_albedo_color(), axis_materials[j]->get_albedo_color());
			MeshInstanceND *mesh_instance = _make_mesh_instance(name, rotation_ring_mesh, rotation_ring_material);
			Ref<TransformND> mesh_rot = _realign_xy_to_axes(i, j);
			mesh_instance->set_transform(mesh_rot);
			rotation_ring_meshes.set(_plane_index_in_triangular_number(i, j, dimension), mesh_instance);
		}
	}
	// The scale meshes are the same as the move meshes.
	_meshes[TRANSFORM_SCALE_AXIS] = _meshes[TRANSFORM_MOVE_AXIS];
	_meshes[TRANSFORM_SCALE_PLANE] = _meshes[TRANSFORM_MOVE_PLANE];
	_meshes[TRANSFORM_STRETCH_POS] = _meshes[TRANSFORM_MOVE_AXIS];
	// Create the stretch negative meshes.
	Vector<MeshInstanceND *> &stretch_neg_meshes = _meshes[TRANSFORM_STRETCH_NEG];
	stretch_neg_meshes.resize(dimension);
	for (int i = 0; i < dimension; i++) {
		const StringName name = StringName("StretchNeg" + String::num_int64(i));
		Ref<WireMaterialND> neg_mat = axis_materials[i]->duplicate();
		neg_mat->set_albedo_color(neg_mat->get_albedo_color().lerp(Color(0.0, 0.0, 0.0), 0.5));
		MeshInstanceND *mesh_instance = _make_mesh_instance(name, move_arrow_mesh, neg_mat);
		if (i == 0) {
			mesh_instance->set_transform(TransformND::from_scale(VectorN{ -1.0 }));
		} else {
			mesh_instance->set_transform(TransformND::from_swap_rotation(i, 0));
		}
		stretch_neg_meshes.set(i, mesh_instance);
	}
	// Set the gizmo mode to set the visibility of these new meshes.
	set_gizmo_mode(_last_gizmo_mode);
}

void EditorTransformGizmoND::_regenerate_gizmo_meshes() {
	// Delete old meshes.
	for (int i = 0; i < TRANSFORM_MAX; i++) {
		Vector<MeshInstanceND *> &mesh_instances = _meshes[i];
		for (int j = 0; j < mesh_instances.size(); j++) {
			if (mesh_instances[j]->get_parent() != nullptr) {
				_mesh_holder->remove_child(mesh_instances[j]);
				mesh_instances[j]->queue_free();
			}
		}
		mesh_instances.clear();
	}
	_generate_gizmo_meshes();
}

void EditorTransformGizmoND::_on_rendering_server_pre_render(CameraND *p_camera, Viewport *p_viewport, RenderingEngineND *p_rendering_engine) {
	_update_gizmo_mesh_transform(p_camera);
}

void EditorTransformGizmoND::_on_editor_inspector_property_edited(const String &p_prop) {
	_update_gizmo_transform();
}

void EditorTransformGizmoND::_on_undo_redo_version_changed() {
	_update_gizmo_transform();
}

void EditorTransformGizmoND::_update_gizmo_transform() {
	int transform_count = 0;
	Ref<TransformND> sum_transform;
	sum_transform.instantiate();
	const int size = _selected_top_nodes.size();
	for (int i = 0; i < size; i++) {
		NodeND *node_nd = Object::cast_to<NodeND>(_selected_top_nodes[i]);
		if (node_nd != nullptr) {
			Ref<TransformND> global_xform = node_nd->get_global_transform();
			_selected_top_node_old_transforms.set(i, global_xform);
			sum_transform = sum_transform->add(global_xform);
			transform_count++;
		}
	}
	if (transform_count == 0) {
		set_visible(false);
	} else {
		set_visible(true);
		const int old_dimension = get_dimension();
		set_transform(sum_transform->divide_scalar(double(transform_count)));
		if (old_dimension != get_dimension()) {
			// The gizmo has changed dimensions, so we need to regenerate the meshes.
			_regenerate_gizmo_meshes();
			_editor_main_screen->update_dimension();
		}
	}
}

void EditorTransformGizmoND::_update_gizmo_mesh_transform(const CameraND *p_camera) {
	// We want to keep the gizmo the same size on the screen regardless of the camera's position.
	const Ref<TransformND> camera_transform = p_camera->get_global_transform();
	Ref<TransformND> global_transform;
	global_transform.instantiate();
	if (_is_use_local_rotation) {
		global_transform = get_transform()->normalized();
	} else {
		global_transform = TransformND::identity_basis(get_dimension());
		global_transform->set_origin(get_transform()->get_origin());
	}
	if (_is_stretch_enabled) {
		const Ref<RectND> bounds = _get_rect_bounds_of_selection(global_transform->inverse());
		global_transform->translate_local(bounds->get_center());
		const VectorN bounds_size = bounds->get_size();
		const bool zero_size = VectorND::is_zero_approx(bounds_size);
		if (!zero_size) {
			const VectorN scale = VectorND::multiply_scalar(bounds_size, 0.5);
			global_transform->scale_local(VectorND::with_dimension(scale, global_transform->get_basis_column_count()));
		}
		_mesh_holder->set_visible(!zero_size);
	} else {
		double scale;
		if (p_camera->get_projection_type() == CameraND::PROJECTION_ORTHOGRAPHIC) {
			scale = p_camera->get_orthographic_size() * 0.4f;
		} else {
			scale = VectorND::distance_to(global_transform->get_origin(), camera_transform->get_origin()) * 0.4f;
		}
		global_transform->scale_uniform(scale);
		_mesh_holder->set_visible(true);
	}
	_mesh_holder->set_global_transform(global_transform);
}

Ref<RectND> EditorTransformGizmoND::_get_rect_bounds_of_selection(const Ref<TransformND> &p_inv_relative_to) const {
	Ref<RectND> bounds;
	bounds.instantiate();
	const int size = _selected_top_nodes.size();
	for (int i = 0; i < size; i++) {
		NodeND *node_nd = Object::cast_to<NodeND>(_selected_top_nodes[i]);
		if (node_nd != nullptr) {
			bounds = bounds->merge(node_nd->get_rect_bounds_recursive(p_inv_relative_to));
		}
	}
	return bounds;
}

VectorN _origin_axis_aligned_biplane_raycast(const VectorN &p_ray_origin, const VectorN &p_ray_direction, const VectorN &p_axis1, const VectorN &p_axis2) {
	VectorN plane_normal = VectorND::slide(p_ray_direction, p_axis1);
	plane_normal = VectorND::slide(plane_normal, p_axis2);
	Ref<PlaneND> plane = PlaneND::from_normal_distance(VectorND::normalized(plane_normal), 0.0);
	Variant intersection = plane->intersect_ray(p_ray_origin, p_ray_direction);
	if (intersection.get_type() == Variant::PACKED_FLOAT64_ARRAY) {
		return intersection;
	}
	return VectorN{};
}

EditorTransformGizmoND::TransformPart EditorTransformGizmoND::_check_for_best_hit(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction, int &r_primary_axis, int &r_secondary_axis) const {
	const int dimension = get_dimension();
	double closest_distance = 0.1;
	double current_distance;
	TransformPart closest_part = TRANSFORM_NONE;
	// Check arrows.
	if (_is_move_linear_enabled || _is_scale_linear_enabled) {
		Vector<VectorN> current_points;
		for (int i = 0; i < dimension; i++) {
			current_points = GeometryND::closest_points_between_line_and_segment(p_local_ray_origin, p_local_ray_direction, VectorN{ 0.0 }, VectorND::value_on_axis(1.0, i));
			current_distance = VectorND::distance_to(current_points[0], current_points[1]);
			if (current_distance < closest_distance) {
				closest_distance = current_distance;
				closest_part = _is_move_linear_enabled ? TRANSFORM_MOVE_AXIS : TRANSFORM_SCALE_AXIS;
				r_primary_axis = i;
			}
		}
	}
	VectorN current_point;
	// Check planes.
	if (_is_move_planar_enabled || _is_scale_planar_enabled) {
		for (int i = 0; i < dimension - 1; i++) {
			for (int j = i + 1; j < dimension; j++) {
				VectorN point_to_check = VectorND::value_on_axis_with_dimension(PLANE_OFFSET_ND, i, dimension);
				point_to_check.set(j, PLANE_OFFSET_ND);
				current_point = GeometryND::closest_point_on_line(p_local_ray_origin, p_local_ray_direction, point_to_check);
				current_distance = VectorND::distance_to(current_point, point_to_check);
				if (current_distance < closest_distance) {
					closest_distance = current_distance;
					closest_part = _is_move_planar_enabled ? TRANSFORM_MOVE_PLANE : TRANSFORM_SCALE_PLANE;
					r_primary_axis = i;
					r_secondary_axis = j;
				}
			}
		}
	}
	// Check stretch.
	if (_is_stretch_enabled) {
		for (int i = 0; i < dimension; i++) {
			current_point = GeometryND::closest_point_on_line(p_local_ray_origin, p_local_ray_direction, VectorND::value_on_axis(1.0, i));
			current_distance = VectorND::distance_to(current_point, VectorND::value_on_axis(1.0, i));
			if (current_distance < closest_distance) {
				closest_distance = current_distance;
				closest_part = TRANSFORM_STRETCH_POS;
				r_primary_axis = i;
			}
			current_point = GeometryND::closest_point_on_line(p_local_ray_origin, p_local_ray_direction, VectorND::value_on_axis(-1.0, i));
			current_distance = VectorND::distance_to(current_point, VectorND::value_on_axis(-1.0, i));
			if (current_distance < closest_distance) {
				closest_distance = current_distance;
				closest_part = TRANSFORM_STRETCH_NEG;
				r_primary_axis = i;
			}
		}
	}
	// Check rotation rings.
	if (_is_rotation_enabled) {
		for (int i = 0; i < dimension - 1; i++) {
			for (int j = i + 1; j < dimension; j++) {
				current_point = _origin_axis_aligned_biplane_raycast(p_local_ray_origin, p_local_ray_direction, VectorND::value_on_axis(1.0, i), VectorND::value_on_axis(1.0, j));
				current_distance = Math::abs(VectorND::length(current_point) - 1.0);
				if (current_distance < closest_distance) {
					closest_distance = current_distance;
					closest_part = TRANSFORM_ROTATE;
					r_primary_axis = i;
					r_secondary_axis = j;
				}
			}
		}
	}
	return closest_part;
}

MeshInstanceND *EditorTransformGizmoND::_get_highlightable_gizmo_mesh() const {
	const Vector<MeshInstanceND *> &mesh_instances = _meshes[_highlighted_transformation];
	if (
			_highlighted_transformation == TRANSFORM_MOVE_PLANE ||
			_highlighted_transformation == TRANSFORM_ROTATE ||
			_highlighted_transformation == TRANSFORM_SCALE_PLANE) {
		return mesh_instances[_plane_index_in_triangular_number(_primary_axis, _secondary_axis, get_dimension())];
	}
	return mesh_instances[_primary_axis];
}

void EditorTransformGizmoND::_unhighlight_mesh() {
	if (_highlighted_transformation == TRANSFORM_NONE) {
		return;
	}
	MeshInstanceND *mesh_instance = _get_highlightable_gizmo_mesh();
	Ref<WireMaterialND> material = mesh_instance->get_meta(StringName("original_material"));
	mesh_instance->set_material_override(material);
}

void EditorTransformGizmoND::_highlight_mesh() {
	if (_highlighted_transformation == TRANSFORM_NONE) {
		return;
	}
	MeshInstanceND *mesh_instance = _get_highlightable_gizmo_mesh();
	Ref<WireMaterialND> material = mesh_instance->get_meta(StringName("original_material"));
	Ref<WireMaterialND> highlight_material = material->duplicate(true);
	WireMaterialND::WireColorSourceND source = highlight_material->get_albedo_source();
	if (source == WireMaterialND::WIRE_COLOR_SOURCE_SINGLE_COLOR) {
		highlight_material->set_albedo_color(highlight_material->get_albedo_color().lightened(0.5));
	} else if (source == WireMaterialND::WIRE_COLOR_SOURCE_PER_EDGE_ONLY) {
		PackedColorArray colors = highlight_material->get_albedo_color_array();
		for (int i = 0; i < colors.size(); i++) {
			colors.set(i, colors[i].lightened(0.5));
		}
		highlight_material->set_albedo_color_array(colors);
	}
	mesh_instance->set_material_override(highlight_material);
}

Variant EditorTransformGizmoND::_get_transform_raycast_value(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction) {
	switch (_current_transformation) {
		case TRANSFORM_NONE: {
			return Variant();
		} break;
		case TRANSFORM_MOVE_AXIS:
		case TRANSFORM_SCALE_AXIS:
		case TRANSFORM_STRETCH_POS:
		case TRANSFORM_STRETCH_NEG: {
			const VectorN endpoint = VectorND::value_on_axis(1.0, _primary_axis);
			return GeometryND::closest_point_between_lines(VectorN{}, endpoint, p_local_ray_origin, p_local_ray_direction);
		} break;
		case TRANSFORM_MOVE_PLANE:
		case TRANSFORM_SCALE_PLANE: {
			const VectorN primary_axis = VectorND::value_on_axis(1.0, _primary_axis);
			const VectorN secondary_axis = VectorND::value_on_axis(1.0, _secondary_axis);
			return _origin_axis_aligned_biplane_raycast(p_local_ray_origin, p_local_ray_direction, primary_axis, secondary_axis);
		} break;
		case TRANSFORM_ROTATE: {
			const VectorN primary_axis = VectorND::value_on_axis(1.0, _primary_axis);
			const VectorN secondary_axis = VectorND::value_on_axis(1.0, _secondary_axis);
			VectorN casted = _origin_axis_aligned_biplane_raycast(p_local_ray_origin, p_local_ray_direction, primary_axis, secondary_axis);
			return Math::atan2(casted[_secondary_axis], casted[_primary_axis]);
		} break;
		case TRANSFORM_MAX: {
			return Variant();
		} break;
	}
	return Variant();
}

void EditorTransformGizmoND::_begin_transformation(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction) {
	_old_transform = _mesh_holder->get_global_transform();
	_transform_reference_value = _get_transform_raycast_value(p_local_ray_origin, p_local_ray_direction);
	_selected_top_node_old_transforms.resize(_selected_top_nodes.size());
	for (int i = 0; i < _selected_top_nodes.size(); i++) {
		NodeND *node_nd = Object::cast_to<NodeND>(_selected_top_nodes[i]);
		if (node_nd != nullptr) {
			_selected_top_node_old_transforms.set(i, node_nd->get_global_transform());
		}
	}
}

void EditorTransformGizmoND::_end_transformation() {
	if (_current_transformation == TRANSFORM_NONE) {
		return;
	}
	// Create an undo/redo action for the transformation.
	const bool is_move_only = _current_transformation == TRANSFORM_MOVE_AXIS || _current_transformation == TRANSFORM_MOVE_PLANE;
	_undo_redo->create_action(is_move_only ? String("Move ND nodes with gizmo") : String("Transform ND nodes with gizmo"));
	const int size = _selected_top_nodes.size();
	for (int i = 0; i < size; i++) {
		NodeND *node_nd = Object::cast_to<NodeND>(_selected_top_nodes[i]);
		if (node_nd != nullptr) {
			if (is_move_only) {
				_undo_redo->add_do_property(node_nd, StringName("global_position"), node_nd->get_global_position());
				_undo_redo->add_undo_property(node_nd, StringName("global_position"), _selected_top_node_old_transforms[i]->get_origin());
			} else {
				_undo_redo->add_do_property(node_nd, StringName("global_transform"), node_nd->get_global_transform());
				_undo_redo->add_undo_property(node_nd, StringName("global_transform"), _selected_top_node_old_transforms[i]);
			}
		}
	}
	_undo_redo->commit_action(false);
	// Clear out the transformation data and mark the scene as unsaved.
	_old_transform = Ref<TransformND>();
	_transform_reference_value = Variant();
	_current_transformation = TRANSFORM_NONE;
	EditorInterface::get_singleton()->mark_scene_as_unsaved();
}

void EditorTransformGizmoND::_process_transform(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction) {
	Ref<TransformND> transform_change;
	transform_change.instantiate();
	Variant casted = _get_transform_raycast_value(p_local_ray_origin, p_local_ray_direction);
	switch (_current_transformation) {
		case TRANSFORM_NONE: {
			// Do nothing.
		} break;
		case TRANSFORM_MOVE_AXIS:
		case TRANSFORM_MOVE_PLANE: {
			transform_change->set_origin(VectorND::subtract(VectorN(casted), VectorN(_transform_reference_value)));
		} break;
		case TRANSFORM_ROTATE: {
			transform_change = TransformND::from_rotation(_primary_axis, _secondary_axis, double(casted) - double(_transform_reference_value));
		} break;
		case TRANSFORM_SCALE_AXIS: {
			const VectorN casted_vec = VectorN(casted);
			ERR_FAIL_COND(casted_vec.size() <= _primary_axis);
			const double scale_factor = casted_vec[_primary_axis] / VectorN(_transform_reference_value)[_primary_axis];
			if (_keep_mode == KeepMode::CONFORMAL) {
				const VectorN scale = VectorND::fill(scale_factor, get_dimension());
				transform_change = TransformND::from_scale(scale);
			} else {
				transform_change->set_basis_element(_primary_axis, _primary_axis, scale_factor);
			}
		} break;
		case TRANSFORM_SCALE_PLANE: {
			const VectorN casted_vec = VectorN(casted);
			ERR_FAIL_COND(casted_vec.size() <= _secondary_axis);
			const double primary_scale = casted_vec[_primary_axis] / VectorN(_transform_reference_value)[_primary_axis];
			const double secondary_scale = casted_vec[_secondary_axis] / VectorN(_transform_reference_value)[_secondary_axis];
			transform_change->set_basis_element(_primary_axis, _primary_axis, primary_scale);
			transform_change->set_basis_element(_secondary_axis, _secondary_axis, secondary_scale);
		} break;
		case TRANSFORM_STRETCH_POS: {
			const double stretch = VectorN(casted)[_primary_axis] * 0.5f;
			if (_keep_mode == KeepMode::CONFORMAL) {
				const VectorN scale = VectorND::fill(stretch + 0.5f, get_dimension());
				transform_change = TransformND::from_scale(scale);
			} else {
				transform_change->set_basis_element(_primary_axis, _primary_axis, stretch + 0.5f);
			}
			transform_change->set_origin_element(_primary_axis, stretch - 0.5f);
		} break;
		case TRANSFORM_STRETCH_NEG: {
			const double stretch = VectorN(casted)[_primary_axis] * 0.5f;
			if (_keep_mode == KeepMode::CONFORMAL) {
				const VectorN scale = VectorND::fill(-stretch + 0.5f, get_dimension());
				transform_change = TransformND::from_scale(scale);
			} else {
				transform_change->set_basis_element(_primary_axis, _primary_axis, -stretch + 0.5f);
			}
			transform_change->set_origin_element(_primary_axis, stretch + 0.5f);
		} break;
		case TRANSFORM_MAX: {
			ERR_FAIL_MSG("Invalid transformation.");
		} break;
	}
	Ref<TransformND> new_transform = _old_transform->compose_square(transform_change);
	set_transform(new_transform);
	// We want the global diff so we can apply it from the left on the global transform of all selected nodes.
	// Without this, the transforms would be relative to each node (ex: moving on X moves on each node's X axis).
	transform_change = _old_transform->transform_to(new_transform);
	for (int i = 0; i < _selected_top_nodes.size(); i++) {
		NodeND *node_nd = Object::cast_to<NodeND>(_selected_top_nodes[i]);
		if (node_nd != nullptr) {
			node_nd->set_global_transform(transform_change->compose_square(_selected_top_node_old_transforms[i]));
			switch (_keep_mode) {
				case KeepMode::FREEFORM: {
					// Do nothing.
				} break;
				case KeepMode::ORTHOGONAL: {
					node_nd->set_transform(node_nd->get_transform()->orthogonalized());
				} break;
				case KeepMode::CONFORMAL: {
					node_nd->set_transform(node_nd->get_transform()->conformalized());
				} break;
				case KeepMode::ORTHONORMAL: {
					node_nd->set_transform(node_nd->get_transform()->orthonormalized());
				} break;
			}
		}
	}
}

void EditorTransformGizmoND::selected_nodes_changed(const TypedArray<Node> &p_top_nodes) {
	_end_transformation();
	_selected_top_nodes = p_top_nodes;
	const int size = _selected_top_nodes.size();
	if (_selected_top_node_old_transforms.size() < size) {
		_selected_top_node_old_transforms.resize(size);
	}
	_update_gizmo_transform();
}

void EditorTransformGizmoND::set_axis_colors(const PackedColorArray &p_axis_colors) {
	_axis_colors = p_axis_colors;
}

void EditorTransformGizmoND::set_gizmo_dimension(const int p_dimension) {
	Ref<TransformND> gizmo_transform = get_transform();
	const int old_dimension = gizmo_transform->get_basis_column_count();
	if (old_dimension == p_dimension) {
		return;
	}
	_mesh_holder->set_dimension(p_dimension);
	gizmo_transform = gizmo_transform->with_dimension(p_dimension)->orthonormalized();
	if (Math::is_zero_approx(gizmo_transform->determinant())) {
		set_transform(TransformND::identity_basis(p_dimension));
	} else {
		set_transform(gizmo_transform);
	}
	_regenerate_gizmo_meshes();
}

void EditorTransformGizmoND::set_keep_mode(const KeepMode p_mode) {
	_keep_mode = p_mode;
	if (_keep_mode == KeepMode::CONFORMAL) {
		_is_scale_planar_enabled = false;
	} else if (_is_scale_linear_enabled) {
		_is_scale_planar_enabled = true;
	}
	const bool is_plane_visible = _is_move_planar_enabled || _is_scale_planar_enabled;
	Vector<MeshInstanceND *> &plane_meshes = _meshes[TRANSFORM_MOVE_PLANE];
	for (int i = 0; i < plane_meshes.size(); i++) {
		plane_meshes[i]->set_visible(is_plane_visible);
	}
}

void EditorTransformGizmoND::set_gizmo_mode(const GizmoMode p_mode) {
	_last_gizmo_mode = p_mode;
	switch (p_mode) {
		case GizmoMode::SELECT: {
			_is_move_linear_enabled = true;
			_is_move_planar_enabled = false;
			_is_rotation_enabled = false;
			_is_scale_linear_enabled = false;
			_is_scale_planar_enabled = false;
			_is_stretch_enabled = false;
		} break;
		case GizmoMode::MOVE: {
			_is_move_linear_enabled = true;
			_is_move_planar_enabled = true;
			_is_rotation_enabled = false;
			_is_scale_linear_enabled = false;
			_is_scale_planar_enabled = false;
			_is_stretch_enabled = false;
		} break;
		case GizmoMode::ROTATE: {
			_is_move_linear_enabled = false;
			_is_move_planar_enabled = false;
			_is_rotation_enabled = true;
			_is_scale_linear_enabled = false;
			_is_scale_planar_enabled = false;
			_is_stretch_enabled = false;
		} break;
		case GizmoMode::SCALE: {
			_is_move_linear_enabled = false;
			_is_move_planar_enabled = false;
			_is_rotation_enabled = false;
			_is_scale_linear_enabled = true;
			_is_scale_planar_enabled = _keep_mode != KeepMode::CONFORMAL;
			_is_stretch_enabled = false;
		} break;
		case GizmoMode::STRETCH: {
			_is_move_linear_enabled = false;
			_is_move_planar_enabled = false;
			_is_rotation_enabled = false;
			_is_scale_linear_enabled = false;
			_is_scale_planar_enabled = false;
			_is_stretch_enabled = true;
		} break;
	}
	// Update mesh instance visibility.
	const bool is_linear_visible = _is_move_linear_enabled || _is_scale_linear_enabled || _is_stretch_enabled;
	Vector<MeshInstanceND *> &arrow_meshes = _meshes[TRANSFORM_MOVE_AXIS];
	for (int i = 0; i < arrow_meshes.size(); i++) {
		arrow_meshes[i]->set_visible(is_linear_visible);
	}
	Vector<MeshInstanceND *> &rotation_meshes = _meshes[TRANSFORM_ROTATE];
	for (int i = 0; i < rotation_meshes.size(); i++) {
		rotation_meshes[i]->set_visible(_is_rotation_enabled);
	}
	const bool is_plane_visible = _is_move_planar_enabled || _is_scale_planar_enabled;
	Vector<MeshInstanceND *> &plane_meshes = _meshes[TRANSFORM_MOVE_PLANE];
	for (int i = 0; i < plane_meshes.size(); i++) {
		plane_meshes[i]->set_visible(is_plane_visible);
	}
	Vector<MeshInstanceND *> &stretch_neg_meshes = _meshes[TRANSFORM_STRETCH_NEG];
	for (int i = 0; i < stretch_neg_meshes.size(); i++) {
		stretch_neg_meshes[i]->set_visible(_is_stretch_enabled);
	}
	set_visible(true);
}

bool EditorTransformGizmoND::gizmo_mouse_input(const Ref<InputEventMouse> &p_mouse_event, const CameraND *p_camera) {
	_update_gizmo_mesh_transform(p_camera);
	if (_selected_top_nodes.is_empty()) {
		set_visible(false);
		return false;
	}
	if (!is_visible()) {
		return false;
	}
	const Vector2 mouse_position = p_mouse_event->get_position();
	const VectorN ray_origin = p_camera->viewport_to_world_ray_origin(mouse_position);
	const VectorN ray_direction = p_camera->viewport_to_world_ray_direction(mouse_position);
	if (_current_transformation == TRANSFORM_NONE) {
		_old_transform = _mesh_holder->get_global_transform();
	}
	if (_old_transform->get_basis_column_count() == 0) {
		// 0D, so nothing to do.
		set_visible(false);
		return false;
	}
	if (Math::is_zero_approx(_old_transform->determinant())) {
		// Something's wrong with our transform. Let's try to fix it.
		set_transform(get_transform()->orthonormalized());
		_mesh_holder->set_transform(_mesh_holder->get_transform()->orthonormalized());
		_old_transform = _mesh_holder->get_global_transform();
		ERR_FAIL_COND_V_MSG(Math::is_zero_approx(_old_transform->determinant()), false, "EditorTransformGizmoND: Gizmo transform is not valid.");
	}
	const Ref<TransformND> global_to_local = _old_transform->inverse();
	const VectorN local_ray_origin = global_to_local->xform(ray_origin);
	const VectorN local_ray_direction = global_to_local->xform_basis(ray_direction);
	return gizmo_mouse_raycast(p_mouse_event, local_ray_origin, local_ray_direction);
}

bool EditorTransformGizmoND::gizmo_mouse_raycast(const Ref<InputEventMouse> &p_mouse_event, const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction) {
	Ref<InputEventMouseButton> mouse_button = p_mouse_event;
	if (mouse_button.is_valid()) {
		if (mouse_button->get_button_index() != MOUSE_BUTTON_LEFT) {
			return false;
		}
		if (mouse_button->is_pressed() && _current_transformation == TRANSFORM_NONE) {
			// If we are not transforming anything and the user clicks, try to start a transformation.
			int new_primary_axis = -1;
			int new_secondary_axis = -1;
			_current_transformation = _check_for_best_hit(p_local_ray_origin, p_local_ray_direction, new_primary_axis, new_secondary_axis);
			if (_current_transformation != TRANSFORM_NONE) {
				_primary_axis = new_primary_axis;
				_secondary_axis = new_secondary_axis;
				_begin_transformation(p_local_ray_origin, p_local_ray_direction);
			}
		} else if (!mouse_button->is_pressed() && _current_transformation != TRANSFORM_NONE) {
			// If we are transforming something and the user releases the click, end the transformation.
			_end_transformation();
		}
		return false;
	}
	Ref<InputEventMouseMotion> mouse_motion = p_mouse_event;
	if (mouse_motion.is_valid()) {
		if (_current_transformation == TRANSFORM_NONE) {
			// If we receive motion but no transformation is happening, it means
			// we need to highlight the closest part of the gizmo (if any).
			int new_primary_axis = -1;
			int new_secondary_axis = -1;
			TransformPart hit = _check_for_best_hit(p_local_ray_origin, p_local_ray_direction, new_primary_axis, new_secondary_axis);
			if (hit == _highlighted_transformation && new_primary_axis == _primary_axis && new_secondary_axis == _secondary_axis) {
				return false;
			}
			_unhighlight_mesh();
			_primary_axis = new_primary_axis;
			_secondary_axis = new_secondary_axis;
			_highlighted_transformation = hit;
			_highlight_mesh();
			return true;
		} else {
			// If we are transforming something, process the transformation.
			_process_transform(p_local_ray_origin, p_local_ray_direction);
			return true;
		}
	}
	return false;
}

bool EditorTransformGizmoND::get_use_local_rotation() const {
	return _is_use_local_rotation;
}

void EditorTransformGizmoND::set_use_local_rotation(const bool p_use_local_transform) {
	_is_use_local_rotation = p_use_local_transform;
	_update_gizmo_transform();
}

void EditorTransformGizmoND::setup(EditorMainScreenND *p_editor_main_screen, EditorUndoRedoManager *p_undo_redo_manager) {
	// Things that we should do in the constructor but can't in GDExtension
	// due to how GDExtension runs the constructor for each registered class.
	for (int i = 0; i < TRANSFORM_MAX; i++) {
		_meshes[i] = Vector<MeshInstanceND *>();
	}
	_mesh_holder = memnew(NodeND);
	_mesh_holder->set_name(StringName("GizmoMeshHolderND"));
	add_child(_mesh_holder);
	RenderingServerND::get_singleton()->connect(StringName("pre_render"), callable_mp(this, &EditorTransformGizmoND::_on_rendering_server_pre_render));
	EditorInterface::get_singleton()->get_inspector()->connect(StringName("property_edited"), callable_mp(this, &EditorTransformGizmoND::_on_editor_inspector_property_edited));

	// Set up things with the arguments (not constructor things).
	_editor_main_screen = p_editor_main_screen;
	_undo_redo = p_undo_redo_manager;
	p_undo_redo_manager->connect(StringName("version_changed"), callable_mp(this, &EditorTransformGizmoND::_on_undo_redo_version_changed));
}
