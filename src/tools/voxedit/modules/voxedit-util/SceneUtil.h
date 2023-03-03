/**
 * @file
 */

#include "math/OBB.h"
#include "math/AABB.h"

#include "voxel/Region.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

math::OBB<float> toOBB(bool sceneMode, const voxel::Region &region, const voxelformat::SceneGraphTransform &transform);
math::AABB<float> toAABB(const voxel::Region& region);

} // namespace voxedit