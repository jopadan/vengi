/**
 * @file
 */
#include "TestGLSLComp.h"
#include "testcore/TestAppMain.h"
#include "core/Color.h"
#include "video/Camera.h"
#include "video/ScopedViewPort.h"
#include "core/Log.h"
#include "video/Types.h"

TestGLSLComp::TestGLSLComp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "testglslcomp");
}

app::AppState TestGLSLComp::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!video::hasFeature(video::Feature::ComputeShaders)) {
		Log::error("This test needs compute shader support");
		return app::AppState::InitFailure;
	}

	camera() = video::uiCamera(windowDimension());

	if (!_renderer.init()) {
		Log::error("Failed to init the texture renderer");
		return app::AppState::InitFailure;
	}

	if (!_testShader.setup()) {
		Log::error("Failed to init the compute shader");
		return app::AppState::InitFailure;
	}

	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA32F);
	_texture = video::createTexture(cfg, 512, 512, appname());
	_texture->upload(nullptr);
	video::bindImage(_texture->handle(), video::AccessMode::Write, _testShader.getImageFormatImgOutput());

	video::clearColor(::core::Color::White());
	return state;
}

app::AppState TestGLSLComp::onCleanup() {
	_testShader.shutdown();
	_renderer.shutdown();
	if (_texture) {
		_texture->shutdown();
	}
	return Super::onCleanup();
}

void TestGLSLComp::doRender() {
	_testShader.activate();
	_testShader.run(glm::uvec3(_texture->width(), _texture->height(), 1), video::MemoryBarrierType::ShaderImageAccess);

	video::ScopedTexture texture(_texture, video::TextureUnit::Zero);
	video::ScopedViewPort viewPort(0, 0, frameBufferDimension().x, frameBufferDimension().y);
	_renderer.render();
}

TEST_APP(TestGLSLComp)
