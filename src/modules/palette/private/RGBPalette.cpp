/**
 * @file
 */

#include "RGBPalette.h"
#include "core/Log.h"
#include "core/Var.h"

namespace palette {

bool RGBPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	for (int i = 0; i < PaletteMaxColors; ++i) {
		core::RGBA color;
		if (stream.readUInt8(color.r) == -1) {
			Log::error("Failed to read color %i", i);
			return false;
		}
		if (stream.readUInt8(color.g) == -1) {
			Log::error("Failed to read color %i", i);
			return false;
		}
		if (stream.readUInt8(color.b) == -1) {
			Log::error("Failed to read color %i", i);
			return false;
		}
		color.a = 255;
		palette.setColor(i, color);
	}
	palette.setSize(PaletteMaxColors);

	int maxColor = 0;
	for (int i = 0; i < PaletteMaxColors; ++i) {
		const core::RGBA rgba = palette.color(i);
		maxColor = core_max(rgba.r, maxColor);
		maxColor = core_max(rgba.g, maxColor);
		maxColor = core_max(rgba.b, maxColor);
	}
	const bool is6Bit = maxColor <= 63;
	if (is6Bit) {
		if (core::Var::getSafe(cfg::PalformatRGB6Bit)->boolVal()) {
			const float scale = (255.0f / 63.0f);
			for (int i = 0; i < PaletteMaxColors; ++i) {
				core::RGBA rgba = palette.color(i);
				rgba.r = (float)rgba.r * scale;
				rgba.g = (float)rgba.g * scale;
				rgba.b = (float)rgba.b * scale;
				palette.setColor(i, rgba);
			}
		} else {
			Log::info("The palette colors are in a 6-bit range, you can set %s to true if your colors don't match-",
					  cfg::PalformatRGB6Bit);
		}
	}

	return true;
}

bool RGBPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	const bool to6Bit = core::Var::getSafe(cfg::PalformatRGB6Bit)->boolVal();
	const float scale = (255.0f / 63.0f);
	for (size_t i = 0; i < palette.size(); ++i) {
		core::RGBA color = palette.color(i);
		if (to6Bit) {
			stream.writeUInt8((uint8_t)((float)color.r / scale));
			stream.writeUInt8((uint8_t)((float)color.g / scale));
			stream.writeUInt8((uint8_t)((float)color.b / scale));
		} else {
			stream.writeUInt8(color.r);
			stream.writeUInt8(color.g);
			stream.writeUInt8(color.b);
		}
	}
	return true;
}

} // namespace palette
