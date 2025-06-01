/**
 * @file
 */

#include "Modifier.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "core/Log.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

Modifier::Modifier(SceneManager *sceneMgr, const SelectionManagerPtr &selectionManager)
	: _selectionManager(selectionManager), _stampBrush(sceneMgr), _selectBrush(_selectionManager), _actionExecuteButton(sceneMgr),
	  _deleteExecuteButton(sceneMgr, ModifierType::Erase) {
	_brushes.push_back(&_planeBrush);
	_brushes.push_back(&_shapeBrush);
	_brushes.push_back(&_stampBrush);
	_brushes.push_back(&_lineBrush);
	_brushes.push_back(&_pathBrush);
	_brushes.push_back(&_paintBrush);
	_brushes.push_back(&_textBrush);
	_brushes.push_back(&_selectBrush);
	_brushes.push_back(&_textureBrush);
	core_assert(_brushes.size() == (int)BrushType::Max - 1);
}

void Modifier::construct() {
	command::Command::registerActionButton("actionexecute", _actionExecuteButton, "Execute the modifier action");
	command::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton,
										   "Execute the modifier action in delete mode");

	command::Command::registerCommand("actionselect", [&](const command::CmdArgs &args) {
		setBrushType(BrushType::Select);
		setModifierType(ModifierType::Select);
	}).setHelp(_("Change the modifier type to 'select'"));

	command::Command::registerCommand("actioncolorpicker", [&](const command::CmdArgs &args) {
		setBrushType(BrushType::None);
		setModifierType(ModifierType::ColorPicker);
	}).setHelp(_("Change the modifier type to 'color picker'"));

	command::Command::registerCommand("actionerase", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Erase);
	}).setHelp(_("Change the modifier type to 'erase'"));

	command::Command::registerCommand("actionplace", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Place);
	}).setHelp(_("Change the modifier type to 'place'"));

	command::Command::registerCommand("actionoverride", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Override);
	}).setHelp(_("Change the modifier type to 'override'"));

	for (const Brush *b : _brushes) {
		const BrushType type = b->type();
		const core::String hlp = core::String::format(_("Change the brush type to '%s'"), b->name().c_str());
		command::Command::registerCommand("brush" + b->name().toLower(), [&, type](const command::CmdArgs &args) {
			setBrushType(type);
		}).setHelp(hlp);
	}
	command::Command::registerCommand("brushnone", [&](const command::CmdArgs &args) {
		setBrushType(BrushType::None);
	}).setHelp(_("Change the brush type to 'none'"));

	command::Command::registerCommand("lock",
									  [&](const command::CmdArgs &args) {
										  if (args.size() != 1) {
											  Log::info("Usage: lock <x|y|z>");
											  return;
										  }
										  const math::Axis axis = math::toAxis(args[0]);
										  const bool unlock = (_brushContext.lockedAxis & axis) == axis;
										  setLockedAxis(axis, unlock);
									  })
		.setHelp(_("Toggle locked mode for the given axis at the current cursor position"))
		.setArgumentCompleter(command::valueCompleter({"x", "y", "z"}));

	command::Command::registerCommand("lockx", [&](const command::CmdArgs &args) {
		const math::Axis axis = math::Axis::X;
		const bool unlock = (_brushContext.lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp(_("Toggle locked mode for the x axis at the current cursor position"));

	command::Command::registerCommand("locky", [&](const command::CmdArgs &args) {
		const math::Axis axis = math::Axis::Y;
		const bool unlock = (_brushContext.lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp(_("Toggle locked mode for the y axis at the current cursor position"));

	command::Command::registerCommand("lockz", [&](const command::CmdArgs &args) {
		const math::Axis axis = math::Axis::Z;
		const bool unlock = (_brushContext.lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp(_("Toggle locked mode for the z axis at the current cursor position"));

	for (Brush *b : _brushes) {
		b->construct();
	}
}

void Modifier::setLockedAxis(math::Axis axis, bool unlock) {
	if (unlock) {
		_brushContext.lockedAxis &= ~axis;
	} else {
		_brushContext.lockedAxis |= axis;
	}
}

bool Modifier::init() {
	for (Brush *b : _brushes) {
		if (!b->init()) {
			Log::error("Failed to initialize the %s brush", b->name().c_str());
		}
	}
	return true;
}

void Modifier::shutdown() {
	reset();
	for (Brush *b : _brushes) {
		b->shutdown();
	}
}

void Modifier::update(double nowSeconds, const video::Camera *camera) {
	_nowSeconds = nowSeconds;
	_brushContext.fixedOrthoSideView = camera == nullptr ? false : camera->isOrthoAligned();
	AABBBrush *aabbBrush = currentAABBBrush();
	if (aabbBrush) {
		if (aabbBrush->singleMode()) {
			if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
				_actionExecuteButton.execute(true);
				_nextSingleExecution = nowSeconds + 0.1;
			}
		}
	} else if (_brushType == BrushType::Stamp) {
		if (_stampBrush.continuousMode()) {
			if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
				_actionExecuteButton.execute(true);
				_nextSingleExecution = nowSeconds + 0.1;
			}
		}
	}
	if (Brush *brush = currentBrush()) {
		brush->update(_brushContext, nowSeconds);
	}
}

void Modifier::reset() {
	_selectionManager->reset();
	_brushContext.gridResolution = 1;
	_brushContext.cursorPosition = glm::ivec3(0);
	_brushContext.cursorFace = voxel::FaceNames::Max;

	_brushContext.modifierType = ModifierType::Place;
	for (Brush *b : _brushes) {
		b->reset();
	}
	setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0));
	setBrushType(BrushType::Shape);
	setModifierType(ModifierType::Place);
}

bool Modifier::start() {
	if (AABBBrush *brush = currentAABBBrush()) {
		return brush->start(_brushContext);
	}
	return false;
}

void Modifier::executeAdditionalAction() {
	if (isMode(ModifierType::Select) || isMode(ModifierType::ColorPicker)) {
		return;
	}
	if (AABBBrush *brush = currentAABBBrush()) {
		brush->step(_brushContext);
	}
}

void Modifier::invert(voxel::RawVolume &volume) {
	_selectionManager->invert(volume);
}

void Modifier::unselect(voxel::RawVolume &volume) {
	_selectionManager->unselect(volume);
}

bool Modifier::select(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	if (_locked) {
		return false;
	}
	_selectionManager->select(volume, mins, maxs);
	return true;
}

void Modifier::setReferencePosition(const glm::ivec3 &pos) {
	_brushContext.referencePos = pos;
}

bool Modifier::needsAdditionalAction() {
	if (isMode(ModifierType::Select)) {
		return false;
	}
	if (const AABBBrush *brush = currentAABBBrush()) {
		return brush->needsAdditionalAction(_brushContext);
	}
	return false;
}

glm::ivec3 Modifier::currentCursorPosition() {
	if (AABBBrush *brush = currentAABBBrush()) {
		return brush->currentCursorPosition(_brushContext);
	}
	return _brushContext.cursorPosition;
}

voxel::Region Modifier::calcBrushRegion() {
	Brush *brush = currentBrush();
	if (brush) {
		return brush->calcRegion(_brushContext);
	}
	return voxel::Region::InvalidRegion;
}

voxel::RawVolumeWrapper Modifier::createRawVolumeWrapper(voxel::RawVolume *volume) const {
	return voxel::RawVolumeWrapper(volume);
}

void Modifier::setHitCursorVoxel(const voxel::Voxel &voxel) {
	_brushContext.hitCursorVoxel = voxel;
}

void Modifier::setVoxelAtCursor(const voxel::Voxel &voxel) {
	_brushContext.voxelAtCursor = voxel;
}

void Modifier::lock() {
	_locked = true;
}

void Modifier::unlock() {
	_locked = false;
}

bool Modifier::execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
					   const ModifiedRegionCallback &callback) {
	if (_locked) {
		return false;
	}
	if (aborted()) {
		return false;
	}

	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		Log::debug("No volume given - can't perform action");
		return false;
	}

	if (isMode(ModifierType::ColorPicker)) {
		setCursorVoxel(hitCursorVoxel());
		return true;
	}

	preExecuteBrush(volume);
	executeBrush(sceneGraph, node, _brushContext.modifierType, _brushContext.cursorVoxel, callback);
	postExecuteBrush();

	return true;
}

/**
 * change the cursor position if the brush region is outside the volume
 * this allows us to keep all voxels inside the volume boundaries even on the
 * +x, +y and +z sides where the voxels are currently flowing out of the volume
 *
 * @todo: this should be a global brush option - also see https://github.com/vengi-voxel/vengi/issues/444
 */
static glm::ivec3 updateCursor(const voxel::Region &region, const voxel::Region &brushRegion,
							   const glm::ivec3 &cursor) {
	if (!brushRegion.isValid()) {
		return cursor;
	}
	if (region.containsRegion(brushRegion)) {
		return cursor;
	}
	glm::ivec3 delta(0);
	if (brushRegion.getUpperX() > region.getUpperX()) {
		delta.x = region.getUpperX() - brushRegion.getUpperX();
	}
	if (brushRegion.getUpperY() > region.getUpperY()) {
		delta.y = region.getUpperY() - brushRegion.getUpperY();
	}
	if (brushRegion.getUpperZ() > region.getUpperZ()) {
		delta.z = region.getUpperZ() - brushRegion.getUpperZ();
	}

	if (brushRegion.getLowerX() < region.getLowerX()) {
		delta.x = glm::abs(region.getLowerX() - brushRegion.getUpperX());
	}
	if (brushRegion.getLowerY() < region.getLowerY()) {
		delta.y = glm::abs(region.getLowerY() - brushRegion.getLowerY());
	}
	if (brushRegion.getLowerZ() < region.getLowerZ()) {
		delta.z = glm::abs(region.getLowerZ() - brushRegion.getLowerZ());
	}

	return cursor + delta;
}

void Modifier::preExecuteBrush(const voxel::RawVolume *volume) {
	if (Brush *brush = currentBrush()) {
		_brushContext.targetVolumeRegion = volume->region();
		_brushContext.prevCursorPosition = _brushContext.cursorPosition;
		if (brush->brushClamping()) {
			const voxel::Region brushRegion = brush->calcRegion(_brushContext);
			_brushContext.cursorPosition =
				updateCursor(_brushContext.targetVolumeRegion, brushRegion, _brushContext.prevCursorPosition);
		}
		brush->preExecute(_brushContext, volume);
	}
}

void Modifier::postExecuteBrush() {
	if (Brush *brush = currentBrush()) {
		if (brush->brushClamping()) {
			_brushContext.cursorPosition = _brushContext.prevCursorPosition;
		}
		brush->postExecute(_brushContext);
	}
}

bool Modifier::executeBrush(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							ModifierType modifierType, const voxel::Voxel &voxel,
							const ModifiedRegionCallback &callback) {
	if (Brush *brush = currentBrush()) {
		ModifierVolumeWrapper wrapper(node, modifierType, _selectionManager->selections());
		voxel::Voxel prevVoxel = _brushContext.cursorVoxel;
		glm::ivec3 prevCursorPos = _brushContext.cursorPosition;
		if (brush->brushClamping()) {
			const voxel::Region brushRegion = brush->calcRegion(_brushContext);
			_brushContext.cursorPosition = updateCursor(_brushContext.targetVolumeRegion, brushRegion, prevCursorPos);
		}
		_brushContext.cursorVoxel = voxel;
		brush->execute(sceneGraph, wrapper, _brushContext);
		const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
		if (modifiedRegion.isValid()) {
			voxel::logRegion("Dirty region", modifiedRegion);
			if (callback) {
				callback(modifiedRegion, _brushContext.modifierType, true);
			}
		}
		_brushContext.cursorPosition = prevCursorPos;
		_brushContext.cursorVoxel = prevVoxel;
		return true;
	}
	return false;
}

Brush *Modifier::currentBrush() {
	for (Brush *b : _brushes) {
		if (b->type() == _brushType) {
			return b;
		}
	}
	return nullptr;
}

const Brush *Modifier::currentBrush() const {
	for (Brush *b : _brushes) {
		if (b->type() == _brushType) {
			return b;
		}
	}
	return nullptr;
}

AABBBrush *Modifier::currentAABBBrush() {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	if (_brushType == BrushType::Paint) {
		return &_paintBrush;
	}
	if (_brushType == BrushType::Select) {
		return &_selectBrush;
	}
	if (_brushType == BrushType::Plane) {
		return &_planeBrush;
	}
	if (_brushType == BrushType::Texture) {
		return &_textureBrush;
	}
	return nullptr;
}

const AABBBrush *Modifier::currentAABBBrush() const {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	if (_brushType == BrushType::Paint) {
		return &_paintBrush;
	}
	if (_brushType == BrushType::Select) {
		return &_selectBrush;
	}
	if (_brushType == BrushType::Plane) {
		return &_planeBrush;
	}
	if (_brushType == BrushType::Texture) {
		return &_textureBrush;
	}
	return nullptr;
}

void Modifier::stop() {
	if (AABBBrush *brush = currentAABBBrush()) {
		brush->stop(_brushContext);
	}
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return isMode(ModifierType::Select) || isMode(ModifierType::Paint) || isMode(ModifierType::Erase) ||
		   isMode(ModifierType::Override);
}

BrushType Modifier::setBrushType(BrushType type) {
	_brushType = type;
	if (_brushType != BrushType::None) {
		// ensure the modifier type is compatible with the brush
		setModifierType(currentBrush()->modifierType(_brushContext.modifierType));
	}
	return _brushType;
}

void Modifier::setGridResolution(int gridSize) {
	_brushContext.gridResolution = core_max(1, gridSize);
}

ModifierType Modifier::setModifierType(ModifierType type) {
	if (_brushType != BrushType::None) {
		_brushContext.modifierType = currentBrush()->modifierType(type);
	} else {
		_brushContext.modifierType = type;
	}
	return _brushContext.modifierType;
}

ModifierType Modifier::checkModifierType() {
	if (_brushType == BrushType::None) {
		return ModifierType::ColorPicker;
	}
	return currentBrush()->modifierType(ModifierType::Mask);
}

} // namespace voxedit
