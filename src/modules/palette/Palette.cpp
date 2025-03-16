/**
 * @file
 */

#include "Palette.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Endian.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/StandardLib.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/Set.h"
#include "engine-config.h"
#include "http/HttpCacheStream.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "math/Math.h"
#include "palette/private/GimpPalette.h"
#include "private/PaletteFormat.h"

#include <float.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>

namespace palette {

Palette::Palette() : _view(this) {
}

const char *Palette::getDefaultPaletteName() {
	return builtIn[0];
}

bool Palette::hasMaterials() const {
	for (int i = 0; i < _colorCount; ++i) {
		if (_materials[i].mask != MaterialNone) {
			return true;
		}
	}
	return false;
}

void Palette::fill() {
	for (int i = _colorCount; i < PaletteMaxColors; ++i) {
		_colors[i] = core::RGBA(64, 64, 64, 255);
	}
	_colorCount = PaletteMaxColors;
}

int Palette::changeSize(int delta) {
	_colorCount = glm::clamp(_colorCount + delta, 0, PaletteMaxColors);
	return _colorCount;
}

void Palette::setSize(int cnt) {
	_colorCount = glm::clamp(cnt, 0, PaletteMaxColors);
}

void Palette::markDirty() {
	core::DirtyState::markDirty();
	_hash._hashColors[0] = core::hash(_colors, sizeof(_colors));
	_hash._hashColors[1] = core::hash(_materials, sizeof(_materials));
}

glm::vec4 Palette::color4(uint8_t i) const {
	return core::Color::fromRGBA(color(i));
}

void Palette::reduce(uint8_t targetColors) {
	core::Color::ColorReductionType reductionType =
		core::Color::toColorReductionType(core::Var::getSafe(cfg::CoreColorReduction)->strVal().c_str());
	PaletteColorArray oldcolors;
	core_memcpy(oldcolors, _colors, sizeof(PaletteColorArray));
	_colorCount = core::Color::quantize(_colors, targetColors, oldcolors, _colorCount, reductionType);
	markDirty();
}

void Palette::quantize(const core::RGBA *inputColors, const size_t inputColorCount) {
	Log::debug("quantize %i colors", (int)inputColorCount);
	core::Color::ColorReductionType reductionType =
		core::Color::toColorReductionType(core::Var::getSafe(cfg::CoreColorReduction)->strVal().c_str());
	_colorCount = core::Color::quantize(_colors, lengthof(_colors), inputColors, inputColorCount, reductionType);
	markDirty();
}

bool Palette::hasColor(core::RGBA rgba) {
	for (int i = 0; i < _colorCount; ++i) {
		if (_colors[i] == rgba) {
			return true;
		}
	}
	return false;
}

void Palette::duplicateColor(uint8_t paletteColorIdx) {
	if (_colorCount < PaletteMaxColors) {
		setColor(_colorCount + 1, color(paletteColorIdx));
	} else {
		// search a free slot
		for (int i = 0; i < PaletteMaxColors; ++i) {
			if (!hasAlpha(i)) {
				setColor(i, color(paletteColorIdx));
				return;
			}
		}
	}
}

void Palette::exchange(uint8_t paletteColorIdx1, uint8_t paletteColorIdx2) {
	if (paletteColorIdx1 == paletteColorIdx2) {
		return;
	}
	const core::RGBA lhs = color(paletteColorIdx1);
	const palette::Material lhsM = material(paletteColorIdx1);
	const core::RGBA rhs = color(paletteColorIdx2);
	const palette::Material rhsM = material(paletteColorIdx2);
	setColor(paletteColorIdx1, rhs);
	setMaterial(paletteColorIdx1, rhsM);
	setColor(paletteColorIdx2, lhs);
	setMaterial(paletteColorIdx2, lhsM);
	markSave();
	markDirty();
}

void Palette::copy(uint8_t fromPaletteColorIdx, uint8_t toPaletteColorIdx) {
	if (fromPaletteColorIdx == toPaletteColorIdx) {
		return;
	}
	_colors[toPaletteColorIdx] = _colors[fromPaletteColorIdx];
	_materials[toPaletteColorIdx] = _materials[fromPaletteColorIdx];
	markDirty();
	markSave();
}

bool Palette::removeColor(uint8_t paletteColorIdx) {
	if (paletteColorIdx < _colorCount && _colorCount > 1) {
		for (int i = paletteColorIdx; i < _colorCount - 1; ++i) {
			_view._uiIndices[i] = _view._uiIndices[i + 1];
		}
		_colors[paletteColorIdx] = core::RGBA(0, 0, 0, 0);
		if (paletteColorIdx == _colorCount - 1) {
			--_colorCount;
		}
		markDirty();
		return true;
	}
	return false;
}

bool Palette::hasFreeSlot() const {
	if (_colorCount < PaletteMaxColors) {
		return true;
	}
	// search a free slot
	for (int i = 0; i < PaletteMaxColors; ++i) {
		if (color(i).a == 0) {
			return true;
		}
	}
	return false;
}

void Palette::setColor(uint8_t i, const core::RGBA &rgba) {
	_colors[i] = rgba;
	if (rgba.a != 0) {
		setSize(core_max((int)size(), i + 1));
	}
	markDirty();
}

void Palette::setMaterial(uint8_t i, const Material &material) {
	_materials[i] = material;
	markDirty();
}

int Palette::findInsignificant(int skipSlotIndex) const {
	int bestIndex = PaletteColorNotFound;
	float bestColorDistance = FLT_MAX;
	for (int i = 0; i < _colorCount; ++i) {
		if (i == skipSlotIndex) {
			continue;
		}

		float minDistance = FLT_MAX;
		int closestColorIdx = PaletteColorNotFound;

		for (int k = 0; k < _colorCount; ++k) {
			if (k == i) {
				continue;
			}
			if (_colors[k].a == 0) {
				continue;
			}
			const float val = core::Color::getDistance(_colors[k], _colors[i], core::Color::Distance::Approximation);
			if (val < minDistance) {
				minDistance = val;
				closestColorIdx = (int)i;
				// if colors are more or less equal, just stop the search
				if (minDistance <= 0.00001f) {
					break;
				}
			}
		}
		if (minDistance < bestColorDistance) {
			bestColorDistance = minDistance;
			bestIndex = closestColorIdx;
			// if colors are more or less equal, just stop the search
			if (bestColorDistance <= 0.00001f) {
				break;
			}
		}
	}
	return bestIndex;
}

bool Palette::tryAdd(core::RGBA rgba, bool skipSimilar, uint8_t *index, bool replaceSimilar, int skipPaletteColorIdx) {
	for (int i = 0; i < _colorCount; ++i) {
		if (_colors[i] == rgba) {
			if (index) {
				*index = i;
			}
			return false;
		}
	}
	static constexpr float MaxHSBThreshold = 0.00014f;
	if (skipSimilar) {
		for (int i = 0; i < _colorCount; ++i) {
			if (abs(_colors[i].a - rgba.a) > 10) {
				continue;
			}
			const float dist = core::Color::getDistance(_colors[i], rgba, core::Color::Distance::HSB);
			if (dist < MaxHSBThreshold) {
				if (index) {
					*index = i;
				}
				return false;
			}
		}
	}

	if (_colorCount == skipPaletteColorIdx && _colorCount < PaletteMaxColors) {
		if (rgba.a != 0) {
			++_colorCount;
		}
	}

	if (_colorCount < PaletteMaxColors) {
		if (index) {
			*index = _colorCount;
		}
		_colors[_colorCount++] = rgba;
		return true;
	}

	for (int i = 0; i < _colorCount; ++i) {
		if (_colors[i].a == 0) {
			if (index) {
				*index = i;
			}
			_colors[i] = rgba;
			return true;
		}
	}

	if (replaceSimilar) {
		// now we are looking for the color in the existing palette entries that is most similar
		// to other entries in the palette. If this entry is than above a certain threshold, we
		// will replace that color with the new rgba value
		int bestIndex = findInsignificant(skipPaletteColorIdx);
		if (bestIndex != PaletteColorNotFound) {
			const float dist = core::Color::getDistance(_colors[bestIndex], rgba, core::Color::Distance::HSB);
			if (dist > MaxHSBThreshold) {
				if (index) {
					*index = bestIndex;
				}
				_colors[bestIndex] = rgba;
				return true;
			}
		}
		if (index) {
			*index = 0;
		}
	}
	return false;
}

core::String Palette::print(const Palette &palette, bool colorAsHex) {
	if (palette._colorCount == 0) {
		return "no colors";
	}
	core::String palStr;
	core::String line;
	for (int i = 0; i < palette._colorCount; ++i) {
		if (i % 16 == 0 && !line.empty()) {
			palStr.append(core::string::format("%03i %s\n", i - 16, line.c_str()));
			line = "";
		}
		const core::String c = core::Color::print(palette._colors[i], colorAsHex);
		line += c;
	}
	if (!line.empty()) {
		palStr.append(core::string::format("%03i %s\n", (palette._colorCount - 1) / 16 * 16, line.c_str()));
	}
	return palStr;
}

int Palette::getClosestMatch(core::RGBA rgba, int skipPaletteColorIdx) const {
	if (size() == 0) {
		return PaletteColorNotFound;
	}
	for (int i = 0; i < _colorCount; ++i) {
		if (i == skipPaletteColorIdx) {
			continue;
		}
		if (_colors[i] == rgba) {
			return i;
		}
	}

	if (rgba.a == 0) {
		for (int i = 0; i < _colorCount; ++i) {
			if (_colors[i].a == 0) {
				return i;
			}
		}
		return PaletteColorNotFound;
	}

	float minDistance = FLT_MAX;
	int minIndex = PaletteColorNotFound;

	for (int i = 0; i < _colorCount; ++i) {
		if (i == skipPaletteColorIdx) {
			continue;
		}
		if (_colors[i].a == 0) {
			continue;
		}
		const float val = core::Color::getDistance(_colors[i], rgba, core::Color::Distance::Approximation);
		if (val < minDistance) {
			minDistance = val;
			minIndex = (int)i;
		}
	}
	return minIndex;
}

uint8_t Palette::findReplacement(uint8_t paletteColorIdx) const {
	if (size() == 0) {
		return paletteColorIdx;
	}
	const core::RGBA rgba = color(paletteColorIdx);
	const int skip = paletteColorIdx;
	for (int i = 0; i < _colorCount; ++i) {
		if (i == skip) {
			continue;
		}
		if (_colors[i] == rgba) {
			return i;
		}
	}

	if (rgba.a == 0) {
		for (int i = 0; i < _colorCount; ++i) {
			if (_colors[i].a == 0) {
				return i;
			}
		}
		return paletteColorIdx;
	}

	float minDistance = FLT_MAX;
	int minIndex = paletteColorIdx;

	float hue;
	float saturation;
	float brightness;
	const glm::vec4 color = core::Color::fromRGBA(rgba);
	core::Color::getHSB(color, hue, saturation, brightness);

	for (int i = 0; i < _colorCount; ++i) {
		if (i == skip) {
			continue;
		}
		if (_colors[i].a == 0) {
			continue;
		}
		const float val = core::Color::getDistance(_colors[i], hue, saturation, brightness);
		if (val < minDistance) {
			minDistance = val;
			minIndex = (int)i;
		}
	}
	return minIndex;
}

void Palette::changeIntensity(float scale) {
	const float f = glm::abs(scale) + 1.0f;
	for (int i = 0; i < _colorCount; ++i) {
		const glm::vec4 &color = core::Color::fromRGBA(_colors[i]);
		const glm::vec4 &newColor = scale < 0.0f ? core::Color::darker(color, f) : core::Color::brighter(color, f);
		_colors[i] = core::Color::getRGBA(newColor);
	}
	markDirty();
	markSave();
}

bool Palette::save(const char *name) const {
	if (name == nullptr || name[0] == '\0') {
		if (_name.empty()) {
			Log::error("No name given to save the current palette");
			return false;
		}
		name = _name.c_str();
	}
	const core::String ext = core::string::extractExtension(name);
	if (ext.empty()) {
		Log::error("No extension found for %s - can't determine the palette format", name);
		return false;
	}
	const io::FilePtr &file = io::filesystem()->open(name, io::FileMode::SysWrite);
	io::FileStream stream(file);
	if (!stream.valid()) {
		Log::error("Failed to open file %s for writing", name);
		return false;
	}
	return palette::savePalette(*this, name, stream);
}

bool Palette::load(const uint8_t *rgbaBuf, size_t bufsize, const char *name) {
	if (bufsize % 4 != 0) {
		Log::warn("Buf size doesn't match expectation: %i", (int)bufsize);
	}
	int ncolors = (int)bufsize / 4;
	if (ncolors <= 0) {
		Log::error("Buffer is not big enough: %i bytes", (int)bufsize);
		return false;
	}
	if (ncolors > PaletteMaxColors) {
		Log::warn("Too many colors given for palette.");
	}
	ncolors = core_min(ncolors, PaletteMaxColors);
	image::ImagePtr img = image::createEmptyImage(name);
	if (!img->loadRGBA(rgbaBuf, ncolors, 1)) {
		return false;
	}
	_name = "";
	return load(img);
}

bool Palette::load(const image::ImagePtr &img) {
	if (img->components() != 4) {
		Log::warn("Palette image has invalid depth (expected: 4bpp, got %i)", img->components());
		return false;
	}
	for (auto &m : _materials) {
		m = Material();
	}
	if (img->width() * img->height() > PaletteMaxColors) {
		return createPalette(img, *this);
	}
	int ncolors = img->width();
	if (ncolors > PaletteMaxColors) {
		ncolors = PaletteMaxColors;
		Log::warn("Palette image has invalid dimensions - we need max 256x1(depth: 4)");
	}
	_colorCount = ncolors;
	for (int i = 0; i < _colorCount; ++i) {
		_colors[i] = img->colorAt(i, 0);
	}
	for (int i = _colorCount; i < PaletteMaxColors; ++i) {
		_colors[i] = core::RGBA(0);
	}
	_name = img->name();
	markDirty();
	Log::debug("Set up %i material colors", _colorCount);
	return true;
}

bool Palette::loadLospec(const core::String &lospecId, const core::String &gimpPalette) {
	const core::String url = "https://lospec.com/palette-list/" + gimpPalette;
	http::HttpCacheStream cacheStream(io::openFilesystemArchive(io::filesystem()), gimpPalette, url);
	if (cacheStream.size() <= 0) {
		Log::warn("Failed to load lospec palette %s", gimpPalette.c_str());
		return false;
	}
	return palette::loadPalette(gimpPalette, cacheStream, *this);
}

bool Palette::load(const char *paletteName) {
	if (paletteName == nullptr || paletteName[0] == '\0') {
		return false;
	}

	if (SDL_strncmp(paletteName, "lospec:", 7) == 0) {
		const core::String lospecId = paletteName + 7;
		const core::String gimpPalette = lospecId + GimpPalette::format().mainExtension(true);
		return loadLospec(lospecId, gimpPalette);
	}

	// this is handled in the scene manager it is just ignored here
	if (SDL_strncmp(paletteName, "node:", 5) == 0) {
		if (_colorCount == 0) {
			nippon();
		}
		_name = paletteName + 5;
		return false;
	}

	if (SDL_strcmp(paletteName, builtIn[0]) == 0) {
		return nippon();
	} else if (SDL_strcmp(paletteName, builtIn[1]) == 0) {
		return minecraft();
	} else if (SDL_strcmp(paletteName, builtIn[2]) == 0) {
		return magicaVoxel();
	} else if (SDL_strcmp(paletteName, builtIn[3]) == 0) {
		return quake1();
	} else if (SDL_strcmp(paletteName, builtIn[4]) == 0) {
		return commandAndConquer();
	} else if (SDL_strcmp(paletteName, builtIn[5]) == 0) {
		return starMade();
	}
	static_assert(lengthof(builtIn) == 6, "Unexpected amount of built-in palettes");

	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr paletteFile = filesystem->open(paletteName);
	if (!paletteFile->validHandle()) {
		paletteFile = filesystem->open(core::string::format("palette-%s.png", paletteName));
		if (!paletteFile->validHandle()) {
			Log::error("Failed to load palette file %s", paletteName);
			return false;
		}
	}
	io::FileStream stream(paletteFile);
	if (!stream.valid()) {
		Log::error("Failed to load image %s", paletteFile->name().c_str());
		return false;
	}

	if (!palette::loadPalette(paletteFile->name(), stream, *this)) {
		const image::ImagePtr &img = image::loadImage(paletteFile);
		if (!img->isLoaded()) {
			Log::error("Failed to load image %s", paletteFile->name().c_str());
			return false;
		}
		return load(img);
	}
	return true;
}

bool Palette::isBuiltIn() const {
	for (int i = 0; i < lengthof(builtIn); ++i) {
		if (_name.equals(builtIn[i])) {
			return true;
		}
	}
	return false;
}

bool Palette::minecraft() {
	uint32_t palette[] = {
		0xff000000, 0xff7d7d7d, 0xff4cb376, 0xff436086, 0xff7a7a7a, 0xff4e7f9c, 0xff256647, 0xff535353, 0xffdcaf70,
		0xffdcaf70, 0xff135bcf, 0xff125ad4, 0xffa0d3db, 0xff7a7c7e, 0xff7c8b8f, 0xff7e8287, 0xff737373, 0xff315166,
		0xff31b245, 0xff54c3c2, 0xfff4f0da, 0xff867066, 0xff894326, 0xff838383, 0xff9fd3dc, 0xff324364, 0xff3634b4,
		0xff23c7f6, 0xff7c7c7c, 0xff77bf8e, 0xffdcdcdc, 0xff296595, 0xff194f7b, 0xff538ba5, 0xff5e96bd, 0xffdddddd,
		0xffe5e5e5, 0xff00ffff, 0xff0d00da, 0xff415778, 0xff0d0fe1, 0xff4eecf9, 0xffdbdbdb, 0xffa1a1a1, 0xffa6a6a6,
		0xff0630bc, 0xff0026af, 0xff39586b, 0xff658765, 0xff1d1214, 0xff00ffff, 0xff005fde, 0xff31271a, 0xff4e87a6,
		0xff2a74a4, 0xff0000ff, 0xff8f8c81, 0xffd5db61, 0xff2e5088, 0xff17593c, 0xff335682, 0xff676767, 0xff00b9ff,
		0xff5b9ab8, 0xff387394, 0xff345f79, 0xff5190b6, 0xff6a6a6a, 0xff5b9ab8, 0xff40596a, 0xff7a7a7a, 0xffc2c2c2,
		0xff65a0c9, 0xff6b6b84, 0xff2d2ddd, 0xff000066, 0xff0061ff, 0xff848484, 0xfff1f1df, 0xffffad7d, 0xfffbfbef,
		0xff1d830f, 0xffb0a49e, 0xff65c094, 0xff3b5985, 0xff42748d, 0xff1b8ce3, 0xff34366f, 0xff334054, 0xff45768f,
		0xffbf0a57, 0xff2198f1, 0xffffffec, 0xffb2b2b2, 0xffb2b2b2, 0xffffffff, 0xff2d5d7e, 0xff7c7c7c, 0xff7a7a7a,
		0xff7cafcf, 0xff78aaca, 0xff6a6c6d, 0xfff4efd3, 0xff28bdc4, 0xff69dd92, 0xff53ae73, 0xff0c5120, 0xff5287a5,
		0xff2a4094, 0xff7a7a7a, 0xff75718a, 0xff767676, 0xff1a162c, 0xff1a162c, 0xff1a162c, 0xff2d28a6, 0xffb1c454,
		0xff51677c, 0xff494949, 0xff343434, 0xffd18934, 0xffa5dfdd, 0xff0f090c, 0xff316397, 0xff42a0e3, 0xff4d84a1,
		0xff49859e, 0xff1f71dd, 0xffa8e2e7, 0xff74806d, 0xff3c3a2a, 0xff7c7c7c, 0xff5a5a5a, 0xff75d951, 0xff345e81,
		0xff84c0ce, 0xff455f88, 0xff868b8e, 0xffd7dd74, 0xff595959, 0xff334176, 0xff008c0a, 0xff17a404, 0xff5992b3,
		0xffb0b0b0, 0xff434347, 0xff1d6b9e, 0xff70fdfe, 0xffe5e5e5, 0xff4c4a4b, 0xffbdc6bf, 0xffddedfb, 0xff091bab,
		0xff4f547d, 0xff717171, 0xffdfe6ea, 0xffe3e8eb, 0xff41819b, 0xff747474, 0xffa1b2d1, 0xfff6f6f6, 0xff878787,
		0xff395ab0, 0xff325cac, 0xff152c47, 0xff65c878, 0xff3534df, 0xffc7c7c7, 0xffa5af72, 0xffbec7ac, 0xff9fd3dc,
		0xffcacaca, 0xff425c96, 0xff121212, 0xfff4bfa2, 0xff1474cf, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff1d56ac,
		0xff1d57ae, 0xff1d57ae, 0xff1d57ae, 0xff243c50, 0xff8dcddd, 0xff4d7aaf, 0xff0e2034, 0xff366bcf, 0xff355d7e,
		0xff7bb8c7, 0xff5f86bb, 0xff1e2e3f, 0xff3a6bc5, 0xff30536e, 0xffe0f3f7, 0xff5077a9, 0xff2955aa, 0xff21374e,
		0xffcdc5dc, 0xff603b60, 0xff856785, 0xffa679a6, 0xffaa7eaa, 0xffa879a8, 0xffa879a8, 0xffa879a8, 0xffaae6e1,
		0xffaae6e1, 0xff457d98, 0xff613f94, 0xff997f4c, 0xffb23f7f, 0xff19cc7f, 0xffa57ff2, 0xffff4040, 0xff5c5c57,
		0xff5c3e4c, 0xff1d195c, 0xff867e16, 0xff2a524c, 0xff85b414, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff242132};

	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = core_swap32le(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette), "minecraft");
}

