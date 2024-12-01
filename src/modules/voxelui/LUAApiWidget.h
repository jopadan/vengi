/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "voxelgenerator/LUAApi.h"

namespace voxelui {

struct LUAApiExecutorContext {
	virtual ~LUAApiExecutorContext() = default;

	// set to true if another script is currently running
	bool isRunning = false;
	// set to the listener to receive the command execution event for the script
	command::CommandExecutionListener *listener = nullptr;
	virtual void notify(const core::String &scriptFilename, const core::DynamicArray<core::String> &args) {
	}
	virtual void runScript(const core::String &script, const core::DynamicArray<core::String> &args) {
	}
};

enum LUAApiWidgetFlags {
	LUAAPI_WIDGET_FLAG_NONE = 0u,
	LUAAPI_WIDGET_FLAG_RUN = 1u << 0,
	LUAAPI_WIDGET_FLAG_NOTIFY = 1u << 1
};

class LUAApiWidget {
public:
	core::String _activeScript;
	core::DynamicArray<voxelgenerator::LUAScript> _scripts;
	const voxelgenerator::LUAScript _dummy{};

private:
	int _currentScript = -1;
	core::DynamicArray<voxelgenerator::LUAParameterDescription> _scriptParameterDescription;
	core::DynamicArray<core::String> _enumValues;
	core::DynamicArray<core::String> _scriptParameters;

public:
	void clear();
	const voxelgenerator::LUAScript &currentScript() const;
	const voxelgenerator::LUAScript &script(int idx) const;
	void reloadScriptParameters(voxelgenerator::LUAApi &luaApi, const core::String &script);
	void reloadScript(voxelgenerator::LUAApi &luaApi);
	/**
	 * @return @c true if the current script is valid
	 */
	bool updateScriptExecutionPanel(voxelgenerator::LUAApi &luaApi, const palette::Palette &palette,
									LUAApiExecutorContext &ctx, uint32_t flags);
};

inline const voxelgenerator::LUAScript &LUAApiWidget::currentScript() const {
	return script(_currentScript);
}

inline const voxelgenerator::LUAScript &LUAApiWidget::script(int idx) const {
	if (idx < 0 || idx >= (int)_scripts.size()) {
		return _dummy;
	}
	return _scripts[idx];
}

} // namespace voxelui