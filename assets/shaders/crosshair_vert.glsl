#version 410 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inTexCoords;

uniform mat4 projection;

out vec2 TexCoord;

void main()
{
	gl_Position.x = inPos.x;
	gl_Position.y = inPos.y;
	gl_Position.z = 0.0f;
	gl_Position.w = 48.0f; // With a simple plane, can be used as a scale.
	//gl_Position = projection * vec4(inPos, 0.0, 1.0);
	TexCoord = inTexCoords;
}