bool Palette::magicaVoxel() {
	uint32_t palette[] = {
		0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff,
		0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff, 0xff6699ff, 0xff3399ff, 0xff0099ff,
		0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff,
		0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff, 0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff,
		0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc,
		0xff66cccc, 0xff33cccc, 0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc,
		0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
		0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc,
		0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99, 0xffcccc99, 0xff99cc99,
		0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999,
		0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699, 0xff006699, 0xffff3399, 0xffcc3399, 0xff993399,
		0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099,
		0xffffff66, 0xffccff66, 0xff99ff66, 0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66,
		0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966,
		0xffff6666, 0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366,
		0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066, 0xff000066,
		0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33,
		0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff669933, 0xff339933, 0xff009933,
		0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333,
		0xff663333, 0xff333333, 0xff003333, 0xffff0033, 0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033,
		0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00,
		0xff66cc00, 0xff33cc00, 0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900,
		0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
		0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee,
		0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044, 0xff000022, 0xff000011,
		0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200,
		0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000, 0xff880000, 0xff770000, 0xff550000, 0xff440000,
		0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555,
		0xff444444, 0xff222222, 0xff111111};

	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = core_swap32le(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette), "magicavoxel");
}

bool Palette::commandAndConquer() {
	uint32_t palette[] = {
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff,
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc0000ff, 0xec0000ff,
		0xdc0000ff, 0xd00000ff, 0xc00000ff, 0xb00000ff, 0xa40000ff, 0x940000ff, 0x840000ff, 0x780000ff, 0x680000ff,
		0x580000ff, 0x4c0000ff, 0x3c0000ff, 0x2c0000ff, 0x200000ff, 0xfcfcfcff, 0xf4f4f4ff, 0xecececff, 0xe0e0e0ff,
		0xd8d8d8ff, 0xd0d0d0ff, 0xc8c8c8ff, 0xc0c0c0ff, 0xb8b8b8ff, 0xb0b0b0ff, 0xa8a8a8ff, 0xa0a0a0ff, 0x989898ff,
		0x909090ff, 0x888888ff, 0x808080ff, 0x787878ff, 0x707070ff, 0x686868ff, 0x606060ff, 0x545454ff, 0x4c4c4cff,
		0x444444ff, 0x3c3c3cff, 0x343434ff, 0x2c2c2cff, 0x242424ff, 0x1c1c1cff, 0x141414ff, 0x0c0c0cff, 0x040404ff,
		0x000000ff, 0xd0d0b8ff, 0xc4c4acff, 0xb8b8a0ff, 0xacac94ff, 0xa0a088ff, 0x94947cff, 0x888870ff, 0x7c7c64ff,
		0x707058ff, 0x64644cff, 0x585840ff, 0x4c4c34ff, 0x404028ff, 0x34341cff, 0x282810ff, 0x1c1c04ff, 0xdcdcf4ff,
		0xd0d0e8ff, 0xc4c4dcff, 0xb8b8d0ff, 0xacacc4ff, 0xa0a0b8ff, 0x9494acff, 0x8888a0ff, 0x7c7c94ff, 0x707088ff,
		0x64647cff, 0x585870ff, 0x4c4c64ff, 0x404058ff, 0x34344cff, 0x282840ff, 0xecbcacff, 0xe0b0a0ff, 0xd4a494ff,
		0xc89888ff, 0xbc8c7cff, 0xb08070ff, 0xa47464ff, 0x986858ff, 0x8c5c4cff, 0x805040ff, 0x744434ff, 0x683828ff,
		0x5c2c1cff, 0x502010ff, 0x441404ff, 0x340400ff, 0x888058ff, 0x807854ff, 0x787450ff, 0x746c4cff, 0x706848ff,
		0x686044ff, 0x605840ff, 0x58503cff, 0x504838ff, 0x484030ff, 0x383828ff, 0x303024ff, 0x2c2c20ff, 0x28281cff,
		0x202014ff, 0x18180cff, 0xd4bc78ff, 0xccb470ff, 0xc4ac70ff, 0xbca468ff, 0xac9c68ff, 0xa49460ff, 0x9c8c58ff,
		0x948450ff, 0x8c7850ff, 0x847048ff, 0x786840ff, 0x70603cff, 0x686048ff, 0x645c44ff, 0x605840ff, 0x5c5440ff,
		0xbc903cff, 0xb08c38ff, 0xa48438ff, 0x987838ff, 0x8c7038ff, 0x846c38ff, 0x786438ff, 0x705c30ff, 0x6c582cff,
		0x68542cff, 0x645028ff, 0x604c28ff, 0x5c4828ff, 0x584828ff, 0x544428ff, 0x4c4028ff, 0x443824ff, 0x3c3420ff,
		0x342c20ff, 0x2c241cff, 0x28201cff, 0x241c1cff, 0x201c1cff, 0x181818ff, 0x88bc74ff, 0x74ac60ff, 0x609c4cff,
		0x50903cff, 0x44802cff, 0x347420ff, 0x286414ff, 0x20580cff, 0xfcfc70ff, 0xfcf46cff, 0xfce868ff, 0xfce064ff,
		0xfcd860ff, 0xfcd060ff, 0xfcc058ff, 0xf4b450ff, 0xecac48ff, 0xe49430ff, 0xd47810ff, 0xc46000ff, 0xb44800ff,
		0xa43800ff, 0x982800ff, 0x881800ff, 0xb0b0fcff, 0x9494e4ff, 0x7c7cccff, 0x6868b4ff, 0x54549cff, 0x404084ff,
		0x30306cff, 0x242458ff, 0xfc0000ff, 0xd00000ff, 0xa40000ff, 0x7c0000ff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff,
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff,
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff,
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff,
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff,
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff,
		0xfc00fcff, 0xfc00fcff, 0xfc00fcff, 0xfc00fcff};
	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = core_swap32be(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette), "commandandconquer");
}

