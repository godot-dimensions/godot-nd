#pragma once

#include "editor_viewport_nd_defines.h"

#include "../../math/rect_nd.h"
#include "../../model/mesh_instance_nd.h"

class CameraND;
class RenderingEngineND;
class WireMaterialND;

class EditorTransformGizmoND : public NodeND {
	GDCLASS(EditorTransformGizmoND, NodeND);

public:
	// Keep this in sync with the toolbar buttons in EditorMainScreenND.
	enum class GizmoMode {
		SELECT,
		MOVE,
		ROTATE,
		SCALE,
		STRETCH,
	};

	// Keep this in sync with the transform settings items in EditorMainScreenND.
	enum class KeepMode {
		FREEFORM,
		ORTHOGONAL,
		CONFORMAL,
		ORTHONORMAL,
	};

private:
	enum TransformPart {
		TRANSFORM_NONE,
		TRANSFORM_MOVE_AXIS,
		TRANSFORM_MOVE_PLANE,
		TRANSFORM_ROTATE,
		TRANSFORM_SCALE_AXIS,
		TRANSFORM_SCALE_PLANE,
		TRANSFORM_STRETCH_POS,
		TRANSFORM_STRETCH_NEG,
		TRANSFORM_MAX,
	};

	NodeND *_mesh_holder = nullptr;
	Vector<NodeND *> _mesh_keep_conformal;
	Vector<MeshInstanceND *> _meshes[TRANSFORM_MAX];
	EditorMainScreenND *_editor_main_screen = nullptr;
	EditorUndoRedoManager *_undo_redo = nullptr;

	KeepMode _keep_mode = KeepMode::FREEFORM;
	TransformPart _current_transformation = TRANSFORM_NONE;
	TransformPart _highlighted_transformation = TRANSFORM_NONE;
	GizmoMode _last_gizmo_mode = GizmoMode::SELECT;
	int _primary_axis = -1;
	int _secondary_axis = -1;

	Ref<TransformND> _old_gizmo_transform;
	Ref<TransformND> _old_mesh_holder_transform;
	Variant _transform_reference_value = Variant();
	TypedArray<Node> _selected_top_nodes;
	Vector<Ref<TransformND>> _selected_top_node_old_transforms;
	PackedColorArray _axis_colors;

	bool _is_move_linear_enabled = true;
	bool _is_move_planar_enabled = false;
	bool _is_rotation_enabled = false;
	bool _is_scale_linear_enabled = false;
	bool _is_scale_planar_enabled = false;
	bool _is_stretch_enabled = false;
	bool _is_use_local_rotation = false;

	// Setup functions.
	MeshInstanceND *_make_mesh_instance(const StringName &p_name, const Ref<ArrayWireMeshND> &p_mesh, const Ref<WireMaterialND> &p_material, NodeND *p_parent);
	void _generate_gizmo_meshes();
	void _regenerate_gizmo_meshes();

	// Misc internal functions.
	void _on_rendering_server_pre_render(CameraND *p_camera, Viewport *p_viewport, RenderingEngineND *p_rendering_engine);
	void _on_editor_inspector_property_edited(const String &p_prop);
	void _on_undo_redo_version_changed();
	void _update_gizmo_transform();
	void _update_gizmo_mesh_transform(const CameraND *p_camera);
	Ref<RectND> _get_rect_bounds_of_selection(const Ref<TransformND> &p_inv_relative_to) const;
	static String _get_transform_part_simple_action_name(const TransformPart p_part);
	static VectorN _origin_axis_aligned_biplane_raycast(const VectorN &p_ray_origin, const VectorN &p_ray_direction, const VectorN &p_axis1, const VectorN &p_axis2);

	// Highlighting functions, used when not transforming.
	TransformPart _check_for_best_hit(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction, int &r_primary_axis, int &r_secondary_axis) const;
	MeshInstanceND *_get_highlightable_gizmo_mesh() const;
	void _unhighlight_mesh();
	void _highlight_mesh();

	// Transformation functions.
	Variant _get_transform_raycast_value(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction);
	void _begin_transformation(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction);
	void _end_transformation();
	void _process_transform(const VectorN &p_local_ray_origin, const VectorN &p_local_ray_direction);

protected:
	static void _bind_methods() {}

public:
	void selected_nodes_changed(const TypedArray<Node> &p_top_nodes);
	void set_axis_colors(const PackedColorArray &p_axis_colors);
	void set_keep_mode(const KeepMode p_mode);
	void set_gizmo_mode(const GizmoMode p_mode);
	void set_gizmo_dimension(const int p_dimension);
	bool gizmo_mouse_input(const Ref<InputEventMouse> &p_mouse_event, const CameraND *p_camera);
	bool gizmo_mouse_raycast(const Ref<InputEventMouse> &p_mouse_event, const VectorN &p_ray_origin, const VectorN &p_ray_direction);

	bool get_use_local_rotation() const;
	void set_use_local_rotation(const bool p_use_local_transform);

	void setup(EditorMainScreenND *p_editor_main_screen, EditorUndoRedoManager *p_undo_redo_manager);
};
