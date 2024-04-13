/**
 * @file
 */

#include "../MainWindow.h"
#include "../WindowTitles.h"
#include "../Viewport.h"
#include "core/Log.h"
#include "imgui_te_context.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"

namespace voxedit {

void MainWindow::registerUITests(ImGuiTestEngine *engine, const char *title) {
#if ENABLE_RENDER_PANEL
	_renderPanel.registerUITests(engine, TITLE_RENDER);
#endif
	_lsystemPanel.registerUITests(engine, TITLE_LSYSTEMPANEL);
	_brushPanel.registerUITests(engine, TITLE_BRUSHPANEL);
	_treePanel.registerUITests(engine, TITLE_TREES);
	_sceneGraphPanel.registerUITests(engine, TITLE_SCENEGRAPH);
	_animationPanel.registerUITests(engine, TITLE_ANIMATION_SETTINGS);
	_toolsPanel.registerUITests(engine, TITLE_TOOLS);
	_assetPanel.registerUITests(engine, TITLE_ASSET);
	_mementoPanel.registerUITests(engine, TITLE_MEMENTO);
	_positionsPanel.registerUITests(engine, TITLE_POSITIONS);
	_palettePanel.registerUITests(engine, TITLE_PALETTE);
	_menuBar.registerUITests(engine, "##menubar");
	_statusBar.registerUITests(engine, TITLE_STATUSBAR);
	_scriptPanel.registerUITests(engine, TITLE_SCRIPT);
	_animationTimeline.registerUITests(engine, TITLE_ANIMATION_TIMELINE);
	_cameraPanel.registerUITests(engine, TITLE_CAMERA);
	for (int i = 0; i < _scenes.size(); i++) {
		_scenes[i]->registerUITests(engine, nullptr);
	}

	IM_REGISTER_TEST(engine, testName(), "new scene unsaved changes")->TestFunc = [=](ImGuiTestContext *ctx) {
		_sceneMgr->markDirty();
		ImGuiContext& g = *ctx->UiContext;
		ctx->SetRef(title);
		IM_CHECK(focusWindow(ctx, title));
		ctx->MenuClick("###File/###New");
		ctx->Yield();
		IM_CHECK_EQ(g.OpenPopupStack.Size, 1);
		IM_CHECK_EQ(g.OpenPopupStack[0].PopupId, ctx->GetID(POPUP_TITLE_UNSAVED));
		ctx->SetRef(POPUP_TITLE_UNSAVED);
		ctx->ItemClick("###Yes");
		ctx->SetRef(POPUP_TITLE_NEW_SCENE);
		ctx->ItemInputValue("##newscenename", "Automated ui test");
		ctx->ItemClick("###OK");
	};

	IM_REGISTER_TEST(engine, testName(), "new scene")->TestFunc = [=](ImGuiTestContext *ctx) {
		_sceneMgr->newScene(true, "", new voxel::RawVolume({0, 1}));
		ctx->SetRef(title);
		IM_CHECK(focusWindow(ctx, title));
		ctx->MenuClick("###File/###New");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_NEW_SCENE);
		ctx->ItemInputValue("##newscenename", "Automated ui test");
		ctx->ItemClick("###OK");
	};
}

} // namespace voxedit