bool Palette::starMade() {
	uint32_t palette[] = {
		0x939396FF, 0xE8D5CEFF, 0xFEFD84FF, 0xFB4825FF, 0x7AB740FF, 0x88C240FF, 0x72543AFF, 0x436030FF, 0xFFFFFFFF,
		0xB97D4DFF, 0xEED689FF, 0x9D5BA9FF, 0x47395DFF, 0x724676FF, 0xE57667FF, 0x2E4B68FF, 0xAA4D40FF, 0xF67F6EFF,
		0x9EDCF7FF, 0x88BDD6FF, 0xAB8459FF, 0x78AD50FF, 0xD4FEFFFF, 0x624B34FF, 0x405433FF, 0xFAB8B9FF, 0xFFFFFFFF,
		0x3E3E41FF, 0x242429FF, 0x000000FF, 0xFDDB90FF, 0xFDDB99FF, 0x573F3FFF, 0xFDDA56FF, 0xFDDA5DFF, 0xFDDA65FF,
		0xFDDA6DFF, 0xFDDA76FF, 0xFDDA7EFF, 0xFDDA87FF, 0xF9262DFF, 0x232323FF, 0xC2C2C2FF, 0x7C1A79FF, 0x2255F4FF,
		0x177B1EFF, 0xFEF83BFF, 0xFC8C2BFF, 0xF92932FF, 0x2C2C2CFF, 0xCCCCCCFF, 0x7C1F79FF, 0x265BF4FF, 0x1D7B23FF,
		0xFEFB40FF, 0xFC912EFF, 0xF92D3AFF, 0x353535FF, 0xD6D6D6FF, 0x7C2479FF, 0x2A60F5FF, 0x227B2AFF, 0xFEFB47FF,
		0xFC9632FF, 0xF93241FF, 0x3E3E3EFF, 0xE0E0E0FF, 0x7C2A79FF, 0x3064F5FF, 0x297B31FF, 0xFEFB53FF, 0xFC9B37FF,
		0xF93949FF, 0x484848FF, 0xEAEAEAFF, 0x7C3179FF, 0x3768F5FF, 0x317B39FF, 0xFEFB60FF, 0xFC9F3DFF, 0xF94052FF,
		0x515151FF, 0xF5F5F5FF, 0x7C3979FF, 0x3E6DF5FF, 0x397C41FF, 0xFEFB70FF, 0xFCA443FF, 0xF9475AFF, 0x5A5A5AFF,
		0xFFFFFFFF, 0x7C4177FF, 0x4671F5FF, 0x417C4AFF, 0xFEF880FF, 0xFCA94BFF, 0x7A908FFF, 0x57A69DFF, 0xF9DBB5FF,
		0x2EBDFBFF, 0xA63E3CFF, 0x38F3FDFF, 0x195A26FF, 0xD97D25FF, 0x439AA3FF, 0x39FDFEFF, 0x73FDFEFF, 0x707070FF,
		0x5A5A5AFF, 0x5D5D5CFF, 0x242428FF, 0x242429FF, 0x794135FF, 0x2A2A2AFF, 0x999999FF, 0x5D4C6EFF, 0x3E6B80FF,
		0x477A37FF, 0x7C743FFF, 0x88663AFF, 0x7A4372FF, 0x3F807FFF, 0xFFFFFFFF, 0xFB7031FF, 0x6C6C6CFF, 0xFDF13AFF,
		0xFCA135FF, 0xED794AFF, 0x2BC7B1FF, 0x51BFCEFF, 0xE1DA67FF, 0x436465FF, 0xFDD446FF, 0x78B580FF, 0x8D8C8BFF,
		0x8E853EFF, 0x43FB5DFF, 0x2CD0A7FF, 0x95FCFEFF, 0xFDBF80FF, 0xFDA7CCFF, 0x8FFEFEFF, 0x5EC0FCFF, 0x28A840FF,
		0x31B244FF, 0x1F947AFF, 0x38FAFEFF, 0xFC8E30FF, 0xFC6CA9FF, 0x51FDFEFF, 0x315D78FF, 0x154920FF, 0x1F8153FF,
		0xEE942AFF, 0x9160EBFF, 0x33E3F0FF, 0xCB4A47FF, 0xD167FCFF, 0xFDDB36FF, 0x2A9EE2FF, 0x17583CFF, 0xB67321FF,
		0x6B46A8FF, 0x26A9B0FF, 0x732E2CFF, 0x9A4EB8FF, 0xB9A228FF, 0x1B6389FF, 0x278EC4FF, 0x5EA1C5FF, 0xC48A28FF,
		0xC5A56DFF, 0x27B4ABFF, 0x69B5AEFF, 0xC3604CFF, 0xC4978FFF, 0x2E5E5BFF, 0x8DFDD2FF, 0x904939FF, 0xFDEA67FF,
		0xC59782FF, 0x245D5CFF, 0x38FDFEFF, 0xFB7D3EFF, 0x439EA9FF, 0x45FDFEFF, 0xBC724DFF, 0x43D067FF, 0xFBC433FF,
		0x915A54FF, 0x000000FF, 0x000000FF, 0x7A272CFF, 0x1C1C1CFF, 0xA9A9B2FF, 0x532750FF, 0x2A367FFF, 0x2F6033FF,
		0x807D40FF, 0x85613BFF, 0x883F5FFF, 0x184C4CFF, 0x713C23FF, 0x24252AFF, 0x18181DFF, 0x464646FF, 0xA0FEFEFF,
		0x727272FF, 0xA0181EFF, 0x242424FF, 0xC2C2CBFF, 0x621D60FF, 0x1629A4FF, 0x237B2AFF, 0xACA83CFF, 0xA76C2DFF,
		0xAC4E78FF, 0x1C5E5EFF, 0x8A4829FF, 0x515151FF, 0xB2A9A9FF, 0xFB2867FF, 0x277DFBFF, 0xCFFC39FF, 0xCC1F25FF,
		0x2C2C2CFF, 0xDBDBE5FF, 0x7C247BFF, 0x1B31CCFF, 0x2BA134FF, 0xCFCB47FF, 0xD18637FF, 0xD86195FF, 0x217070FF,
		0xA85731FF, 0x646464FF, 0xD1C7C7FF, 0x000000FF, 0xAFA327FF, 0x27602AFF, 0xF9262DFF, 0x353535FF, 0xF5F5FFFF,
		0x9B2B96FF, 0x203BF4FF, 0x32BD3CFF, 0xFEFB56FF, 0xFBA140FF, 0xFA6FAFFF, 0x247D7DFF, 0xCC6A39FF, 0x777777FF,
		0xEFE5E5FF, 0xF56B46FF, 0xA0FEFEFF, 0x00000000};
	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = core_swap32be(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette), "starmade");
}

