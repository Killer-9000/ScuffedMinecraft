#version 460 core

layout (location = 0) in uvec3 aPos;
layout (location = 1) in uvec2 aTexCoord;
layout (location = 2) in int aDirection;

out vec2 TexCoord;
out vec3 Normal;

uniform float texMultiplier;

uniform vec3 models[768];
uniform mat4 view;
uniform mat4 projection;
uniform float time;

// Array of possible normals based on direction
const vec3 normals[] = vec3[](
	vec3( 0,  0,  1), // 0
	vec3( 0,  0, -1), // 1
	vec3( 1,  0,  0), // 2
	vec3(-1,  0,  0), // 3
	vec3( 0,  1,  0), // 4
	vec3( 0, -1,  0), // 5
	vec3( 0, -1,  0)  // 6
);

const int aFrames = 32;
const float animationTime = 5;
const int texNum = 16;
void main()
{
	vec3 pos = aPos;
	if (aDirection == 5)
	{
		pos.y -= .1;
		pos.y += (sin(pos.x * 3.1415926535 / 2 + time) + sin(pos.z * 3.1415926535 / 2 + time * 1.5)) * .05;
	}
	gl_Position = projection * view * vec4(models[gl_BaseInstance] + pos, 1.0);
	vec2 currentTex = aTexCoord;
	currentTex.x += mod(floor(mod(time / animationTime, 1) * aFrames), texNum);
	currentTex.y += floor(floor(mod(time / animationTime, 1) * aFrames) / texNum);
	TexCoord = currentTex * texMultiplier;

	Normal = normals[aDirection];
}