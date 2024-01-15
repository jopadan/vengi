/**
 * @file
 */

#pragma once

#include "Brush.h"

namespace voxedit {

class PathBrush : public Brush {
private:
	using Super = Brush;

public:
	virtual ~PathBrush() = default;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;
};

} // namespace voxedit