/**
 * @file
 */

#pragma once

#include "http/RequestContext.h"
#include "io/Stream.h"

namespace http {

bool http_request(io::WriteStream &stream, int *statusCode, Headers *outheaders,
				  RequestContext &ctx);

inline bool http_supported() {
	return true;
}

} // namespace http
