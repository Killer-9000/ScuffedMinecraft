#version 410 core

layout(location = 0) in vec3 oPos;

out vec4 FragColor;

uniform float time;
uniform bool chunkBorder;

int CHUNK_WIDTH = 32; // x/z
int CHUNK_HEIGHT = 256; // y

float BORDER_LINE_WIDTH = 0.03f;
vec3 BORDER_LINE_COLOUR = vec3(0.0f, 0.0f, 0.0f);
float BLOCK_LINE_WIDTH = 0.02f;
vec3 BLOCK_LINE_COLOUR = vec3(1.0f, 0.8f, 0.0f);

bool IsChunkBorder(vec3 chunkPos, float lineWidth)
{
	bool x = chunkPos.x < lineWidth || CHUNK_WIDTH - chunkPos.x < lineWidth;
	bool y = chunkPos.y < lineWidth || CHUNK_HEIGHT - chunkPos.y < lineWidth;
	bool z = chunkPos.z < lineWidth || CHUNK_WIDTH - chunkPos.z < lineWidth;
	return (x && y) || (x && z) || (y && z);
}

bool IsBlockBorder(vec3 blockPos, float lineWidth)
{
	bool x = (mod(blockPos.x, 2.0f)) < lineWidth || 2.0f - (mod(blockPos.x, 2.0f)) < lineWidth;
	bool y = (mod(blockPos.y, 2.0f)) < lineWidth || 2.0f - (mod(blockPos.y, 2.0f)) < lineWidth;
	bool z = (mod(blockPos.z, 2.0f)) < lineWidth || 2.0f - (mod(blockPos.z, 2.0f)) < lineWidth;
	return x || y || z;
}

void main()
{
	if (chunkBorder)
	{
		vec3 blockPos = vec3(oPos.x * CHUNK_WIDTH, oPos.y * CHUNK_HEIGHT, oPos.z * CHUNK_WIDTH);

		if (IsChunkBorder(blockPos, BORDER_LINE_WIDTH))
			FragColor = vec4(BORDER_LINE_COLOUR, 1.0);
		else if (IsBlockBorder(blockPos, BLOCK_LINE_WIDTH))
			FragColor = vec4(BLOCK_LINE_COLOUR, 1.0);
		else
		{
			FragColor = vec4(0.0);
			discard;
		}
	}
	else
	{
		vec3 blockPos = vec3(oPos.x * CHUNK_WIDTH, oPos.y * CHUNK_HEIGHT, oPos.z * CHUNK_WIDTH);

		if (IsChunkBorder(blockPos, BORDER_LINE_WIDTH * 10))
			FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		else
		{
			FragColor = vec4(0.0);
			discard;
		}
	}
}