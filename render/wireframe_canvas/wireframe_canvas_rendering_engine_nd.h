#pragma once

#include "../rendering_engine_nd.h"

class WireframeRenderCanvasND;

// Trivial CPU-based renderer that draws wireframes to a Control-based canvas.
// Very inefficient, but easy to implement, and even once we have a better
// renderer, this can still be useful for testing and debugging.
class WireframeCanvasRenderingEngineND : public RenderingEngineND {
	GDCLASS(WireframeCanvasRenderingEngineND, RenderingEngineND);

	static Color _get_material_edge_color(const Ref<MaterialND> &p_material, const Ref<MeshND> &p_mesh, int p_edge_index);
	static WireframeRenderCanvasND *_get_valid_render_canvas(const Viewport *p_viewport);

protected:
	static void _bind_methods() {}

public:
	virtual String get_friendly_name() const override { return "Wireframe Canvas"; }
	virtual bool requires_transparent_background() const override { return false; }
	virtual bool supports_godot_rendering_method(const String &) const override { return true; }
	virtual void setup_for_viewport() override;
	virtual void cleanup_for_viewport() override;
	virtual void render_frame() override;
};
