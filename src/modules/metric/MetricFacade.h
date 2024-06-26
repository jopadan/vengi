/**
 * @file
 */

#pragma once

#include "Metric.h"

namespace metric {

bool count(const core::String &key, int delta = 1, const TagMap &tags = {});
bool init(const core::String &appname);
void shutdown();

} // namespace metric
