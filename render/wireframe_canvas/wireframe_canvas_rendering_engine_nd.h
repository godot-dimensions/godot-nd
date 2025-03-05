#pragma once

#include "../rendering_engine_nd.h"

// Trivial CPU-based renderer that draws wireframes to a Control-based canvas.
// Very inefficient, but easy to implement, and even once we have a better
// renderer, this can still be useful for testing and debugging.
class WireframeCanvasRenderingEngineND : public RenderingEngineND {
	GDCLASS(WireframeCanvasRenderingEngineND, RenderingEngineND);

protected:
	static void _bind_methods() {}

public:
	void setup_for_viewport() override;
	void render_frame() override;
};
