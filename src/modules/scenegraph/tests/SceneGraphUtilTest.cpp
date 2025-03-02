/**
 * @file
 */

#include "scenegraph/SceneGraphUtil.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "scenegraph/SceneGraphNode.h"

namespace scenegraph {

class SceneGraphUtilTest : public app::AbstractTest {};

TEST_F(SceneGraphUtilTest, testAddSceneGraphNodes) {
	SceneGraph source;
	int groupNodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("group");
		groupNodeId = source.emplace(core::move(node));
	}
	{
		SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		source.emplace(core::move(node), groupNodeId);
	}
	SceneGraph target;
	EXPECT_EQ(1, addSceneGraphNodes(target, source, target.root().id()));
	ASSERT_TRUE(target.hasNode(1));
	EXPECT_EQ(SceneGraphNodeType::Group, target.node(1).type());
	ASSERT_TRUE(target.hasNode(2));
	EXPECT_EQ(SceneGraphNodeType::Model, target.node(2).type());
	ASSERT_EQ(1, target.node(2).parent());
}

} // namespace voxelformat
