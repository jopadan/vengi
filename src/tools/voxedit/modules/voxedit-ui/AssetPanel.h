/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/collection/ConcurrentQueue.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxelcollection/CollectionManager.h"
#include "CollectionPanel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class AssetPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	video::TexturePoolPtr _texturePool;
	io::FilesystemPtr _filesystem;
	SceneManagerPtr _sceneMgr;
	voxelcollection::CollectionManagerPtr _collectionMgr;
	CollectionPanel _collectionPanel;
	core::ConcurrentQueue<image::ImagePtr> _images;

public:
	AssetPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
			   const voxelcollection::CollectionManagerPtr &collectionMgr, const video::TexturePoolPtr &texturePool, const io::FilesystemPtr &filesystem);
	bool init();
	void update(const char *id, command::CommandExecutionListener &listener);
	void shutdown();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