bool Palette::quake1() {
	uint32_t palette[] = {
		0x000000ff, 0x0f0f0fff, 0x1f1f1fff, 0x2f2f2fff, 0x3f3f3fff, 0x4b4b4bff, 0x5b5b5bff, 0x6b6b6bff, 0x7b7b7bff,
		0x8b8b8bff, 0x9b9b9bff, 0xabababff, 0xbbbbbbff, 0xcbcbcbff, 0xdbdbdbff, 0xebebebff, 0x0f0b07ff, 0x170f0bff,
		0x1f170bff, 0x271b0fff, 0x2f2313ff, 0x372b17ff, 0x3f2f17ff, 0x4b371bff, 0x533b1bff, 0x5b431fff, 0x634b1fff,
		0x6b531fff, 0x73571fff, 0x7b5f23ff, 0x836723ff, 0x8f6f23ff, 0x0b0b0fff, 0x13131bff, 0x1b1b27ff, 0x272733ff,
		0x2f2f3fff, 0x37374bff, 0x3f3f57ff, 0x474767ff, 0x4f4f73ff, 0x5b5b7fff, 0x63638bff, 0x6b6b97ff, 0x7373a3ff,
		0x7b7bafff, 0x8383bbff, 0x8b8bcbff, 0x000000ff, 0x070700ff, 0x0b0b00ff, 0x131300ff, 0x1b1b00ff, 0x232300ff,
		0x2b2b07ff, 0x2f2f07ff, 0x373707ff, 0x3f3f07ff, 0x474707ff, 0x4b4b0bff, 0x53530bff, 0x5b5b0bff, 0x63630bff,
		0x6b6b0fff, 0x070000ff, 0x0f0000ff, 0x170000ff, 0x1f0000ff, 0x270000ff, 0x2f0000ff, 0x370000ff, 0x3f0000ff,
		0x470000ff, 0x4f0000ff, 0x570000ff, 0x5f0000ff, 0x670000ff, 0x6f0000ff, 0x770000ff, 0x7f0000ff, 0x131300ff,
		0x1b1b00ff, 0x232300ff, 0x2f2b00ff, 0x372f00ff, 0x433700ff, 0x4b3b07ff, 0x574307ff, 0x5f4707ff, 0x6b4b0bff,
		0x77530fff, 0x835713ff, 0x8b5b13ff, 0x975f1bff, 0xa3631fff, 0xaf6723ff, 0x231307ff, 0x2f170bff, 0x3b1f0fff,
		0x4b2313ff, 0x572b17ff, 0x632f1fff, 0x733723ff, 0x7f3b2bff, 0x8f4333ff, 0x9f4f33ff, 0xaf632fff, 0xbf772fff,
		0xcf8f2bff, 0xdfab27ff, 0xefcb1fff, 0xfff31bff, 0x0b0700ff, 0x1b1300ff, 0x2b230fff, 0x372b13ff, 0x47331bff,
		0x533723ff, 0x633f2bff, 0x6f4733ff, 0x7f533fff, 0x8b5f47ff, 0x9b6b53ff, 0xa77b5fff, 0xb7876bff, 0xc3937bff,
		0xd3a38bff, 0xe3b397ff, 0xab8ba3ff, 0x9f7f97ff, 0x937387ff, 0x8b677bff, 0x7f5b6fff, 0x775363ff, 0x6b4b57ff,
		0x5f3f4bff, 0x573743ff, 0x4b2f37ff, 0x43272fff, 0x371f23ff, 0x2b171bff, 0x231313ff, 0x170b0bff, 0x0f0707ff,
		0xbb739fff, 0xaf6b8fff, 0xa35f83ff, 0x975777ff, 0x8b4f6bff, 0x7f4b5fff, 0x734353ff, 0x6b3b4bff, 0x5f333fff,
		0x532b37ff, 0x47232bff, 0x3b1f23ff, 0x2f171bff, 0x231313ff, 0x170b0bff, 0x0f0707ff, 0xdbc3bbff, 0xcbb3a7ff,
		0xbfa39bff, 0xaf978bff, 0xa3877bff, 0x977b6fff, 0x876f5fff, 0x7b6353ff, 0x6b5747ff, 0x5f4b3bff, 0x533f33ff,
		0x433327ff, 0x372b1fff, 0x271f17ff, 0x1b130fff, 0x0f0b07ff, 0x6f837bff, 0x677b6fff, 0x5f7367ff, 0x576b5fff,
		0x4f6357ff, 0x475b4fff, 0x3f5347ff, 0x374b3fff, 0x2f4337ff, 0x2b3b2fff, 0x233327ff, 0x1f2b1fff, 0x172317ff,
		0x0f1b13ff, 0x0b130bff, 0x070b07ff, 0xfff31bff, 0xefdf17ff, 0xdbcb13ff, 0xcbb70fff, 0xbba70fff, 0xab970bff,
		0x9b8307ff, 0x8b7307ff, 0x7b6307ff, 0x6b5300ff, 0x5b4700ff, 0x4b3700ff, 0x3b2b00ff, 0x2b1f00ff, 0x1b0f00ff,
		0x0b0700ff, 0x0000ffff, 0x0b0befff, 0x1313dfff, 0x1b1bcfff, 0x2323bfff, 0x2b2bafff, 0x2f2f9fff, 0x2f2f8fff,
		0x2f2f7fff, 0x2f2f6fff, 0x2f2f5fff, 0x2b2b4fff, 0x23233fff, 0x1b1b2fff, 0x13131fff, 0x0b0b0fff, 0x2b0000ff,
		0x3b0000ff, 0x4b0700ff, 0x5f0700ff, 0x6f0f00ff, 0x7f1707ff, 0x931f07ff, 0xa3270bff, 0xb7330fff, 0xc34b1bff,
		0xcf632bff, 0xdb7f3bff, 0xe3974fff, 0xe7ab5fff, 0xefbf77ff, 0xf7d38bff, 0xa77b3bff, 0xb79b37ff, 0xc7c337ff,
		0xe7e357ff, 0x7fbfffff, 0xabe7ffff, 0xd7ffffff, 0x670000ff, 0x8b0000ff, 0xb30000ff, 0xd70000ff, 0xff0000ff,
		0xfff393ff, 0xfff7c7ff, 0xffffffff, 0x9f5b53ff};
	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = core_swap32be(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette), "quake1");
}

