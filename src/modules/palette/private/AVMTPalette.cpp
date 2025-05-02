/**
 * @file
 */

#include "AVMTPalette.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Set.h"
#include "io/TokenStream.h"
#include "palette/Material.h"

namespace palette {

class AVMTStream : public io::TokenStream {
private:
	using Super = io::TokenStream;
	int _blockDepth = 0;
	int _arrayDepth = 0;

public:
	AVMTStream(io::SeekableReadStream &stream) : Super(stream, {}, " (){},;\n\t") {
	}

	core::String next() override {
		core::String n = Super::next();
		if (n == "{") {
			++_blockDepth;
			return next();
		} else if (n == "}") {
			--_blockDepth;
			return next();
		} else if (n == "[") {
			++_arrayDepth;
			return next();
		} else if (n == "]") {
			--_arrayDepth;
			return next();
		}
		return n;
	}

	core::String nextStringValue() {
		// skip =
		const core::String equalSign = next();
		if (equalSign != "=") {
			Log::error("Expected '=' but got '%s'", equalSign.c_str());
		}
		return next();
	}

	int blockDepth() const {
		return _blockDepth;
	}

	int arrayDepth() const {
		return _arrayDepth;
	}
};

struct AVMTMaterial {
	core::RGBA rgba;
	glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
	core::String name;
	palette::Material mat;
};

bool AVMTPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	AVMTStream avmtStream(stream);
	core::DynamicArray<AVMTMaterial, 128> materials;

	AVMTMaterial currentMaterial;
	bool currentMatColorFound = false;

	core::Set<core::RGBA, 521> colorSet;

	while (!avmtStream.eos()) {
		const core::String &token = avmtStream.next();
		if (avmtStream.arrayDepth() == 1 && avmtStream.blockDepth() == 2) {
			if (token == "Name") {
				palette.setName(avmtStream.nextStringValue());
			}
		} else if (avmtStream.arrayDepth() == 2 && avmtStream.blockDepth() >= 3) {
			if (token == "r") {
				currentMaterial.color.r = avmtStream.nextStringValue().toFloat();
				currentMatColorFound = true;
			} else if (token == "g") {
				currentMaterial.color.g = avmtStream.nextStringValue().toFloat();
				currentMatColorFound = true;
			} else if (token == "b") {
				currentMaterial.color.b = avmtStream.nextStringValue().toFloat();
				currentMatColorFound = true;
			} else if (token == "metallic") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialMetal, v);
			} else if (token == "indexOfRefraction") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialIndexOfRefraction, 1.0f - v);
			} else if (token == "surfaceTransmission") {
				const float alpha = avmtStream.nextStringValue().toFloat();
				if (alpha >= 1.0f) {
					currentMaterial.mat.type = palette::MaterialType::Media;
					currentMaterial.mat.setValue(MaterialProperty::MaterialMedia, 1.0f);
				} else if (alpha > 0.0f) {
					currentMaterial.mat.type = palette::MaterialType::Blend;
					// currentMaterial.color.a = alpha; // TODO: MATERIAL: not really the alpha value...
				}
			} else if (token == "absorptionLength") {
				/*const float v =*/avmtStream.nextStringValue().toFloat();
				// currentMaterial.mat.setValue(MaterialProperty::MaterialAbsorptionLength, v);
			} else if (token == "scatterLength") {
				/*const float v =*/avmtStream.nextStringValue().toFloat();
				// currentMaterial.mat.setValue(MaterialProperty::MaterialScatterLength, v);
			} else if (token == "phase") {
				const float v = avmtStream.nextStringValue().toFloat();
				if (v > 0.0f) {
				 	currentMaterial.mat.setValue(MaterialProperty::MaterialPhase, v);
				}
			} else if (token == "smooth") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialRoughness, 1.0f - v);
			} else if (token == "emissive") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialEmit, v);
			} else if (token == "name") {
				currentMaterial.name = avmtStream.nextStringValue();
			} else if (token == "materialTransparency") {
				// skip =
				if (avmtStream.next() != "=") {
					Log::error("Expected '=' after materialTransparency but got '%s'", token.c_str());
				}
			} else {
				Log::debug("Unhandled token: '%s' (expected are: r, g, b, metallic, smooth, emissive, name)",
						   token.c_str());
			}
		} else {
			if (currentMatColorFound) {
				currentMaterial.rgba = core::Color::getRGBA(currentMaterial.color);
				if (!colorSet.insert(currentMaterial.rgba)) {
					Log::trace("Duplicate color found: %s", core::Color::print(currentMaterial.rgba).c_str());
					currentMaterial = {};
					currentMatColorFound = false;
					continue;
				}
				materials.push_back(currentMaterial);
				currentMaterial = {};
				currentMatColorFound = false;
			}
			Log::trace("Token %s at depth %i and array depth %i", token.c_str(), avmtStream.blockDepth(),
					   avmtStream.arrayDepth());
		}
	}

	if (materials.empty()) {
		Log::error("No materials found in %s", filename.c_str());
		return false;
	}

	core::Buffer<core::RGBA> colorBuffer;
	colorBuffer.reserve(materials.size());
	for (const auto &e : materials) {
		colorBuffer.push_back(e.rgba);
	}
	palette.quantize(colorBuffer.data(), colorBuffer.size());
	Log::debug("Palette has %i colors from %i materials", palette.colorCount(), (int)materials.size());

	for (size_t i = 0; i < materials.size(); ++i) {
		const AVMTMaterial &m = materials[i];
		const int palIdx = palette.getClosestMatch(m.rgba);
		if (palIdx == PaletteColorNotFound) {
			continue;
		}
		palette.setColorName(palIdx, m.name);
		palette.setMaterial(palIdx, m.mat);
	}

	return true;
}

