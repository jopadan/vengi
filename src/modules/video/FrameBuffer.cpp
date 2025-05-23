/**
 * @file
 */

#include "FrameBuffer.h"
#include "Texture.h"
#include "RenderBuffer.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include "Renderer.h"
#include "image/Image.h"

namespace video {

FrameBuffer::~FrameBuffer() {
	core_assert_msg(_fbo == video::InvalidId, "Framebuffer was not properly shut down");
	shutdown();
}

void FrameBuffer::addColorAttachment(FrameBufferAttachment attachment, const TexturePtr& texture) {
	if (texture && texture->config().samples() != _samples) {
		Log::error("Texture samples do not match framebuffer samples for attachment %i", (int)attachment);
	}
	_colorAttachments[core::enumVal(attachment)] = texture;
}

bool FrameBuffer::hasColorAttachment(FrameBufferAttachment attachment) {
	return (bool)_colorAttachments[core::enumVal(attachment)];
}

void FrameBuffer::addBufferAttachment(FrameBufferAttachment attachment, const RenderBufferPtr& renderBuffer) {
	if (renderBuffer && renderBuffer->samples() != _samples) {
		Log::error("Renderbuffer samples do not match framebuffer samples for attachment %i", (int)attachment);
	}
	_bufferAttachments[core::enumVal(attachment)] = renderBuffer;
}

bool FrameBuffer::hasBufferAttachment(FrameBufferAttachment attachment) {
	return (bool)_bufferAttachments[core::enumVal(attachment)];
}

bool FrameBuffer::prepareAttachments(const FrameBufferConfig& cfg) {
	_samples = cfg.samples();
	const glm::ivec2& dim = cfg.dimension();
	for (const auto& a : cfg.textureAttachments()) {
		const FrameBufferAttachment key = a->key;
		const TextureConfig& textureConfig = a->value;
		addColorAttachment(key, video::createTexture(textureConfig, dim.x, dim.y));
		if (key == FrameBufferAttachment::Depth) {
			_clearFlag |= ClearFlag::Depth;
		} else if (key == FrameBufferAttachment::DepthStencil) {
			_clearFlag |= ClearFlag::Depth;
			_clearFlag |= ClearFlag::Stencil;
		} else if (key == FrameBufferAttachment::Stencil) {
			_clearFlag |= ClearFlag::Stencil;
		} else {
			_clearFlag |= ClearFlag::Color;
		}
	}
	if (cfg.useColorTexture() && !hasColorAttachment(FrameBufferAttachment::Color0) && !hasBufferAttachment(FrameBufferAttachment::Color0)) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.colorTextureFormat());
		textureCfg.samples(cfg.samples());
		addColorAttachment(FrameBufferAttachment::Color0, video::createTexture(textureCfg, dim.x, dim.y));
		_clearFlag |= ClearFlag::Color;
	}

	const bool depthStencil = hasColorAttachment(FrameBufferAttachment::Depth)
					&& hasBufferAttachment(FrameBufferAttachment::Depth)
					&& hasColorAttachment(FrameBufferAttachment::DepthStencil)
					&& hasBufferAttachment(FrameBufferAttachment::DepthStencil);
	if (cfg.useDepthTexture() && !depthStencil) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.depthTextureFormat());
		addColorAttachment(FrameBufferAttachment::Depth, video::createTexture(textureCfg, dim.x, dim.y));
		_clearFlag |= ClearFlag::Depth;
	} else if (cfg.useDepthBuffer() && !depthStencil) {
		if (cfg.useStencilBuffer()) {
			addBufferAttachment(FrameBufferAttachment::DepthStencil, video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y, cfg.samples()));
			_clearFlag |= ClearFlag::Depth;
			_clearFlag |= ClearFlag::Stencil;
		} else {
			addBufferAttachment(FrameBufferAttachment::Depth, video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y, cfg.samples()));
			_clearFlag |= ClearFlag::Depth;
		}
	} else if (cfg.useStencilBuffer()) {
		addBufferAttachment(FrameBufferAttachment::Stencil, video::createRenderBuffer(TextureFormat::S8, dim.x, dim.y, cfg.samples()));
		_clearFlag |= ClearFlag::Stencil;
	}

	return true;
}