bool Palette::nippon() {
	uint32_t palette[] = {
		0xffffffff, 0xffb49fdc, 0xff8c6be1, 0xff4a358e, 0xffcdc3f8, 0xffb9a7f4, 0xff3c3664, 0xffaa96f5, 0xff5b49b5,
		0xff907ae8, 0xff6e5ad0, 0xff6d4ddb, 0xffe1dffe, 0xff7a7a9e, 0xff4c10d0, 0xff3a359f, 0xff451bcb, 0xffa9a9ee,
		0xff6667bf, 0xff3f4786, 0xff9396b1, 0xff777aeb, 0xff454a95, 0xff6063a9, 0xff4240cb, 0xff3a3bab, 0xffbbc4d7,
		0xff404890, 0xff384373, 0xff3a3ec7, 0xff364255, 0xff394699, 0xff8394f1, 0xff3444b5, 0xff7d88b9, 0xff677cf1,
		0xff3a4c88, 0xff1530e8, 0xff5554d7, 0xff4c5db5, 0xff364885, 0xff475ea3, 0xff3a54cc, 0xff324872, 0xff2f5cf7,
		0xff28406a, 0xff34509a, 0xff4362c4, 0xff3c5faf, 0xff6e96fb, 0xff384972, 0xff5771b4, 0xff718edb, 0xff1c5ef0,
		0xff4a78ed, 0xff5378ca, 0xff375cb3, 0xff2e3f56, 0xff6e91e3, 0xff3c5a8f, 0xff86a9f0, 0xff4b67a0, 0xff3c69c1,
		0xff6699fb, 0xff6d7a94, 0xff3663a3, 0xff6094e7, 0xff2c537d, 0xff5085c7, 0xff2a5f98, 0xff79a6e1, 0xff325b85,
		0xff4d9ffc, 0xff84baff, 0xff2a8be9, 0xff68a3e9, 0xff4478b1, 0xff2e6396, 0xff2c7aca, 0xff1b3443, 0xff8ab8ec,
		0xff2b5578, 0xff3677b0, 0xff497296, 0xff3b94e2, 0xff2d80c7, 0xff236e9b, 0xff2f556e, 0xff71b4eb, 0xff8eb9d7,
		0xff3a6682, 0xff558eb6, 0xff779fbc, 0xff336687, 0xff268ac1, 0xff1bb1ff, 0xff2698d1, 0xff2da5dd, 0xff3398c9,
		0xff45bff9, 0xff79b8dc, 0xff3291ba, 0xff47b6e8, 0xff42c2f7, 0xff466c7d, 0xffa6c9da, 0xff89d6fa, 0xff42abd9,
		0xff55c5f6, 0xff08c4ff, 0xff24bbef, 0xff5fadca, 0xff2a748d, 0xff82a5b4, 0xff6c7f87, 0xff557d89, 0xff3e6774,
		0xff378ca2, 0xff24606c, 0xff357886, 0xff2c5962, 0xff4ccde9, 0xff4cd9f7, 0xff51e2fb, 0xff90cdd9, 0xff42a1ad,
		0xff3bd2dd, 0xff51a0a5, 0xff3fc2be, 0xff2d6a6c, 0xff509693, 0xff2d8a83, 0xff79b4b1, 0xff386161, 0xff2a4e4b,
		0xff2e625b, 0xff39514d, 0xff6b9189, 0xff4bb490, 0xff70ad91, 0xffa0cab5, 0xff586a64, 0xff3fa27b, 0xff66c186,
		0xff3d594a, 0xff2d6042, 0xff416e51, 0xff93b491, 0xff7c8f80, 0xff3e811b, 0xff81ac5d, 0xff3c5636, 0xff517d22,
		0xffb9d8a8, 0xff72836a, 0xff4b6d2d, 0xff4c5d46, 0xff6e9324, 0xff97a686, 0xff6c8900, 0xff486109, 0xff4f6020,
		0xff3a4c0f, 0xff6c724f, 0xff90aa00, 0xffacb069, 0xff3d4526, 0xffb7ba66, 0xff858726, 0xff555b40, 0xff565a30,
		0xffc4c278, 0xff6d6b37, 0xffe4dea5, 0xff9a9677, 0xffa19966, 0xffd4c781, 0xffb8a633, 0xff42480c, 0xff61560d,
		0xffa78900, 0xff746733, 0xff595325, 0xffa8881e, 0xff736c56, 0xff8a7c57, 0xffdcb258, 0xff755f2b, 0xffb78f3a,
		0xff6e5c2e, 0xff846200, 0xffdeb97d, 0xffdda851, 0xffdfa92e, 0xff13100b, 0xff40250f, 0xff2d1908, 0xffaf5c00,
		0xff6e340b, 0xffd2907b, 0xffa4756e, 0xff471e26, 0xff853211, 0xff974f4e, 0xff551e21, 0xffc3818b, 0xff9a6470,
		0xffc2909b, 0xffbe6b8a, 0xff9c4c6a, 0xffb5778f, 0xff5b3d53, 0xffce8fb2, 0xffb26d98, 0xff8d4277, 0xff412f3c,
		0xff5d224a, 0xff7c3266, 0xff632c59, 0xff81336f, 0xff574c57, 0xffbb81b4, 0xff362b3f, 0xff3f2a57, 0xff503d5e,
		0xff6e6372, 0xff542962, 0xff5b2e6d, 0xff8e32c1, 0xff7a49a8, 0xff372e56, 0xff8a3ce0, 0xff3e3760, 0xfff2fafc,
		0xfffbffff, 0xffbac0bd, 0xff9f9891, 0xff7b7d78, 0xff747c70, 0xff656765, 0xff535953, 0xff484f4f, 0xff3d4352,
		0xff383c37, 0xff26323a, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555,
		0xff444444, 0xff222222, 0xff111111, 0xff000000};
	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = core_swap32le(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette), "nippon");
}