bool AVMTPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	stream.writeString("VoxelMaterialArray =\t{\n", false);
	stream.writeString("\tmaterials =\t[\n", false);
	stream.writeString("\t\t{\n", false);
	stream.writeStringFormat(false, "\t\t\tName =\t\"%s\"\n", palette.name().c_str());
	stream.writeString("\t\t\tType =\t1\n", false);
	stream.writeString("\t\t\tPaletteSize =\t{\n", false);
	stream.writeString("\t\t\t\tx =\t1\n", false);
	stream.writeStringFormat(false, "\t\t\t\ty =\t%i\n", palette.colorCount());
	stream.writeString("\t\t\t}\n", false);
	stream.writeString("\t\t\tVoxMaterialParams =\t[\n", false);
	int added = 0;
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (palette.color(i).a == 0) {
			continue;
		}
		if (added != 0) {
			stream.writeString(",\n", false);
		}
		++added;
		const glm::vec4 &c = palette.color4(i);
		stream.writeString("\t\t\t\t{\n", false);
		stream.writeStringFormat(false, "\t\t\t\t\tr =\t%0.6f\n", c.r);
		stream.writeStringFormat(false, "\t\t\t\t\tg =\t%0.6f\n", c.g);
		stream.writeStringFormat(false, "\t\t\t\t\tb =\t%0.6f\n", c.b);
		const Material &mat = palette.material(i);
		stream.writeStringFormat(false, "\t\t\t\t\tmetallic =\t%f\n", mat.metal);
		stream.writeStringFormat(false, "\t\t\t\t\tsmooth =\t%f\n", 1.0f - mat.roughness);
		stream.writeStringFormat(false, "\t\t\t\t\temissive =\t%f\n", mat.emit);
		stream.writeString("\t\t\t\t\tmaterialTransparency =\t{\n", false);
		if (mat.type == palette::MaterialType::Glass || mat.type == palette::MaterialType::Blend) {
			// TODO: MATERIAL: not really the alpha value...
			stream.writeStringFormat(false, "\t\t\t\t\t\tsurfaceTransmission =\t%f\n", c.a);
		} else if (mat.type == palette::MaterialType::Media) {
			stream.writeString("\t\t\t\t\t\tsurfaceTransmission =\t1.0\n", false);
		} else {
			stream.writeString("\t\t\t\t\t\tsurfaceTransmission =\t0.0\n", false);
		}
		// stream.writeStringFormat(false, "\t\t\t\t\t\tabsorptionLength =\t%f\n", mat.absorptionLength);
		// stream.writeStringFormat(false, "\t\t\t\t\t\tscatterLength =\t%f\n", mat.scatterLength);
		stream.writeStringFormat(false, "\t\t\t\t\t\tindexOfRefraction =\t%f\n", 1.0f + mat.indexOfRefraction);
		if (mat.media == 1.0f) {
			stream.writeStringFormat(false, "\t\t\t\t\t\tphase =\t%f\n", mat.phase);
		} else {
			stream.writeString("\t\t\t\t\t\tphase =\t0.0\n", false);
		}
		stream.writeString("\t\t\t\t\t}\n", false);
		stream.writeStringFormat(false, "\t\t\t\t\tname =\t\"%s\"\n", palette.colorName(i).c_str());
		stream.writeString("\t\t\t\t}", false);
	}
	stream.writeString("\n\t\t\t\t}\n", false);
	stream.writeString("\t\t\t]\n", false);
	stream.writeString("\t\t\tStrength =\t1\n", false);
	stream.writeString("\t\t}\n", false);
	stream.writeString("\t]\n", false);
	stream.writeString("\t}\n", false);
	stream.writeString("\t]\n", false);
	stream.writeString("\tpalette =\t[]\n", false);
	stream.writeString("\tpalettes =\t[\n", false);
	stream.writeString("\t\t{\n", false);
	stream.writeString("\t\t\tname =\t\"Default\"\n", false);
	stream.writeString("\t\t\tpalette =\t[]\n", false);
	stream.writeString("\t\t\twidth =\t15\n", false);
	stream.writeString("\t\t}\n", false);
	stream.writeString("\t]\n", false);
	stream.writeString("\tactivePaletteEditToolShapes =\t0\n", false);
	stream.writeString("\tactivePaletteEditToolProcedural =\t0\n", false);
	stream.writeString("\tactivePaletteEditToolModifierRandomise =\t0\n", false);
	stream.writeString("\tactivePaletteMaterials =\t0\n", false);
	return true;
}

} // namespace palette
