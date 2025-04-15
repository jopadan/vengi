/**
 * @file
 */
#pragma once

#include "app/App.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "palette/Palette.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxelutil {

template<typename Sampler>
static bool isHidden(const Sampler &srcSampler) {
	Sampler sampler1 = srcSampler;
	const glm::ivec3 pos = sampler1.position() - 1;
	// TODO: use copy sampler pattern and only move on one axis and skip the more expensive setPosition
	for (int32_t childZ = -1; childZ <= 1; ++childZ) {
		for (int32_t childY = -1; childY <= 1; ++childY) {
			for (int32_t childX = -1; childX <= 1; ++childX) {
				if (childZ == 0 && childY == 0 && childX == 0) {
					continue;
				}
				if (!sampler1.setPosition(pos.x + childX, pos.y + childY, pos.z + childZ)) {
					return false;
				}
				if (!isBlocked(sampler1.voxel().getMaterial())) {
					return false;
				}
			}
		}
	}
	return true;
}

/**
 * @brief Rescales a volume by sampling two voxels to produce one output voxel.
 * @param[in] sourceVolume The source volume to resample
 * @param[in] destVolume The destination volume to resample into
 * @param[in] sourceRegion The region of the source volume to resample
 * @param[in] destRegion The region of the destination volume to resample into. Usually this should
 * be exactly half of the size of the sourceRegion.
 */
template<typename SourceVolume, typename DestVolume>
void scaleDown(const SourceVolume &sourceVolume, const palette::Palette &palette, const voxel::Region &sourceRegion,
			   DestVolume &destVolume, const voxel::Region &destRegion) {
	core_trace_scoped(ScaleVolumeDown);
	typename SourceVolume::Sampler srcSampler(sourceVolume);

	const int32_t depth = destRegion.getDepthInVoxels();
	const int32_t height = destRegion.getHeightInVoxels();
	const int32_t width = destRegion.getWidthInVoxels();
	// First of all we iterate over all destination voxels and compute their color as the
	// avg of the colors of the eight corresponding voxels in the higher resolution version.
	for (int32_t z = 0; z < depth; ++z) {
		for (int32_t y = 0; y < height; ++y) {
			for (int32_t x = 0; x < width; ++x) {
				const glm::ivec3 curPos(x, y, z);
				const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + curPos * 2;
				const glm::ivec3 dstPos = destRegion.getLowerCorner() + curPos;

				float colorContributors = 0.0f;
				float solidVoxels = 0.0f;
				float avgColorRed = 0.0f;
				float avgColorGreen = 0.0f;
				float avgColorBlue = 0.0f;
				voxel::Voxel colorGuardVoxel;
				for (int32_t childZ = 0; childZ < 2; ++childZ) {
					for (int32_t childY = 0; childY < 2; ++childY) {
						for (int32_t childX = 0; childX < 2; ++childX) {
							srcSampler.setPosition(srcPos + glm::ivec3(childX, childY, childZ));
							if (!srcSampler.currentPositionValid()) {
								continue;
							}
							const voxel::Voxel &child = srcSampler.voxel();

							if (isBlocked(child.getMaterial())) {
								++solidVoxels;
								if (isHidden(srcSampler)) {
									colorGuardVoxel = child;
									continue;
								}
								const glm::vec4 &color = core::Color::fromRGBA(palette.color(child.getColor()));
								avgColorRed += color.r;
								avgColorGreen += color.g;
								avgColorBlue += color.b;
								++colorContributors;
							}
						}
					}
				}

				// We only make a voxel solid if the eight corresponding voxels are also all solid. This
				// means that higher LOD meshes actually shrink away which ensures cracks aren't visible.
				if (solidVoxels >= 7.0f) {
					if (colorContributors <= 0.0f) {
						const glm::vec4 &color = core::Color::fromRGBA(palette.color(colorGuardVoxel.getColor()));
						avgColorRed += color.r;
						avgColorGreen += color.g;
						avgColorBlue += color.b;
						++colorContributors;
					}
					const glm::vec4 avgColor(avgColorRed / colorContributors, avgColorGreen / colorContributors,
											 avgColorBlue / colorContributors, 1.0f);
					core::RGBA avgRGBA = core::Color::getRGBA(avgColor);
					const int index = palette.getClosestMatch(avgRGBA);
					voxel::Voxel voxel = voxel::createVoxel(palette, index);
					destVolume.setVoxel(dstPos, voxel);
				} else {
					const voxel::Voxel voxelAir;
					destVolume.setVoxel(dstPos, voxelAir);
				}
			}
		}
	}

	// At this point the results are usable, but we have a problem with thin structures disappearing.
	// For example, if we have a solid blue sphere with a one voxel thick layer of red voxels on it,
	// then we don't care that the shape changes then the red voxels are lost but we do care that the
	// color changes, as this is very noticeable. Our solution is to process again only those voxels
	// which lie on a material-air boundary, and to recompute their color using a larger neighborhood
	// while also accounting for how visible the child voxels are.
	typename DestVolume::Sampler dstSampler(destVolume);
	for (int32_t z = 0; z < destRegion.getDepthInVoxels(); ++z) {
		for (int32_t y = 0; y < destRegion.getHeightInVoxels(); ++y) {
			for (int32_t x = 0; x < destRegion.getWidthInVoxels(); ++x) {
				const glm::ivec3 curPos(x, y, z);
				const glm::ivec3 dstPos = destRegion.getLowerCorner() + curPos;

				dstSampler.setPosition(dstPos);

				// Skip empty voxels
				if (dstSampler.voxel().getMaterial() == voxel::VoxelType::Air) {
					continue;
				}
				// Only process voxels on a material-air boundary.
				if (dstSampler.peekVoxel0px0py1nz().getMaterial() != voxel::VoxelType::Air &&
					dstSampler.peekVoxel0px0py1pz().getMaterial() != voxel::VoxelType::Air &&
					dstSampler.peekVoxel0px1ny0pz().getMaterial() != voxel::VoxelType::Air &&
					dstSampler.peekVoxel0px1py0pz().getMaterial() != voxel::VoxelType::Air &&
					dstSampler.peekVoxel1nx0py0pz().getMaterial() != voxel::VoxelType::Air &&
					dstSampler.peekVoxel1px0py0pz().getMaterial() != voxel::VoxelType::Air) {
					continue;
				}
				const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + curPos * 2;

				float totalRed = 0.0f;
				float totalGreen = 0.0f;
				float totalBlue = 0.0f;
				float totalExposedFaces = 0.0f;

				// Look at the 64 (4x4x4) children
				for (int32_t childZ = -1; childZ < 3; childZ++) {
					for (int32_t childY = -1; childY < 3; childY++) {
						for (int32_t childX = -1; childX < 3; childX++) {
							srcSampler.setPosition(srcPos + glm::ivec3(childX, childY, childZ));

							const voxel::Voxel &child = srcSampler.voxel();
							if (child.getMaterial() == voxel::VoxelType::Air) {
								continue;
							}

							// For each small voxel, count the exposed faces and use this
							// to determine the importance of the color contribution.
							float exposedFaces = 0.0f;
							if (srcSampler.peekVoxel0px0py1nz().getMaterial() == voxel::VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel0px0py1pz().getMaterial() == voxel::VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel0px1ny0pz().getMaterial() == voxel::VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel0px1py0pz().getMaterial() == voxel::VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel1nx0py0pz().getMaterial() == voxel::VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel1px0py0pz().getMaterial() == voxel::VoxelType::Air) {
								++exposedFaces;
							}

							const glm::vec4 &color = core::Color::fromRGBA(palette.color(child.getColor()));
							totalRed += color.r * exposedFaces;
							totalGreen += color.g * exposedFaces;
							totalBlue += color.b * exposedFaces;

							totalExposedFaces += exposedFaces;
						}
					}
				}

				// Avoid divide by zero if there were no exposed faces.
				if (totalExposedFaces <= 0.01f) {
					++totalExposedFaces;
				}

				const glm::vec4 avgColor(totalRed / totalExposedFaces, totalGreen / totalExposedFaces,
										 totalBlue / totalExposedFaces, 1.0f);
				core::RGBA avgRGBA = core::Color::getRGBA(avgColor);
				const int index = palette.getClosestMatch(avgRGBA);
				const voxel::Voxel voxel = voxel::createVoxel(palette, index);
				destVolume.setVoxel(dstPos, voxel);
			}
		}
	}
}

