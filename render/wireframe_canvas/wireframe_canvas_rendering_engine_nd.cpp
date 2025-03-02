#include "wireframe_canvas_rendering_engine_nd.h"

#include "../../math/vector_nd.h"
#include "../../mesh/wire/wire_material_nd.h"
#include "wireframe_render_canvas_nd.h"

Color _get_material_edge_color(const Ref<MaterialND> &p_material, const Ref<MeshND> &p_mesh, int p_edge_index) {
	if (p_material.is_null()) {
		return Color(1.0f, 1.0f, 1.0f);
	}
	const PackedColorArray &color_array = p_material->get_albedo_color_array();
	const Color single_color = p_material->get_albedo_color();
	if (p_edge_index < color_array.size()) {
		return color_array[p_edge_index] * single_color;
	}
	return single_color;
}

void WireframeCanvasRenderingEngineND::setup_for_viewport() {
	WireframeRenderCanvasND *wire_canvas = memnew(WireframeRenderCanvasND);
	wire_canvas->set_name("WireframeRenderCanvasND");
	get_viewport()->add_child(wire_canvas);
}

void WireframeCanvasRenderingEngineND::render_frame() {
	WireframeRenderCanvasND *wire_canvas = GET_NODE_TYPE(get_viewport(), WireframeRenderCanvasND, "WireframeRenderCanvasND");
	ERR_FAIL_NULL_MSG(wire_canvas, "WireframeCanvasRenderingEngineND: Canvas was null.");
	const CameraND *camera = get_camera();
	Vector<PackedColorArray> edge_colors_to_draw;
	PackedFloat32Array edge_thicknesses_to_draw;
	Vector<PackedVector2Array> edge_vertices_to_draw;
	TypedArray<MeshInstanceND> mesh_instances = get_mesh_instances();
	TypedArray<TransformND> mesh_relative_transforms = get_mesh_relative_transforms();
	const bool camera_has_perspective = camera->get_projection_type() == CameraND::PROJECTION_PERSPECTIVE;
	const bool camera_has_perp_fading = camera->get_perp_fade_mode() != CameraND::PERP_FADE_DISABLED;
	const bool camera_has_perp_fade_hue_shift = camera->get_perp_fade_mode() & CameraND::PERP_FADE_HUE_SHIFT;
	const bool camera_has_perp_fade_transparency = camera->get_perp_fade_mode() & CameraND::PERP_FADE_TRANSPARENCY;
	for (int mesh_index = 0; mesh_index < mesh_instances.size(); mesh_index++) {
		MeshInstanceND *mesh_inst = Object::cast_to<MeshInstanceND>(mesh_instances[mesh_index]);
		ERR_CONTINUE(mesh_inst == nullptr);
		Ref<MaterialND> material = mesh_inst->get_active_material();
		const Ref<TransformND> mesh_relative_transform = mesh_relative_transforms[mesh_index];
		const Vector<VectorN> camera_relative_vertices = mesh_relative_transform->xform_many(mesh_inst->get_mesh()->get_vertices());
		const bool direct_project = camera_relative_vertices[0].size() < 3;
		PackedVector2Array projected_vertices;
		for (int vertex = 0; vertex < camera_relative_vertices.size(); vertex++) {
			projected_vertices.push_back(camera->world_to_viewport_local_normal(camera_relative_vertices[vertex], direct_project));
		}
		PackedColorArray edge_colors;
		PackedVector2Array edge_vertices;
		const PackedInt32Array edge_indices = mesh_inst->get_mesh()->get_edge_indices();
		for (int edge_index = 0; edge_index < edge_indices.size() / 2; edge_index++) {
			const int a_index = edge_indices[edge_index * 2];
			const int b_index = edge_indices[edge_index * 2 + 1];
			const VectorN a_vert_nd = camera_relative_vertices[a_index];
			const VectorN b_vert_nd = camera_relative_vertices[b_index];
			Color edge_color;
			if (direct_project) {
				// No clipping or fading is required for 0D, 1D, or 2D relative vertices.
				edge_vertices.push_back(projected_vertices[a_index]);
				edge_vertices.push_back(projected_vertices[b_index]);
				edge_color = _get_material_edge_color(material, mesh_inst->get_mesh(), edge_index);
			} else {
				const double a_z = a_vert_nd[2];
				const double b_z = b_vert_nd[2];
				if (a_z > -camera->get_near()) {
					if (b_z > -camera->get_near()) {
						// Both points are behind the camera, so we skip this edge.
						continue;
					} else {
						// A is behind the camera, while B is in front of the camera.
						const double factor = (a_z + camera->get_near()) / (a_z - b_z);
						const VectorN clipped = VectorND::lerp(a_vert_nd, b_vert_nd, factor);
						edge_vertices.push_back(camera->world_to_viewport_local_normal(clipped));
						edge_vertices.push_back(projected_vertices[b_index]);
					}
				} else {
					edge_vertices.push_back(projected_vertices[a_index]);
					if (b_z > -camera->get_near()) {
						// B is behind the camera, while A is in front of the camera.
						const double factor = (b_z + camera->get_near()) / (b_z - a_z);
						const VectorN clipped = VectorND::lerp(b_vert_nd, a_vert_nd, factor);
						edge_vertices.push_back(camera->world_to_viewport_local_normal(clipped));
					} else {
						// Both points are in front of the camera, so render the edge as-is.
						edge_vertices.push_back(projected_vertices[b_index]);
					}
				}
				edge_color = _get_material_edge_color(material, mesh_inst->get_mesh(), edge_index);
				if (camera_has_perp_fading) {
					double fade_denom = camera->get_perp_fade_distance();
					if (camera_has_perspective) {
						fade_denom += camera->get_perp_fade_slope() * -0.5f * (a_z + b_z);
					}
					const VectorN a_perp = VectorND::drop_first_dimensions(a_vert_nd, 3);
					const VectorN b_perp = VectorND::drop_first_dimensions(b_vert_nd, 3);
					const VectorN perp_dimensions = VectorND::multiply_scalar(VectorND::add(a_perp, b_perp), 0.5 / fade_denom);
					switch (perp_dimensions.size()) {
						case 0:
							break;
						case 1: {
							const double perp_w = perp_dimensions[0];
							const double perp_magnitude = ABS(perp_w);
							if (camera_has_perp_fade_hue_shift) {
								const float value = edge_color.get_v();
								const Color target_color = perp_w > 0.0 ? Color(value, 0.0f, 0.0f) : Color(0.0f, value, value);
								edge_color = edge_color.lerp(target_color, MIN(1.0, perp_magnitude));
							}
							if (camera_has_perp_fade_transparency) {
								edge_color.a = 1.0 - MIN(1.0, perp_magnitude);
							}
						} break;
						default: {
							const double perp_w = perp_dimensions[0];
							const double perp_v = perp_dimensions[1];
							const double perp_magnitude = VectorND::length(perp_dimensions);
							if (camera_has_perp_fade_hue_shift) {
								const float target_hue = Math::atan2(perp_v, perp_w) / Math_TAU + 1.0f;
								const Color target_color = Color::from_hsv(target_hue, 1.0, edge_color.get_v());
								edge_color = edge_color.lerp(target_color, MIN(1.0, perp_magnitude));
							}
							if (camera_has_perp_fade_transparency) {
								edge_color.a = 1.0 - MIN(1.0, perp_magnitude);
							}
						} break;
					}
				}
			}
			if (camera->get_depth_fade()) {
				const double depth = abs((VectorND::length(a_vert_nd) + VectorND::length(b_vert_nd)) * 0.5);
				double alpha = 1.0;

				const double far = camera->get_far();
				const double start = camera->get_depth_fade_start();

				if (depth > far) {
					alpha = 0.0;
				} else if (depth < start) {
					alpha = 1.0;
				} else {
					const double unit_distance = (depth - start) / (far - start); // Inverse lerp
					alpha = 1.0 - unit_distance;
				}

				edge_color.a *= alpha;
			}
			edge_colors.push_back(edge_color);
		}
		if (edge_vertices.is_empty()) {
			continue;
		}
		edge_colors_to_draw.push_back(edge_colors);
		Ref<WireMaterialND> wire_material = material;
		if (wire_material.is_valid()) {
			edge_thicknesses_to_draw.push_back(wire_material->get_line_thickness() > 0.0 ? wire_material->get_line_thickness() : -1.0);
		} else {
			edge_thicknesses_to_draw.push_back(-1.0);
		}
		edge_vertices_to_draw.push_back(edge_vertices);
	}
	wire_canvas->set_camera_aspect(camera->get_keep_aspect());
	wire_canvas->set_edge_colors_to_draw(edge_colors_to_draw);
	wire_canvas->set_edge_thicknesses_to_draw(edge_thicknesses_to_draw);
	wire_canvas->set_edge_vertices_to_draw(edge_vertices_to_draw);
	wire_canvas->queue_redraw();
}
