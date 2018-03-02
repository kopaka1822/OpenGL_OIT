#pragma once
#include "../opengl.h"

namespace gl
{
	inline GLint getMaxCombinedTextureImageUnits()
	{
		static auto s_maxUnits = 0;
		if(!s_maxUnits)
			glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &s_maxUnits);
		return s_maxUnits;
	}
}
