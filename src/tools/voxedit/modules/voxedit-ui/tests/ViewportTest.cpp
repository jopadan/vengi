/**
 * @file
 */

#include "../Viewport.h"
#include "command/CommandHandler.h"
#include "imgui_te_context.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void Viewport::registerUITests(ImGuiTestEngine *engine, const char *) {
	ImGuiTest *testSetVoxel = IM_REGISTER_TEST(engine, testName(), "set voxel");
	testSetVoxel->TestFunc = [=](ImGuiTestContext *ctx) {
		if (isSceneMode()) {
			return;
		}
		ctx->SetRef(_uiId.c_str());
		IM_CHECK(focusWindow(ctx, _uiId.c_str()));
		ImGuiWindow* window = ImGui::FindWindowByName(_uiId.c_str());
		IM_CHECK_SILENT(window != nullptr);
		ctx->MouseMoveToPos(window->Rect().GetCenter());
		command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
	};
}

} // namespace voxedit