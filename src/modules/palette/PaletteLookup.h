/**
 * @file
 */

#pragma once

#include "core/Color.h"
#include "core/collection/Map.h"
#include "palette/Palette.h"

namespace palette {

/**
 * @brief A lookup table for palette colors, allowing fast retrieval of the closest color index
 * from a given RGBA color value.
 *
 * This class uses a hash map to store the mapping between RGBA colors and their corresponding
 * palette indices, enabling efficient lookups.
 */
class PaletteLookup {
private:
	palette::Palette _palette;
	core::Map<core::RGBA, uint8_t, 521> _paletteMap;
public:
	PaletteLookup(const palette::Palette &palette, int maxSize = 32768) : _palette(palette), _paletteMap(maxSize) {
		if (_palette.colorCount() <= 0) {
			_palette.nippon();
		}
	}
	PaletteLookup(int maxSize = 32768) : _paletteMap(maxSize) {
		_palette.nippon();
	}

	inline const palette::Palette &palette() const {
		return _palette;
	}

	inline palette::Palette &palette() {
		return _palette;
	}

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @param color Normalized color value [0.0-1.0]
	 * @sa core::Color::getClosestMatch()
	 */
	inline uint8_t findClosestIndex(const glm::vec4 &color) {
		return findClosestIndex(core::Color::getRGBA(color));
	}

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @sa core::Color::getClosestMatch()
	 */
	uint8_t findClosestIndex(core::RGBA rgba) {
		uint8_t paletteIndex = 0;
		if (!_paletteMap.get(rgba, paletteIndex)) {
			paletteIndex = _palette.getClosestMatch(rgba);
			if (_paletteMap.size() < _paletteMap.capacity()) {
				_paletteMap.put(rgba, paletteIndex);
			}
		}
		return paletteIndex;
	}
};

} // namespace voxel
