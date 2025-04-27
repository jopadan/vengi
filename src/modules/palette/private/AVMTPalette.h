/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"
#include "io/FormatDescription.h"

namespace palette {

/**
 * https://www.enkisoftware.com/products#downloads
 * @ingroup Formats
 */
class AVMTPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) override;
	bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;

	static const io::FormatDescription &format() {
		static const io::FormatDescription desc = {"Avoyd Material", {"avmt"}, {}, FORMAT_FLAG_SAVE};
		return desc;
	}
};

} // namespace palette
