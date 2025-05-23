/**
 * @file
 */

#include "GimpPalette.h"
#include "core/Log.h"
#include "core/Var.h"
#include "engine-config.h"

namespace palette {

bool GimpPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	char line[2048];
	bool alpha = false;
	int colorCount = 0;
	while (stream.readLine(sizeof(line), line)) {
		if (strncmp("#Palette Name", line, 13) == 0) {
			// _name = line + 13;
			// if (_name[0] == ':') {
			// 	_name = _name.substr(1);
			// }
			// _name = _name.trim();
			// Log::debug("found name %s", _name.c_str());
			continue;
		}
		if (strncmp("Name", line, 4) == 0) {
			// _name = line + 4;
			// if (_name[0] == ':') {
			// 	_name = _name.substr(1);
			// }
			// _name = _name.trim();
			// Log::debug("found name %s", _name.c_str());
			continue;
		}
		if (line[0] == '#') {
			Log::debug("skip comment line: %s", line);
			continue;
		}
		if (strcmp("GIMP Palette", line) == 0) {
			continue;
		}
		// aseprite extension: https://github.com/aseprite/aseprite/blob/main/docs/gpl-palette-extension.md
		if (strcmp("Channels: RGBA", line) == 0) {
			alpha = true;
			Log::debug("Gimp palette with aseprite alpha channel extension found");
			continue;
		}

		int r, g, b, a = 255;
		if (alpha) {
			if (SDL_sscanf(line, "%i %i %i %i", &r, &g, &b, &a) != 4) {
				Log::error("Failed to parse line '%s'", line);
				continue;
			}
		} else if (SDL_sscanf(line, "%i %i %i", &r, &g, &b) != 3) {
			Log::error("Failed to parse line '%s'", line);
			continue;
		}
		if (colorCount >= PaletteMaxColors) {
			Log::warn("Not all colors were loaded");
			break;
		}
		palette.setColor(colorCount, core::RGBA(r, g, b, a));
		++colorCount;
	}
	palette.setSize(colorCount);
	return colorCount > 0;
}

bool GimpPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	stream.writeLine("GIMP Palette");
	stream.writeStringFormat(false, "Name: %s\n", palette.name().c_str());
	stream.writeStringFormat(false, "#Palette Name: %s\n", palette.name().c_str());
	stream.writeLine("# Generated by vengi " PROJECT_VERSION " github.com/vengi-voxel/vengi");
	const bool useRGBA = core::Var::getSafe(cfg::PalformatGimpRGBA)->boolVal();
	if (useRGBA) {
		stream.writeLine("Channels: RGBA");
	}
	for (size_t i = 0; i < palette.size(); ++i) {
		const core::RGBA &color = palette.color(i);
		core::String name = palette.colorName(i);
		if (name.empty()) {
			name = core::String::format("color index %i", (int)i);
		}
		if (useRGBA) {
			stream.writeStringFormat(false, "%3i %3i %3i %3i\t%s\n", color.r, color.g, color.b, color.a, name.c_str());
		} else {
			stream.writeStringFormat(false, "%3i %3i %3i\t%s\n", color.r, color.g, color.b, name.c_str());
		}
	}
	return true;
}

} // namespace voxel
