/**
 * @file
 */

#include "FormatConfig.h"
#include "app/I18N.h"
#include "core/Color.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"

namespace palette {

static bool colorReductionValidator(const core::String &value) {
	return core::Color::toColorReductionType(value.c_str()) != core::Color::ColorReductionType::Max;
}

bool FormatConfig::init() {
	core::Var::get(cfg::CoreColorReduction,
				   core::Color::toColorReductionTypeString(core::Color::ColorReductionType::MedianCut),
				   _("Controls the algorithm that is used to perform the color reduction"), colorReductionValidator);

	core::Var::get(cfg::PalformatRGB6Bit, "false", core::CV_NOPERSIST,
				   _("Use 6 bit color values for the palette (0-63) - used e.g. in C&C pal files"),
				   core::Var::boolValidator);
	core::Var::get(cfg::PalformatMaxSize, "512", core::CV_NOPERSIST,
				   _("The maximum size of an image in x and y direction to quantize to a palette"));

	core::Var::get(cfg::VoxelPalette, palette::Palette::getDefaultPaletteName(),
				   _("This is the NAME part of palette-<NAME>.png or absolute png file to use (1x256)"));
	core::Var::get(cfg::NormalPalette, palette::NormalPalette::builtIn[0], core::CV_NOPERSIST,
				   _("The normal palette to use for voxelization"));

	return true;
}

} // namespace palette