core::String Palette::extractPaletteName(const core::String &file) {
	if (!core::string::startsWith(file, "palette-")) {
		return "";
	}
	const core::String &nameWithExtension = file.substr(8);
	const size_t extPos = nameWithExtension.rfind('.');
	if (extPos != core::String::npos) {
		return nameWithExtension.substr(0, extPos);
	}
	return nameWithExtension;
}

bool Palette::createPalette(const image::ImagePtr &image, palette::Palette &palette) {
	if (!image || !image->isLoaded()) {
		return false;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	const int maxSize = core::Var::getSafe(cfg::PalformatMaxSize)->intVal();
	if (imageWidth * imageHeight > maxSize * maxSize) {
		Log::error(
			"Failed to convert image to palette - scale it down to max %i:%i or change the cvar %s to a higher value",
			maxSize, maxSize, cfg::PalformatMaxSize);
		return false;
	}
	core::Set<core::RGBA, 521> colorSet(imageWidth * imageHeight);
	Log::debug("Create palette for image: %s (%i:%i)", image->name().c_str(), imageWidth, imageHeight);
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const core::RGBA data = image->colorAt(x, y);
			colorSet.insert(data);
		}
	}

	core::Buffer<core::RGBA> colors;
	colors.reserve(colorSet.size());
	for (const auto &e : colorSet) {
		colors.push_back(e->first);
	}
	palette.setName(image->name());
	palette.quantize(colors.data(), colors.size());
	palette.markDirty();
	return true;
}

