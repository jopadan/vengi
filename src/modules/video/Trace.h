/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#ifdef TRACY_ENABLE
#include "video/gl/GLRenderer.h"
#include "core/tracy/public/tracy/TracyOpenGL.hpp"
#endif

namespace video {

class VideoTraceScoped {
public:
	VideoTraceScoped(const char* name, const char *msg = nullptr);
	~VideoTraceScoped();
};

extern void traceVideoInit();
extern void traceVideoShutdown();
extern void traceVideoBegin(const char* name);
extern void traceVideoEnd();
extern void traceVideoFrameEnd();

#define video_trace_init() video::traceVideoInit()
#define video_trace_shutdown() video::traceVideoShutdown()
#define video_trace_frame_end() video::traceVideoFrameEnd()
#ifdef TRACY_ENABLE
#define video_trace_begin(name)
#define video_trace_begin_dynamic(name)
#define video_trace_end()
#define video_trace_scoped(name) TracyGpuNamedZone(__tracy_scoped_##name, #name, true)
#else
#define video_trace_begin(name) video::traceVideoBegin(#name)
#define video_trace_begin_dynamic(name) video::traceVideoBegin(#name)
#define video_trace_end() video::traceVideoEnd()
#define video_trace_scoped(name) video::VideoTraceScoped __trace__##name(#name)
#endif

}
