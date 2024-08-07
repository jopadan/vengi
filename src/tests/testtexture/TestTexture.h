/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Texture.h"
#include "testcore/TextureRenderer.h"

class TestTexture: public TestApp {
private:
	using Super = TestApp;
	video::TexturePtr _texture;
	render::TextureRenderer _renderer;

	void doRender() override;
public:
	TestTexture(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	app::AppState onCleanup() override;
};