void Palette::setMaterialType(uint8_t paletteColorIdx, MaterialType type) {
	_materials[paletteColorIdx].type = type;
	markDirty();
}

bool Palette::setMaterialProperty(uint8_t paletteColorIdx, const core::String &name, float value) {
	Material &mat = _materials[paletteColorIdx];
	for (uint32_t i = 0; i < MaterialProperty::MaterialMax - 1; ++i) {
		if (MaterialPropertyNames[i] == name) {
			mat.setValue((MaterialProperty)(i + 1), value);
			markDirty();
			return true;
		}
	}
	return false;
}

float Palette::materialProperty(uint8_t paletteColorIdx, const core::String &name) const {
	const Material &mat = _materials[paletteColorIdx];
	for (uint32_t i = 0; i < MaterialProperty::MaterialMax - 1; ++i) {
		if (MaterialPropertyNames[i] == name) {
			return mat.value((MaterialProperty)(i + 1));
		}
	}
	return 0.0f;
}

bool Palette::hasAlpha(uint8_t paletteColorIdx) const {
	return _colors[paletteColorIdx].a < 255;
}

bool Palette::hasEmit(uint8_t paletteColorIdx) const {
	return _materials[paletteColorIdx].has(MaterialEmit);
}

void Palette::setMaterialValue(uint8_t paletteColorIdx, MaterialProperty property, float factor) {
	_materials[paletteColorIdx].setValue(property, factor);
	markDirty();
}

