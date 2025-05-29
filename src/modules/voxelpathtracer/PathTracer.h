/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/SharedPtr.h"
#include <yocto_scene.h>
#include <yocto_trace.h>

namespace video {
class Camera;
}

namespace image {
class Image;
typedef core::SharedPtr<Image> ImagePtr;
} // namespace image

namespace palette {
class Palette;
}

namespace voxel {
class Mesh;
} // namespace voxel

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
class SceneGraphNodeCamera;
} // namespace scenegraph

namespace voxelpathtracer {

struct PathTracerState {
	yocto::trace_context context;
	yocto::scene_data scene;
	yocto::trace_bvh bvh;
	yocto::trace_params params;
	yocto::trace_lights lights;
	yocto::trace_state state;
	bool started = false;

	PathTracerState() : context(yocto::make_trace_context({})) {
	}
};

class PathTracer {
private:
	PathTracerState _state;

	void addCamera(const scenegraph::SceneGraphNodeCamera &node);
	void addCamera(const char *name, const video::Camera &cam);

	bool addNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
				 const voxel::Mesh &mesh, bool opaque);
	bool createScene(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera);

public:
	~PathTracer();
	PathTracerState &state() {
		return _state;
	}
	bool start(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera = nullptr);
	bool restart(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera = nullptr);
	bool stop();
	bool started() const;

	/**
	 * @brief Update the path tracer. This will render a batch of samples and must get called until either stop() was
	 * called or @c false is returned.
	 * @return @c true if rendering is done, @c false otherwise
	 * @sa image()
	 */
	bool update(int *currentSample = nullptr);

	image::ImagePtr image();
};

} // namespace voxelpathtracer
