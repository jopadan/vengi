/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "voxelutil/VolumeVisitor.h"

class ModifierVolumeWrapperBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraphNode *node = nullptr;

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);

		const voxel::Region region(0, 0, 0, 10, 10, 10);

		node = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		node->setVolume(new voxel::RawVolume(region), true);
	}

	void TearDown(::benchmark::State &state) override {
		delete node;
		app::AbstractBenchmark::TearDown(state);
	}
};

BENCHMARK_DEFINE_F(ModifierVolumeWrapperBenchmark, Place)(benchmark::State &state) {
	const glm::ivec3 &mins = node->region().getLowerCorner();
	const glm::ivec3 &dim = node->region().getDimensionsInVoxels();
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Place);
	for (auto _ : state) {
		wrapper.fill(voxel::Voxel(voxel::VoxelType::Air, 0));
		voxelgenerator::shape::createCubeNoCenter(wrapper, mins, dim, voxel);
	}
}

BENCHMARK_DEFINE_F(ModifierVolumeWrapperBenchmark, Override)(benchmark::State &state) {
	const glm::ivec3 &mins = node->region().getLowerCorner();
	const glm::ivec3 &dim = node->region().getDimensionsInVoxels();
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
	for (auto _ : state) {
		wrapper.fill(voxel::Voxel(voxel::VoxelType::Air, 0));
		voxelgenerator::shape::createCubeNoCenter(wrapper, mins, dim, voxel);
	}
}

BENCHMARK_DEFINE_F(ModifierVolumeWrapperBenchmark, Erase)(benchmark::State &state) {
	const glm::ivec3 &mins = node->region().getLowerCorner();
	const glm::ivec3 &dim = node->region().getDimensionsInVoxels();
	const voxel::Voxel voxel;

	voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Erase);
	for (auto _ : state) {
		wrapper.fill(voxel::Voxel(voxel::VoxelType::Generic, 1));
		voxelgenerator::shape::createCubeNoCenter(wrapper, mins, dim, voxel);
	}
}

BENCHMARK_REGISTER_F(ModifierVolumeWrapperBenchmark, Place);
BENCHMARK_REGISTER_F(ModifierVolumeWrapperBenchmark, Override);
BENCHMARK_REGISTER_F(ModifierVolumeWrapperBenchmark, Erase);

BENCHMARK_MAIN();
