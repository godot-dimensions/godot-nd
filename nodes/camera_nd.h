#pragma once

#include "node_nd.h"

class CameraND : public NodeND {
	GDCLASS(CameraND, NodeND);

public:
	enum KeepAspect {
		KEEP_WIDTH,
		KEEP_HEIGHT
	};

	enum ProjectionType {
		PROJECTION_ORTHOGRAPHIC = 0,
		PROJECTION_PERSPECTIVE = 1,
	};

	enum ViewAngleType {
		VIEW_ANGLE_FOCAL_LENGTH = 0,
		VIEW_ANGLE_FIELD_OF_VIEW = 1,
	};

	enum PerpFadeMode {
		PERP_FADE_DISABLED = 0,
		PERP_FADE_TRANSPARENCY = 1,
		PERP_FADE_HUE_SHIFT = 2,
		PERP_FADE_TRANSPARENCY_HUE_SHIFT = 3,
	};

private:
	String _rendering_engine = "";
	KeepAspect _keep_aspect = KEEP_HEIGHT;
	ProjectionType _projection_type = PROJECTION_PERSPECTIVE;
	ViewAngleType _view_angle_type = VIEW_ANGLE_FOCAL_LENGTH;
	PerpFadeMode _perp_fade_mode = PERP_FADE_TRANSPARENCY;

	// This has wrappers with trig functions, so let's use double to avoid precision loss.
	double _focal_length = 1.0;

	double _depth_fade_start = 25.0;
	double _orthographic_size = 5.0;
	double _clip_near = 0.05;
	double _clip_far = 4000.0;
	double _perp_fade_distance = 5.0;
	double _perp_fade_slope = 1.0;
	bool _is_current = false;
	bool _use_depth_fade = false;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;
	void _notification(int p_what);

public:
	bool is_current() const;
	void set_current(bool p_enabled);
	void clear_current(bool p_enable_next = true);
	void make_current();

	bool is_position_behind(const VectorN &p_global_position) const;
	VectorN viewport_to_world_ray_origin(const Vector2 &p_viewport_position) const;
	VectorN viewport_to_world_ray_direction(const Vector2 &p_viewport_position) const;
	Vector2 world_to_viewport_local_normal(const VectorN &p_local_position, const bool p_force_orthographic = false) const;
	Vector2 world_to_viewport(const VectorN &p_global_position) const;

	String get_rendering_engine() const;
	void set_rendering_engine(const String &p_rendering_engine);

	KeepAspect get_keep_aspect() const;
	void set_keep_aspect(const KeepAspect p_keep_aspect);

	ProjectionType get_projection_type() const;
	void set_projection_type(const ProjectionType p_projection_type_nd);

	ViewAngleType get_view_angle_type() const;
	void set_view_angle_type(const ViewAngleType p_view_angle_type);

	double get_focal_length() const;
	void set_focal_length(const double p_focal_length_nd);

	double get_field_of_view() const;
	void set_field_of_view(const double p_field_of_view_nd);

	double get_orthographic_size() const;
	void set_orthographic_size(const double p_orthographic_size);

	double get_clip_near() const;
	void set_clip_near(const double p_clip_near);

	double get_clip_far() const;
	void set_clip_far(const double p_clip_far);

	PerpFadeMode get_perp_fade_mode() const;
	void set_perp_fade_mode(const PerpFadeMode p_perp_fade_mode);

	double get_perp_fade_distance() const;
	void set_perp_fade_distance(const double p_perp_fade_distance);

	double get_perp_fade_slope() const;
	void set_perp_fade_slope(const double p_perp_fade_slope);

	bool get_depth_fade() const;
	void set_depth_fade(const bool p_depth_fade);

	double get_depth_fade_start() const;
	void set_depth_fade_start(const double p_depth_fade_start);
};

VARIANT_ENUM_CAST(CameraND::KeepAspect);
VARIANT_ENUM_CAST(CameraND::ProjectionType);
VARIANT_ENUM_CAST(CameraND::ViewAngleType);
VARIANT_ENUM_CAST(CameraND::PerpFadeMode);
