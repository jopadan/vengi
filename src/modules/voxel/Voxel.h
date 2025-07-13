/**
 * @file
 * @defgroup Voxel Voxel
 * @{
 */

#pragma once

#include <stdint.h>
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/RGBA.h"

namespace palette {
class Palette;
}

/**
 * Voxel manipulation, meshing and storage
 */
namespace voxel {

/**
 * @brief material types 0 - 31 (5 bits)
 * @note These must match the compute kernel source enum
 */
enum class VoxelType : uint8_t {
	// this must be 0
	Air = 0,
	Transparent,
	Generic,

	Max
};

#define NO_NORMAL 255u

static constexpr const char* VoxelTypeStr[] = {
	"Air",
	"Transparent",
	"Generic"
};
static_assert(lengthof(VoxelTypeStr) == (int)VoxelType::Max, "voxel type string array size doesn't match the available voxel types");

class Voxel {
public:
	CORE_FORCE_INLINE constexpr Voxel()
		: _material(VoxelType::Air), _flags(0), _unused(0), _colorIndex(0), _normalIndex(NO_NORMAL), _unused2(0) {
	}

	CORE_FORCE_INLINE constexpr Voxel(VoxelType material, uint8_t colorIndex, uint8_t normalIndex = NO_NORMAL, uint8_t flags = 0u)
		: _material(material), _flags(flags), _unused(0), _colorIndex(colorIndex), _normalIndex(normalIndex), _unused2(0) {
	}

	/**
	 * @brief Compares by the material type
	 */
	CORE_FORCE_INLINE bool operator==(const Voxel& rhs) const {
		return _material == rhs._material;
	}

	/**
	 * @brief Compares by the material type
	 */
	CORE_FORCE_INLINE bool operator!=(const Voxel& rhs) const {
		return !(*this == rhs);
	}

	CORE_FORCE_INLINE bool isSame(const Voxel& other) const {
		return _material == other._material && _colorIndex == other._colorIndex && _normalIndex == other._normalIndex;
	}

	/**
	 * @brief Compares by the material type
	 */
	CORE_FORCE_INLINE bool isSameType(const Voxel& other) const {
		return _material == other._material;
	}

	CORE_FORCE_INLINE uint8_t getColor() const {
		return _colorIndex;
	}

	CORE_FORCE_INLINE uint8_t getNormal() const {
		return _normalIndex;
	}

	CORE_FORCE_INLINE void setColor(uint8_t colorIndex) {
		_colorIndex = colorIndex;
	}

	CORE_FORCE_INLINE VoxelType getMaterial() const {
		return _material;
	}

	CORE_FORCE_INLINE void setMaterial(VoxelType material) {
		_material = material;
	}

	CORE_FORCE_INLINE uint8_t getFlags() const {
		return _flags;
	}

	void setFlags(uint8_t flags);

	void setOutline() {
		setFlags(1); // FlagOutline
	}

private:
	VoxelType _material:2;
	uint8_t _flags:1;
	uint8_t _unused:5; // VoxelVertex::padding
	uint8_t _colorIndex;
	uint8_t _normalIndex; // 255 is not set
public:
	uint8_t _unused2; // used to store the ambient occlusion value for the voxel in the VoxelVertex struct
};
static_assert(sizeof(Voxel) == 4, "Voxel size is expected to be 4 bytes");

CORE_NO_SANITIZE_ADDRESS constexpr Voxel createVoxel(VoxelType type, uint8_t colorIndex, uint8_t normalIndex = 0u, uint8_t flags = 0u) {
	return Voxel(type, colorIndex, normalIndex, flags);
}

voxel::Voxel createVoxelFromColor(const palette::Palette &pal, core::RGBA color);
CORE_NO_SANITIZE_ADDRESS voxel::Voxel createVoxel(const palette::Palette &pal, uint8_t index, uint8_t normalIndex = 0u, uint8_t flags = 0u);

CORE_FORCE_INLINE bool isBlocked(VoxelType material) {
	return material != VoxelType::Air;
}

CORE_FORCE_INLINE bool isAir(VoxelType material) {
	return material == VoxelType::Air;
}

CORE_FORCE_INLINE bool isTransparent(VoxelType material) {
	return material == VoxelType::Transparent;
}

}

/**
 * @}
 */
