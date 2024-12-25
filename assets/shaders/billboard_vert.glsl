#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in int aDirection;

out vec2 TexCoord;
out vec3 Normal;

uniform float texMultiplier;

uniform mat4 models[128];
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * models[gl_BaseInstance] * vec4(aPos, 1.0);
	TexCoord = aTexCoord * texMultiplier;
}