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