void Palette::setEmit(uint8_t paletteColorIdx, float factor) {
	if (factor < 0.0f || factor > 1.0f) {
		Log::warn("Unexpected emit factor %f for palette color %i", factor, paletteColorIdx);
	}
	setMaterialValue(paletteColorIdx, MaterialEmit, glm::clamp(factor, 0.0f, 1.0f));
}

void Palette::setMetal(uint8_t paletteColorIdx, float factor) {
	if (factor < 0.0f || factor > 1.0f) {
		Log::warn("Unexpected metal factor %f for palette color %i", factor, paletteColorIdx);
	}
	setMaterialValue(paletteColorIdx, MaterialMetal, glm::clamp(factor, 0.0f, 1.0f));
}

void Palette::setRoughness(uint8_t paletteColorIdx, float factor) {
	if (factor < 0.0f || factor > 1.0f) {
		Log::warn("Unexpected roughness factor %f for palette color %i", factor, paletteColorIdx);
	}
	setMaterialValue(paletteColorIdx, MaterialRoughness, glm::clamp(factor, 0.0f, 1.0f));
}

void Palette::setSpecular(uint8_t paletteColorIdx, float factor) {
	if (factor < 0.0f || factor > 1.0f) {
		Log::warn("Unexpected specular factor %f for palette color %i", factor, paletteColorIdx);
	}
	setMaterialValue(paletteColorIdx, MaterialSpecular, glm::clamp(factor, 0.0f, 1.0f));
}

void Palette::setIndexOfRefraction(uint8_t paletteColorIdx, float factor) {
	if (factor < 0.0f || factor > 3.0f) {
		Log::warn("Unexpected ior value %f for palette color %i", factor, paletteColorIdx);
	}
	setMaterialValue(paletteColorIdx, MaterialIndexOfRefraction, glm::clamp(factor, 1.0f, 3.0f));
}

void Palette::setAttenuation(uint8_t paletteColorIdx, float factor) {
	setMaterialValue(paletteColorIdx, MaterialAttenuation, factor);
}

void Palette::setFlux(uint8_t paletteColorIdx, float factor) {
	setMaterialValue(paletteColorIdx, MaterialFlux, factor);
}

void Palette::setAlpha(uint8_t paletteColorIdx, float factor) {
	if (factor < 0.0f || factor > 1.0f) {
		Log::warn("Unexpected alpha factor %f for palette color %i", factor, paletteColorIdx);
	}
	_colors[paletteColorIdx].a = (float)_colors[paletteColorIdx].a * glm::clamp(factor, 0.0f, 1.0f);
	markDirty();
}

void Palette::setDensity(uint8_t paletteColorIdx, float factor) {
	setMaterialValue(paletteColorIdx, MaterialDensity, factor);
}

void Palette::setSp(uint8_t paletteColorIdx, float factor) {
	setMaterialValue(paletteColorIdx, MaterialSp, factor);
}

void Palette::setGlossiness(uint8_t paletteColorIdx, float factor) {
	if (factor < 0.0f || factor > 1.0f) {
		Log::warn("Unexpected glossiness factor %f for palette color %i", factor, paletteColorIdx);
	}
	setMaterialValue(paletteColorIdx, MaterialGlossiness, glm::clamp(factor, 0.0f, 1.0f));
}

void Palette::setMedia(uint8_t paletteColorIdx, float factor) {
	setMaterialValue(paletteColorIdx, MaterialMedia, factor);
}

void Palette::setLowDynamicRange(uint8_t paletteColorIdx, float factor) {
	setMaterialValue(paletteColorIdx, MaterialLowDynamicRange, factor);
}

void Palette::toVec4f(core::DynamicArray<glm::vec4> &vec4f) const {
	vec4f.reserve(PaletteMaxColors);
	for (int i = 0; i < _colorCount; ++i) {
		vec4f.push_back(core::Color::fromRGBA(_colors[i]));
	}
	for (int i = _colorCount; i < PaletteMaxColors; ++i) {
		vec4f.emplace_back(0.0f);
	}
}

void Palette::emitToVec4f(core::DynamicArray<glm::vec4> &vec4f) const {
	vec4f.reserve(PaletteMaxColors);
	for (int i = 0; i < _colorCount; ++i) {
		glm::vec4 color = core::Color::fromRGBA(_colors[i]);
		vec4f.push_back(color * _materials[i].emit);
	}
	for (int i = _colorCount; i < PaletteMaxColors; ++i) {
		vec4f.emplace_back(0.0f);
	}
}

bool Palette::convertImageToPalettePng(const image::ImagePtr &image, const char *paletteFile) {
	if (!image || !image->isLoaded() || paletteFile == nullptr) {
		return false;
	}
	if (!core::string::endsWith(paletteFile, ".png")) {
		Log::error("Failed to save palette as png - invalid file extension");
		return false;
	}
	Palette palette;
	if (!createPalette(image, palette)) {
		return false;
	}
	const image::ImagePtr &paletteImg = image::createEmptyImage("**palette**");
	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr file = filesystem->open(paletteFile, io::FileMode::SysWrite);
	io::FileStream stream(file);
	if (!stream.valid()) {
		Log::error("Failed to open file %s for saving the palette as png", paletteFile);
		return false;
	}
	core::RGBA colors[PaletteMaxColors];
	for (int i = 0; i < PaletteMaxColors; i++) {
		colors[i] = palette.color(i);
	}
	return paletteImg->writePNG(stream, (const uint8_t *)colors, (int)palette.size(), 1, 4);
}

} // namespace palette