template<typename SourceVolume, typename DestVolume>
void scaleDown(const SourceVolume &sourceVolume, const palette::Palette &palette, DestVolume &destVolume) {
	scaleDown(sourceVolume, palette, sourceVolume.region(), destVolume, destVolume.region());
}

[[nodiscard]] voxel::RawVolume *scaleUp(const voxel::RawVolume &sourceVolume) {
	const voxel::Region srcRegion = sourceVolume.region();
	const glm::ivec3 &dim = srcRegion.getDimensionsInVoxels();
	const glm::ivec3 &mins = srcRegion.getLowerCorner();
	const glm::ivec3 &targetDimensions = dim * 2 - 1;
	const voxel::Region destRegion(mins, mins + targetDimensions);
	if (!app::App::getInstance()->hasEnoughMemory(voxel::RawVolume::size(destRegion))) {
		return nullptr;
	}

	static const glm::ivec3 directions[8] = {glm::ivec3(0, 0, 0), glm::ivec3(1, 0, 0), glm::ivec3(0, 1, 0),
											 glm::ivec3(1, 1, 0), glm::ivec3(0, 0, 1), glm::ivec3(1, 0, 1),
											 glm::ivec3(0, 1, 1), glm::ivec3(1, 1, 1)};

	voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
	voxel::RawVolume::Sampler sourceSampler(sourceVolume);
	sourceSampler.setPosition(mins);
	for (int32_t x = 0; x < dim.x; ++x) {
		voxel::RawVolume::Sampler sampler2 = sourceSampler;
		for (int32_t y = 0; y < dim.y; ++y) {
			voxel::RawVolume::Sampler sampler3 = sampler2;
			for (int32_t z = 0; z < dim.z; ++z) {
				const voxel::Voxel &voxel = sampler3.voxel();
				const glm::ivec3 targetPos(mins.x + x * 2, mins.y + y * 2, mins.z + z * 2);
				for (int i = 0; i < 8; ++i) {
					destVolume->setVoxel(targetPos + directions[i], voxel);
				}
				sampler3.movePositiveZ(1);
			}
			sampler2.movePositiveY(1);
		}
		sourceSampler.movePositiveX(1);
	}
	return destVolume;
}

} // namespace voxelutil
