/**
 * @file
 */

#include "MeshTri.h"
#include <glm/ext/scalar_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/type_aligned.hpp>

namespace voxelformat {

glm::vec2 MeshTri::centerUV() const {
	return (uv[0] + uv[1] + uv[2]) / 3.0f;
}

core::RGBA MeshTri::blendedColor() const {
	if (material) {
		const core::RGBA c1 = material->apply(color[0]);
		const core::RGBA c2 = material->apply(color[1]);
		const core::RGBA c3 = material->apply(color[2]);
		return core::RGBA::mix(core::RGBA::mix(c1, c2), c3);
	}
	return core::RGBA::mix(core::RGBA::mix(color[0], color[1]), color[2]);
}

core::RGBA MeshTri::centerColor() const {
	const glm::vec2 &c = centerUV();
	return colorAt(c);
}

core::RGBA MeshTri::colorAt(const glm::vec2 &inputuv, bool originUpperLeft) const {
	core::RGBA rgba;
	if (!material || !material->colorAt(rgba, inputuv, originUpperLeft)) {
		rgba = blendedColor();
	}
	return rgba;
}

// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
bool MeshTri::calcUVs(const glm::vec3 &pos, glm::vec2 &outUV) const {
	const glm::vec3 &b = calculateBarycentric(pos);

	// Check if barycentric coordinates are within [0, 1]
	if (b.x >= 0.0f && b.x <= 1.0f && b.y >= 0.0f && b.y <= 1.0f && b.z >= 0.0f && b.z <= 1.0f) {
		// Interpolate UVs using barycentric coordinates
		outUV = b.x * uv[0] + b.y * uv[1] + b.z * uv[2];
		return true;
	}

	return false;
}

} // namespace voxelformat
