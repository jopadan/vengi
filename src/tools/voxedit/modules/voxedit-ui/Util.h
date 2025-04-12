/**
 * @file
 */

#pragma once

#include "ui/ScopedStyle.h"
#include "command/CommandHandler.h"
#include "math/Axis.h"

namespace voxedit {
namespace veui {

void AxisStyleButton(ui::ScopedStyle &style, math::Axis axis);
void AxisStyleText(ui::ScopedStyle &style, math::Axis axis, bool dark = true);
bool AxisButton(math::Axis axis, const char *name, const char *command, const char *icon, const char *tooltip,
				float width, command::CommandExecutionListener *listener);
bool InputAxisInt(math::Axis axis, const char *name, int* value, int step = 1);
bool CheckboxAxisFlags(math::Axis axis, const char *name, math::Axis* value);

} // namespace veui
} // namespace voxedit
