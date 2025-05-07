/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "voxel/RawVolume.h"
#include "palette/Palette.h"
#include "core/Trace.h"
#include "core/Assert.h"
#include "voxel/Voxel.h"

namespace voxelutil {

/**
 * @brief Will skip air voxels on volume merges
 */
struct MergeSkipEmpty {
	inline bool operator() (voxel::Voxel& voxel) const {
		return !isAir(voxel.getMaterial());
	}
};

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 * @note The given merge condition function must return false for voxels that should be skipped.
 * @sa MergeSkipEmpty
 */
template<typename MergeCondition = MergeSkipEmpty, class Volume1, class Volume2>
int mergeVolumes(Volume1 *destination, const Volume2 *source, const voxel::Region &destReg,
				 const voxel::Region &sourceReg, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	typename Volume2::Sampler sourceSampler(source);
	typename Volume1::Sampler destSampler(destination);
	const int relX = destReg.getLowerX();
	sourceSampler.setPosition(sourceReg.getLowerCorner());
	for (int32_t z = sourceReg.getLowerZ(); z <= sourceReg.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
		typename Volume2::Sampler sourceSampler2 = sourceSampler;
		for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
			typename Volume2::Sampler sourceSampler3 = sourceSampler2;
			destSampler.setPosition(relX, destY, destZ);
			for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
				voxel::Voxel srcVoxel = sourceSampler3.voxel();
				sourceSampler3.movePositiveX();
				if (!mergeCondition(srcVoxel)) {
					destSampler.movePositiveX();
					continue;
				}
				if (destSampler.setVoxel(srcVoxel)) {
					++cnt;
				}
				destSampler.movePositiveX();
			}
			sourceSampler2.movePositiveY();
		}
		sourceSampler.movePositiveZ();
	}
	return cnt;
}

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 * @note The given merge condition function must return false for voxels that should be skipped.
 * @sa MergeSkipEmpty
 */
template<typename MergeCondition = MergeSkipEmpty, class Volume1, class Volume2>
int mergeVolumes(Volume1 *destination, const palette::Palette &destinationPalette, const Volume2 *source,
				 const palette::Palette &sourcePalette, const voxel::Region &destReg, const voxel::Region &sourceReg,
				 MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	typename Volume2::Sampler sourceSampler(source);
	typename Volume1::Sampler destSampler(destination);
	const int relX = destReg.getLowerX();
	sourceSampler.setPosition(sourceReg.getLowerCorner());
	for (int32_t z = sourceReg.getLowerZ(); z <= sourceReg.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
		typename Volume2::Sampler sourceSampler2 = sourceSampler;
		for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
			typename Volume2::Sampler sourceSampler3 = sourceSampler2;
			destSampler.setPosition(relX, destY, destZ);
			for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
				voxel::Voxel srcVoxel = sourceSampler3.voxel();
				sourceSampler3.movePositiveX();
				if (!mergeCondition(srcVoxel)) {
					destSampler.movePositiveX();
					continue;
				}
				int idx = destinationPalette.getClosestMatch(sourcePalette.color(srcVoxel.getColor()));
				if (idx == palette::PaletteColorNotFound) {
					idx = 0;
				}
				const voxel::Voxel destVoxel = voxel::createVoxel(destinationPalette, idx);
				if (destSampler.setVoxel(destVoxel)) {
					++cnt;
				}
				destSampler.movePositiveX();
			}
			sourceSampler2.movePositiveY();
		}
		sourceSampler.movePositiveZ();
	}
	return cnt;
}

/**
 * The given merge condition function must return false for voxels that should be skipped.
 * @sa MergeSkipEmpty
 */
template<typename MergeCondition = MergeSkipEmpty>
inline int mergeRawVolumesSameDimension(voxel::RawVolume* destination, const voxel::RawVolume* source, MergeCondition mergeCondition = MergeCondition()) {
	core_assert(source->region() == destination->region());
	return mergeVolumes(destination, source, destination->region(), source->region());
}

[[nodiscard]] voxel::RawVolume* merge(const core::Buffer<voxel::RawVolume*>& volumes);
[[nodiscard]] voxel::RawVolume* merge(const core::Buffer<const voxel::RawVolume*>& volumes);

}