bool FrameBuffer::init(const FrameBufferConfig& cfg) {
	_dimension = cfg.dimension();
	_fbo = genFramebuffer();
	Id prev = video::bindFramebuffer(_fbo);
	bool retVal = prepareAttachments(cfg);
	if (retVal) {
		retVal = video::setupFramebuffer(_fbo, _colorAttachments, _bufferAttachments);
	}
	video::bindFramebuffer(prev);
	return retVal;
}

glm::vec4 FrameBuffer::uv() const {
	return video::framebufferUV();
}

void FrameBuffer::shutdown() {
	for (int i = 0; i < lengthof(_colorAttachments); ++i) {
		if (_colorAttachments[i]) {
			_colorAttachments[i]->shutdown();
			_colorAttachments[i] = TexturePtr();
		}
	}
	for (int i = 0; i < lengthof(_bufferAttachments); ++i) {
		if (_bufferAttachments[i]) {
			_bufferAttachments[i]->shutdown();
			_bufferAttachments[i] = RenderBufferPtr();
		}
	}

	video::deleteFramebuffer(_fbo);
	_clearFlag = ClearFlag::None;
	_oldFramebuffer = InvalidId;
	for (int i = 0; i < lengthof(_viewport); ++i) {
		_viewport[i] = 0;
	}
	_dimension = glm::ivec2(0);
}

TexturePtr FrameBuffer::texture(FrameBufferAttachment attachment) const {
	const TexturePtr& tex = _colorAttachments[core::enumVal(attachment)];
	if (!tex) {
		Log::warn("Could not find framebuffer texture for %i", (int)attachment);
	}
	return tex;
}

image::ImagePtr FrameBuffer::image(const core::String &name, FrameBufferAttachment attachment) const {
	const video::TexturePtr &fboTexture = texture(attachment);
	uint8_t *pixels = nullptr;
	image::ImagePtr image;
	if (video::readTexture(video::TextureUnit::Upload, fboTexture->type(), fboTexture->format(), fboTexture->handle(),
						   fboTexture->width(), fboTexture->height(), &pixels)) {
		image::Image::flipVerticalRGBA(pixels, fboTexture->width(), fboTexture->height());
		image = image::createEmptyImage(name);
		image->loadRGBA(pixels, fboTexture->width(), fboTexture->height());
	} else {
		Log::error("Failed to read framebuffer");
	}
	core_free(pixels);
	return image;
}

void FrameBuffer::bind(bool clear) {
	video::getViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::viewport(0, 0, _dimension.x, _dimension.y);
	_oldFramebuffer = video::bindFramebuffer(_fbo);
	if (clear) {
		video::clear(_clearFlag);
	}
}

bool FrameBuffer::bindTextureAttachment(FrameBufferAttachment attachment, int layerIndex, bool clear) {
	auto tex = _colorAttachments[core::enumVal(attachment)];
	if (!tex) {
		Log::warn("Could not find texture attachment for attachment %i", (int)attachment);
		return false;
	}
	if (layerIndex < 0 || layerIndex >= tex->layers()) {
		Log::warn("Given layer index (%i) is out of bounds: %i", layerIndex, (int)tex->layers());
		return false;
	}
	return video::bindFrameBufferAttachment(_fbo, tex->handle(), attachment, layerIndex, clear);
}

void FrameBuffer::unbind() {
	video::viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::bindFramebuffer(_oldFramebuffer);
	_oldFramebuffer = InvalidId;
}

bool bindTexture(TextureUnit unit, const FrameBuffer& frameBuffer, FrameBufferAttachment attachment) {
	const TexturePtr& tex = frameBuffer.texture(attachment);
	if (!tex) {
		return false;
	}
	video::bindTexture(unit, tex->type(), tex->handle());
	return true;
}

}
