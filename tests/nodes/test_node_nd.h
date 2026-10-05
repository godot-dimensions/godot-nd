#pragma once

#include "../../nodes/node_nd.h"

#include "tests/test_macros.h"

namespace TestNodeND {
TEST_CASE("[NodeND]") {
	NodeND test = NodeND();
}

TEST_CASE("[NodeND] Global to local and local to global") {
	NodeND *parent = memnew(NodeND);
	NodeND *child = memnew(NodeND);
	parent->add_child(child);
	parent->set_transform(TransformND::from_position_scale(VectorN{ 10.0, 0.0, 0.0, 0.0 }, VectorN{ 2.0, 2.0, 2.0, 2.0 }));
	child->set_position(VectorN{ 0.0, 1.0, 0.0, 0.0 });
	// The child's origin is 1 unit along Y in the parent, which is scaled by 2 and moved 10 units along X.
	CHECK(VectorND::is_equal_approx(child->local_to_global(VectorN{ 0.0, 0.0, 0.0, 0.0 }), VectorN{ 10.0, 2.0, 0.0, 0.0 }));
	CHECK(VectorND::is_equal_approx(child->local_to_global(VectorN{ 0.0, 0.0, 0.0, 1.0 }), VectorN{ 10.0, 2.0, 0.0, 2.0 }));
	CHECK(VectorND::is_equal_approx(child->global_to_local(VectorN{ 10.0, 2.0, 0.0, 2.0 }), VectorN{ 0.0, 0.0, 0.0, 1.0 }));
	const VectorN point = VectorN{ 3.0, -4.0, 5.0, 6.0 };
	CHECK_MESSAGE(VectorND::is_equal_approx(child->global_to_local(child->local_to_global(point)), point), "The conversions must be inverses of each other.");
	memdelete(parent);
}
} // namespace TestNodeND
