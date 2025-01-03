#include "Physics.h"
#include <iostream>
#include "tracy/Tracy.hpp"
#include "Blocks.h"

Physics::RaycastResult Physics::Raycast(const glm::vec3 startPos, const glm::vec3 direction, const float maxDistance)
{
	ZoneScoped;
	float currentDistance = 0;

	while (currentDistance < maxDistance)
	{
		currentDistance += Physics::RAY_STEP;
		if (currentDistance > maxDistance)
			currentDistance = maxDistance;

		// Get chunk
		glm::vec3 resultPos = startPos + direction * currentDistance;
		int chunkX = resultPos.x >= 0 ? resultPos.x / CHUNK_WIDTH : resultPos.x / CHUNK_WIDTH - 1;
		int chunkY = resultPos.y >= 0 ? resultPos.y / CHUNK_HEIGHT : resultPos.y / CHUNK_HEIGHT - 1;
		int chunkZ = resultPos.z >= 0 ? resultPos.z / CHUNK_WIDTH : resultPos.z / CHUNK_WIDTH - 1;
		Chunk::Ptr chunk = Planet::planet->GetChunk(ChunkPos(chunkX, chunkY, chunkZ));
		if (chunk == nullptr)
			continue;

		// Get block pos
		int localBlockX = (int)floor(resultPos.x) % CHUNK_WIDTH;
		int localBlockY = (int)floor(resultPos.y) % CHUNK_HEIGHT;
		int localBlockZ = (int)floor(resultPos.z) % CHUNK_WIDTH;

		// Get block from chunk
		unsigned int block = chunk->GetBlockAtPos(localBlockX, localBlockY, localBlockZ);

		// Get result pos
		int blockX = resultPos.x >= 0 ? (int)resultPos.x : (int)resultPos.x - 1;
		int blockY = resultPos.y >= 0 ? (int)resultPos.y : (int)resultPos.y - 1;
		int blockZ = resultPos.z >= 0 ? (int)resultPos.z : (int)resultPos.z - 1;

		// Return true if it hit a block
		if (block != 0 && Blocks::blocks[block].blockType != Block::LIQUID)
			return { true, resultPos, chunk, 
			blockX, blockY, blockZ,
			localBlockX, localBlockY, localBlockZ};
	}

	return { false, glm::vec3(0), nullptr, 
		0, 0, 0, 
		0, 0, 0};
}