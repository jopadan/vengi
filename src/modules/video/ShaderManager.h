/**
 * @file
 */
#pragma once

#include "core/collection/Buffer.h"

namespace video {

class Shader;

/**
 * Register @c Shader instances here to let them automatically recompile
 * on @c core::CV_SHADER @c core::CVar change.
 *
 * @ingroup Video
 */
class ShaderManager {
private:
	typedef core::Buffer<Shader*> Shaders;
	Shaders _shaders;
public:
	void registerShader(Shader* shader);
	void unregisterShader(Shader* shader);

	/**
	 * @brief Checks whether a shader var was changed, and recompile all shaders if needed.
	 */
	void update();
};

}
