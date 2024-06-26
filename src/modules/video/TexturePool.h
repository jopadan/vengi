/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "Texture.h"
#include "image/Image.h"
#include "core/String.h"
#include "core/SharedPtr.h"
#include "core/collection/DynamicStringMap.h"

namespace video {

/**
 * @ingroup Video
 */
class TexturePool : public core::IComponent {
private:
	core::DynamicStringMap<TexturePtr> _cache;
	core::DynamicStringMap<image::ImagePtr> _images;
	TexturePtr _empty;
public:
	video::TexturePtr load(const core::String& name, bool emptyAsFallback = true);
	video::TexturePtr load(const core::String& name, const uint8_t *rgba, size_t size);
	video::TexturePtr get(const core::String& name);
	bool has(const core::String& name) const;
	image::ImagePtr loadImage(const core::String& name);
	image::ImagePtr loadImage(const core::String& name, const uint8_t *rgba, size_t size);
	video::TexturePtr addImage(const image::ImagePtr &image);

	const core::DynamicStringMap<TexturePtr> &cache() {
		return _cache;
	}

	void construct() override;
	bool init() override;
	void shutdown() override;
	void clear();
};

typedef core::SharedPtr<TexturePool> TexturePoolPtr;

}
