#pragma once

#include <glad/glad.h>

class ScopedEnable
{
public:
	ScopedEnable(GLenum flag, bool enable = true)
	{
		_flag = flag;
		_oldState = glIsEnabled(_flag);
		if (enable)
			glEnable(_flag);
		else
			glDisable(_flag);
	}
	~ScopedEnable()
	{
		if (_oldState)
			glEnable(_flag);
		else
			glDisable(_flag);
	}

private:
	GLenum _flag;
	GLboolean _oldState;
};

class ScopedPolygonMode
{
public:
	ScopedPolygonMode(GLenum mode)
	{
		glGetIntegerv(GL_POLYGON_MODE, (GLint*)&oldMode);
		glPolygonMode(GL_FRONT_AND_BACK, mode);
	}

	~ScopedPolygonMode()
	{
		glPolygonMode(GL_FRONT_AND_BACK, oldMode);
	}

private:
	GLenum oldMode;
};