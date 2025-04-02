/**
 * @file
 */

#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class ModifierVolumeWrapperTest : public app::AbstractTest {};

TEST_F(ModifierVolumeWrapperTest, testPlace) {
	voxel::RawVolume volume(voxel::Region(-3, 3));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	ModifierVolumeWrapper wrapper(node, ModifierType::Place);
	ASSERT_FALSE(wrapper.dirtyRegion().isValid());
	ASSERT_TRUE(wrapper.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(wrapper.dirtyRegion().isValid());
	ASSERT_EQ(wrapper.dirtyRegion(), voxel::Region(0, 0));
	ASSERT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()));
}

TEST_F(ModifierVolumeWrapperTest, testPlaceSelection) {
	voxel::RawVolume volume(voxel::Region(-3, 3));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	Selections selections;
	selections.push_back(voxel::Region(1, 1));
	ModifierVolumeWrapper wrapper(node, ModifierType::Place, selections);
	ASSERT_FALSE(wrapper.dirtyRegion().isValid());
	ASSERT_FALSE(wrapper.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(wrapper.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(wrapper.dirtyRegion().isValid());
	ASSERT_EQ(wrapper.dirtyRegion(), voxel::Region(1, 1));
	ASSERT_TRUE(voxel::isBlocked(volume.voxel(1, 1, 1).getMaterial()));
	ASSERT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()));
}

TEST_F(ModifierVolumeWrapperTest, testErase) {
	voxel::RawVolume volume(voxel::Region(-3, 3));
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	ModifierVolumeWrapper wrapper(node, ModifierType::Erase);
	ASSERT_FALSE(wrapper.dirtyRegion().isValid());
	ASSERT_TRUE(wrapper.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(wrapper.dirtyRegion().isValid());
	ASSERT_EQ(wrapper.dirtyRegion(), voxel::Region(0, 0));
	ASSERT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()));
}

TEST_F(ModifierVolumeWrapperTest, testPaint) {
	voxel::RawVolume volume(voxel::Region(-3, 3));
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	ModifierVolumeWrapper wrapper(node, ModifierType::Paint);
	ASSERT_FALSE(wrapper.dirtyRegion().isValid());
	ASSERT_TRUE(wrapper.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2)));
	ASSERT_FALSE(wrapper.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2)));
	ASSERT_TRUE(wrapper.dirtyRegion().isValid());
	ASSERT_EQ(wrapper.dirtyRegion(), voxel::Region(0, 0));
	ASSERT_EQ(2, volume.voxel(0, 0, 0).getColor());
	ASSERT_TRUE(voxel::isAir(volume.voxel(1, 1, 1).getMaterial()));
}

TEST_F(ModifierVolumeWrapperTest, testOverride) {
	voxel::RawVolume volume(voxel::Region(-3, 3));
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);
	ASSERT_FALSE(wrapper.dirtyRegion().isValid());
	ASSERT_TRUE(wrapper.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2)));
	ASSERT_TRUE(wrapper.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2)));
	ASSERT_TRUE(wrapper.dirtyRegion().isValid());
	ASSERT_EQ(wrapper.dirtyRegion(), voxel::Region(0, 1));
	ASSERT_EQ(2, volume.voxel(0, 0, 0).getColor());
	ASSERT_EQ(2, volume.voxel(1, 1, 1).getColor());
}

} // namespace voxedit
