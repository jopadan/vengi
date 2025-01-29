/**
 * @file
 */

#include "../PalettePanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void PalettePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "switch built-in")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		for (int i = lengthof(palette::Palette::builtIn) - 1; i >= 0; --i) {
			ctx->SetRef(id);
			ctx->MenuClick("File/Switch");
			ctx->SetRef(POPUP_TITLE_LOAD_PALETTE);
			ctx->ItemClick("##type");
			core::String name = core::string::format("//$FOCUSED/%s", palette::Palette::builtIn[i]);
			ctx->ItemClick(name.c_str());
			ctx->ItemClick("###Ok");
			palette::Palette check;
			check.load(palette::Palette::builtIn[i]);
			const palette::Palette& activePalette = _sceneMgr->activePalette();
			IM_CHECK_EQ(activePalette.colorCount(), check.colorCount());
			IM_CHECK_EQ(activePalette.color(0), check.color(0));
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "lospec")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		ctx->MenuClick("File/Lospec/ID");
		ctx->ItemInputValue("//$FOCUSED/ID", "commodore64");
		ctx->ItemClick("//$FOCUSED/Ok");
		ctx->MenuClick("File/Export");
		saveFile(ctx, "palette-lospec.png");
		const palette::Palette& activePalette = _sceneMgr->activePalette();
		IM_CHECK_EQ(activePalette.colorCount(), 16);
		IM_CHECK_EQ(activePalette.color(0), core::RGBA(0, 0, 0, 255));
		IM_CHECK_EQ(activePalette.color(4), core::RGBA(255, 255, 255, 255));
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag and drop color")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		const palette::Palette& activePalette = _sceneMgr->activePalette();
		const core::RGBA slot0 = activePalette.color(0);
		const core::RGBA slot1 = activePalette.color(1);
		ctx->ItemDragAndDrop("$$0", "$$1");
		ctx->Yield();
		IM_CHECK_EQ(activePalette.color(0), slot1);
		IM_CHECK_EQ(activePalette.color(1), slot0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag and drop color ctrl")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		const palette::Palette& activePalette = _sceneMgr->activePalette();
		const core::RGBA slot0 = activePalette.color(0);
		const core::RGBA slot1 = activePalette.color(1);
		const int index0 = activePalette.uiIndex(0);
		const int index1 = activePalette.uiIndex(1);
		ctx->KeyDown(ImGuiMod_Ctrl);
		ctx->ItemDragAndDrop("$$0", "$$1");
		ctx->KeyUp(ImGuiMod_Ctrl);
		IM_CHECK_EQ(activePalette.color(0), slot0);
		IM_CHECK_EQ(activePalette.color(1), slot1);
		IM_CHECK_EQ(activePalette.uiIndex(0), index1);
		IM_CHECK_EQ(activePalette.uiIndex(1), index0);
		ctx->MenuClick("Sort/Original");
	};
}

} // namespace voxedit
