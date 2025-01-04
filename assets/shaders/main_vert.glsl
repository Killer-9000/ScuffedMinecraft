#version 460 core

layout (location = 0) in uvec3 aPos;
layout (location = 1) in uvec2 aTexCoord;
layout (location = 2) in uint aDirection;

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

void main()
{
	gl_Position = projection * view * vec4(models[gl_BaseInstance] + aPos, 1);
	TexCoord = aTexCoord * texMultiplier;

	Normal = normals[aDirection];
}